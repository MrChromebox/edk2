/** @file
 *
 *  AMD Picasso SDHCI eMMC driver
 *
 *  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>
 *  Copyright (c) 2022, Patrick Wildt <patrick@blueri.se>
 *  Copyright (c) 2023, Mario Bălănică <mariobalanica02@gmail.com>
 *  Copyright (c) 2024, CoolStar <coolstarorganization@gmail.com>
 *  Copyright (c) 2025, Matt DeVillier <matt.devillier@gmail.com>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/NonDiscoverableDevice.h>
#include <Protocol/SdMmcOverride.h>

#include "AmdPcoSdhciDxe.h"


STATIC EFI_HANDLE mSdMmcControllerHandle;

/**
  Override function for SDHCI capability bits

  @param[in]      ControllerHandle      The EFI_HANDLE of the controller.
  @param[in]      Slot                  The 0 based slot index.
  @param[in,out]  SdMmcHcSlotCapability The SDHCI capability structure.
  @param[in,out]  BaseClkFreq           The base clock frequency value that
                                        optionally can be updated.

  @retval EFI_SUCCESS           The override function completed successfully.
  @retval EFI_NOT_FOUND         The specified controller or slot does not exist.
  @retval EFI_INVALID_PARAMETER SdMmcHcSlotCapability is NULL

**/
STATIC
EFI_STATUS
EFIAPI
EmmcSdMmcCapability (
  IN      EFI_HANDLE                      ControllerHandle,
  IN      UINT8                           Slot,
  IN OUT  VOID                            *SdMmcHcSlotCapability,
  IN OUT  UINT32                          *BaseClkFreq
  )
{
  SD_MMC_HC_SLOT_CAP *Capability = SdMmcHcSlotCapability;

  DEBUG ((DEBUG_BLKIO, "%a\n", __FUNCTION__));

  if (SdMmcHcSlotCapability == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Override SlotType
  if (ControllerHandle == mSdMmcControllerHandle) {
    Capability->SlotType = EmbeddedSlot;
  }

  return EFI_SUCCESS;
}

/**

  Override function for SDHCI controller operations

  @param[in]      ControllerHandle      The EFI_HANDLE of the controller.
  @param[in]      Slot                  The 0 based slot index.
  @param[in]      PhaseType             The type of operation and whether the
                                        hook is invoked right before (pre) or
                                        right after (post)
  @param[in,out]  PhaseData             The pointer to a phase-specific data.

  @retval EFI_SUCCESS           The override function completed successfully.
  @retval EFI_NOT_FOUND         The specified controller or slot does not exist.
  @retval EFI_INVALID_PARAMETER PhaseType is invalid

**/
STATIC
EFI_STATUS
EFIAPI
EmmcSdMmcNotifyPhase (
  IN      EFI_HANDLE                      ControllerHandle,
  IN      UINT8                           Slot,
  IN      EDKII_SD_MMC_PHASE_TYPE         PhaseType,
  IN OUT  VOID                            *PhaseData
  )
{
  DEBUG ((DEBUG_INFO, "%a\n", __FUNCTION__));

  if (ControllerHandle != mSdMmcControllerHandle) {
    return EFI_SUCCESS;
  }

  ASSERT (Slot == 0);


  return EFI_SUCCESS;
}

STATIC EDKII_SD_MMC_OVERRIDE mSdMmcOverride = {
  EDKII_SD_MMC_OVERRIDE_PROTOCOL_VERSION,
  EmmcSdMmcCapability,
  EmmcSdMmcNotifyPhase,
};

EFI_STATUS
EFIAPI
AmdPcoSdhciDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                      Status;
  UINT32                          Capabilities;
  UINT16                          HostVersion;
  UINT32                          PresentState;

  DEBUG ((DEBUG_BLKIO, "%a\n", __FUNCTION__));

  //
  // Check if eMMC controller is present by reading SDHCI registers
  // If the controller is not present, reads will return 0xFFFFFFFF or 0x00000000
  //
  Capabilities = MmioRead32 ((UINTN)(AMD_PCO_SDHCI_BASE + SDHCI_CAPABILITIES));
  HostVersion = MmioRead16 ((UINTN)(AMD_PCO_SDHCI_BASE + SDHCI_HOST_VERSION));

  if (Capabilities == 0xFFFFFFFF || Capabilities == 0x00000000 ||
      HostVersion == 0xFFFF || HostVersion == 0x0000) {
    DEBUG ((DEBUG_INFO, "%a: eMMC controller not present at 0x%lx (Capabilities=0x%x, Version=0x%x)\n",
            __FUNCTION__, (UINT64)AMD_PCO_SDHCI_BASE, Capabilities, HostVersion));
    return EFI_NOT_FOUND;
  }

  //
  // Check Present State Register to verify if an eMMC card is actually inserted
  // For embedded eMMC, this should show the card as present if populated
  // Refer to SD Host Controller Simplified spec 3.0 Section 3.1
  //
  PresentState = MmioRead32 ((UINTN)(AMD_PCO_SDHCI_BASE + SDHCI_PRESENT_STATE));

  if ((PresentState & SDHCI_CARD_PRESENT) == 0) {
    DEBUG ((DEBUG_INFO, "%a: eMMC card not present at 0x%lx (PresentState=0x%x)\n",
            __FUNCTION__, (UINT64)AMD_PCO_SDHCI_BASE, PresentState));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "%a: eMMC device detected at 0x%lx (Capabilities=0x%x, Version=0x%x, PresentState=0x%x)\n",
          __FUNCTION__, (UINT64)AMD_PCO_SDHCI_BASE, Capabilities, HostVersion, PresentState));

  Status = RegisterNonDiscoverableMmioDevice (
             NonDiscoverableDeviceTypeSdhci,
             NonDiscoverableDeviceDmaTypeCoherent,
             NULL,
             &mSdMmcControllerHandle,
             1,
             (UINTN)AMD_PCO_SDHCI_BASE, (UINTN)0x1000);
  ASSERT_EFI_ERROR (Status);

  //
  // Install the SdMmcOverride protocol on the same handle as the non-discoverable device
  // This ensures the override only applies to this specific controller, not all SD/MMC devices
  //
  Status = gBS->InstallProtocolInterface (&mSdMmcControllerHandle,
                  &gEdkiiSdMmcOverrideProtocolGuid,
                  EFI_NATIVE_INTERFACE, (VOID **)&mSdMmcOverride);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
