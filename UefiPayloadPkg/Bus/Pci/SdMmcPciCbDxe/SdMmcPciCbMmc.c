/** @file
  MMC/eMMC Protocol Functions (ported from Depthcharge mmc.c)

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciCbDxe.h"

/**
  Initialize eMMC device and read parameters.

  @param[in] Device  Device context

  @retval EFI_SUCCESS  eMMC initialized successfully
  @retval other        Initialization failed
**/
EFI_STATUS
MmcStartup (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Response[4];
  UINT32      OcrValue;
  UINT32      Retry;

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: MmcStartup begin\n"));

  //
  // CMD0: GO_IDLE_STATE
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD0\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_GO_IDLE_STATE, 0, MMC_RSP_NONE, NULL, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD0 failed: %r\n", Status));
    return Status;
  }

  gBS->Stall (1000); // 1ms delay after CMD0

  //
  // CMD1: SEND_OP_COND (with retry loop like Depthcharge)
  //
  OcrValue = 0x40FF8000; // High capacity, voltage range
  Retry = 100;

  while (Retry > 0) {
    // DEBUG ((DEBUG_INFO, "SdMmcPciCb: Sending CMD1 (retry=%d)\n", Retry));
    Status = SdhciSendCommand (Device, MMC_CMD_SEND_OP_COND, OcrValue, MMC_RSP_R3, Response, NULL, 0, 0, FALSE);

    if (!EFI_ERROR (Status)) {
      // DEBUG ((DEBUG_INFO, "SdMmcPciCb: CMD1 response = 0x%08x\n", Response[0]));

      //
      // Check if busy bit is clear (card ready)
      //
      if (Response[0] & OCR_BUSY) {
        //
        // Check for High Capacity Support (block addressing)
        //
        Device->HighCapacity = ((Response[0] & OCR_HCS) == OCR_HCS);
        DEBUG ((DEBUG_INFO, "SdMmcPciCb: eMMC ready! OCR=0x%08x, HighCapacity=%d\n",
                Response[0], Device->HighCapacity));
        break;
      }
    }

    gBS->Stall (10000); // 10ms delay between retries
    Retry--;
  }

  if (Retry == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD1 never succeeded\n"));
    return EFI_DEVICE_ERROR;
  }

  //
  // CMD2: ALL_SEND_CID
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD2\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_ALL_SEND_CID, 0, MMC_RSP_R2, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD2 failed: %r\n", Status));
    return Status;
  }

  // Save CID for DiskInfo protocol
  // Per SD Host Controller Simplified Spec 3.0 Table 2-12, CID is in Response[0-3]
  // but we need to skip the first byte and copy 15 bytes for EMMC_CID structure
  CopyMem (((UINT8 *)&Device->Cid.EmmcCid) + 1, &Response[0], sizeof (EMMC_CID) - 1);

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: CID = %08x %08x %08x %08x\n",
          Response[0], Response[1], Response[2], Response[3]));

  //
  // CMD3: SET_RELATIVE_ADDR
  //
  Device->RelativeCardAddress = 1; // Use RCA = 1
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD3\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_SET_RELATIVE_ADDR,
                             Device->RelativeCardAddress << 16, MMC_RSP_R1, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD3 failed: %r\n", Status));
    return Status;
  }

  //
  // CMD7: SELECT_CARD - Put the card in transfer state
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD7 (SELECT_CARD)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_SELECT_CARD,
                             Device->RelativeCardAddress << 16, MMC_RSP_R1, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD7 failed: %r\n", Status));
    return Status;
  }

  //
  // CMD16: SET_BLOCKLEN to 512 bytes
  //
  Device->BlockSize = 512;
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD16 (SET_BLOCKLEN to %d)\n", Device->BlockSize));
  Status = SdhciSendCommand (Device, MMC_CMD_SET_BLOCKLEN,
                             Device->BlockSize, MMC_RSP_R1, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD16 failed: %r\n", Status));
    return Status;
  }

  //
  // CMD8: SEND_EXT_CSD to get extended CSD register
  //
  UINT8  *ExtCsd;
  ExtCsd = AllocatePool (EXT_CSD_SIZE);
  if (ExtCsd == NULL) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to allocate EXT_CSD buffer\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SdhciSendCommand (Device, MMC_CMD_SEND_EXT_CSD, 0, MMC_RSP_R1, Response,
                             ExtCsd, EXT_CSD_SIZE, 1, TRUE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD8 (SEND_EXT_CSD) failed: %r\n", Status));
    FreePool (ExtCsd);
    return Status;
  }

  //
  // Get capacity from EXT_CSD
  //
  UINT32  SecCount = *(UINT32 *)&ExtCsd[EXT_CSD_SEC_CNT];
  Device->TotalBlocks = SecCount;

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: EXT_CSD: REV=%d, CARD_TYPE=0x%02x, SEC_CNT=%d, STROBE=%d\n",
          ExtCsd[EXT_CSD_REV], ExtCsd[EXT_CSD_CARD_TYPE], SecCount, ExtCsd[EXT_CSD_STROBE_SUPPORT]));

  //
  // Switch to best supported speed mode
  // Priority: HS400-ES > HS400 > HS200 > HS
  //
  UINT32  SwitchArg;
  UINT8   HostControl;
  BOOLEAN SupportsHS400 = (ExtCsd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_HS400_1_8V) != 0;
  BOOLEAN SupportsHS200 = (ExtCsd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_HS200_1_8V) != 0;
  BOOLEAN SupportsStrobe = (ExtCsd[EXT_CSD_STROBE_SUPPORT] != 0);

  if (SupportsHS400 || SupportsHS200) {
    //
    // Card supports HS200 - switch to it for maximum performance (200 MHz)
    //
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switching to HS200 mode (200 MHz)...\n"));

    // Step 1: Switch bus width to 8-bit
    SwitchArg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                (EXT_CSD_BUS_WIDTH << 16) |
                (EXT_CSD_BUS_WIDTH_8 << 8);
    Status = SdhciSendCommand (Device, MMC_CMD_SWITCH, SwitchArg, MMC_RSP_R1B, Response, NULL, 0, 0, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to switch to 8-bit: %r\n", Status));
      goto FallbackToHS;
    }

    // Step 2: Disable clock (REQUIRED before timing changes per SDHCI spec)
    SdhciWritew (Device, 0, SDHCI_CLOCK_CONTROL);
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Clock disabled for timing change\n"));

    // Set host controller to 8-bit mode + high speed
    HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
    HostControl |= SDHCI_CTRL_8BITBUS | SDHCI_CTRL_HISPD;
    SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);

    // Configure HOST_CONTROL2 for HS200 (SDR104 timing @ 1.8V)
    UINT16  HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
    HostControl2 &= ~SDHCI_CTRL_UHS_MASK;
    HostControl2 &= ~SDHCI_CTRL_DRV_TYPE_MASK;
    HostControl2 |= SDHCI_CTRL_UHS_SDR104;  // HS200 uses SDR104 timing
    HostControl2 |= SDHCI_CTRL_DRV_TYPE_A;  // Driver strength Type A (recommended for HS200)
    HostControl2 |= SDHCI_CTRL_180V_SIGNALING_ENABLE;  // Enable 1.8V signaling
    SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: HOST_CONTROL2 configured for HS200\n"));

    // Step 3: Switch eMMC to HS200 timing
    SwitchArg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                (EXT_CSD_HS_TIMING << 16) |
                (EXT_CSD_TIMING_HS200 << 8);
    Status = SdhciSendCommand (Device, MMC_CMD_SWITCH, SwitchArg, MMC_RSP_R1B, Response, NULL, 0, 0, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to switch to HS200: %r\n", Status));
      goto FallbackToHS;
    }

    // Step 4: Set clock to 200 MHz
    Status = SdhciSetClock (Device, 200000000);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to set 200 MHz clock: %r\n", Status));
      goto FallbackToHS;
    }
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switched to HS200 @ 200 MHz, 8-bit, 1.8V!\n"));

    //
    // If card supports HS400, switch from HS200 to HS400 (or HS400-ES) for DDR performance
    // HS400 tuning sequence: HS200 → HS → HS400 (or HS400-ES if strobe supported)
    //
    if (SupportsHS400) {
      BOOLEAN  UseEnhancedStrobe = SupportsStrobe && Device->IsGL9763E;  // GL9763E supports ES

      if (UseEnhancedStrobe) {
        DEBUG ((DEBUG_INFO, "SdMmcPciCb: Upgrading to HS400-ES (DDR 200 MHz + Enhanced Strobe)...\n"));
      } else {
        DEBUG ((DEBUG_INFO, "SdMmcPciCb: Upgrading to HS400 (DDR 200 MHz)...\n"));
      }

      // Step 1: Switch card back to HS timing at ≤52 MHz
      SdhciWritew (Device, 0, SDHCI_CLOCK_CONTROL);  // Disable clock

      SwitchArg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                  (EXT_CSD_HS_TIMING << 16) |
                  (EXT_CSD_TIMING_HS << 8);
      Status = SdhciSendCommand (Device, MMC_CMD_SWITCH, SwitchArg, MMC_RSP_R1B, Response, NULL, 0, 0, FALSE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to switch to HS for HS400: %r\n", Status));
        // Stay at HS200
        goto HS200Complete;
      }

      // Set clock to 52 MHz for HS
      Status = SdhciSetClock (Device, 52000000);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to set 52 MHz clock: %r\n", Status));
        goto HS200Complete;
      }

      // Step 2: Switch to 8-bit DDR bus width (with Enhanced Strobe if supported)
      SdhciWritew (Device, 0, SDHCI_CLOCK_CONTROL);  // Disable clock

      UINT8  BusWidthValue = EXT_CSD_DDR_BUS_WIDTH_8;
      if (UseEnhancedStrobe) {
        BusWidthValue |= EXT_CSD_BUS_WIDTH_STROBE;  // Enable Enhanced Strobe
      }

      SwitchArg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                  (EXT_CSD_BUS_WIDTH << 16) |
                  (BusWidthValue << 8);
      Status = SdhciSendCommand (Device, MMC_CMD_SWITCH, SwitchArg, MMC_RSP_R1B, Response, NULL, 0, 0, FALSE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to switch to 8-bit DDR%s: %r\n",
                UseEnhancedStrobe ? "+Strobe" : "", Status));
        goto HS200Complete;
      }

      // Step 3: Configure HOST_CONTROL2 for HS400
      HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
      HostControl2 &= ~SDHCI_CTRL_UHS_MASK;
      HostControl2 &= ~SDHCI_CTRL_DRV_TYPE_MASK;
      HostControl2 |= SDHCI_CTRL_HS400;  // HS400 mode
      HostControl2 |= SDHCI_CTRL_DRV_TYPE_A;
      HostControl2 |= SDHCI_CTRL_180V_SIGNALING_ENABLE;
      SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);

      // Step 4: Switch card to HS400 timing
      SwitchArg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                  (EXT_CSD_HS_TIMING << 16) |
                  (EXT_CSD_TIMING_HS400 << 8);
      Status = SdhciSendCommand (Device, MMC_CMD_SWITCH, SwitchArg, MMC_RSP_R1B, Response, NULL, 0, 0, FALSE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to switch to HS400: %r\n", Status));
        goto HS200Complete;
      }

      // Step 5: Set clock to 200 MHz (DDR = 400 MHz effective)
      Status = SdhciSetClock (Device, 200000000);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to set 200 MHz for HS400: %r\n", Status));
        goto HS200Complete;
      }

      // Step 6: Enable Enhanced Strobe in GL9763E register if using HS400-ES
      if (UseEnhancedStrobe && Device->IsGL9763E) {
        Gl9763eSetEnhancedStrobe (Device, TRUE);
      }

      if (UseEnhancedStrobe) {
        DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switched to HS400-ES @ 200 MHz DDR (400 MHz effective + Strobe), 8-bit, 1.8V!\n"));
      } else {
        DEBUG ((DEBUG_INFO, "SdMmcPiCb: Switched to HS400 @ 200 MHz DDR (400 MHz effective), 8-bit, 1.8V!\n"));
      }
    }
HS200Complete:
    ;  // Empty statement for label
  } else {
FallbackToHS:
    //
    // Fallback to HS mode (52 MHz)
    //
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switching to HS mode (52 MHz)...\n"));

    SwitchArg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                (EXT_CSD_HS_TIMING << 16) |
                (EXT_CSD_TIMING_HS << 8);
    Status = SdhciSendCommand (Device, MMC_CMD_SWITCH, SwitchArg, MMC_RSP_R1B, Response, NULL, 0, 0, FALSE);
    if (!EFI_ERROR (Status)) {
      // Switch bus width to 8-bit
      SwitchArg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
                  (EXT_CSD_BUS_WIDTH << 16) |
                  (EXT_CSD_BUS_WIDTH_8 << 8);
      Status = SdhciSendCommand (Device, MMC_CMD_SWITCH, SwitchArg, MMC_RSP_R1B, Response, NULL, 0, 0, FALSE);
      if (!EFI_ERROR (Status)) {
        HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
        HostControl |= SDHCI_CTRL_8BITBUS | SDHCI_CTRL_HISPD;
        SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);
      }

      // Set clock to 52 MHz
      Status = SdhciSetClock (Device, 52000000);
      if (!EFI_ERROR (Status)) {
        DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switched to HS mode @ 52 MHz, 8-bit!\n"));
      }
    }
  }

  FreePool (ExtCsd);

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: eMMC initialization complete!\n"));
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: RCA = %d\n", Device->RelativeCardAddress));
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Block size = %d bytes\n", Device->BlockSize));
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Total blocks = %ld\n", Device->TotalBlocks));

  return EFI_SUCCESS;
}

