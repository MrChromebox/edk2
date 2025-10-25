/** @file
  SD card initialization routines for Depthcharge-based SD/MMC driver.

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciGliDxe.h"

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
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD55 failed: %r\n", Status));
    return Status;
  }

  // Then send the actual application command
  Status = SdhciSendCommand (Device, Acmd, Arg, ResponseType, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: ACMD%d failed: %r\n", Acmd, Status));
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
  UINT16      HostControl2;
  UINT32      PresentState;
  UINT8       HostControl;
  UINT32      Arg;
  BOOLEAN     CardSupports1_8V;
  UINT32      Caps1;
  BOOLEAN     ControllerSupportsUHS;
  UINT32      DatTimeout;
  UINT32      DatLines;
  UINT8       CsdStructure;
  UINT32      ReadBlLen;
  UINT32      CSize;
  UINT32      CMult;
  UINT8       *SwitchStatus;
  UINT8       SupportedModes;
  UINT8       TargetMode;
  UINT32      TargetClock;
  UINT16      UhsMode;
  CONST CHAR8 *ModeName;
  UINT32      SwitchArg;
  BOOLEAN     TuningSuccess;
  UINT32      i;
  UINT32      Timeout;
  UINT32      IntStatus;

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: SdStartup begin\n"));

  //
  // Check if card is present (for removable SD cards)
  //
  PresentState = SdhciReadl (Device, SDHCI_PRESENT_STATE);
  if (!(PresentState & SDHCI_CARD_PRESENT)) {
    DEBUG ((DEBUG_WARN, "SdMmcPciGli: No SD card present (PRESENT_STATE=0x%08x)\n", PresentState));
    return EFI_NOT_FOUND;
  }
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Card present detected\n"));

  //
  // Clear any leftover timing modes from previous boot/hot-plug
  // This is critical for release builds where controller state might be inconsistent
  // IMPORTANT: Only clear timing/bus width bits, preserve DMA mode set by SdhciInit
  //
  HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
  HostControl &= ~(SDHCI_CTRL_HISPD | SDHCI_CTRL_4BITBUS | SDHCI_CTRL_8BITBUS);  // Clear timing modes
  SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);

  // Clear HOST_CONTROL2 UHS mode bits (preserve 1.8V signaling if set)
  HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
  HostControl2 &= ~0x0007;  // Clear UHS mode select bits [0:2], preserve 1.8V signaling [3]
  SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Cleared timing modes for clean start\n"));

  //
  // CMD0: GO_IDLE_STATE
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Sending CMD0 (GO_IDLE_STATE)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_GO_IDLE_STATE, 0, 0, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD0 failed: %r\n", Status));
    return Status;
  }

  // 10ms delay after CMD0 - SD card needs time to exit idle state
  // This delay is critical for release builds (debug output provides natural delay)
  gBS->Stall (25000);

  //
  // CMD8: SEND_IF_COND (check if card supports 2.7-3.6V and SD 2.0+)
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Sending CMD8 (SEND_IF_COND)\n"));
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
    DEBUG ((DEBUG_WARN, "SdMmcPciGli: CMD8 not supported - SD 1.x card?\n"));
    // Continue anyway - we'll try ACMD41 without HCS bit
  } else {
    // Check echo-back pattern
    if ((Response[0] & 0xFF) != SD_CMD8_CHECK_PATTERN) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD8 pattern mismatch: 0x%02x\n", Response[0] & 0xFF));
      return EFI_DEVICE_ERROR;
    }
    DEBUG ((DEBUG_INFO, "SdMmcPciGli: SD 2.0+ card detected\n"));
  }

  //
  // ACMD41: SD_SEND_OP_COND - Poll until card is ready
  //
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Polling ACMD41 (SD_SEND_OP_COND)\n"));

  // Note: For ACMD41 during initialization, RCA is 0
  Device->RelativeCardAddress = 0;

  Retry = 100;
  while (Retry > 0) {
    // Send ACMD41 with HCS (High Capacity Support), S18R (1.8V request), and voltage range
    Arg = SD_OCR_HCS | SD_OCR_S18R | SD_OCR_VDD_32_33 | 0x00FF8000;  // 2.7-3.6V range + 1.8V switching

    Status = SdSendAppCmd (Device, SD_ACMD_SEND_OP_COND, Arg, MMC_RSP_R3, Response);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: ACMD41 failed: %r\n", Status));
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
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: ACMD41 timeout - card not ready\n"));
    return EFI_DEVICE_ERROR;
  }

  // Check if High Capacity
  Device->HighCapacity = (Ocr & OCR_HCS) != 0;
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: SD card ready! OCR=0x%08x, HighCapacity=%d\n",
          Ocr, Device->HighCapacity));

  //
  // Perform voltage switch to 1.8V if supported (UHS-I requirement)
  // Must be done early, before CMD2/CMD3
  //
  // Check voltage switch support
  CardSupports1_8V = (Ocr & SD_OCR_S18R) != 0;
  Caps1 = SdhciReadl (Device, SDHCI_CAPABILITIES_1);
  ControllerSupportsUHS = ((Caps1 & (SDHCI_SUPPORT_SDR50 | SDHCI_SUPPORT_SDR104 | SDHCI_SUPPORT_DDR50)) != 0);

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: UHS-I capability check: Ctrl SDR50=%d, Ctrl 1.8V=%d, Card 1.8V=%d, IsGL9750=%d\n",
          (Caps1 & SDHCI_SUPPORT_SDR50) ? 1 : 0,
          ControllerSupportsUHS ? 1 : 0,
          CardSupports1_8V ? 1 : 0,
          Device->IsGL9750 ? 1 : 0));

  if (CardSupports1_8V && ControllerSupportsUHS) {
    // GL9750 voltage switch may fail on some boards (board lacks 1.8V regulator)
    // If voltage switch fails and recovery also fails, just skip UHS-I for GL9750
    // Card will work fine at High Speed (50 MHz) in 3.3V mode
    DEBUG ((DEBUG_INFO, "SdMmcPciGli: Attempting voltage switch to 1.8V for UHS-I...\n"));

    // Send CMD11
    Status = SdhciSendCommand (Device, SD_VOLTAGE_SWITCH, 0, MMC_RSP_R1, Response, NULL, 0, 0, FALSE);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "SdMmcPciGli: CMD11 failed: %r, continuing without UHS\n", Status));
    } else {
      // Stop SD clock
      SdhciWritew (Device, SDHCI_CLOCK_CONTROL, 0);
      gBS->Stall (10000);  // 10ms

      // Check DAT[3:0] lines should be 0
      PresentState = SdhciReadl (Device, SDHCI_PRESENT_STATE);
      if (((PresentState >> 20) & 0xF) != 0) {
        DEBUG ((DEBUG_WARN, "SdMmcPciGli: Voltage switch failed - DAT lines not 0 (PresentState=0x%08x)\n", PresentState));
      } else {
        // Set 1.8V signaling enable bit
        HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
        HostControl2 |= SDHCI_CTRL_180V_SIGNALING_ENABLE;
        SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);

        // GL9750/GL9755 require extended delay for voltage regulator stabilization
        // Per Linux kernel sdhci-pci-gli.c: Both use sdhci_gli_voltage_switch()
        // which waits 100ms - "controller requires far longer than 5ms before
        // the card will actually work reliably"
        gBS->Stall (100000);

        // Verify bit is set
        HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
        if (!(HostControl2 & SDHCI_CTRL_180V_SIGNALING_ENABLE)) {
          DEBUG ((DEBUG_WARN, "SdMmcPciGli: 1.8V signaling bit not set\n"));
        } else {
          DEBUG ((DEBUG_INFO, "SdMmcPciGli: Voltage switched to 1.8V successfully\n"));
        }
      }

      // Restart SD clock
      SdhciSetClock (Device, 400000);  // Restart at 400 KHz

      // Poll for DAT[3:0] to go high (card signals voltage switch complete)
      DatTimeout = 100000;  // 100ms timeout
      DatLines = 0;
      while (DatTimeout > 0) {
        PresentState = SdhciReadl (Device, SDHCI_PRESENT_STATE);
        DatLines = (PresentState >> 20) & 0xF;
        if (DatLines == 0xF) {
          break;  // Success!
        }
        gBS->Stall (1000);  // 1ms between polls
        DatTimeout -= 1000;
      }

      DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Voltage switch: DAT[3:0] went high in %d ms\n",
              (100000 - DatTimeout) / 1000));

      if (DatLines != 0xF) {
        DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Voltage switch validation failed - DAT[3:0]=0x%x (expected 0xF)\n", DatLines));
        DEBUG ((DEBUG_ERROR, "SdMmcPciGli: All GL devices support 1.8V - this indicates a hardware issue\n"));
        return EFI_DEVICE_ERROR;
      }
    }
  }

  //
  // CMD2: ALL_SEND_CID
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Sending CMD2 (ALL_SEND_CID)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_ALL_SEND_CID, 0, MMC_RSP_R2, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD2 failed: %r\n", Status));
    return Status;
  }

  // Save CID for DiskInfo protocol (SD format, not eMMC format)
  CopyMem (((UINT8 *)&Device->Cid.SdCid) + 1, &Response[0], sizeof (SD_CID) - 1);

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: CID = %08x %08x %08x %08x\n",
          Response[0], Response[1], Response[2], Response[3]));

  //
  // CMD3: SEND_RELATIVE_ADDR (SD card selects its own RCA)
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Sending CMD3 (SEND_RELATIVE_ADDR)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_SET_RELATIVE_ADDR, 0, MMC_RSP_R6, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD3 failed: %r\n", Status));
    return Status;
  }

  // Extract RCA from response (upper 16 bits)
  Device->RelativeCardAddress = Response[0] >> 16;
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: RCA = 0x%04x\n", Device->RelativeCardAddress));

  //
  // CMD9: SEND_CSD - Get Card Specific Data
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Sending CMD9 (SEND_CSD)\n"));
  Status = SdhciSendCommand (
             Device,
             SD_CMD_SEND_CSD,
             Device->RelativeCardAddress << 16,
             MMC_RSP_R2,
             Response,
             NULL, 0, 0, FALSE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD9 failed: %r\n", Status));
    return Status;
  }

  // Parse CSD to get capacity
  // Extract CSD_STRUCTURE from bits [127:126] (first 2 bits of Response[0])
  CsdStructure = (Response[0] >> 30) & 0x03;

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: CSD Response = %08x %08x %08x %08x\n",
          Response[0], Response[1], Response[2], Response[3]));
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: CSD_STRUCTURE = %d\n", CsdStructure));

  if (CsdStructure == 0) {
    // CSD Version 1.0 (SDSC): C_SIZE at bits [73:62], C_SIZE_MULT at bits [49:47], READ_BL_LEN at bits [83:80]
    ReadBlLen = (Response[1] >> 16) & 0x0F;  // bits [83:80]
    CSize = ((Response[1] & 0x03FF) << 2) | ((Response[2] >> 30) & 0x03);  // bits [73:62]
    CMult = (Response[2] >> 15) & 0x07;  // bits [49:47]

    Device->TotalBlocks = (CSize + 1) * (1 << (CMult + 2)) * (1 << ReadBlLen) / 512;
  } else if (CsdStructure == 1) {
    // CSD Version 2.0 (SDHC/SDXC): C_SIZE at bits [69:48]
    CSize = ((Response[1] & 0x3F) << 16) | (Response[2] >> 16);  // bits [69:48]
    Device->TotalBlocks = (CSize + 1) * 1024;  // In 512-byte blocks
  } else {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Unknown CSD structure version: %d\n", CsdStructure));
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: SD capacity: %lld blocks (CSD v%d)\n",
          Device->TotalBlocks, CsdStructure));

  //
  // CMD7: SELECT_CARD
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Sending CMD7 (SELECT_CARD)\n"));
  Status = SdhciSendCommand (
             Device,
             MMC_CMD_SELECT_CARD,
             Device->RelativeCardAddress << 16,
             MMC_RSP_R1B,
             Response,
             NULL, 0, 0, FALSE
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD7 failed: %r\n", Status));
    return Status;
  }

  //
  // Set block size to 512 bytes
  //
  Device->BlockSize = 512;
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Sending CMD16 (SET_BLOCKLEN to 512)\n"));
  Status = SdhciSendCommand (Device, MMC_CMD_SET_BLOCKLEN, 512, MMC_RSP_R1, Response, NULL, 0, 0, FALSE);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD16 failed: %r\n", Status));
    return Status;
  }

  //
  // Switch to 4-bit bus width using ACMD6
  //
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Switching to 4-bit bus width\n"));
  Status = SdSendAppCmd (Device, SD_ACMD_SET_BUS_WIDTH, 2, MMC_RSP_R1, Response);  // 2 = 4-bit
  if (!EFI_ERROR (Status)) {
    // Set controller to 4-bit mode
    HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
    HostControl |= SDHCI_CTRL_4BITBUS;
    SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Switched to 4-bit bus width\n"));
  }

  //
  // Switch to High Speed mode (50 MHz) if supported
  // Read Switch Function status to check HS support
  // Note: Must be DMA-aligned for ADMA transfers
  //
  SwitchStatus = AllocateAlignedPages (EFI_SIZE_TO_PAGES (64), 512);
  if (SwitchStatus == NULL) {
    DEBUG ((DEBUG_WARN, "SdMmcPciGli: Failed to allocate SwitchStatus buffer\n"));
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
      DEBUG ((DEBUG_INFO, "SdMmcPciGli: High Speed supported, switching...\n"));

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
        SdhciWritew (Device, SDHCI_CLOCK_CONTROL, 0);

        // Set HISPD bit in HOST_CONTROL
        HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
        HostControl |= SDHCI_CTRL_HISPD;
        SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);

        // Increase clock to 50 MHz
        Status = SdhciSetClock (Device, 50000000);
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "SdMmcPciGli: Switched to High Speed @ 50 MHz!\n"));
        }

        //
        // Try to upgrade to UHS-I modes if 1.8V signaling is enabled
        //
        HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
        if (HostControl2 & SDHCI_CTRL_180V_SIGNALING_ENABLE) {
          DEBUG ((DEBUG_INFO, "SdMmcPciGli: 1.8V signaling enabled, attempting UHS-I mode upgrade...\n"));

          // Try SDR104 first (fastest), then SDR50
          TargetMode = 0;
          TargetClock = 0;
          UhsMode = 0;
          ModeName = NULL;

          // Check card capabilities via CMD6 mode 0 (check)
          Status = SdhciSendCommand (
                     Device,
                     MMC_CMD_SWITCH,
                     0x00FFFF01,  // Mode=0 (check), Group1=0xF (query all)
                     MMC_RSP_R1,
                     Response,
                     SwitchStatus,
                     64,
                     1,
                     TRUE
                     );

          if (!EFI_ERROR (Status)) {
            // Check which UHS modes are supported (byte 13, bits 400-407)
            // Bit 403=SDR104, 402=SDR50, 401=HS (already set), 400=Default
            SupportedModes = SwitchStatus[13];
            DEBUG ((DEBUG_INFO, "SdMmcPciGli: Card supported modes = 0x%02x\n", SupportedModes));

            // Try SDR104 @ 200 MHz (bit 3) - will be programmed to 205MHz by PLL
            if (SupportedModes & 0x08) {
              TargetMode = 3;  // SDR104
              TargetClock = 200000000;
              UhsMode = SDHCI_CTRL_UHS_SDR104;
              ModeName = "SDR104";
            }
            // Try SDR50 @ 100 MHz (bit 2)
            else if (SupportedModes & 0x04) {
              TargetMode = 2;  // SDR50
              TargetClock = 100000000;
              UhsMode = SDHCI_CTRL_UHS_SDR50;
              ModeName = "SDR50";
            }

            if (TargetMode != 0) {
              DEBUG ((DEBUG_INFO, "SdMmcPciGli: Attempting to switch to %a...\n", ModeName));

              // CMD6 mode 1 (switch) to target mode
              SwitchArg = 0x80FFFF00 | TargetMode;  // Mode=1 (switch), Group1=target
              Status = SdhciSendCommand (
                         Device,
                         MMC_CMD_SWITCH,
                         SwitchArg,
                         MMC_RSP_R1,
                         Response,
                         SwitchStatus,
                         64,
                         1,
                         TRUE
                         );

              if (!EFI_ERROR (Status)) {
                // Stop clock before changing timing
                SdhciWritew (Device, SDHCI_CLOCK_CONTROL, 0);

                // Set UHS mode in HOST_CONTROL2
                HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
                HostControl2 &= ~SDHCI_CTRL_UHS_MASK;
                HostControl2 |= UhsMode;
                SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);

                // Set the target clock frequency
                Status = SdhciSetClock (Device, TargetClock);
                if (!EFI_ERROR (Status)) {
                  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Switched to UHS-I %a @ %d MHz!\n",
                          ModeName, TargetClock / 1000000));

                  // Perform tuning for SDR104 mode (required by SD specification)
                  // For GL9750, follow Linux driver's two-pass RX inversion approach
                  if (UhsMode == SDHCI_CTRL_UHS_SDR104) {
                    DEBUG ((DEBUG_INFO, "SdMmcPciGli: Performing SDR104 tuning...\n"));

                    TuningSuccess = FALSE;

                    // Setup GL9750-specific tuning parameters if needed
                    if (Device->IsGL9750) {
                      Gl9750SetupTuning (Device);
                    }

                    // Standard SDHCI tuning (works for both GL9750 and GL9755)
                    HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
                    HostControl2 |= SDHCI_CTRL_EXEC_TUNING;
                    SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);

                    // Send tuning commands (CMD19) - up to 40 iterations
                    // Note: Linux uses PIO mode for tuning, not ADMA
                    for (i = 0; i < 40; i++) {
                      // Clear interrupts
                      SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);

                      // Set block size (64 bytes for 4-bit SD bus)
                      SdhciWritew (Device, SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, 64));

                      // Set transfer mode (read, NO DMA - tuning uses PIO)
                      SdhciWritew (Device, SDHCI_TRANSFER_MODE, SDHCI_TRNS_READ);

                      // Set argument (0 for CMD19)
                      SdhciWritel (Device, SDHCI_ARGUMENT, 0);

                      // Send CMD19
                      SdhciWritew (Device, SDHCI_COMMAND,
                        SDHCI_MAKE_CMD(19, (SDHCI_CMD_RESP_SHORT | SDHCI_CMD_CRC |
                                           SDHCI_CMD_INDEX | SDHCI_CMD_DATA)));

                      // Wait for command/data completion
                      Timeout = 50000;  // 50ms per tuning command
                      while (Timeout > 0) {
                        IntStatus = SdhciReadl (Device, SDHCI_INT_STATUS);
                        if (IntStatus & (SDHCI_INT_ERROR | SDHCI_INT_DATA_AVAIL | SDHCI_INT_DATA_END)) {
                          // Clear the interrupt
                          SdhciWritel (Device, SDHCI_INT_STATUS, IntStatus);
                          break;
                        }
                        gBS->Stall (10);
                        Timeout -= 10;
                      }

                      // Check if tuning is complete (hardware clears EXEC_TUNING when done)
                      HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
                      if (!(HostControl2 & SDHCI_CTRL_EXEC_TUNING)) {
                        // Tuning finished
                        if (HostControl2 & SDHCI_CTRL_TUNED_CLK) {
                          DEBUG ((DEBUG_INFO, "SdMmcPciGli: SDR104 tuning succeeded after %d iterations\n", i + 1));
                          TuningSuccess = TRUE;
                        }
                        break;
                      }
                    }

                    if (!TuningSuccess) {
                      DEBUG ((DEBUG_WARN, "SdMmcPciGli: SDR104 tuning failed - falling back to SDR50\n"));

                      // Clear tuning bits
                      HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
                      HostControl2 &= ~(SDHCI_CTRL_TUNED_CLK | SDHCI_CTRL_EXEC_TUNING);
                      SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);

                      // Fall back to SDR50
                      HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
                      HostControl2 &= ~SDHCI_CTRL_UHS_MASK;
                      HostControl2 |= SDHCI_CTRL_UHS_SDR50;
                      SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);

                      // Set SDR50 clock (100 MHz) - SdhciSetClock will program PLL
                      SdhciSetClock (Device, 100000000);

                      DEBUG ((DEBUG_INFO, "SdMmcPciGli: Switched to SDR50 @ 100 MHz\n"));
                    }
                  }
                } else {
                  DEBUG ((DEBUG_WARN, "SdMmcPciGli: Failed to set %d MHz clock\n", TargetClock / 1000000));
                }
              } else {
                DEBUG ((DEBUG_WARN, "SdMmcPciGli: CMD6 switch to %a failed: %r\n", ModeName, Status));
              }
            } else {
              DEBUG ((DEBUG_INFO, "SdMmcPciGli: No UHS-I modes supported by card, staying at HS\n"));
            }
          } else {
            DEBUG ((DEBUG_WARN, "SdMmcPciGli: CMD6 check for UHS modes failed: %r\n", Status));
          }
        }
      }  // Close if (!EFI_ERROR (Status)) from CMD6 mode 1 for HS
    } // Close if (SwitchStatus[13] & 0x02) - HS supported check
    else {  // HS not supported
      DEBUG ((DEBUG_INFO, "SdMmcPciGli: High Speed not supported, staying at 25 MHz\n"));
      Status = SdhciSetClock (Device, 25000000);
    }
  } // Close if (!EFI_ERROR (Status)) from line 304 (CMD6 mode 0 for HS)
  else {
    // CMD6 failed, stay at default speed
    DEBUG ((DEBUG_WARN, "SdMmcPciGli: CMD6 failed, staying at default speed\n"));
    Status = SdhciSetClock (Device, 25000000);
  }

  // Free the aligned buffer
  FreeAlignedPages (SwitchStatus, EFI_SIZE_TO_PAGES (64));

SkipHighSpeed:
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: SD card initialization complete!\n"));
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: RCA = 0x%04x\n", Device->RelativeCardAddress));
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Block size = %d bytes\n", Device->BlockSize));
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Total blocks = %lld\n", Device->TotalBlocks));

  return EFI_SUCCESS;
}

