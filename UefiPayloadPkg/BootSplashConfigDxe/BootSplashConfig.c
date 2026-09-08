/** @file
  Boot Splash Config DXE driver.

  Provides a Platform Setup formset to enable/disable the boot splash,
  choose default vs custom logo, and browse for a custom BMP file.

  Copyright (c) 2026
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "BootSplashConfig.h"

extern UINT8  BootSplashConfigVfrBin[];
extern UINT8  BootSplashConfigDxeStrings[];

STATIC BOOT_SPLASH_CONFIG_PRIVATE  mBootSplashPrivate;

STATIC HII_VENDOR_DEVICE_PATH  mBootSplashVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    BOOT_SPLASH_CONFIG_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**
  Seed missing boot splash NVRAM variables with defaults.
**/
STATIC
VOID
BootSplashSeedDefaults (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  UINT8       Value;

  DataSize = sizeof (Value);
  Status   = gRT->GetVariable (
                    BOOT_SPLASH_ENABLE_VARIABLE_NAME,
                    &gUefiPayloadBootSplashGuid,
                    NULL,
                    &DataSize,
                    &Value
                    );
  if (Status == EFI_NOT_FOUND) {
    Value = BOOT_SPLASH_ENABLE_DEFAULT;
    Status = gRT->SetVariable (
                    BOOT_SPLASH_ENABLE_VARIABLE_NAME,
                    &gUefiPayloadBootSplashGuid,
                    BOOT_SPLASH_VARIABLE_ATTRIBUTES,
                    sizeof (Value),
                    &Value
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: failed to seed BootSplashEnable: %r\n", __func__, Status));
    }
  }

  DataSize = sizeof (Value);
  Status   = gRT->GetVariable (
                    BOOT_SPLASH_TYPE_VARIABLE_NAME,
                    &gUefiPayloadBootSplashGuid,
                    NULL,
                    &DataSize,
                    &Value
                    );
  if (Status == EFI_NOT_FOUND) {
    Value = BOOT_SPLASH_TYPE_DEFAULT_VALUE;
    Status = gRT->SetVariable (
                    BOOT_SPLASH_TYPE_VARIABLE_NAME,
                    &gUefiPayloadBootSplashGuid,
                    BOOT_SPLASH_VARIABLE_ATTRIBUTES,
                    sizeof (Value),
                    &Value
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "%a: failed to seed BootSplashType: %r\n", __func__, Status));
    }
  }
}

/**
  Update the path display string from BootSplashPath.
**/
STATIC
VOID
BootSplashUpdatePathDisplay (
  IN EFI_HII_HANDLE  HiiHandle
  )
{
  EFI_STATUS                Status;
  UINTN                     DataSize;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  CHAR16                    *PathText;

  DevicePath = AllocateZeroPool (BOOT_SPLASH_PATH_MAX_SIZE);
  if (DevicePath == NULL) {
    return;
  }

  DataSize = BOOT_SPLASH_PATH_MAX_SIZE;
  Status   = gRT->GetVariable (
                    BOOT_SPLASH_PATH_VARIABLE_NAME,
                    &gUefiPayloadBootSplashGuid,
                    NULL,
                    &DataSize,
                    DevicePath
                    );
  if (EFI_ERROR (Status) || (DataSize < sizeof (EFI_DEVICE_PATH_PROTOCOL)) ||
      !IsDevicePathValid (DevicePath, DataSize))
  {
    HiiSetString (HiiHandle, STRING_TOKEN (STR_BOOT_SPLASH_PATH_VALUE), L"<none selected>", NULL);
    FreePool (DevicePath);
    return;
  }

  PathText = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  FreePool (DevicePath);
  if (PathText == NULL) {
    HiiSetString (HiiHandle, STRING_TOKEN (STR_BOOT_SPLASH_PATH_VALUE), L"<none selected>", NULL);
    return;
  }

  HiiSetString (HiiHandle, STRING_TOKEN (STR_BOOT_SPLASH_PATH_VALUE), PathText, NULL);
  FreePool (PathText);
}

/**
  Persist a chosen file device path as BootSplashPath.
**/
STATIC
EFI_STATUS
BootSplashSavePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  UINTN  PathSize;

  if (FilePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PathSize = GetDevicePathSize (FilePath);
  if ((PathSize == 0) || (PathSize > BOOT_SPLASH_PATH_MAX_SIZE)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return gRT->SetVariable (
                BOOT_SPLASH_PATH_VARIABLE_NAME,
                &gUefiPayloadBootSplashGuid,
                BOOT_SPLASH_VARIABLE_ATTRIBUTES,
                PathSize,
                FilePath
                );
}

/**
  ExtractConfig - EFI varstores are handled by the browser.
**/
EFI_STATUS
EFIAPI
BootSplashExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  RouteConfig - EFI varstores are handled by the browser.
**/
EFI_STATUS
EFIAPI
BootSplashRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;
  return EFI_NOT_FOUND;
}

/**
  HII callback for browse action and form open refresh.
**/
EFI_STATUS
EFIAPI
BootSplashCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *FileDevPath;
  BOOT_SPLASH_CONFIG_PRIVATE  *Private;

  if ((This == NULL) || (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
  Private        = BASE_CR (This, BOOT_SPLASH_CONFIG_PRIVATE, ConfigAccess);

  if (Action == EFI_BROWSER_ACTION_FORM_OPEN) {
    BootSplashUpdatePathDisplay (Private->HiiHandle);
    return EFI_SUCCESS;
  }

  if ((Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_CHANGED)) {
    return EFI_UNSUPPORTED;
  }

  if (QuestionId != BOOT_SPLASH_BROWSE_QUESTION_ID) {
    return EFI_SUCCESS;
  }

  FileDevPath = NULL;
  Status      = ChooseFile (NULL, L".bmp", NULL, &FileDevPath);
  if (EFI_ERROR (Status) || (FileDevPath == NULL)) {
    return EFI_SUCCESS;
  }

  Status = BootSplashSavePath (FileDevPath);
  if (!EFI_ERROR (Status)) {
    BootSplashUpdatePathDisplay (Private->HiiHandle);
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_APPLY;
  }

  FreePool (FileDevPath);
  return EFI_SUCCESS;
}

/**
  Install HII packages and device path / ConfigAccess protocols.
**/
STATIC
EFI_STATUS
InstallHiiPages (
  VOID
  )
{
  EFI_STATUS  Status;

  mBootSplashPrivate.ConfigAccess.ExtractConfig = BootSplashExtractConfig;
  mBootSplashPrivate.ConfigAccess.RouteConfig   = BootSplashRouteConfig;
  mBootSplashPrivate.ConfigAccess.Callback      = BootSplashCallback;
  mBootSplashPrivate.DriverHandle               = NULL;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mBootSplashPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mBootSplashVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mBootSplashPrivate.ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  mBootSplashPrivate.HiiHandle = HiiAddPackages (
                                   &gBootSplashConfigFormSetGuid,
                                   mBootSplashPrivate.DriverHandle,
                                   BootSplashConfigDxeStrings,
                                   BootSplashConfigVfrBin,
                                   NULL
                                   );
  if (mBootSplashPrivate.HiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mBootSplashPrivate.DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mBootSplashVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &mBootSplashPrivate.ConfigAccess,
           NULL
           );
    return EFI_OUT_OF_RESOURCES;
  }

  BootSplashUpdatePathDisplay (mBootSplashPrivate.HiiHandle);
  return EFI_SUCCESS;
}

/**
  Driver entry point.
**/
EFI_STATUS
EFIAPI
BootSplashConfigDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  BootSplashSeedDefaults ();
  return InstallHiiPages ();
}
