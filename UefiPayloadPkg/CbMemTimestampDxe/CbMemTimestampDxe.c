/** @file
  Add late UPL lifecycle timestamps to coreboot CBMEM.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <Library/CbMemLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

STATIC
VOID
EFIAPI
OnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  (VOID)Event;
  (VOID)Context;
  CbMemTimestampAdd (CBMEM_TS_UPL_READY_TO_BOOT);
}

STATIC
VOID
EFIAPI
OnExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  (VOID)Event;
  (VOID)Context;
  CbMemTimestampAdd (CBMEM_TS_UPL_EXIT_BOOT);
}

EFI_STATUS
EFIAPI
CbMemTimestampDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ReadyToBootEvent;
  EFI_EVENT   ExitBootServicesEvent;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  CbMemTimestampAdd (CBMEM_TS_UPL_DXE_DRIVER);

  Status = gBS->CreateEventEx (
                 EVT_NOTIFY_SIGNAL,
                 TPL_CALLBACK,
                 OnReadyToBoot,
                 NULL,
                 &gEfiEventReadyToBootGuid,
                 &ReadyToBootEvent
                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "CBMEM: failed to register ReadyToBoot timestamp: %r\n", Status));
    return Status;
  }

  Status = gBS->CreateEventEx (
                 EVT_NOTIFY_SIGNAL,
                 TPL_CALLBACK,
                 OnExitBootServices,
                 NULL,
                 &gEfiEventExitBootServicesGuid,
                 &ExitBootServicesEvent
                 );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "CBMEM: failed to register ExitBootServices timestamp: %r\n", Status));
  }

  return EFI_SUCCESS;
}
