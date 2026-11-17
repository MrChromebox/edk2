/** @file
  EC ACPI Battery DXE Driver

  This driver communicates with Embedded Controllers (EC) via ACPI ports to
  retrieve battery information and exposes it through the Battery Status Protocol.
  Supports multiple EC types including ChromeOS EC and Merlin EC.

  Copyright (c) 2025
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EcAcpiBatteryStatusDxe.h"
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Protocol/BatteryStatus.h>
#include <Protocol/Smbios.h>

//
// Driver Private Data
//
EC_BATTERY_PRIVATE_DATA  *mEcBatteryPrivate = NULL;

//
// SMBIOS mainboard vendors per EC type (NULL-terminated)
//
STATIC CONST CHAR8  *mChromeOsEcVendors[] = { "Google", NULL };
STATIC CONST CHAR8  *mMerlinEcVendors[]   = { "Star Labs", "StarLabs", NULL };
STATIC CONST CHAR8  *mThinkPadEcVendors[] = { "LENOVO", NULL };

//
// EC Profiles
//
STATIC EC_PROFILE  mEcProfiles[] = {
  {
    EcTypeChromeOs,
    "ChromeEC",
    mChromeOsEcVendors,
    CheckChromeOsEcPresent,
    GetChromeOsBatteryInfo
  },
  {
    EcTypeMerlin,
    "Merlin EC",
    mMerlinEcVendors,
    CheckMerlinEcPresent,
    GetMerlinBatteryInfo
  },
  {
    EcTypeThinkPad,
    "ThinkPad EC",
    mThinkPadEcVendors,
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

/*
 * Drain the EC output buffer and test the interface for stuck 0xFF values.
 *
 * @retval EFI_SUCCESS         EC output buffer drained successfully.
 * @retval EFI_TIMEOUT         EC output buffer not drained in time.
 * @retval EFI_DEVICE_ERROR    EC interface stuck with 0xFF values. 
 */
EFI_STATUS
DrainOBFAndTestInterface (
  VOID
)
{
  UINT8  Status;
  UINT8  Data;
  UINTN  SeenFFs;
  UINTN  Loops;

  Loops = 0;
  SeenFFs = 0;
  Status = IoRead8 (EC_SC);
  while ((Status & EC_OBF) != 0 && Loops < 100) {
    Loops++;
    if (Status == 0xff) {
      SeenFFs++;
    }

    Data = IoRead8 (EC_DATA);
    if (Data == 0xFF) {
      SeenFFs++;
    }
    Status = IoRead8 (EC_SC);
  }

  if (Loops == 100 && SeenFFs > 100) {
    return EFI_DEVICE_ERROR;
  }

  return (Status & EC_OBF) == 0 ? EFI_SUCCESS : EFI_TIMEOUT;
}

//
// Unified Driver Functions
//

/**
  Get a string from an SMBIOS record by its 1-based string number.

  @param[in] Record        Pointer to the SMBIOS record.
  @param[in] StringNumber  1-based index of the string.

  @return Pointer to the string, or NULL if not present.
**/
STATIC
CONST CHAR8 *
GetSmbiosString (
  IN EFI_SMBIOS_TABLE_HEADER  *Record,
  IN SMBIOS_TABLE_STRING      StringNumber
  )
{
  CHAR8  *String;

  if (StringNumber == 0) {
    return NULL;
  }

  String = (CHAR8 *)Record + Record->Length;
  while (--StringNumber != 0) {
    if (*String == '\0') {
      return NULL;
    }

    String += AsciiStrSize (String);
  }

  return (*String != '\0') ? String : NULL;
}

/**
  Get the mainboard vendor from SMBIOS.

  @param[out] Vendor1  Pointer to the system vendor string, or NULL if not available.
  @param[out] Vendor2  Pointer to the baseboard vendor string, or NULL if not available.
**/
STATIC
EFI_STATUS
GetMainboardVendor (
  OUT CONST CHAR8 **Vendor1,
  OUT CONST CHAR8 **Vendor2
  )
{
  EFI_STATUS               Status;
  EFI_SMBIOS_PROTOCOL      *Smbios;
  EFI_SMBIOS_HANDLE        Handle;
  EFI_SMBIOS_TABLE_HEADER  *Record;
  EFI_SMBIOS_TYPE          Type;

  if (Vendor1 == NULL || Vendor2 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Vendor1 = NULL;
  *Vendor2 = NULL;

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  Handle = SMBIOS_HANDLE_PI_RESERVED;
  while (*Vendor1 == NULL || *Vendor2 == NULL) {
    Status = Smbios->GetNext (Smbios, &Handle, NULL, &Record, NULL);
    if (EFI_ERROR (Status) || (Record == NULL)) {
      break;
    }

    Type = Record->Type;
    if (Type == SMBIOS_TYPE_END_OF_TABLE) {
      break;
    }

    if ((Type == SMBIOS_TYPE_SYSTEM_INFORMATION) &&
        (Record->Length > OFFSET_OF (SMBIOS_TABLE_TYPE1, Manufacturer)))
    {
      *Vendor1 = GetSmbiosString (Record, ((SMBIOS_TABLE_TYPE1 *)Record)->Manufacturer);
    }

    if ((Type == SMBIOS_TYPE_BASEBOARD_INFORMATION) &&
        (Record->Length > OFFSET_OF (SMBIOS_TABLE_TYPE2, Manufacturer)))
    {
      *Vendor2 = GetSmbiosString (Record, ((SMBIOS_TABLE_TYPE2 *)Record)->Manufacturer);
    }
  }

  return (*Vendor1 != NULL || *Vendor2 != NULL) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

/**
  Check whether an EC profile is known to be used by the given vendor.

  @param[in] Profile  EC profile.
  @param[in] Vendor1   Mainboard vendor string, or NULL if unknown.
  @param[in] Vendor2   Baseboard vendor string, or NULL if unknown.

  @retval TRUE   The profile should be probed.
  @retval FALSE  The profile can be skipped.
**/
STATIC
BOOLEAN
IsVendorMatch (
  IN CONST EC_PROFILE  *Profile,
  IN CONST CHAR8       *Vendor1,
  IN CONST CHAR8       *Vendor2
  )
{
  UINTN  i;

  //
  // Probe everything if vendor or lookup table is unavailable
  //
  if ((Vendor1 == NULL && Vendor2 == NULL) || (Profile->Vendors == NULL)) {
    return TRUE;
  }

  for (i = 0; Profile->Vendors[i] != NULL; i++) {
    if ((Vendor1 != NULL && AsciiStriCmp (Vendor1, Profile->Vendors[i]) == 0) ||
        (Vendor2 != NULL && AsciiStriCmp (Vendor2, Profile->Vendors[i]) == 0)) {
      return TRUE;
    }
  }

  return FALSE;
}

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
  UINTN        i;
  CONST CHAR8  *Vendor1;
  CONST CHAR8  *Vendor2;

  Vendor1 = NULL;
  Vendor2 = NULL;
  GetMainboardVendor (&Vendor1, &Vendor2);

  DEBUG ((DEBUG_INFO, "EcAcpiBattery: Mainboard vendor: %a/%a\n",
          (Vendor1 != NULL) ? Vendor1 : "<unknown>",
          (Vendor2 != NULL) ? Vendor2 : "<unknown>"));

  for (i = 0; i < (sizeof (mEcProfiles) / sizeof (mEcProfiles[0])); i++) {
    if (!IsVendorMatch (&mEcProfiles[i], Vendor1, Vendor2)) {
      DEBUG ((DEBUG_VERBOSE, "EcAcpiBattery: Skipping %a, vendor mismatch\n", mEcProfiles[i].Name));
      continue;
    }

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

