/** @file
  Boot Splash Config DXE driver header.

  Copyright (c) 2026
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BOOT_SPLASH_CONFIG_DXE_H_
#define BOOT_SPLASH_CONFIG_DXE_H_

#include <Uefi.h>
#include <Guid/BootSplashConfig.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FileExplorerLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigAccess.h>

#define BOOT_SPLASH_FORM_ID             0x1000
#define BOOT_SPLASH_BROWSE_QUESTION_ID  0x1001

typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HANDLE                        DriverHandle;
  EFI_HII_HANDLE                    HiiHandle;
} BOOT_SPLASH_CONFIG_PRIVATE;

#endif
