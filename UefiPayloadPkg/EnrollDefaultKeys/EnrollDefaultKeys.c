/** @file
   Enroll default PK, KEK, DB and DBX

   Copyright (C) 2014, Red Hat, Inc.

   This program and the accompanying materials are licensed and made available
   under the terms and conditions of the BSD License which accompanies this
   distribution. The full text of the license may be found at
   http://opensource.org/licenses/bsd-license.

   THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
   WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/

#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/GlobalVariable.h>
#include <Guid/ImageAuthentication.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include "EnrollDefaultKeys.h"

/**
   Enroll a set of certificates in a global variable, overwriting it.

   The variable will be rewritten with NV+BS+RT+AT attributes.

   @param[in] VariableName  The name of the variable to overwrite.

   @param[in] VendorGuid    The namespace (ie. vendor GUID) of the variable to
                           overwrite.

   @param[in] CertType      The GUID determining the type of all the
                           certificates in the set that is passed in. For
                           example, gEfiCertX509Guid stands for DER-encoded
                           X.509 certificates, while gEfiCertSha256Guid stands
                           for SHA256 image hashes.

   @param[in] ...           A list of

                             IN CONST UINT8    *Cert,
                             IN UINTN          CertSize,
                             IN CONST EFI_GUID *OwnerGuid

                           triplets. If the first component of a triplet is
                           NULL, then the other two components are not
                           accessed, and processing is terminated. The list of
                           certificates is enrolled in the variable specified,
                           overwriting it. The OwnerGuid component identifies
                           the agent installing the certificate.

   @retval EFI_INVALID_PARAMETER  The triplet list is empty (ie. the first Cert
                                 value is NULL), or one of the CertSize values
                                 is 0, or one of the CertSize values would
                                 overflow the accumulated UINT32 data size.

   @retval EFI_OUT_OF_RESOURCES   Out of memory while formatting variable
                                 payload.

   @retval EFI_SUCCESS            Enrollment successful; the variable has been
                                 overwritten (or created).

   @return                        Error codes from gRT->GetTime() and
                                 gRT->SetVariable().
 **/
STATIC
EFI_STATUS
EFIAPI
EnrollListOfCerts (
  IN CHAR16   *VariableName,
  IN EFI_GUID *VendorGuid,
  IN EFI_GUID *CertType,
  ...
  )
{
  UINTN DataSize;
  SINGLE_HEADER    *SingleHeader;
  REPEATING_HEADER *RepeatingHeader;
  VA_LIST Marker;
  CONST UINT8      *Cert;
  EFI_STATUS Status;
  UINT8            *Data;
  UINT8            *Position;

  Status = EFI_SUCCESS;

  //
  // compute total size first, for UINT32 range check, and allocation
  //
  DataSize = sizeof *SingleHeader;
  VA_START (Marker, CertType);
  for (Cert = VA_ARG (Marker, CONST UINT8 *);
       Cert != NULL;
       Cert = VA_ARG (Marker, CONST UINT8 *)) {
    UINTN CertSize;

    CertSize = VA_ARG (Marker, UINTN);
    (VOID)VA_ARG (Marker, CONST EFI_GUID *);

    if (CertSize == 0 ||
        CertSize > MAX_UINT32 - sizeof *RepeatingHeader ||
        DataSize > MAX_UINT32 - sizeof *RepeatingHeader - CertSize) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }
    DataSize += sizeof *RepeatingHeader + CertSize;
  }
  VA_END (Marker);

  if (DataSize == sizeof *SingleHeader) {
    Status = EFI_INVALID_PARAMETER;
  }
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: Invalid certificate parameters\n"));
    goto Out;
  }

  Data = AllocatePool (DataSize);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Out;
  }

  Position = Data;

  SingleHeader = (SINGLE_HEADER *)Position;
  Status = gRT->GetTime (&SingleHeader->TimeStamp, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "EnrollDefaultKeys: GetTime failed\n"));
    // Fill in dummy values
    SingleHeader->TimeStamp.Year       = 2018;
    SingleHeader->TimeStamp.Month      = 1;
    SingleHeader->TimeStamp.Day        = 1;
    SingleHeader->TimeStamp.Hour       = 0;
    SingleHeader->TimeStamp.Minute     = 0;
    SingleHeader->TimeStamp.Second     = 0;
    Status = EFI_SUCCESS;
  }
  SingleHeader->TimeStamp.Pad1       = 0;
  SingleHeader->TimeStamp.Nanosecond = 0;
  SingleHeader->TimeStamp.TimeZone   = 0;
  SingleHeader->TimeStamp.Daylight   = 0;
  SingleHeader->TimeStamp.Pad2       = 0;

  //
  // This looks like a bug in edk2. According to the UEFI specification,
  // dwLength is "The length of the entire certificate, including the length of
  // the header, in bytes". That shouldn't stop right after CertType -- it
  // should include everything below it.
  //
  SingleHeader->dwLength         = sizeof *SingleHeader - sizeof SingleHeader->TimeStamp;
  SingleHeader->wRevision        = 0x0200;
  SingleHeader->wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&SingleHeader->CertType, &gEfiCertPkcs7Guid);
  Position += sizeof *SingleHeader;

  VA_START (Marker, CertType);
  for (Cert = VA_ARG (Marker, CONST UINT8 *);
       Cert != NULL;
       Cert = VA_ARG (Marker, CONST UINT8 *)) {
    UINTN CertSize;
    CONST EFI_GUID   *OwnerGuid;

    CertSize  = VA_ARG (Marker, UINTN);
    OwnerGuid = VA_ARG (Marker, CONST EFI_GUID *);

    RepeatingHeader = (REPEATING_HEADER *)Position;
    CopyGuid (&RepeatingHeader->SignatureType, CertType);
    RepeatingHeader->SignatureListSize   =
      (UINT32)(sizeof *RepeatingHeader + CertSize);
    RepeatingHeader->SignatureHeaderSize = 0;
    RepeatingHeader->SignatureSize       =
      (UINT32)(sizeof RepeatingHeader->SignatureOwner + CertSize);
    CopyGuid (&RepeatingHeader->SignatureOwner, OwnerGuid);
    Position += sizeof *RepeatingHeader;

    CopyMem (Position, Cert, CertSize);
    Position += CertSize;
  }
  VA_END (Marker);

  ASSERT (Data + DataSize == Position);

  Status = gRT->SetVariable (VariableName, VendorGuid,
           (EFI_VARIABLE_NON_VOLATILE |
            EFI_VARIABLE_RUNTIME_ACCESS |
            EFI_VARIABLE_BOOTSERVICE_ACCESS |
            EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
           DataSize, Data);

  FreePool (Data);

Out:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: %a(\"%s\", %g): %r\n", __FUNCTION__, VariableName,
      VendorGuid, Status));
  }
  return Status;
}

/**
  Read a UEFI variable into a caller-allocated buffer, enforcing an exact size.

  @param[in] VariableName  The name of the variable to read; passed to
                           gRT->GetVariable().

  @param[in] VendorGuid    The vendor (namespace) GUID of the variable to read;
                           passed to gRT->GetVariable().

  @param[out] Data         The caller-allocated buffer that is supposed to
                           receive the variable's contents. On error, the
                           contents of Data are indeterminate.

  @param[in] DataSize      The size in bytes that the caller requires the UEFI
                           variable to have. The caller is responsible for
                           providing room for DataSize bytes in Data.

  @param[in] AllowMissing  If FALSE, the variable is required to exist. If
                           TRUE, the variable is permitted to be missing.

  @retval EFI_SUCCESS           The UEFI variable exists, has the required size
                                (DataSize), and has been read into Data.

  @retval EFI_SUCCESS           The UEFI variable doesn't exist, and
                                AllowMissing is TRUE. DataSize bytes in Data
                                have been zeroed out.

  @retval EFI_NOT_FOUND         The UEFI variable doesn't exist, and
                                AllowMissing is FALSE.

  @retval EFI_BUFFER_TOO_SMALL  The UEFI variable exists, but its size is
                                greater than DataSize.

  @retval EFI_PROTOCOL_ERROR    The UEFI variable exists, but its size is
                                smaller than DataSize.

  @return                       Error codes propagated from gRT->GetVariable().

**/
STATIC
EFI_STATUS
EFIAPI
GetExact (
  IN CHAR16   *VariableName,
  IN EFI_GUID *VendorGuid,
  OUT VOID    *Data,
  IN UINTN DataSize,
  IN BOOLEAN AllowMissing
  )
{
  UINTN Size;
  EFI_STATUS Status;

  Size = DataSize;
  Status = gRT->GetVariable (VariableName, VendorGuid, NULL, &Size, Data);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND && AllowMissing) {
      ZeroMem (Data, DataSize);
      return EFI_SUCCESS;
    }

    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: GetVariable(\"%s\", %g): %r\n", VariableName,
      VendorGuid, Status));
    return Status;
  }

  if (Size != DataSize) {
    DEBUG ((EFI_D_INFO, "EnrollDefaultKeys: GetVariable(\"%s\", %g): expected size 0x%Lx, "
      "got 0x%Lx\n", VariableName, VendorGuid, (UINT64)DataSize, (UINT64)Size));
    return EFI_PROTOCOL_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Populate a SETTINGS structure from the underlying UEFI variables.

  The following UEFI variables are standard variables:
  - L"SetupMode"  (EFI_SETUP_MODE_NAME)
  - L"SecureBoot" (EFI_SECURE_BOOT_MODE_NAME)
  - L"VendorKeys" (EFI_VENDOR_KEYS_VARIABLE_NAME)

  The following UEFI variables are edk2 extensions:
  - L"SecureBootEnable" (EFI_SECURE_BOOT_ENABLE_NAME)
  - L"CustomMode"       (EFI_CUSTOM_MODE_NAME)

  The L"SecureBootEnable" UEFI variable is permitted to be missing, in which
  case the corresponding field in the SETTINGS object will be zeroed out. The
  rest of the covered UEFI variables are required to exist; otherwise, the
  function will fail.

  @param[out] Settings      The SETTINGS object to fill.
  @param[in]  AllowMissing  If FALSE, the required variables must exist; if
                            TRUE, the function can succeed even if some 
                            variables are missing.

  @retval EFI_SUCCESS       Settings has been populated.

  @return                  Error codes propagated from the GetExact() function. The
                           contents of Settings are indeterminate.
**/
STATIC
EFI_STATUS
EFIAPI
GetSettings (
  OUT SETTINGS *Settings,
  BOOLEAN AllowMissing
  )
{
  EFI_STATUS Status;

  ZeroMem (Settings, sizeof(SETTINGS));

  Status = GetExact (EFI_SETUP_MODE_NAME, &gEfiGlobalVariableGuid,
         &Settings->SetupMode, sizeof Settings->SetupMode, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid,
         &Settings->SecureBoot, sizeof Settings->SecureBoot, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_SECURE_BOOT_ENABLE_NAME,
         &gEfiSecureBootEnableDisableGuid, &Settings->SecureBootEnable,
         sizeof Settings->SecureBootEnable, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
         &Settings->CustomMode, sizeof Settings->CustomMode, AllowMissing);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetExact (EFI_VENDOR_KEYS_VARIABLE_NAME, &gEfiGlobalVariableGuid,
         &Settings->VendorKeys, sizeof Settings->VendorKeys, AllowMissing);
  return Status;
}

/**

  Print the contents of a SETTINGS structure to the UEFI console.

  @param[in] Settings  The SETTINGS object to print the contents of.

**/
STATIC
VOID
EFIAPI
PrintSettings (
  IN CONST SETTINGS *Settings
  )
{
  DEBUG ((EFI_D_INFO, "EnrollDefaultKeys: SetupMode=%d SecureBoot=%d SecureBootEnable=%d "
    "CustomMode=%d VendorKeys=%d\n", Settings->SetupMode, Settings->SecureBoot,
    Settings->SecureBootEnable, Settings->CustomMode, Settings->VendorKeys));
}

/**
  Install SecureBoot certificates once the VariableDriver is running.

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
EFIAPI
EnrollDefaultKeys (
  IN EFI_EVENT                      Event,
  IN VOID                           *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Protocol;
  SETTINGS Settings;

  UINT8 *DbMicrosoftUefi2011 = 0;
  UINTN DbMicrosoftUefi2011Size;
  UINT8 *DbMicrosoftUefi2023 = 0;
  UINTN DbMicrosoftUefi2023Size;
  UINT8 *DbMicrosoftWin2011 = 0;
  UINTN DbMicrosoftWin2011Size;
  UINT8 *DbMicrosoftWinuefi2023 = 0;
  UINTN DbMicrosoftWinuefi2023Size;
  UINT8 *DbxMicrosoftUpdate = 0;
  UINTN DbxMicrosoftUpdateSize;
  UINT8 *KekMicrosoft2011 = 0;
  UINTN KekMicrosoft2011Size;
  UINT8 *KekMicrosoft2023 = 0;
  UINTN KekMicrosoft2023Size;
  UINT8 *KekMicrosoftUefi2023 = 0;
  UINTN KekMicrosoftUefi2023Size;
  UINT8 *PkMicrosoftOem2023 = 0;
  UINTN PkMicrosoftOem2023Size;

  Status = gBS->LocateProtocol (&gEfiVariableWriteArchProtocolGuid, NULL, (VOID **)&Protocol);
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = GetSettings (&Settings, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: Failed to get current settings\n"));
    return;
  }

  if (Settings.SetupMode != SETUP_MODE) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: already in User Mode\n"));
    return;
  }
  PrintSettings (&Settings);

  if (Settings.CustomMode != CUSTOM_SECURE_BOOT_MODE) {
    Settings.CustomMode = CUSTOM_SECURE_BOOT_MODE;
    Status = gRT->SetVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
             (EFI_VARIABLE_NON_VOLATILE |
              EFI_VARIABLE_BOOTSERVICE_ACCESS),
             sizeof Settings.CustomMode, &Settings.CustomMode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: SetVariable(\"%s\", %g): %r\n", EFI_CUSTOM_MODE_NAME,
        &gEfiCustomModeEnableGuid, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }

  Status = GetSectionFromAnyFv(&gMicrosoftDbUefi2011Guid, EFI_SECTION_RAW, 0, (void **)&DbMicrosoftUefi2011, &DbMicrosoftUefi2011Size);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftDbUefi2023Guid, EFI_SECTION_RAW, 0, (void **)&DbMicrosoftUefi2023, &DbMicrosoftUefi2023Size);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftDbWin2011Guid, EFI_SECTION_RAW, 0, (void **)&DbMicrosoftWin2011, &DbMicrosoftWin2011Size);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftDbWinUefi2023Guid, EFI_SECTION_RAW, 0, (void **)&DbMicrosoftWinuefi2023, &DbMicrosoftWinuefi2023Size);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftDbxUpdateGuid, EFI_SECTION_RAW, 0, (void **)&DbxMicrosoftUpdate, &DbxMicrosoftUpdateSize);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftKek2011Guid, EFI_SECTION_RAW, 0, (void **)&KekMicrosoft2011, &KekMicrosoft2011Size);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftKek2023Guid, EFI_SECTION_RAW, 0, (void **)&KekMicrosoft2023, &KekMicrosoft2023Size);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftKekUefi2023Guid, EFI_SECTION_RAW, 0, (void **)&KekMicrosoftUefi2023, &KekMicrosoftUefi2023Size);
  ASSERT_EFI_ERROR(Status);
  Status = GetSectionFromAnyFv(&gMicrosoftPkOem2023Guid, EFI_SECTION_RAW, 0, (void **)&PkMicrosoftOem2023, &PkMicrosoftOem2023Size);
  ASSERT_EFI_ERROR(Status);

  Status = gRT->SetVariable (EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid,
           (EFI_VARIABLE_NON_VOLATILE |
            EFI_VARIABLE_RUNTIME_ACCESS |
            EFI_VARIABLE_BOOTSERVICE_ACCESS |
            EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS),
            DbxMicrosoftUpdateSize, DbxMicrosoftUpdate);
  ASSERT_EFI_ERROR (Status);

  Status = EnrollListOfCerts (
    EFI_IMAGE_SECURITY_DATABASE,
    &gEfiImageSecurityDatabaseGuid,
    &gEfiCertX509Guid,
    DbMicrosoftUefi2011, DbMicrosoftUefi2011Size, DbMicrosoftWinuefi2023Size,
    DbMicrosoftUefi2023, DbMicrosoftUefi2023Size, DbMicrosoftWinuefi2023Size,
    DbMicrosoftWin2011, DbMicrosoftWin2011Size, DbMicrosoftWinuefi2023Size,
    DbMicrosoftWinuefi2023, DbMicrosoftWinuefi2023Size, DbMicrosoftWinuefi2023Size,
    NULL);
  ASSERT_EFI_ERROR (Status);

  Status = EnrollListOfCerts (
    EFI_KEY_EXCHANGE_KEY_NAME,
    &gEfiGlobalVariableGuid,
    &gEfiCertX509Guid,
    KekMicrosoft2011, KekMicrosoft2011Size, gMicrosoftVendorGuid,
    KekMicrosoft2023, KekMicrosoft2023Size, gMicrosoftVendorGuid,
    KekMicrosoftUefi2023, KekMicrosoftUefi2023Size, gMicrosoftVendorGuid,
    NULL);
  ASSERT_EFI_ERROR (Status);

  Status = EnrollListOfCerts (
    EFI_PLATFORM_KEY_NAME,
    &gEfiGlobalVariableGuid,
    &gEfiCertX509Guid,
    PkMicrosoftOem2023, PkMicrosoftOem2023Size, gMicrosoftVendorGuid,
    NULL);
  ASSERT_EFI_ERROR (Status);

  FreePool(DbMicrosoftUefi2011);
  FreePool(DbMicrosoftUefi2023);
  FreePool(DbMicrosoftWin2011);
  FreePool(DbMicrosoftWinuefi2023);
  FreePool(DbxMicrosoftUpdate);
  FreePool(KekMicrosoft2011);
  FreePool(KekMicrosoft2023);
  FreePool(KekMicrosoftUefi2023);
  FreePool(PkMicrosoftOem2023);

  Settings.CustomMode = STANDARD_SECURE_BOOT_MODE;
  Status = gRT->SetVariable (EFI_CUSTOM_MODE_NAME, &gEfiCustomModeEnableGuid,
           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof Settings.CustomMode, &Settings.CustomMode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: SetVariable(\"%s\", %g): %r\n", EFI_CUSTOM_MODE_NAME,
      &gEfiCustomModeEnableGuid, Status));
    ASSERT_EFI_ERROR (Status);
  }

  // FIXME: Force SecureBoot to ON. The AuthService will do this if authenticated variables
  // are supported, which aren't as the SMM handler isn't able to verify them.

  Settings.SecureBootEnable = SECURE_BOOT_DISABLE;
  Status = gRT->SetVariable (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid,
           EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           sizeof Settings.SecureBootEnable, &Settings.SecureBootEnable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: SetVariable(\"%s\", %g): %r\n", EFI_SECURE_BOOT_ENABLE_NAME,
      &gEfiSecureBootEnableDisableGuid, Status));
    ASSERT_EFI_ERROR (Status);
  }

  Settings.SecureBoot = SECURE_BOOT_DISABLE;
  Status = gRT->SetVariable (EFI_SECURE_BOOT_MODE_NAME, &gEfiGlobalVariableGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
           sizeof Settings.SecureBoot, &Settings.SecureBoot);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: SetVariable(\"%s\", %g): %r\n", EFI_SECURE_BOOT_MODE_NAME,
      &gEfiGlobalVariableGuid, Status));
    ASSERT_EFI_ERROR (Status);
  }

  Status = GetSettings (&Settings, FALSE);
  ASSERT_EFI_ERROR (Status);

  //
  // Final sanity check:
  //
  //                                 [SetupMode]
  //                        (read-only, standardized by UEFI)
  //                                /                \_
  //                               0               1, default
  //                              /                    \_
  //                      PK enrolled                   no PK enrolled yet,
  //              (this is called "User Mode")          PK enrollment possible
  //                             |
  //                             |
  //                     [SecureBootEnable]
  //         (read-write, edk2-specific, boot service only)
  //                /                           \_
  //               0                         1, default
  //              /                               \_
  //       [SecureBoot]=0                     [SecureBoot]=1
  // (read-only, standardized by UEFI)  (read-only, standardized by UEFI)
  //     images are not verified         images are verified, platform is
  //                                      operating in Secure Boot mode
  //                                                 |
  //                                                 |
  //                                           [CustomMode]
  //                          (read-write, edk2-specific, boot service only)
  //                                /                           \_
  //                          0, default                         1
  //                              /                               \_
  //                      PK, KEK, db, dbx                PK, KEK, db, dbx
  //                    updates are verified          updates are not verified
  //

  PrintSettings (&Settings);

  if (Settings.SetupMode != 0 || Settings.SecureBoot != 1 ||
      Settings.SecureBootEnable != 1 || Settings.CustomMode != 0 ||
      Settings.VendorKeys != 0) {
    DEBUG ((DEBUG_ERROR, "EnrollDefaultKeys: disabled\n"));
    return;
  }

  DEBUG ((EFI_D_INFO, "EnrollDefaultKeys: SecureBoot enabled\n"));
}

EFI_STATUS
EFIAPI
DriverEntry (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE            *SystemTable
  )
{
  VOID *Registration;

  DEBUG ((DEBUG_INFO, "EnrollDefaultKeys hook\n"));

  //
  // Create event callback, because we need access variable on SecureBootPolicyVariable
  // We should use VariableWriteArch instead of VariableArch, because Variable driver
  // may update SecureBoot value based on last setting.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    EnrollDefaultKeys,
    NULL,
    &Registration);

  return EFI_SUCCESS;
}
