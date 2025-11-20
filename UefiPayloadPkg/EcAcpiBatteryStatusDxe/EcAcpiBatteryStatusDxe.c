/** @file
  EC ACPI Battery DXE Driver

  This driver communicates with Embedded Controllers (EC) via ACPI ports to
  retrieve battery information and exposes it through the Battery Status Protocol.
  Supports multiple EC types including ChromeOS EC and Merlin EC.

  Copyright (c) 2025
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EcAcpiBatteryStatusDxe.h"
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/BatteryStatus.h>

//
// Driver Private Data
//
EC_BATTERY_PRIVATE_DATA  *mEcBatteryPrivate = NULL;

//
// EC Profiles
//
STATIC EC_PROFILE  mEcProfiles[] = {
  {
    EcTypeChromeOs,
    "ChromeEC",
    CheckChromeOsEcPresent,
    GetChromeOsBatteryInfo
  },
  {
    EcTypeMerlin,
    "Merlin EC",
    CheckMerlinEcPresent,
    GetMerlinBatteryInfo
  },
  {
    EcTypeThinkPad,
    "ThinkPad EC",
    CheckThinkPadEcPresent,
    GetThinkPadBatteryInfo
  }
};

//
// Common EC Communication Functions
//

/**
  Wait for EC input buffer to be ready (not full).

  @retval EFI_SUCCESS   EC is ready to receive
  @retval EFI_TIMEOUT   Timeout waiting for EC
**/
EFI_STATUS
WaitForEcReadySend (
  VOID
  )
{
  UINT64  ElapsedUs;
  UINT8   Status;

  ElapsedUs = 0;

  while (ElapsedUs < EC_TIMEOUT_US) {
    Status = IoRead8 (EC_SC);
    if ((Status & EC_IBF) == 0) {
      return EFI_SUCCESS;
    }

    gBS->Stall (100); // 100 microseconds
    ElapsedUs += 100;
  }

  return EFI_TIMEOUT;
}

/**
  Wait for EC output buffer to be ready (data available).

  @retval EFI_SUCCESS   EC has data ready
  @retval EFI_TIMEOUT   Timeout waiting for EC
**/
EFI_STATUS
WaitForEcReadyRecv (
  VOID
  )
{
  UINT64  ElapsedUs;
  UINT8   Status;

  ElapsedUs = 0;

  while (ElapsedUs < EC_TIMEOUT_US) {
    Status = IoRead8 (EC_SC);
    if ((Status & EC_OBF) != 0) {
      return EFI_SUCCESS;
    }

    gBS->Stall (100); // 100 microseconds
    ElapsedUs += 100;
  }

  return EFI_TIMEOUT;
}

//
// Unified Driver Functions
//

/**
  Detect EC type by trying each profile's detection method.

  @retval EFI_SUCCESS      EC type detected
  @retval EFI_NOT_FOUND    No supported EC found
**/
STATIC
EFI_STATUS
DetectEcType (
  VOID
  )
{
  UINTN  i;

  for (i = 0; i < (sizeof (mEcProfiles) / sizeof (mEcProfiles[0])); i++) {
    if (mEcProfiles[i].CheckEcPresent != NULL) {
      if (!EFI_ERROR (mEcProfiles[i].CheckEcPresent ())) {
        mEcBatteryPrivate->EcType   = mEcProfiles[i].Type;
        mEcBatteryPrivate->Profile  = &mEcProfiles[i];
        DEBUG ((DEBUG_INFO, "EcAcpiBattery: Detected %a\n", mEcProfiles[i].Name));
        return EFI_SUCCESS;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get battery information.

  @param[in]  This               Pointer to the EFI_BATTERY_STATUS_PROTOCOL instance.
  @param[out] BatteryPercentage  Battery charge percentage (0-100). 0xFF indicates error/unknown.
  @param[out] BatteryPresent     TRUE if battery is present, FALSE otherwise.
  @param[out] BatteryCharging    TRUE if battery is charging, FALSE otherwise.

  @retval EFI_SUCCESS            Battery information retrieved successfully.
  @retval EFI_DEVICE_ERROR       Communication error.
  @retval EFI_UNSUPPORTED        Battery not available or not supported.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.

**/
STATIC
EFI_STATUS
EFIAPI
EcAcpiBatteryGetBatteryInfo (
  IN  EFI_BATTERY_STATUS_PROTOCOL  *This,
  OUT UINT8                         *BatteryPercentage,
  OUT BOOLEAN                       *BatteryPresent,
  OUT BOOLEAN                       *BatteryCharging
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (BatteryPercentage == NULL) || (BatteryPresent == NULL) || (BatteryCharging == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mEcBatteryPrivate == NULL || !mEcBatteryPrivate->EcPresent || mEcBatteryPrivate->Profile == NULL) {
    *BatteryPercentage = 0xFF;
    *BatteryPresent    = FALSE;
    *BatteryCharging   = FALSE;
    return EFI_UNSUPPORTED;
  }

  Status = mEcBatteryPrivate->Profile->GetBatteryInfo (BatteryPercentage, BatteryPresent, BatteryCharging);
  if (EFI_ERROR (Status)) {
    *BatteryPercentage = 0xFF;
    *BatteryPresent    = FALSE;
    *BatteryCharging   = FALSE;
  }

  return Status;
}

/**
  Entry point for EC ACPI Battery DXE Driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval EFI_DEVICE_ERROR  An error occurred during initialization.

**/
EFI_STATUS
EFIAPI
EcAcpiBatteryStatusDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  // Allocate private data
  mEcBatteryPrivate = AllocateZeroPool (sizeof (EC_BATTERY_PRIVATE_DATA));
  if (mEcBatteryPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize protocol
  mEcBatteryPrivate->Protocol.GetBatteryInfo = EcAcpiBatteryGetBatteryInfo;

  // Detect EC type
  Status = DetectEcType ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "EcAcpiBattery: No supported EC found, driver not loaded.\n"));
    FreePool (mEcBatteryPrivate);
    mEcBatteryPrivate = NULL;
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "EcAcpiBattery: EC initialized successfully (%a)\n", mEcBatteryPrivate->Profile->Name));
  mEcBatteryPrivate->EcPresent      = TRUE;
  mEcBatteryPrivate->EcInitialized  = TRUE;

  // Install protocol
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiBatteryStatusProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mEcBatteryPrivate->Protocol
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EcAcpiBattery: Failed to install protocol: %r\n", Status));
    FreePool (mEcBatteryPrivate);
    mEcBatteryPrivate = NULL;
    return Status;
  }

  DEBUG ((DEBUG_INFO, "EcAcpiBattery: Driver initialized successfully\n"));

  return EFI_SUCCESS;
}

