/** @file
  TCG/TPM setup UI configuration helpers.

  Copyright (c) 2026, MrChromebox. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/TcgUiConfigLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC
VOID
TcgUiConfigSanitize (
  IN OUT TCG_UI_CONFIG_VARSTORE_DATA  *Config
  )
{
  if (Config->Tpm2UiMode != TCG_UI_MODE_STANDARD) {
    Config->Tpm2UiMode = TCG_UI_MODE_ADVANCED;
  }

  if (Config->TcgStorageUiMode != TCG_UI_MODE_STANDARD) {
    Config->TcgStorageUiMode = TCG_UI_MODE_ADVANCED;
  }
}

VOID
EFIAPI
TcgUiConfigGetDefaults (
  OUT TCG_UI_CONFIG_VARSTORE_DATA  *Config
  )
{
  ASSERT (Config != NULL);

  Config->Tpm2UiMode       = PcdGetBool (PcdTcg2ConfigSimpleUi) ? TCG_UI_MODE_STANDARD : TCG_UI_MODE_ADVANCED;
  Config->TcgStorageUiMode = PcdGetBool (PcdTcgStorageSimpleUi) ? TCG_UI_MODE_STANDARD : TCG_UI_MODE_ADVANCED;

  TcgUiConfigSanitize (Config);
}

EFI_STATUS
EFIAPI
TcgUiConfigLoad (
  OUT TCG_UI_CONFIG_VARSTORE_DATA  *Config
  )
{
  EFI_STATUS  Status;
  UINTN       Size;

  if (Config == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Size   = sizeof (*Config);
  Status = gRT->GetVariable (
                  TCG_UI_CONFIG_VAR_NAME,
                  &gTcgUiConfigFormSetGuid,
                  NULL,
                  &Size,
                  Config
                  );
  if (!EFI_ERROR (Status) && (Size == sizeof (*Config))) {
    TcgUiConfigSanitize (Config);
    return EFI_SUCCESS;
  }

  TcgUiConfigGetDefaults (Config);
  return EFI_NOT_FOUND;
}

VOID
EFIAPI
TcgUiConfigEnsureVariable (
  VOID
  )
{
  EFI_STATUS                    Status;
  TCG_UI_CONFIG_VARSTORE_DATA   Config;
  UINTN                         Size;

  Size   = sizeof (Config);
  Status = gRT->GetVariable (
                  TCG_UI_CONFIG_VAR_NAME,
                  &gTcgUiConfigFormSetGuid,
                  NULL,
                  &Size,
                  &Config
                  );
  if (!EFI_ERROR (Status) && (Size == sizeof (Config))) {
    return;
  }

  TcgUiConfigGetDefaults (&Config);
  TcgUiConfigSanitize (&Config);

  Status = gRT->SetVariable (
                  TCG_UI_CONFIG_VAR_NAME,
                  &gTcgUiConfigFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (Config),
                  &Config
                  );
  DEBUG ((
    DEBUG_INFO,
    "TcgUiConfigLib: seeded %s (tpm2=%u tcgStorage=%u) %r\n",
    TCG_UI_CONFIG_VAR_NAME,
    Config.Tpm2UiMode,
    Config.TcgStorageUiMode,
    Status
    ));
}

BOOLEAN
EFIAPI
TcgUiConfigIsTpm2SimpleUiEnabled (
  VOID
  )
{
  TCG_UI_CONFIG_VARSTORE_DATA  Config;

  TcgUiConfigLoad (&Config);
  return Config.Tpm2UiMode == TCG_UI_MODE_STANDARD;
}

BOOLEAN
EFIAPI
TcgUiConfigIsTcgStorageSimpleUiEnabled (
  VOID
  )
{
  TCG_UI_CONFIG_VARSTORE_DATA  Config;

  TcgUiConfigLoad (&Config);
  return Config.TcgStorageUiMode == TCG_UI_MODE_STANDARD;
}
