/** @file
  GL9763E-specific initialization (ported from Depthcharge sdhci_gli.c)

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciCbDxe.h"

/**
  Initialize GL9763E vendor-specific registers.

  @param[in] Device  Device context

  @retval EFI_SUCCESS  Initialization successful
**/
EFI_STATUS
Gl9763eInit (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Gl9763eInit start\n"));

  //
  // Configure CFG_REG_2: Disable L0s and set L1 entry delay
  // This matches what the Linux driver does
  //
  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GLI_CFG_REG_2,
                                1,
                                &Value
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to read CFG_REG_2: %r\n", Status));
    return Status;
  }

  //
  // Disable L0s
  //
  Value &= ~GLI_CFG_REG_2_L0S;

  //
  // Set L1 delay to 84 units (0x54) to match Linux and working coreboot config
  //
  Value &= ~(GLI_CFG_REG_2_L1DLY_MASK);
  Value |= (0x54 << GLI_CFG_REG_2_L1DLY_SHIFT);

  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GLI_CFG_REG_2,
                                1,
                                &Value
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to write CFG_REG_2: %r\n", Status));
    return Status;
  }

  //
  // Read back to verify
  //
  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GLI_CFG_REG_2,
                                1,
                                &Value
                                );

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Gl9763eInit complete\n"));

  return EFI_SUCCESS;
}

/**
  Enable or disable Enhanced Strobe for HS400-ES mode.
  Based on Depthcharge's gl9763e_set_ios().

  @param[in] Device  Device context
  @param[in] Enable  TRUE to enable Enhanced Strobe, FALSE to disable
**/
VOID
Gl9763eSetEnhancedStrobe (
  IN SD_MMC_CB_DEVICE  *Device,
  IN BOOLEAN           Enable
  )
{
  UINT32  Ctrl;

  //
  // Read current EMMC_CTRL register value
  //
  Ctrl = SdhciReadl (Device, GL9763E_SDHC_EMMC_CTRL);

  if (Enable) {
    Ctrl |= GL9763E_ENHANCED_STROBE;
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: GL9763E Enhanced Strobe enabled\n"));
  } else {
    Ctrl &= ~GL9763E_ENHANCED_STROBE;
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: GL9763E Enhanced Strobe disabled\n"));
  }

  //
  // Write updated value back
  //
  SdhciWritel (Device, GL9763E_SDHC_EMMC_CTRL, Ctrl);
}

