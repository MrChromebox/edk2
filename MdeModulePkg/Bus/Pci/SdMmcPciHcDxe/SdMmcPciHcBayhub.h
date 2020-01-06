/** @file
  Bayhub/O2 Micro eMMC controller quirk support.

  This module isolates all Bayhub (O2 Micro vendor 0x1217) eMMC controller
  logic so the main SdMmcPciHcDxe driver stays generic. The driver calls
  into these functions when BhtHostPciSupport() returns TRUE.

  Copyright (c) 2020, 2025. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _SD_MMC_PCI_HC_BAYHUB_H_
#define _SD_MMC_PCI_HC_BAYHUB_H_

#include <Protocol/PciIo.h>

//
// O2/Bayhub PCI device IDs (Vendor 0x1217)
//
#define BHT_PCI_DEV_ID_RJ    0x8320
#define BHT_PCI_DEV_ID_SDS0  0x8420
#define BHT_PCI_DEV_ID_SDS1  0x8421
#define BHT_PCI_DEV_ID_FJ2  0x8520
#define BHT_PCI_DEV_ID_SB0  0x8620
#define BHT_PCI_DEV_ID_SB1  0x8621

//
// O2/Bayhub BAR1 PCIR mapping registers
//
#define BHT_PCIRMappingVal    (0x200)
#define BHT_PCIRMappingCtl    (0x204)
#define BHT_PCIRMappingEn     (0x208)
#define BHT_GPIOCTL           (0x210)
#define BHT_CLK_SRC_SWITCH    (0x354)
#define BHT_PCR_SD_SEL_DLL    (1 << 16)

/**
  Check if the PCI device is a supported Bayhub/O2 Micro eMMC controller.

  @param[in] PciIo  The PCI I/O protocol.

  @retval TRUE   Controller is Bayhub (O2 Micro VID 0x1217, supported DIDs).
  @retval FALSE  Otherwise.
**/
BOOLEAN
BhtHostPciSupport (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  );

/**
  Read a 32-bit Bayhub vendor register (via BAR1 PCIR mapping).

  @param[in] PciIo   The PCI I/O protocol.
  @param[in] Offset  Register offset in vendor space.

  @return The 32-bit value read.
**/
UINT32
PciBhtRead32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset
  );

/**
  Write a 32-bit Bayhub vendor register (via BAR1 PCIR mapping).

  @param[in] PciIo   The PCI I/O protocol.
  @param[in] Offset  Register offset in vendor space.
  @param[in] Value   Value to write.
**/
VOID
PciBhtWrite32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  );

/**
  OR a 32-bit value into a Bayhub vendor register.

  @param[in] PciIo   The PCI I/O protocol.
  @param[in] Offset  Register offset in vendor space.
  @param[in] Value   Value to OR.
**/
VOID
PciBhtOr32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  );

/**
  AND a 32-bit value into a Bayhub vendor register.

  @param[in] PciIo   The PCI I/O protocol.
  @param[in] Offset  Register offset in vendor space.
  @param[in] Value   Value to AND.
**/
VOID
PciBhtAnd32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  );

/**
  Bayhub-specific host initialization (replaces standard path when BhtHostPciSupport).
  Caller must ensure BhtHostPciSupport(Private->PciIo) is TRUE.

  @param[in] Private  SD/MMC host controller private data.
  @param[in] Slot     Slot index.

  @retval EFI_SUCCESS  Init succeeded.
  @retval other        Init failed.
**/
EFI_STATUS
BayhubInitHost (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   Slot
  );

/**
  Bayhub 1.8V signaling enable (HostCtrl2 BIT3) and stall.
  Called from SdMmcHcInitPowerVoltage when BhtHostPciSupport.

  @param[in] PciIo  The PCI I/O protocol.
  @param[in] Slot  Slot index.

  @retval EFI_SUCCESS  Success.
  @retval other        SdMmcHcOrMmio failed.
**/
EFI_STATUS
BayhubInitPowerVoltageExtra (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Slot
  );

/**
  Bayhub pre-identification: clear clock source selection before reset.
  Called from EmmcIdentification when BhtHostPciSupport.

  @param[in] PciIo  The PCI I/O protocol.
**/
VOID
BayhubEmmcIdentificationPre (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  );

/**
  Bayhub HS200 tuning: initial setup (4-bit, hardware tuning, send tuning block with 4).
  Caller then runs the tuning loop; for Bayhub, use Stall instead of SendTuningBlk each
  iteration, and on success call BayhubEmmcTuningRestoreBusWidth.

  @param[in] PciIo    The PCI I/O protocol.
  @param[in] PassThru Pass-thru protocol.
  @param[in] Slot     Slot index.

  @retval EFI_SUCCESS  Setup succeeded.
  @retval other        Failed.
**/
EFI_STATUS
BayhubEmmcTuningStart (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot
  );

/**
  Bayhub HS200 tuning: restore bus width after successful tuning.

  @param[in] PciIo     The PCI I/O protocol.
  @param[in] Slot      Slot index.
  @param[in] BusWidth  Target bus width (e.g. 8).
**/
EFI_STATUS
BayhubEmmcTuningRestoreBusWidth (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Slot,
  IN UINT8                BusWidth
  );

/**
  Bayhub HS200 switch: extra waits after EmmcSwitchBusTiming (0x1cc BIT14, debounce, BIT11).
  Caller does EmmcSwitchBusTiming then this, then EmmcTuningClkForHs200.

  @param[in] PciIo   The PCI I/O protocol.
  @param[in] Slot    Slot index.

  @retval EFI_SUCCESS  Success.
  @retval other        Timeout or error.
**/
EFI_STATUS
BayhubEmmcSwitchToHS200Post (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Slot
  );

/**
  Apply EMMC_FORCE_CARD_MODE and related variables to BusMode for Bayhub.
  Called from EmmcSetBusMode when BhtHostPciSupport.

  @param[in,out] BusMode  Bus settings to update.
**/
VOID
BayhubEmmcSetBusModeFromVariable (
  IN OUT SD_MMC_BUS_SETTINGS  *BusMode
  );

/**
  Bayhub HS200 failure: apply HS100 phase (0x300, EMMC_HS100_ALLPASS_PHASE, 0x3C, 0x2C) and set
  BusMode.ClockFreq to 100. Caller then retries EmmcSwitchToHS200 and on failure can try
  EmmcSwitchToHighSpeed. Called from EmmcSetBusMode when HS200 switch failed and BhtHostPciSupport.

  @param[in] PciIo     The PCI I/O protocol.
  @param[in] Slot      Slot index.
  @param[in,out] BusMode  Bus settings (ClockFreq set to 100).

  @retval EFI_SUCCESS  Phase applied.
**/
EFI_STATUS
BayhubEmmcApplyHs100Phase (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT8                Slot,
  IN OUT SD_MMC_BUS_SETTINGS  *BusMode
  );

/**
  Bayhub post-init: read various MMIO registers (workaround).
  Called from SdMmcPciHcDriverBindingStart when BhtHostPciSupport.

  @param[in] PciIo  The PCI I/O protocol.
**/
VOID
BayhubPostInitReads (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  );

#endif /* _SD_MMC_PCI_HC_BAYHUB_H_ */
