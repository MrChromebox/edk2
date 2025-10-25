/** @file
  SD card initialization routines for Depthcharge-based SD/MMC driver.

  Copyright (c) 2024, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciCbDxe.h"

/**
  Send SD application command (ACMD).
  This is a helper that sends CMD55 followed by the actual ACMD.

  @param[in]  Device        Device context
  @param[in]  Acmd          Application command index
  @param[in]  Arg           Command argument
  @param[in]  ResponseType  Expected response type
  @param[out] Response      Response buffer

  @retval EFI_SUCCESS       Command succeeded
  @retval Others            Command failed
**/
STATIC
EFI_STATUS
SdSendAppCmd (
  IN  SD_MMC_CB_DEVICE  *Device,
  IN  UINT32            Acmd,
  IN  UINT32            Arg,
  IN  UINT32            ResponseType,
  OUT UINT32            *Response
  )
{
  EFI_STATUS  Status;
  UINT32      Rca;

  // First send CMD55 (APP_CMD) with RCA
  Rca = Device->RelativeCardAddress << 16;
  Status = SdhciSendCommand (Device, SD_CMD_APP_CMD, Rca, MMC_RSP_R1, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD55 failed: %r\n", Status));
    return Status;
  }

  // Then send the actual application command
  Status = SdhciSendCommand (Device, Acmd, Arg, ResponseType, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: ACMD%d failed: %r\n", Acmd, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Initialize SD card.

  Based on SD Physical Layer Simplified Specification and Depthcharge's sd.c

  @param[in] Device  Device context

  @retval EFI_SUCCESS           SD card initialized successfully
  @retval EFI_DEVICE_ERROR      SD card initialization failed
  @retval EFI_NOT_FOUND         No SD card present
**/
EFI_STATUS
SdStartup (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Response[4];
  UINT32      Ocr;
  UINT32      Retry;
  UINT8       Csd[16];

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: SdStartup begin\n"));

  //
  // Check if card is present (for removable SD cards)
  //
  UINT32  PresentState = SdhciReadl (Device, SDHCI_PRESENT_STATE);
  if (!(PresentState & SDHCI_CARD_PRESENT)) {
    DEBUG ((DEBUG_WARN, "SdMmcPciCb: No SD card present (PRESENT_STATE=0x%08x)\n", PresentState));
    return EFI_NOT_FOUND;
  }
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Card present detected\n"));

  //
  // CMD0: GO_IDLE_STATE
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD0 (GO_IDLE_STATE)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_GO_IDLE_STATE, 0, 0, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD0 failed: %r\n", Status));
    return Status;
  }

  //
  // CMD8: SEND_IF_COND (check if card supports 2.7-3.6V and SD 2.0+)
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD8 (SEND_IF_COND)\n"));
  Status = SdhciSendCommand (
             Device,
             SD_CMD_SEND_IF_COND,
             SD_CMD8_VHS_27_36 | SD_CMD8_CHECK_PATTERN,
             MMC_RSP_R7,
             Response,
             NULL, 0, 0, FALSE
             );

  if (EFI_ERROR (Status)) {
    // Card doesn't support CMD8 - might be SD 1.x
    DEBUG ((DEBUG_WARN, "SdMmcPciCb: CMD8 not supported - SD 1.x card?\n"));
    // Continue anyway - we'll try ACMD41 without HCS bit
  } else {
    // Check echo-back pattern
    if ((Response[0] & 0xFF) != SD_CMD8_CHECK_PATTERN) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD8 pattern mismatch: 0x%02x\n", Response[0] & 0xFF));
      return EFI_DEVICE_ERROR;
    }
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: SD 2.0+ card detected\n"));
  }

  //
  // ACMD41: SD_SEND_OP_COND - Poll until card is ready
  //
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Polling ACMD41 (SD_SEND_OP_COND)\n"));

  // Note: For ACMD41 during initialization, RCA is 0
  Device->RelativeCardAddress = 0;

  Retry = 100;
  while (Retry > 0) {
    // Send ACMD41 with HCS (High Capacity Support) and voltage range
    UINT32 Arg = SD_OCR_HCS | SD_OCR_VDD_32_33 | 0x00FF8000;  // 2.7-3.6V range

    Status = SdSendAppCmd (Device, SD_ACMD_SEND_OP_COND, Arg, MMC_RSP_R3, Response);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: ACMD41 failed: %r\n", Status));
      return Status;
    }

    Ocr = Response[0];

    // Check if card is ready (busy bit clear)
    if (Ocr & OCR_BUSY) {
      break;
    }

    gBS->Stall (10000);  // 10ms delay
    Retry--;
  }

  if (Retry == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: ACMD41 timeout - card not ready\n"));
    return EFI_DEVICE_ERROR;
  }

  // Check if High Capacity
  Device->HighCapacity = (Ocr & OCR_HCS) != 0;
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: SD card ready! OCR=0x%08x, HighCapacity=%d\n",
          Ocr, Device->HighCapacity));

  //
  // CMD2: ALL_SEND_CID
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD2 (ALL_SEND_CID)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_ALL_SEND_CID, 0, MMC_RSP_R2, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD2 failed: %r\n", Status));
    return Status;
  }

  // Save CID for DiskInfo protocol (SD format, not eMMC format)
  CopyMem (((UINT8 *)&Device->Cid.SdCid) + 1, &Response[0], sizeof (SD_CID) - 1);

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: CID = %08x %08x %08x %08x\n",
          Response[0], Response[1], Response[2], Response[3]));

  //
  // CMD3: SEND_RELATIVE_ADDR (SD card selects its own RCA)
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD3 (SEND_RELATIVE_ADDR)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_SET_RELATIVE_ADDR, 0, MMC_RSP_R6, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD3 failed: %r\n", Status));
    return Status;
  }

  // Extract RCA from response (upper 16 bits)
  Device->RelativeCardAddress = Response[0] >> 16;
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: RCA = 0x%04x\n", Device->RelativeCardAddress));

  //
  // CMD9: SEND_CSD - Get Card Specific Data
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD9 (SEND_CSD)\n"));
  Status = SdhciSendCommand (
             Device,
             SD_CMD_SEND_CSD,
             Device->RelativeCardAddress << 16,
             MMC_RSP_R2,
             Response,
             NULL, 0, 0, FALSE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD9 failed: %r\n", Status));
    return Status;
  }

  // Parse CSD to get capacity
  CopyMem (((UINT8 *)Csd) + 1, &Response[0], 15);

  UINT8 CsdStructure = (Csd[0] & 0xC0) >> 6;

  if (CsdStructure == 0) {
    // CSD Version 1.0 (SDSC)
    UINT32 ReadBlLen = Csd[5] & 0x0F;
    UINT32 CMult = ((Csd[9] & 0x03) << 1) | ((Csd[10] & 0x80) >> 7);
    UINT32 CSize = ((Csd[6] & 0x03) << 10) | (Csd[7] << 2) | ((Csd[8] & 0xC0) >> 6);

    Device->TotalBlocks = (CSize + 1) * (1 << (CMult + 2)) * (1 << ReadBlLen) / 512;
  } else if (CsdStructure == 1) {
    // CSD Version 2.0 (SDHC/SDXC)
    UINT32 CSize = ((Csd[7] & 0x3F) << 16) | (Csd[8] << 8) | Csd[9];
    Device->TotalBlocks = (CSize + 1) * 1024;  // In 512-byte blocks
  } else {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Unknown CSD structure version: %d\n", CsdStructure));
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: SD capacity: %lld blocks (CSD v%d)\n",
          Device->TotalBlocks, CsdStructure));

  //
  // CMD7: SELECT_CARD
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD7 (SELECT_CARD)\n"));
  Status = SdhciSendCommand (
             Device,
             MMC_CMD_SELECT_CARD,
             Device->RelativeCardAddress << 16,
             MMC_RSP_R1B,
             Response,
             NULL, 0, 0, FALSE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD7 failed: %r\n", Status));
    return Status;
  }

  //
  // Set block size to 512 bytes
  //
  Device->BlockSize = 512;
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Sending CMD16 (SET_BLOCKLEN to 512)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_SET_BLOCKLEN, 512, MMC_RSP_R1, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD16 failed: %r\n", Status));
    return Status;
  }

  //
  // Switch to 4-bit bus width using ACMD6
  //
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switching to 4-bit bus width\n"));
  Status = SdSendAppCmd (Device, SD_ACMD_SET_BUS_WIDTH, 2, MMC_RSP_R1, Response);  // 2 = 4-bit
  if (!EFI_ERROR (Status)) {
    // Set controller to 4-bit mode
    UINT8  HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
    HostControl |= SDHCI_CTRL_4BITBUS;
    SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switched to 4-bit bus width\n"));
  }

  //
  // Switch to High Speed mode (50 MHz) if supported
  // Read Switch Function status to check HS support
  // Note: Must be DMA-aligned for ADMA transfers
  //
  UINT8  *SwitchStatus = AllocateAlignedPages (EFI_SIZE_TO_PAGES (64), 512);
  if (SwitchStatus == NULL) {
    DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to allocate SwitchStatus buffer\n"));
    Status = SdhciSetClock (Device, 25000000);
    goto SkipHighSpeed;
  }

  // CMD6: SWITCH_FUNC - Check if High Speed is supported (Mode 0 = check)
  Status = SdhciSendCommand (
             Device,
             MMC_CMD_SWITCH,
             0x00FFFF01,  // Mode=0 (check), Group1=1 (HS), keep others at 0xF
             MMC_RSP_R1,
             Response,
             SwitchStatus,
             64,
             1,
             TRUE
             );

  if (!EFI_ERROR (Status)) {
    // Check if High Speed is supported (bits 401-415 in switch status)
    if (SwitchStatus[13] & 0x02) {
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: High Speed supported, switching...\n"));

      // CMD6: SWITCH_FUNC - Switch to High Speed (Mode 1 = switch)
      Status = SdhciSendCommand (
                 Device,
                 MMC_CMD_SWITCH,
                 0x80FFFF01,  // Mode=1 (switch), Group1=1 (HS)
                 MMC_RSP_R1,
                 Response,
                 SwitchStatus,
                 64,
                 1,
                 TRUE
                 );

      if (!EFI_ERROR (Status)) {
        // Disable clock before changing timing (required by SDHCI spec)
        SdhciWritew (Device, 0, SDHCI_CLOCK_CONTROL);

        // Set HISPD bit in HOST_CONTROL
        UINT8  HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
        HostControl |= SDHCI_CTRL_HISPD;
        SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);

        // Increase clock to 50 MHz
        Status = SdhciSetClock (Device, 50000000);
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "SdMmcPciCb: Switched to High Speed @ 50 MHz!\n"));
        }

        // Reset CMD and DATA lines to clear any stuck state from CMD6
        SdhciReset (Device, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
        DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Controller reset after speed switch\n"));
      }
    } else {
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: High Speed not supported, staying at 25 MHz\n"));
      Status = SdhciSetClock (Device, 25000000);
    }
  } else {
    // CMD6 failed, stay at default speed
    DEBUG ((DEBUG_WARN, "SdMmcPciCb: CMD6 failed, staying at default speed\n"));
    Status = SdhciSetClock (Device, 25000000);
  }

  // Free the aligned buffer
  FreeAlignedPages (SwitchStatus, EFI_SIZE_TO_PAGES (64));

SkipHighSpeed:
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: SD card initialization complete!\n"));
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: RCA = 0x%04x\n", Device->RelativeCardAddress));
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Block size = %d bytes\n", Device->BlockSize));
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Total blocks = %lld\n", Device->TotalBlocks));

  return EFI_SUCCESS;
}

