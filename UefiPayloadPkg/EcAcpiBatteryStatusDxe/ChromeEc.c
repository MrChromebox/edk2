/** @file
  ChromeOS EC Implementation

  Copyright (c) 2025
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EcAcpiBatteryStatusDxe.h"
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

//
// ChromeOS EC Memmap Offsets
//
#define CHROMEOS_EC_ACPI_MEM_MAPPED_BEGIN      0x20  // Base offset for ACPI memmap addresses
#define CHROMEOS_EC_LPC_ADDR_MEMMAP            0x900 // Direct LPC memmap base address
#define CHROMEOS_EC_MEMMAP_ID                  0x20  // EC ID ('E' at 0x20, 'C' at 0x21)
#define CHROMEOS_EC_MEMMAP_BATT_FLAG           0x4c  // Battery state flags (8-bit)
#define CHROMEOS_EC_MEMMAP_BATT_CAP            0x48  // Battery Remaining Capacity (32-bit)
#define CHROMEOS_EC_MEMMAP_BATT_LFCC           0x58  // Battery Last Full Charge Capacity (32-bit)
#define CHROMEOS_EC_MEMMAP_BATT_UNKNOWN_VALUE  ((UINT32)0xFFFFFFFF)  // Unknown/invalid value

//
// ChromeOS Battery Flag Bits
//
#define CHROMEOS_EC_BATT_FLAG_BATT_PRESENT  0x02  // Battery is present
#define CHROMEOS_EC_BATT_FLAG_CHARGING      0x08  // Battery is charging

/**
  Read a byte from ChromeOS EC memmap via ACPI interface.

  @param[in]  Offset   Memmap offset (0x00-0xdf)
  @param[out] Data     Pointer to store the read data

  @retval EFI_SUCCESS         Data read successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
ChromeOsEcReadMemmapByteAcpi (
  IN  UINT8   Offset,
  OUT UINT8   *Data
  )
{
  EFI_STATUS  Status;
  UINT8       AcpiAddress;

  // Wait for EC to be ready
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Send ACPI read command
  IoWrite8 (EC_SC, EC_CMD_ACPI_READ);

  // Wait for EC to be ready
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Small delay for EC to process the command
  gBS->Stall (50);  // 50 microseconds

  // Write memmap offset (add CHROMEOS_EC_ACPI_MEM_MAPPED_BEGIN to get ACPI address)
  AcpiAddress = Offset + CHROMEOS_EC_ACPI_MEM_MAPPED_BEGIN;
  DEBUG ((DEBUG_VERBOSE, "EcAcpiBattery: Reading memmap offset 0x%02x (ACPI address 0x%02x)\n", Offset, AcpiAddress));

  // Wait for EC to be ready before writing address
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "EcAcpiBattery: WaitForEcReadySend before writing address failed: %r\n", Status));
    return Status;
  }

  IoWrite8 (EC_DATA, AcpiAddress);

  // Wait for data to be ready
  Status = WaitForEcReadyRecv ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Read data
  *Data = IoRead8 (EC_DATA);

  return EFI_SUCCESS;
}

/**
  Read a byte from ChromeOS EC memmap via direct LPC access.
  Used for non-MEC ECs (Sandy Bridge, Ivy Bridge, Haswell, Baytrail).
 *
  @param[in]  Offset   Memmap offset (0x00-0xdf)
  @param[out] Data     Pointer to store the read data
 *
  @retval EFI_SUCCESS         Data read successfully
**/
STATIC
EFI_STATUS
ChromeOsEcReadMemmapByteDirectLpc (
  IN  UINT8   Offset,
  OUT UINT8   *Data
  )
{
  // Direct LPC access: read from 0x900 + offset
  // This is used for non-MEC ECs on older platforms (Link, Swanky, Wolf)
  *Data = IoRead8 (CHROMEOS_EC_LPC_ADDR_MEMMAP + Offset);
  DEBUG ((DEBUG_VERBOSE, "EcAcpiBattery: Direct LPC read from 0x%04x (offset 0x%02x): 0x%02x\n",
          CHROMEOS_EC_LPC_ADDR_MEMMAP + Offset, Offset, *Data));
  return EFI_SUCCESS;
}

/**
  Read a byte from ChromeOS EC memmap.
  Tries direct LPC access first (for non-MEC ECs), then falls back to ACPI access (for MEC ECs).
 *
  @param[in]  Offset   Memmap offset (0x00-0xdf)
  @param[out] Data     Pointer to store the read data
 *
  @retval EFI_SUCCESS         Data read successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
ChromeOsEcReadMemmapByte (
  IN  UINT8   Offset,
  OUT UINT8   *Data
  )
{
  STATIC BOOLEAN  UseDirectLpc = TRUE;  // Try direct LPC first
  STATIC BOOLEAN  MethodDetermined = FALSE;

  // On first call, try to determine which method to use
  // Try direct LPC first (faster, works for non-MEC ECs)
  if (!MethodDetermined) {
    UINT8  TestId1, TestId2;

    // Try direct LPC read
    TestId1 = IoRead8 (CHROMEOS_EC_LPC_ADDR_MEMMAP + CHROMEOS_EC_MEMMAP_ID);
    TestId2 = IoRead8 (CHROMEOS_EC_LPC_ADDR_MEMMAP + CHROMEOS_EC_MEMMAP_ID + 1);

    if ((TestId1 == 'E') && (TestId2 == 'C')) {
      UseDirectLpc = TRUE;
      MethodDetermined = TRUE;
      DEBUG ((DEBUG_INFO, "EcAcpiBattery: Using direct LPC memmap access (non-MEC EC: Link/Swanky/Wolf)\n"));
    } else {
      // Direct LPC didn't work, will use ACPI method
      UseDirectLpc = FALSE;
      MethodDetermined = TRUE;
      DEBUG ((DEBUG_INFO, "EcAcpiBattery: Direct LPC failed (read 0x%02x 0x%02x), using ACPI memmap access (MEC EC: Chell/Cyan)\n", TestId1, TestId2));
    }
  }

  if (UseDirectLpc) {
    return ChromeOsEcReadMemmapByteDirectLpc (Offset, Data);
  } else {
    return ChromeOsEcReadMemmapByteAcpi (Offset, Data);
  }
}

/**
  Read a 32-bit word from ChromeOS EC memmap (little-endian).

  @param[in]  Offset   Memmap offset (0x00-0xdf)
  @param[out] Data     Pointer to store the read data

  @retval EFI_SUCCESS         Data read successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
ChromeOsEcReadMemmapWord (
  IN  UINT8    Offset,
  OUT UINT32   *Data
  )
{
  EFI_STATUS  Status;
  UINT8       Bytes[4];
  UINTN       i;

  for (i = 0; i < 4; i++) {
    Status = ChromeOsEcReadMemmapByte (Offset + i, &Bytes[i]);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  *Data = (UINT32)((Bytes[3] << 24) | (Bytes[2] << 16) | (Bytes[1] << 8) | Bytes[0]);

  return EFI_SUCCESS;
}

/**
  Check if ChromeOS EC is present by reading memmap ID.

  @retval EFI_SUCCESS      EC is present
  @retval EFI_NOT_FOUND    EC not found
**/
EFI_STATUS
CheckChromeOsEcPresent (
  VOID
  )
{
  UINT8       Id1;
  UINT8       Id2;
  EFI_STATUS  Status;

  // Read EC memmap ID (should be 'E' and 'C')
  Status = ChromeOsEcReadMemmapByte (CHROMEOS_EC_MEMMAP_ID, &Id1);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Status = ChromeOsEcReadMemmapByte (CHROMEOS_EC_MEMMAP_ID + 1, &Id2);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  // Check if ID matches "EC"
  if ((Id1 != 'E') || (Id2 != 'C')) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Get battery information from ChromeOS EC memmap.

  @param[out] BatteryPercentage  Battery charge percentage (0-100)
  @param[out] BatteryPresent     TRUE if battery is present
  @param[out] BatteryCharging    TRUE if battery is charging

  @retval EFI_SUCCESS            Battery information retrieved successfully
  @retval EFI_DEVICE_ERROR       Communication error
  @retval EFI_UNSUPPORTED        Battery not available
**/
EFI_STATUS
GetChromeOsBatteryInfo (
  OUT UINT8    *BatteryPercentage,
  OUT BOOLEAN  *BatteryPresent,
  OUT BOOLEAN  *BatteryCharging
  )
{
  EFI_STATUS  Status;
  UINT8       BatteryFlag;
  UINT32      BatteryCap;
  UINT32      BatteryLfcc;
  UINT32      Percentage;

  if (mEcBatteryPrivate == NULL || !mEcBatteryPrivate->EcPresent) {
    return EFI_UNSUPPORTED;
  }

  // Read battery flag
  Status = ChromeOsEcReadMemmapByte (CHROMEOS_EC_MEMMAP_BATT_FLAG, &BatteryFlag);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "EcAcpiBattery: Failed to read battery flag: %r\n", Status));
    return Status;
  }

  // Check if battery is present
  *BatteryPresent = ((BatteryFlag & CHROMEOS_EC_BATT_FLAG_BATT_PRESENT) != 0);

  if (!*BatteryPresent) {
    *BatteryCharging   = FALSE;
    *BatteryPercentage = 0xFF;
    DEBUG ((DEBUG_INFO, "EcAcpiBattery: [ChromeOS] No battery present\n"));
    return EFI_UNSUPPORTED;
  }

  // Check if battery is charging
  *BatteryCharging = ((BatteryFlag & CHROMEOS_EC_BATT_FLAG_CHARGING) != 0);

  // Read remaining capacity
  Status = ChromeOsEcReadMemmapWord (CHROMEOS_EC_MEMMAP_BATT_CAP, &BatteryCap);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "EcAcpiBattery: Failed to read battery capacity: %r\n", Status));
    return Status;
  }

  // Read last full charge capacity
  Status = ChromeOsEcReadMemmapWord (CHROMEOS_EC_MEMMAP_BATT_LFCC, &BatteryLfcc);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "EcAcpiBattery: Failed to read battery full capacity: %r\n", Status));
    return Status;
  }

  // Calculate percentage: (remaining / full) * 100
  // Check for invalid/unknown values (0xFFFFFFFF)
  if ((BatteryCap == CHROMEOS_EC_MEMMAP_BATT_UNKNOWN_VALUE) ||
      (BatteryLfcc == CHROMEOS_EC_MEMMAP_BATT_UNKNOWN_VALUE) ||
      (BatteryLfcc == 0))
  {
    *BatteryPercentage = 0xFF;
  } else {
    // Calculate percentage: (remaining / full) * 100
    // Use 64-bit intermediate to avoid overflow when multiplying by 100
    // Add half of divisor for proper rounding: (cap * 100 + lfcc/2) / lfcc
    Percentage = (UINT32)(((UINT64)BatteryCap * 100 + (BatteryLfcc / 2)) / BatteryLfcc);
    if (Percentage > 100) {
      DEBUG ((DEBUG_WARN, "EcAcpiBattery: Calculated percentage > 100%% (%u%%), capping at 100%%\n", Percentage));
      Percentage = 100;
    }

    *BatteryPercentage = (UINT8)Percentage;
  }

  DEBUG ((
    DEBUG_INFO,
    "EcAcpiBattery: [ChromeOS] Battery %d%%, Present=%d, Charging=%d (cap=%u, lfcc=%u, flag=0x%02x)\n",
    *BatteryPercentage,
    *BatteryPresent,
    *BatteryCharging,
    BatteryCap,
    BatteryLfcc,
    BatteryFlag
    ));

  return EFI_SUCCESS;
}

