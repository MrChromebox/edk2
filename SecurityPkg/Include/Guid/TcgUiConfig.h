/** @file
  Shared definitions for TCG/TPM setup UI mode selection.

  Values are persisted in a non-volatile UEFI variable so TPM2 and OPAL setup
  drivers can select the appropriate HII form at boot. Platform PCDs supply the
  defaults on first boot.

  Copyright (c) 2026, MrChromebox. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TCG_UI_CONFIG_H_
#define TCG_UI_CONFIG_H_

//
// {A4E8C2F1-6B3D-4A9E-9F12-8C7D5E4B2A10}
//
#define TCG_UI_CONFIG_FORMSET_GUID \
  { 0xa4e8c2f1, 0x6b3d, 0x4a9e, { 0x9f, 0x12, 0x8c, 0x7d, 0x5e, 0x4b, 0x2a, 0x10 } }

#define TCG_UI_CONFIG_VAR_NAME  L"TcgUiConfig"

//
// Advanced exposes the full technical setup forms (default).
// Standard exposes the simplified consumer-oriented forms.
//
#define TCG_UI_MODE_ADVANCED   0
#define TCG_UI_MODE_STANDARD   1
#define TCG_UI_MODE_DEFAULT    TCG_UI_MODE_ADVANCED

#pragma pack(1)
typedef struct {
  UINT8    Tpm2UiMode;
  UINT8    TcgStorageUiMode;
} TCG_UI_CONFIG_VARSTORE_DATA;
#pragma pack()

extern EFI_GUID  gTcgUiConfigFormSetGuid;

#endif // TCG_UI_CONFIG_H_
