/** @file
  TCG/TPM setup UI configuration helpers.

  Copyright (c) 2026, MrChromebox. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef TCG_UI_CONFIG_LIB_H_
#define TCG_UI_CONFIG_LIB_H_

#include <Uefi.h>
#include <Guid/TcgUiConfig.h>

VOID
EFIAPI
TcgUiConfigGetDefaults (
  OUT TCG_UI_CONFIG_VARSTORE_DATA  *Config
  );

EFI_STATUS
EFIAPI
TcgUiConfigLoad (
  OUT TCG_UI_CONFIG_VARSTORE_DATA  *Config
  );

VOID
EFIAPI
TcgUiConfigEnsureVariable (
  VOID
  );

BOOLEAN
EFIAPI
TcgUiConfigIsTpm2SimpleUiEnabled (
  VOID
  );

BOOLEAN
EFIAPI
TcgUiConfigIsTcgStorageSimpleUiEnabled (
  VOID
  );

#endif // TCG_UI_CONFIG_LIB_H_
