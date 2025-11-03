/** @file
  GL9763E/GL9750/GL9755-specific initialization (ported from Depthcharge sdhci_gli.c)

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciGliDxe.h"

/**
  Lock or unlock Genesys Logic vendor configuration registers.
  Equivalent to Linux's gl9750_wt_on() / gl9750_wt_off().

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
  @param[in] Lock    TRUE to lock, FALSE to unlock.

  @retval EFI_SUCCESS  Operation successful
  @retval Others       PCI I/O error
**/
STATIC
EFI_STATUS
GliVendorConfigLockUnlock (
  IN SD_MMC_CB_DEVICE  *Device,
  IN BOOLEAN           Lock
  )
{
  EFI_STATUS  Status;
  UINT32      Value;

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GLI_CFG,
                                1,
                                &Value
                                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Lock) {
    Value &= ~GLI_CFG_EN;  // Clear CFG_EN to lock
  } else {
    Value |= GLI_CFG_EN;   // Set CFG_EN to unlock
  }

  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GLI_CFG,
                                1,
                                &Value
                                );

  return Status;
}

/**
  Initialize Genesys Logic vendor-specific registers (GL9750/GL9755).
  Based on src/drivers/genesyslogic/ in coreboot.

  @param[in] Device  Device context

  @retval EFI_SUCCESS  Initialization successful
**/
EFI_STATUS
Gl975xInit (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Value;
  UINT32      Cfg2Reg;

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Genesys Logic vendor init start\n"));

  //
  // Step 1: UNLOCK vendor config registers (GL9750/GL9755 only)
  //
  Status = GliVendorConfigLockUnlock (Device, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to unlock vendor config: %r\n", Status));
    return Status;
  }
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: GL vendor config UNLOCKED\n"));

  //
  // Step 2: GL9755-specific LTR configuration (not needed for GL9750)
  //
  if (Device->IsGL9755) {
    Value = GL9755_LTR_SNOOP_VALUE | GL9755_LTR_SNOOP_SCALE |
            GL9755_LTR_NO_SNOOP_VALUE | GL9755_LTR_NO_SNOOP_SCALE;

    Status = Device->PciIo->Pci.Write (
                                  Device->PciIo,
                                  EfiPciIoWidthUint32,
                                  GL9755_LTR,
                                  1,
                                  &Value
                                  );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to write GL9755 LTR: %r\n", Status));
      return Status;
    }
    Device->PciIo->Flush (Device->PciIo);
    DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 LTR configured = 0x%08X\n", Value));

    //
    // GL9755: Configure PECONF - clear LFCLK and DMACLK for proper DMA operation
    //
    Status = Device->PciIo->Pci.Read (
                                  Device->PciIo,
                                  EfiPciIoWidthUint32,
                                  GL9755_PECONF,
                                  1,
                                  &Value
                                  );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read GL9755 PECONF: %r\n", Status));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 PECONF before = 0x%08X\n", Value));

    Value &= ~GL9755_PECONF_LFCLK_MASK;  // Clear LFCLK [14:12]
    Value &= ~GL9755_PECONF_DMACLK;      // Clear DMACLK [29]

    Status = Device->PciIo->Pci.Write (
                                  Device->PciIo,
                                  EfiPciIoWidthUint32,
                                  GL9755_PECONF,
                                  1,
                                  &Value
                                  );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to write GL9755 PECONF: %r\n", Status));
      return Status;
    }
    Device->PciIo->Flush (Device->PciIo);
    DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 PECONF after = 0x%08X (LFCLK/DMACLK cleared)\n", Value));

    //
    // GL9755: Enable short circuit protection in SerDes register
    // Clear SCP_DIS (bit 19) to enable SCP/OCP
    //
    Status = Device->PciIo->Pci.Read (
                                  Device->PciIo,
                                  EfiPciIoWidthUint32,
                                  GL9755_SERDES,
                                  1,
                                  &Value
                                  );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read GL9755 SerDes: %r\n", Status));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 SerDes before = 0x%08X\n", Value));

    Value &= ~GL9755_SERDES_SCP_DIS;  // Clear SCP_DIS (bit 19)

    Status = Device->PciIo->Pci.Write (
                                  Device->PciIo,
                                  EfiPciIoWidthUint32,
                                  GL9755_SERDES,
                                  1,
                                  &Value
                                  );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to write GL9755 SerDes: %r\n", Status));
      return Status;
    }
    Device->PciIo->Flush (Device->PciIo);
    DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 SerDes after = 0x%08X (SCP enabled)\n", Value));
  }

  //
  // Step 3: Configure CFG2 (ASPM L0s/L1 settings)
  // Each GL variant uses a different register offset
  //
  if (Device->IsGL9755) {
    Cfg2Reg = GL9755_CFG2;
  } else if (Device->IsGL9750) {
    Cfg2Reg = GL9750_CFG2;
  }

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                Cfg2Reg,
                                1,
                                &Value
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read CFG2: %r\n", Status));
    return Status;
  }

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: GL CFG2 (0x%03X) before = 0x%08X\n", Cfg2Reg, Value));

  // Disable L0s support (all GL variants)
  if (Device->IsGL9755) {
    Value &= ~GL9755_CFG2_L0S_SUPPORT;  // Bit 6 for GL9755
  } else if (Device->IsGL9750) {
    Value &= ~GL9750_CFG2_L0S_SUPPORT;  // Bit 6 for GL9750
  }

  // Configure L1 latency
  if (Device->IsGL9755) {
    // GL9755: Set L1 exit latency to 64us (different format: two bit fields)
    Value &= ~GL9755_CFG2_L1_LAT_MASK;
    Value |= GL9755_CFG2_L1_LAT_64US;
  } else {
    // GL9750: Set L1 delay to 84us - single field
    Value &= ~GL9750_CFG2_L1DLY_MASK;
    Value |= (GL9750_CFG2_L1DLY_84US << GL9750_CFG2_L1DLY_SHIFT);
  }

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: GL CFG2 target = 0x%08X\n", Value));

  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                Cfg2Reg,
                                1,
                                &Value
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to write CFG2: %r\n", Status));
    return Status;
  }
  Device->PciIo->Flush (Device->PciIo);

  // Read back to verify
  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                Cfg2Reg,
                                1,
                                &Value
                                );

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: GL CFG2 after  = 0x%08X\n", Value));

  //
  // Step 4: Disable ASPM support for troubleshooting (optional)
  // This can help with stability issues on some boards
  //
  if (Device->IsGL9750 || Device->IsGL9755) {
    // Disable ASPM L0s and L1 support
    Status = Device->PciIo->Pci.Read (
                                  Device->PciIo,
                                  EfiPciIoWidthUint32,
                                  PCI_LINK_CONTROL_OFFSET,
                                  1,
                                  &Value
                                  );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "SdMmcPciGli: PCI Link Status before ASPM disable = 0x%08X\n", Value));

      // Clear ASPM support bits
      Value &= ~PCI_ASPM_L0S_L1_MASK;  // Clear L0s and L1 support

      Status = Device->PciIo->Pci.Write (
                                    Device->PciIo,
                                    EfiPciIoWidthUint32,
                                    PCI_LINK_CONTROL_OFFSET,
                                    1,
                                    &Value
                                    );
      if (!EFI_ERROR (Status)) {
        Device->PciIo->Flush (Device->PciIo);
        DEBUG ((DEBUG_INFO, "SdMmcPciGli: ASPM support disabled for troubleshooting\n"));
      }
    }
  }

  //
  // Step 5: LOCK vendor config registers
  //
  Status = GliVendorConfigLockUnlock (Device, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to lock vendor config: %r\n", Status));
    return Status;
  }
  Device->PciIo->Flush (Device->PciIo);
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: GL vendor config LOCKED\n"));

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Genesys Logic vendor init complete\n"));

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
    DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9763E Enhanced Strobe enabled\n"));
  } else {
    Ctrl &= ~GL9763E_ENHANCED_STROBE;
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: GL9763E Enhanced Strobe disabled\n"));
  }

  //
  // Write updated value back
  //
  SdhciWritel (Device, GL9763E_SDHC_EMMC_CTRL, Ctrl);
}

/**
  Check if GL9750 SSC (Spread Spectrum Clocking) is enabled.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.

  @retval TRUE   SSC is enabled.
  @retval FALSE  SSC is disabled.
**/
STATIC
BOOLEAN
Gl9750SscEnable (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  UINT32  Misc;

  Misc = SdhciReadl (Device, GL9750_MISC);
  return (Misc & GL9750_MISC_SSC_OFF) == 0;  // SSC enabled if bit is NOT set
}

/**
  Disable GL9750 SSC PLL before clock change.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9750DisableSscPll (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  UINT32  Pll;

  Pll = SdhciReadl (Device, GL9750_PLL);
  Pll &= ~(GL9750_PLL_DIR | GL9750_PLLSSC_EN);
  SdhciWritel (Device, GL9750_PLL, Pll);
}

/**
  Set GL9750 SSC (Spread Spectrum Clocking) parameters.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
  @param[in] Enable  Enable or disable SSC.
  @param[in] Step    SSC step value.
  @param[in] Ppm     SSC PPM (parts per million) value.
**/
STATIC
VOID
Gl9750SetSsc (
  IN SD_MMC_CB_DEVICE  *Device,
  IN BOOLEAN           Enable,
  IN UINT8             Step,
  IN UINT16            Ppm
  )
{
  UINT32  Pll;
  UINT32  Ssc;

  Pll = SdhciReadl (Device, GL9750_PLL);
  Ssc = SdhciReadl (Device, GL9750_PLLSSC);

  Pll &= ~(GL9750_PLLSSC_STEP | GL9750_PLLSSC_EN);
  Pll |= ((UINT32)Step << 24) | (Enable ? GL9750_PLLSSC_EN : 0);

  Ssc &= ~GL9750_PLLSSC_PPM;
  Ssc |= ((UINT32)Ppm << 16);  // GL9750 PPM is in bits [31:16]

  SdhciWritel (Device, GL9750_PLLSSC, Ssc);
  SdhciWritel (Device, GL9750_PLL, Pll);
}

/**
  Set GL9750 PLL divider parameters.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
  @param[in] Dir     PLL direction.
  @param[in] Ldiv    PLL loop divider.
  @param[in] Pdiv    PLL P divider.
**/
STATIC
VOID
Gl9750SetPll (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT8             Dir,
  IN UINT16            Ldiv,
  IN UINT8             Pdiv
  )
{
  UINT32  Pll;

  Pll = SdhciReadl (Device, GL9750_PLL);

  Pll &= ~(GL9750_PLL_LDIV | GL9750_PLL_PDIV | GL9750_PLL_DIR);
  Pll |= ((UINT32)Ldiv & 0x3FF) | (((UINT32)Pdiv & 0x7) << 12) | (Dir ? GL9750_PLL_DIR : 0);

  SdhciWritel (Device, GL9750_PLL, Pll);

  // Wait for PLL to stabilize
  gBS->Stall (1000);  // 1ms
}

/**
  Configure GL9750 PLL for 205 MHz (SDR104).

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9750SetSscPll205Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  BOOLEAN  Enable;

  Enable = Gl9750SscEnable (Device);

  // SSC: Step=0xF (15), PPM=0x5A1D (23069)
  Gl9750SetSsc (Device, Enable, 0xF, 0x5A1D);

  // PLL: Dir=1, Ldiv=0x246 (582), Pdiv=0
  Gl9750SetPll (Device, 0x1, 0x246, 0x0);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9750 PLL set to 205 MHz (SDR104)\n"));
}

/**
  Configure GL9750 PLL for 100 MHz (SDR50).

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9750SetSscPll100Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  BOOLEAN  Enable;

  Enable = Gl9750SscEnable (Device);

  // SSC: Step=0xE (14), PPM=0x51EC (20972)
  Gl9750SetSsc (Device, Enable, 0xE, 0x51EC);

  // PLL: Dir=1, Ldiv=0x244 (580), Pdiv=1
  Gl9750SetPll (Device, 0x1, 0x244, 0x1);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9750 PLL set to 100 MHz (SDR50)\n"));
}

/**
  Configure GL9750 PLL for 50 MHz (High Speed).

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9750SetSscPll50Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  BOOLEAN  Enable;

  Enable = Gl9750SscEnable (Device);

  // SSC: Step=0xE (14), PPM=0x51EC (20972)
  Gl9750SetSsc (Device, Enable, 0xE, 0x51EC);

  // PLL: Dir=1, Ldiv=0x244 (580), Pdiv=3
  Gl9750SetPll (Device, 0x1, 0x244, 0x3);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9750 PLL set to 50 MHz (High Speed)\n"));
}

/**
  Configure GL9750 tuning parameters for SDR104 mode.
  Based on Linux driver's gli_set_9750() function.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9750SetupTuning (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  UINT32  DrivingValue;
  UINT32  SwCtrlValue;
  UINT32  MiscValue;
  UINT32  ParameterValue;
  UINT32  ControlValue;
  UINT16  Ctrl2;
  UINT32  PllValue;

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Setting up GL9750 tuning parameters\n"));

  // Unlock vendor registers (gl9750_wt_on equivalent)
  GliVendorConfigLockUnlock (Device, FALSE);

  // Read current values
  DrivingValue = SdhciReadl (Device, 0x860);  // SDHCI_GLI_9750_DRIVING
  SwCtrlValue = SdhciReadl (Device, 0x874);  // SDHCI_GLI_9750_SW_CTRL
  MiscValue = SdhciReadl (Device, GL9750_MISC);
  ParameterValue = SdhciReadl (Device, 0x544);  // SDHCI_GLI_9750_TUNING_PARAMETERS
  ControlValue = SdhciReadl (Device, 0x540);  // SDHCI_GLI_9750_TUNING_CONTROL

  // Configure driving values
  DrivingValue &= ~0x0FFF;  // Clear DRIVING_1
  DrivingValue &= ~0x0C000000;  // Clear DRIVING_2
  DrivingValue |= 0xFFF;  // DRIVING_1 = 0xFFF
  DrivingValue |= 0x0C000000;  // DRIVING_2 = 0x3
  DrivingValue &= ~0x1A000000;  // Clear SEL_1, SEL_2, ALL_RST
  DrivingValue |= 0x80000000;  // Set SEL_2
  SdhciWritel (Device, 0x860, DrivingValue);

  // Configure switch control
  SwCtrlValue &= ~0xC0;  // Clear SW_CTRL_4
  SwCtrlValue |= 0xC0;  // SW_CTRL_4 = 0x3
  SdhciWritel (Device, 0x874, SwCtrlValue);

  // Configure PLL tuning parameters
  PllValue = SdhciReadl (Device, GL9750_PLL);
  PllValue &= ~0x00800000;  // Clear TX2_INV
  PllValue &= ~0x00700000;  // Clear TX2_DLY
  PllValue |= 0x00800000;  // TX2_INV = 1
  // TX2_DLY = 0 (already cleared)
  SdhciWritel (Device, GL9750_PLL, PllValue);

  // Configure MISC tuning parameters
  MiscValue &= ~0x04;  // Clear TX1_INV
  MiscValue &= ~0x08;  // Clear RX_INV
  MiscValue &= ~0x70;  // Clear TX1_DLY
  // TX1_INV = 0 (already cleared)
  // RX_INV = 0 (already cleared)
  MiscValue |= 0x50;  // TX1_DLY = 0x5
  SdhciWritel (Device, GL9750_MISC, MiscValue);

  // Configure tuning parameters
  ParameterValue &= ~0x07;  // Clear RX_DLY
  ParameterValue |= 0x01;  // RX_DLY = 0x1
  SdhciWritel (Device, 0x544, ParameterValue);

  // Configure tuning control
  ControlValue &= ~0x10;  // Clear GLITCH_1
  ControlValue &= ~0x180000;  // Clear GLITCH_2
  ControlValue |= 0x10;  // GLITCH_1 = 1
  ControlValue |= 0x100000;  // GLITCH_2 = 0x2

  // Disable tuned clk
  Ctrl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
  Ctrl2 &= ~SDHCI_CTRL_TUNED_CLK;
  SdhciWritew (Device, SDHCI_HOST_CONTROL2, Ctrl2);

  // Enable tuning parameters control
  ControlValue &= ~0x10;  // Clear TUNING_CONTROL_EN
  ControlValue |= 0x10;  // TUNING_CONTROL_EN = 1
  SdhciWritel (Device, 0x540, ControlValue);

  // Write tuning parameters
  SdhciWritel (Device, 0x544, ParameterValue);

  // Disable tuning parameters control
  ControlValue &= ~0x10;  // Clear TUNING_CONTROL_EN
  SdhciWritel (Device, 0x540, ControlValue);

  // Clear tuned clk
  Ctrl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
  Ctrl2 &= ~SDHCI_CTRL_TUNED_CLK;
  SdhciWritew (Device, SDHCI_HOST_CONTROL2, Ctrl2);

  // Lock vendor registers (gl9750_wt_off equivalent)
  GliVendorConfigLockUnlock (Device, TRUE);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9750 tuning parameters configured\n"));
}


/**
  Check if GL9755 SSC (Spread Spectrum Clocking) is enabled.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.

  @retval TRUE   SSC is enabled.
  @retval FALSE  SSC is disabled.
**/
STATIC
BOOLEAN
Gl9755SscEnable (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Pll;

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLL,
                                1,
                                &Pll
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "SdMmcPciGli: Failed to read GL9755 PLL register: %r\n", Status));
    return FALSE;
  }

  return (Pll & GL9755_PLLSSC_EN) != 0;
}

/**
  Disable GL9755 SSC PLL before clock change.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9755DisableSscPll (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Pll;
  UINT32      Misc;

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLL,
                                1,
                                &Pll
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read GL9755 PLL: %r\n", Status));
    return;
  }

  // Clear both DIR and SSC_EN to fully disable PLL (as Linux does)
  Pll &= ~(GL9755_PLL_DIR | GL9755_PLLSSC_EN);
  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLL,
                                1,
                                &Pll
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to disable GL9755 PLL SSC: %r\n", Status));
    return;
  }

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_MISC,
                                1,
                                &Misc
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read GL9755 MISC: %r\n", Status));
    return;
  }

  Misc |= GL9755_MISC_SSC_OFF;
  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_MISC,
                                1,
                                &Misc
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to set GL9755 MISC SSC off: %r\n", Status));
  }

  Device->PciIo->Flush (Device->PciIo);
}

/**
  Set GL9755 SSC (Spread Spectrum Clocking) parameters.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
  @param[in] Enable  Enable or disable SSC.
  @param[in] Step    SSC step value.
  @param[in] Ppm     SSC PPM (parts per million) value.
**/
STATIC
VOID
Gl9755SetSsc (
  IN SD_MMC_CB_DEVICE  *Device,
  IN BOOLEAN           Enable,
  IN UINT8             Step,
  IN UINT16            Ppm
  )
{
  EFI_STATUS  Status;
  UINT32      Pll;
  UINT32      Ssc;

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLL,
                                1,
                                &Pll
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read GL9755 PLL: %r\n", Status));
    return;
  }

  Pll &= ~(GL9755_PLLSSC_STEP | GL9755_PLLSSC_EN);
  Pll |= ((UINT32)Step << 24) | (Enable ? GL9755_PLLSSC_EN : 0);

  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLL,
                                1,
                                &Pll
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to write GL9755 PLL: %r\n", Status));
    return;
  }

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLLSSC,
                                1,
                                &Ssc
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read GL9755 PLLSSC: %r\n", Status));
    return;
  }

  Ssc &= ~GL9755_PLLSSC_PPM;
  Ssc |= Ppm;

  // Set signal integrity bits (per Linux gl9755_vendor_init)
  Ssc &= ~GL9755_PLLSSC_RTL;
  Ssc |= GL9755_PLLSSC_RTL;  // RTL = 1

  Ssc &= ~GL9755_PLLSSC_TRANS_PASS;
  Ssc |= GL9755_PLLSSC_TRANS_PASS;  // TRANS_PASS = 1

  Ssc &= ~GL9755_PLLSSC_RECV;
  // RECV = 0 (already cleared)

  Ssc &= ~GL9755_PLLSSC_TRAN;
  Ssc |= GL9755_PLLSSC_TRAN;  // TRAN = 3 (both bits set)

  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLLSSC,
                                1,
                                &Ssc
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to write GL9755 PLLSSC: %r\n", Status));
  }

  Device->PciIo->Flush (Device->PciIo);
}

/**
  Set GL9755 PLL divider parameters.

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
  @param[in] Dir     PLL direction.
  @param[in] Ldiv    PLL loop divider.
  @param[in] Pdiv    PLL P divider.
**/
STATIC
VOID
Gl9755SetPll (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT8             Dir,
  IN UINT16            Ldiv,
  IN UINT8             Pdiv
  )
{
  EFI_STATUS  Status;
  UINT32      Pll;

  Status = Device->PciIo->Pci.Read (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLL,
                                1,
                                &Pll
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to read GL9755 PLL: %r\n", Status));
    return;
  }

  Pll &= ~(GL9755_PLL_LDIV | GL9755_PLL_PDIV | GL9755_PLL_DIR);
  Pll |= ((UINT32)Ldiv & 0x3FF) | (((UINT32)Pdiv & 0x7) << 12) | (Dir ? GL9755_PLL_DIR : 0);

  Status = Device->PciIo->Pci.Write (
                                Device->PciIo,
                                EfiPciIoWidthUint32,
                                GL9755_PLL,
                                1,
                                &Pll
                                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to write GL9755 PLL: %r\n", Status));
  }

  Device->PciIo->Flush (Device->PciIo);

  // Wait for PLL to stabilize (per Linux kernel)
  gBS->Stall (1000);  // 1ms
}

/**
  Configure GL9755 PLL for 205 MHz (SDR104).

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9755SetSscPll205Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  BOOLEAN  Enable;

  Enable = Gl9755SscEnable (Device);

  // SSC: Step=0xF (15), PPM=0x5A1D (23069)
  Gl9755SetSsc (Device, Enable, 0xF, 0x5A1D);

  // PLL: Dir=1, Ldiv=0x246 (582), Pdiv=0
  Gl9755SetPll (Device, 0x1, 0x246, 0x0);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 PLL set to 205 MHz (SDR104)\n"));
}

/**
  Configure GL9755 PLL for 100 MHz (SDR50).

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9755SetSscPll100Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  BOOLEAN  Enable;

  Enable = Gl9755SscEnable (Device);

  // SSC: Step=0xE (14), PPM=0x51EC (20972)
  Gl9755SetSsc (Device, Enable, 0xE, 0x51EC);

  // PLL: Dir=1, Ldiv=0x244 (580), Pdiv=1
  Gl9755SetPll (Device, 0x1, 0x244, 0x1);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 PLL set to 100 MHz (SDR50)\n"));
}

/**
  Configure GL9755 PLL for 50 MHz (High Speed).

  @param[in] Device  Pointer to SD_MMC_CB_DEVICE structure.
**/
VOID
Gl9755SetSscPll50Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  BOOLEAN  Enable;

  Enable = Gl9755SscEnable (Device);

  // SSC: Step=0xE (14), PPM=0x51EC (20972)
  Gl9755SetSsc (Device, Enable, 0xE, 0x51EC);

  // PLL: Dir=1, Ldiv=0x244 (580), Pdiv=3
  Gl9755SetPll (Device, 0x1, 0x244, 0x3);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: GL9755 PLL set to 50 MHz (High Speed)\n"));
}

