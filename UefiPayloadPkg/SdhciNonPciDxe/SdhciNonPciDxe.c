/** @file
 *
 *  Non-discoverable SDHCI driver for non-PCI controllers.
 *
 *  Covers fixed-MMIO SDHCI (e.g. AMD eMMC) and PCI controllers left in ACPI
 *  mode (e.g. Baytrail/Braswell SCC).  Register devices from
 *  CBMEM_ID_SDHCI_NONPCI so SdMmcPciHcDxe can bind without a PCI probe.
 *
 *  Copyright (c) 2026, Matt DeVillier <matt.devillier@gmail.com>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <PiDxe.h>
#include <Guid/SdhciNonPciInfoGuid.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/NonDiscoverableDevice.h>
#include <Protocol/SdMmcOverride.h>

#include "SdhciNonPciDxe.h"

STATIC EFI_HANDLE  mEmmcControllerHandle;

/**
  Override function for SDHCI capability bits
**/
STATIC
EFI_STATUS
EFIAPI
SdhciNonPciSdMmcCapability (
  IN      EFI_HANDLE  ControllerHandle,
  IN      UINT8       Slot,
  IN OUT  VOID        *SdMmcHcSlotCapability,
  IN OUT  UINT32      *BaseClkFreq
  )
{
  SD_MMC_HC_SLOT_CAP  *Capability;

  if (SdMmcHcSlotCapability == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Mark eMMC as embedded.
  //
  if (ControllerHandle == mEmmcControllerHandle) {
    Capability           = (SD_MMC_HC_SLOT_CAP *)SdMmcHcSlotCapability;
    Capability->SlotType = EmbeddedSlot;
  }

  return EFI_SUCCESS;
}

STATIC EDKII_SD_MMC_OVERRIDE  mSdMmcOverride = {
  EDKII_SD_MMC_OVERRIDE_PROTOCOL_VERSION,
  SdhciNonPciSdMmcCapability,
  NULL,
};

/**
  Return TRUE if an SDHCI controller appears present at Base.
**/
STATIC
BOOLEAN
SdhciLooksPresent (
  IN UINTN  Base
  )
{
  UINT32  Capabilities;
  UINT16  HostVersion;

  if (Base == 0) {
    return FALSE;
  }

  Capabilities = MmioRead32 (Base + SDHCI_CAPABILITIES);
  HostVersion  = MmioRead16 (Base + SDHCI_HOST_VERSION);

  if ((Capabilities == 0xFFFFFFFF) || (Capabilities == 0) ||
      (HostVersion == 0xFFFF) || (HostVersion == 0))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Return a debug name for the controller slot.
**/
STATIC
CONST CHAR8 *
SdhciSlotName (
  IN UINT8  Slot
  )
{
  switch (Slot) {
    case SDHCI_NONPCI_SLOT_EMMC:
      return "eMMC";
    case SDHCI_NONPCI_SLOT_SD:
      return "SD";
    case SDHCI_NONPCI_SLOT_SDIO:
      return "SDIO";
    default:
      return "SDHCI";
  }
}

/**
  Register one non-PCI SDHCI controller from CBMEM info.
**/
STATIC
EFI_STATUS
RegisterSdhciNonPci (
  IN CONST SDHCI_NONPCI_CONTROL  *Ctrl
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    Handle;
  BOOLEAN       IsEmbedded;
  CONST CHAR8   *Name;

  IsEmbedded = (Ctrl->Flags & SDHCI_NONPCI_FLAG_EMBEDDED) != 0;
  Name       = SdhciSlotName (Ctrl->Slot);

  if (!SdhciLooksPresent ((UINTN)Ctrl->MmioBase)) {
    DEBUG ((DEBUG_INFO, "%a: %a not present at 0x%x\n", __func__, Name, Ctrl->MmioBase));
    return EFI_NOT_FOUND;
  }

  Handle = NULL;
  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeSdhci,
             NonDiscoverableDeviceDmaTypeCoherent,
             NULL,
             &Handle,
             1,
             (UINTN)Ctrl->MmioBase,
             (UINTN)((Ctrl->MmioSize != 0) ? Ctrl->MmioSize : SDHCI_NONPCI_MMIO_SIZE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to register %a at 0x%x: %r\n", __func__, Name, Ctrl->MmioBase, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: registered %a at 0x%x\n", __func__, Name, Ctrl->MmioBase));

  if (IsEmbedded) {
    mEmmcControllerHandle = Handle;
    Status = gBS->InstallProtocolInterface (
                    &mEmmcControllerHandle,
                    &gEdkiiSdMmcOverrideProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    (VOID *)&mSdMmcOverride
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: SdMmcOverride install failed: %r\n", __func__, Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Driver entry point.
**/
EFI_STATUS
EFIAPI
SdhciNonPciDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS       Status;
  VOID             *Hob;
  SDHCI_NONPCI_INFO  *Info;
  UINTN            Index;
  BOOLEAN          Registered;

  Hob = GetFirstGuidHob (&gUefiSdhciNonPciInfoGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  Info = (SDHCI_NONPCI_INFO *)GET_GUID_HOB_DATA (Hob);
  if ((Info->Version != SDHCI_NONPCI_INFO_VERSION) ||
      (Info->Count == 0) ||
      (Info->Count > SDHCI_NONPCI_CTRL_MAX))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: invalid SDHCI info (ver=%u count=%u)\n",
      __func__,
      Info->Version,
      Info->Count
      ));
    return EFI_UNSUPPORTED;
  }

  Registered = FALSE;
  for (Index = 0; Index < Info->Count; Index++) {
    if (Info->Ctrl[Index].MmioBase == 0) {
      continue;
    }

    Status = RegisterSdhciNonPci (&Info->Ctrl[Index]);
    if (!EFI_ERROR (Status)) {
      Registered = TRUE;
    }
  }

  return Registered ? EFI_SUCCESS : EFI_NOT_FOUND;
}
