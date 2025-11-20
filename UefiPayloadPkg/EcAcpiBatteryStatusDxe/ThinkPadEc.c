/** @file
  ThinkPad EC Implementation

  Copyright (c) 2025
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EcAcpiBatteryStatusDxe.h"
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

//
// ThinkPad EC RAM Offsets
//
#define THINKPAD_ECRAM_PAGE           0x81  // Information Page Selector
#define THINKPAD_ECRAM_BATTERY_STATE  0x38  // Battery 0 state flags
#define THINKPAD_ECRAM_BATTERY_CAP    0xa0  // Battery capacity (PAGE 0x00)

//
// ThinkPad Battery State Bits (at offset 0x38)
//
#define THINKPAD_B0PR  BIT0  // Battery 0 present
#define THINKPAD_B0CH  BIT1  // Battery 0 charging
#define THINKPAD_B0DI  BIT2  // Battery 0 discharging

//
// ThinkPad Battery Capacity Offsets (when PAGE = 0x00)
//
#define THINKPAD_BARC_OFFSET  0xa0  // Battery remaining capacity (16-bit)
#define THINKPAD_BAFC_OFFSET  0xa2  // Battery full charge capacity (16-bit)

/**
  Read a byte from ThinkPad EC RAM.

  @param[in]  Address   EC RAM address to read
  @param[out] Data      Pointer to store the read data

  @retval EFI_SUCCESS         Data read successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
ThinkPadEcReadByte (
  IN  UINT8   Address,
  OUT UINT8   *Data
  )
{
  EFI_STATUS  Status;

  // Send read command
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IoWrite8 (EC_SC, RD_EC);

  // Send address
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IoWrite8 (EC_DATA, Address);

  // Read data
  Status = WaitForEcReadyRecv ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Data = IoRead8 (EC_DATA);

  return EFI_SUCCESS;
}

/**
  Write a byte to ThinkPad EC RAM.

  @param[in]  Address   EC RAM address to write
  @param[in]  Data      Data to write

  @retval EFI_SUCCESS         Data written successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
ThinkPadEcWriteByte (
  IN  UINT8   Address,
  IN  UINT8   Data
  )
{
  EFI_STATUS  Status;

  // Send write command
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IoWrite8 (EC_SC, WR_EC);

  // Send address
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IoWrite8 (EC_DATA, Address);

  // Send data
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IoWrite8 (EC_DATA, Data);

  // Wait for write to complete (IBF should clear)
  Status = WaitForEcReadySend ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Read a 16-bit word from ThinkPad EC RAM (little-endian).

  @param[in]  Address   EC RAM address to read
  @param[out] Data      Pointer to store the read data

  @retval EFI_SUCCESS         Data read successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
ThinkPadEcReadWord (
  IN  UINT8    Address,
  OUT UINT16   *Data
  )
{
  EFI_STATUS  Status;
  UINT8       Low;
  UINT8       High;

  Status = ThinkPadEcReadByte (Address, &Low);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ThinkPadEcReadByte (Address + 1, &High);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Data = (UINT16)((High << 8) | Low);

  return EFI_SUCCESS;
}

/**
  Check if ThinkPad EC is present by reading a known register.

  @retval EFI_SUCCESS      EC is present
  @retval EFI_NOT_FOUND    EC not found
**/
EFI_STATUS
CheckThinkPadEcPresent (
  VOID
  )
{
  UINT8       Data;
  EFI_STATUS  Status;

  // Try to read EC RAM at offset 0x00
  // If we get 0xFF or timeout, EC is likely not present
  Status = ThinkPadEcReadByte (0x00, &Data);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  // If we get 0xFF, EC is likely not present
  if (Data == 0xFF) {
    return EFI_NOT_FOUND;
  }

  // Try to read battery state register to verify EC is responsive
  Status = ThinkPadEcReadByte (THINKPAD_ECRAM_BATTERY_STATE, &Data);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Get battery information from ThinkPad EC.

  @param[out] BatteryPercentage  Battery charge percentage (0-100)
  @param[out] BatteryPresent     TRUE if battery is present
  @param[out] BatteryCharging    TRUE if battery is charging

  @retval EFI_SUCCESS            Battery information retrieved successfully
  @retval EFI_DEVICE_ERROR       Communication error
  @retval EFI_UNSUPPORTED        Battery not available
**/
EFI_STATUS
GetThinkPadBatteryInfo (
  OUT UINT8    *BatteryPercentage,
  OUT BOOLEAN  *BatteryPresent,
  OUT BOOLEAN  *BatteryCharging
  )
{
  EFI_STATUS  Status;
  UINT8       BatteryState;
  UINT16      BatteryRemaining;
  UINT16      BatteryFull;
  UINT32      Percentage;

  if (mEcBatteryPrivate == NULL || !mEcBatteryPrivate->EcPresent) {
    return EFI_UNSUPPORTED;
  }

  // Read battery state flags
  Status = ThinkPadEcReadByte (THINKPAD_ECRAM_BATTERY_STATE, &BatteryState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EcAcpiBattery: Failed to read battery state at offset 0x%02x: %r\n", THINKPAD_ECRAM_BATTERY_STATE, Status));
    return Status;
  }

  // Check if battery is present
  *BatteryPresent = ((BatteryState & THINKPAD_B0PR) != 0);

  if (!*BatteryPresent) {
    *BatteryPercentage = 0xFF;
    *BatteryCharging   = FALSE;
    DEBUG ((DEBUG_INFO, "EcAcpiBattery: [ThinkPad] No battery present\n"));
    return EFI_UNSUPPORTED;
  }

  // Check if battery is charging
  *BatteryCharging = ((BatteryState & THINKPAD_B0CH) != 0);

  // Set PAGE register to 0x00 to access battery capacity information
  Status = ThinkPadEcWriteByte (THINKPAD_ECRAM_PAGE, 0x00);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EcAcpiBattery: Failed to set PAGE register: %r\n", Status));
    return Status;
  }

  // Small delay to allow EC to switch pages
  gBS->Stall (1000); // 1ms

  // Read remaining capacity
  Status = ThinkPadEcReadWord (THINKPAD_BARC_OFFSET, &BatteryRemaining);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "EcAcpiBattery: Failed to read battery remaining capacity: %r\n", Status));
    return Status;
  }

  // Read full charge capacity
  Status = ThinkPadEcReadWord (THINKPAD_BAFC_OFFSET, &BatteryFull);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "EcAcpiBattery: Failed to read battery full capacity: %r\n", Status));
    return Status;
  }

  // Calculate percentage
  if ((BatteryRemaining == 0xFFFF) || (BatteryFull == 0xFFFF) || (BatteryFull == 0)) {
    *BatteryPercentage = 0xFF;
  } else {
    Percentage = (BatteryRemaining * 100) / BatteryFull;
    if (Percentage > 100) {
      Percentage = 100;
    }

    *BatteryPercentage = (UINT8)Percentage;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "EcAcpiBattery: [ThinkPad] Battery %d%%, Present=%d, Charging=%d (remaining=%u, full=%u, state=0x%02x)\n",
    *BatteryPercentage,
    *BatteryPresent,
    *BatteryCharging,
    BatteryRemaining,
    BatteryFull,
    BatteryState
    ));

  return EFI_SUCCESS;
}

