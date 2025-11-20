/** @file
  Merlin EC Implementation

  Copyright (c) 2025
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EcAcpiBatteryStatusDxe.h"
#include <Library/DebugLib.h>
#include <Library/IoLib.h>

//
// Merlin EC RAM Battery Offsets
//
#define MERLIN_ECRAM_POWER_STATE               0x80  // Power state register
  #define MERLIN_BATTERY_PRESENT               BIT1  // Battery is present
  #define MERLIN_BATTERY_DETECTED              BIT2  // Battery is detected
#define MERLIN_ECRAM_BATTERY_STATE             0x8c  // Battery state register
  #define MERLIN_BATTERY_CHARGING              BIT1  // Battery is charging
#define MERLIN_ECRAM_BATTERY_REL_STATE_OF_CHRG 0x93  // 2 bytes

/**
  Read a byte from Merlin EC RAM.

  @param[in]  Address   EC RAM address to read
  @param[out] Data      Pointer to store the read data

  @retval EFI_SUCCESS         Data read successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
MerlinEcReadByte (
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
  Read a 16-bit word from Merlin EC RAM (little-endian).

  @param[in]  Address   EC RAM address to read
  @param[out] Data      Pointer to store the read data

  @retval EFI_SUCCESS         Data read successfully
  @retval EFI_DEVICE_ERROR    EC communication error
  @retval EFI_TIMEOUT         Timeout waiting for EC
**/
STATIC
EFI_STATUS
MerlinEcReadWord (
  IN  UINT8    Address,
  OUT UINT16   *Data
  )
{
  EFI_STATUS  Status;
  UINT8       Low;
  UINT8       High;

  Status = MerlinEcReadByte (Address, &Low);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MerlinEcReadByte (Address + 1, &High);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Data = (UINT16)((High << 8) | Low);

  return EFI_SUCCESS;
}

/**
  Check if Merlin EC is present by reading a known register.

  @retval EFI_SUCCESS      EC is present
  @retval EFI_NOT_FOUND    EC not found
**/
EFI_STATUS
CheckMerlinEcPresent (
  VOID
  )
{
  UINT8       Data;
  EFI_STATUS  Status;

  // Try to read EC RAM version (offset 0x00 in ACPI region)
  Status = MerlinEcReadByte (0x00, &Data);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  // If we get 0xFF, EC is likely not present
  if (Data == 0xFF) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Get battery information from Merlin EC.

  @param[out] BatteryPercentage  Battery charge percentage (0-100)
  @param[out] BatteryPresent     TRUE if battery is present
  @param[out] BatteryCharging    TRUE if battery is charging

  @retval EFI_SUCCESS            Battery information retrieved successfully
  @retval EFI_DEVICE_ERROR       Communication error
  @retval EFI_UNSUPPORTED        Battery not available
**/
EFI_STATUS
GetMerlinBatteryInfo (
  OUT UINT8    *BatteryPercentage,
  OUT BOOLEAN  *BatteryPresent,
  OUT BOOLEAN  *BatteryCharging
  )
{
  EFI_STATUS  Status;
  UINT8       PowerState;
  UINT8       BatteryState;
  UINT16      RelativeStateOfCharge;

  if (mEcBatteryPrivate == NULL || !mEcBatteryPrivate->EcPresent) {
    return EFI_UNSUPPORTED;
  }

  // Read power state to determine battery presence
  Status = MerlinEcReadByte (MERLIN_ECRAM_POWER_STATE, &PowerState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EcAcpiBattery: Failed to read power state at offset 0x%02x: %r\n", MERLIN_ECRAM_POWER_STATE, Status));
    return Status;
  }

  // Check if battery is present and detected
  *BatteryPresent = ((PowerState & MERLIN_BATTERY_PRESENT) != 0) && ((PowerState & MERLIN_BATTERY_DETECTED) != 0);

  if (!*BatteryPresent) {
    *BatteryCharging   = FALSE;
    *BatteryPercentage = 0xFF;
    DEBUG ((DEBUG_INFO, "EcAcpiBattery: [Merlin] No battery present (power_state=0x%02x)\n", PowerState));
    return EFI_UNSUPPORTED;
  }

  // Read battery state for charging status
  Status = MerlinEcReadByte (MERLIN_ECRAM_BATTERY_STATE, &BatteryState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EcAcpiBattery: Failed to read battery state at offset 0x%02x: %r\n", MERLIN_ECRAM_BATTERY_STATE, Status));
    return Status;
  }

  // Read relative state of charge (percentage)
  Status = MerlinEcReadWord (MERLIN_ECRAM_BATTERY_REL_STATE_OF_CHRG, &RelativeStateOfCharge);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "EcAcpiBattery: Failed to read battery percentage: %r\n", Status));
    return Status;
  }

  // Validate percentage value
  if (RelativeStateOfCharge <= 100) {
    *BatteryPercentage = (UINT8)RelativeStateOfCharge;
  } else {
    *BatteryPercentage = 0xFF;
  }

  // Determine charging status: BATTERY_CHARGING bit set
  *BatteryCharging = ((BatteryState & MERLIN_BATTERY_CHARGING) != 0);

  DEBUG ((
    DEBUG_VERBOSE,
    "EcAcpiBattery: [Merlin] Battery %d%%, Present=%d, Charging=%d (power_state=0x%02x, battery_state=0x%02x)\n",
    *BatteryPercentage,
    *BatteryPresent,
    *BatteryCharging,
    PowerState,
    BatteryState
    ));

  return EFI_SUCCESS;
}
