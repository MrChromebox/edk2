/** @file
  Boot splash NVRAM configuration definitions.

  Copyright (c) 2026
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BOOT_SPLASH_CONFIG_H_
#define BOOT_SPLASH_CONFIG_H_

#define UEFI_PAYLOAD_BOOT_SPLASH_GUID \
  { \
    0x8a7c3e21, 0x4f5b, 0x4d91, { 0x9c, 0x2e, 0x1a, 0x3b, 0x5d, 0x7f, 0x8e, 0x90 } \
  }

#define BOOT_SPLASH_CONFIG_FORMSET_GUID \
  { \
    0xb2d4e6f8, 0x1a3c, 0x4e5f, { 0x8b, 0x9d, 0x0c, 0x2e, 0x4a, 0x6f, 0x8d, 0xb1 } \
  }

#define BOOT_SPLASH_ENABLE_VARIABLE_NAME  L"BootSplashEnable"
#define BOOT_SPLASH_TYPE_VARIABLE_NAME    L"BootSplashType"
#define BOOT_SPLASH_PATH_VARIABLE_NAME    L"BootSplashPath"

#define BOOT_SPLASH_VARIABLE_ATTRIBUTES  \
  (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

#define BOOT_SPLASH_TYPE_DEFAULT  0
#define BOOT_SPLASH_TYPE_CUSTOM   1

#define BOOT_SPLASH_ENABLE_DEFAULT      1
#define BOOT_SPLASH_TYPE_DEFAULT_VALUE  BOOT_SPLASH_TYPE_DEFAULT

#define BOOT_SPLASH_PATH_MAX_SIZE  512

#pragma pack(1)

typedef struct {
  UINT8    Value;
} BOOT_SPLASH_UINT8_VAR;

#pragma pack()

#ifndef VFRCOMPILE
extern EFI_GUID  gUefiPayloadBootSplashGuid;
extern EFI_GUID  gBootSplashConfigFormSetGuid;
#endif

#endif
