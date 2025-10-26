/** @file
  SDHCI Core Functions (ported from Depthcharge sdhci.c)

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciCbDxe.h"

/**
  Reset the SDHCI controller.

  @param[in] Device  Device context
  @param[in] Mask    Reset mask (SDHCI_RESET_ALL, SDHCI_RESET_CMD, SDHCI_RESET_DATA)

  @retval EFI_SUCCESS  Reset successful
  @retval other        Reset failed
**/
EFI_STATUS
SdhciReset (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT8             Mask
  )
{
  UINT32  Timeout;
  UINT8   Value;

  // DEBUG ((DEBUG_INFO, "SdMmcPciCb: SdhciReset(0x%02x)\n", Mask));

  //
  // Write reset mask
  //
  SdhciWriteb (Device, SDHCI_SOFTWARE_RESET, Mask);

  //
  // Wait for reset to complete (max 100ms)
  //
  Timeout = 100000; // 100ms in microseconds
  while (Timeout > 0) {
    Value = SdhciReadb (Device, SDHCI_SOFTWARE_RESET);
    if ((Value & Mask) == 0) {
      DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Reset complete\n"));
      return EFI_SUCCESS;
    }
    gBS->Stall (10);
    Timeout -= 10;
  }

  DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Reset timeout!\n"));
  return EFI_TIMEOUT;
}

/**
  Set power mode and voltage.

  @param[in] Device     Device context
  @param[in] PowerMode  Power mode (SDHCI_POWER_330, etc.)

  @retval EFI_SUCCESS  Power set successfully
**/
EFI_STATUS
SdhciSetPower (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT8             PowerMode
  )
{
  // DEBUG ((DEBUG_INFO, "SdMmcPciCb: SdhciSetPower(0x%02x)\n", PowerMode));

  //
  // Set voltage and power on
  //
  SdhciWriteb (Device, SDHCI_POWER_CONTROL, PowerMode | SDHCI_POWER_ON);

  //
  // Wait 10ms for power to stabilize
  //
  gBS->Stall (10000);

  return EFI_SUCCESS;
}

/**
  Set clock frequency.

  @param[in] Device  Device context
  @param[in] Clock   Desired clock frequency in Hz

  @retval EFI_SUCCESS  Clock set successfully
**/
EFI_STATUS
SdhciSetClock (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT32            Clock
  )
{
  UINT32  Div;
  UINT16  ClkReg;
  UINT32  Timeout;

  // DEBUG ((DEBUG_INFO, "SdMmcPciCb: SdhciSetClock(%d Hz)\n", Clock));

  //
  // Disable clock first
  //
  SdhciWritew (Device, SDHCI_CLOCK_CONTROL, 0);

  if (Clock == 0) {
    return EFI_SUCCESS;
  }

  //
  // Calculate divisor (SDHCI 3.0 spec: Actual Clock = Base Clock / (2 * Div))
  // Div = 0 means no division (full speed)
  //
  Div = 0;
  UINT32  ActualClock = Clock;
  if (Device->ClockBase > 0) {
    for (Div = 1; Div < 256; Div++) {
      if ((Device->ClockBase / (2 * Div)) <= Clock) {
        break;
      }
    }
    if (Div == 256) {
      Div = 255;
    }
    Div >>= 1;

    // Calculate actual clock
    if (Div == 0) {
      ActualClock = Device->ClockBase;
    } else {
      ActualClock = Device->ClockBase / (2 * Div);
    }

    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Clock: requested=%u, divider=%u, actual=%u Hz\n",
            Clock, Div, ActualClock));
  }

  //
  // Set divisor and enable internal clock
  //
  ClkReg = (Div << SDHCI_DIVIDER_SHIFT) | SDHCI_CLOCK_INT_EN;
  SdhciWritew (Device, SDHCI_CLOCK_CONTROL, ClkReg);

  //
  // Wait for internal clock to stabilize
  //
  Timeout = 20000; // 20ms
  while (Timeout > 0) {
    if (SdhciReadw (Device, SDHCI_CLOCK_CONTROL) & SDHCI_CLOCK_INT_STABLE) {
      break;
    }
    gBS->Stall (10);
    Timeout -= 10;
  }

  if (Timeout == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Internal clock never stabilized\n"));
    return EFI_TIMEOUT;
  }

  //
  // Enable SD clock
  //
  ClkReg |= SDHCI_CLOCK_CARD_EN;
  SdhciWritew (Device, SDHCI_CLOCK_CONTROL, ClkReg);

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Clock enabled\n"));

  return EFI_SUCCESS;
}

/**
  Send command (using Depthcharge's simple polling approach).

  @param[in]  Device        Device context
  @param[in]  Cmd           Command index
  @param[in]  Arg           Command argument
  @param[in]  ResponseType  Expected response type (MMC_RSP_*)
  @param[out] Response      Response buffer (4 x UINT32)
  @param[in]  DataBuffer    Optional data buffer for read/write
  @param[in]  BlockSize     Block size for data transfer
  @param[in]  BlockCount    Number of blocks to transfer
  @param[in]  IsRead        TRUE for read, FALSE for write

  @retval EFI_SUCCESS  Command successful
  @retval other        Command failed
**/
EFI_STATUS
SdhciSendCommand (
  IN     SD_MMC_CB_DEVICE  *Device,
  IN     UINT32            Cmd,
  IN     UINT32            Arg,
  IN     UINT32            ResponseType,
  OUT    UINT32            *Response,
  IN     VOID              *DataBuffer    OPTIONAL,
  IN     UINT32            BlockSize      OPTIONAL,
  IN     UINT32            BlockCount     OPTIONAL,
  IN     BOOLEAN           IsRead         OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32  PresentState;
  UINT32  Timeout;
  UINT32  IntStatus;
  UINT16  CmdReg;
  UINT16  TransferMode;
  UINT8   Flags;

  // DEBUG ((DEBUG_INFO, "SdMmcPciCb: SdhciSendCommand CMD%d arg=0x%08x%s\n",
  //         Cmd, Arg, (DataBuffer != NULL) ? " [WITH DATA]" : ""));

  //
  // Wait for CMD line ready
  // For data commands, only wait for CMD_INHIBIT (not DATA_INHIBIT)
  //
  Timeout = 10000;
  UINT32 InhibitMask = SDHCI_CMD_INHIBIT;
  if (DataBuffer != NULL) {
    InhibitMask |= SDHCI_DATA_INHIBIT;
  }

  while (Timeout > 0) {
    PresentState = SdhciReadl (Device, SDHCI_PRESENT_STATE);
    if ((PresentState & InhibitMask) == 0) {
      break;
    }
    gBS->Stall (10);
    Timeout -= 10;
  }

  if (Timeout == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD%d line not ready! PresentState=0x%08x (need 0x%08x clear)\n",
            Cmd, PresentState, InhibitMask));
    return EFI_TIMEOUT;
  }

  //
  // Clear all status bits
  //
  SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);

  //
  // Set up data transfer if needed - Always use ADMA (like coreboot does)
  //
  if (DataBuffer != NULL && BlockSize > 0 && BlockCount > 0) {
    //
    // Set up ADMA descriptors
    //
    Status = SdhciSetupAdma (Device, DataBuffer, BlockSize * BlockCount, IsRead);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to setup ADMA: %r\n", Status));
      return Status;
    }

    //
    // Set block size and count
    //
    SdhciWritew (Device, SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, BlockSize));
    SdhciWritew (Device, SDHCI_BLOCK_COUNT, (UINT16)BlockCount);

    //
    // Set transfer mode with DMA enabled
    //
    TransferMode = SDHCI_TRNS_DMA;
    if (IsRead) {
      TransferMode |= SDHCI_TRNS_READ;
    }
    if (BlockCount > 1) {
      TransferMode |= SDHCI_TRNS_MULTI | SDHCI_TRNS_BLK_CNT_EN | SDHCI_TRNS_ACMD12;
    }
    SdhciWritew (Device, SDHCI_TRANSFER_MODE, TransferMode);
  }

  //
  // Set argument
  //
  SdhciWritel (Device, SDHCI_ARGUMENT, Arg);

  //
  // Build command register
  //
  Flags = 0;
  if (DataBuffer != NULL) {
    Flags |= SDHCI_CMD_DATA;
  }
  if (ResponseType & MMC_RSP_PRESENT) {
    if (ResponseType & MMC_RSP_136) {
      Flags |= SDHCI_CMD_RESP_LONG;  // Fixed: OR instead of assign
    } else if (ResponseType & MMC_RSP_BUSY) {
      Flags |= SDHCI_CMD_RESP_SHORT_BUSY;  // Fixed: OR instead of assign
    } else {
      Flags |= SDHCI_CMD_RESP_SHORT;  // Fixed: OR instead of assign
    }
  } else {
    Flags |= SDHCI_CMD_RESP_NONE;  // Fixed: OR instead of assign (though 0)
  }

  if (ResponseType & MMC_RSP_CRC) {
    Flags |= SDHCI_CMD_CRC;
  }
  if (ResponseType & MMC_RSP_OPCODE) {
    Flags |= SDHCI_CMD_INDEX;
  }

  CmdReg = SDHCI_MAKE_CMD (Cmd, Flags);

  //
  // Send command (debug output removed for performance)
  //
  SdhciWritew (Device, SDHCI_COMMAND, CmdReg);

  //
  // Poll for command complete
  //
  Timeout = 100000; // 100ms
  while (Timeout > 0) {
    IntStatus = SdhciReadl (Device, SDHCI_INT_STATUS);

    if (IntStatus & SDHCI_INT_ERROR) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: CMD%d error! IntStatus=0x%08x (", Cmd, IntStatus));
      if (IntStatus & SDHCI_INT_TIMEOUT) DEBUG ((DEBUG_ERROR, "TIMEOUT "));
      if (IntStatus & SDHCI_INT_CRC) DEBUG ((DEBUG_ERROR, "CRC "));
      if (IntStatus & SDHCI_INT_END_BIT) DEBUG ((DEBUG_ERROR, "ENDBIT "));
      if (IntStatus & SDHCI_INT_INDEX) DEBUG ((DEBUG_ERROR, "INDEX "));
      if (IntStatus & SDHCI_INT_DATA_TIMEOUT) DEBUG ((DEBUG_ERROR, "DATA_TIMEOUT "));
      if (IntStatus & SDHCI_INT_DATA_CRC) DEBUG ((DEBUG_ERROR, "DATA_CRC "));
      if (IntStatus & SDHCI_INT_DATA_END_BIT) DEBUG ((DEBUG_ERROR, "DATA_ENDBIT "));
      if (IntStatus & SDHCI_INT_ADMA_ERROR) DEBUG ((DEBUG_ERROR, "ADMA_ERROR "));
      DEBUG ((DEBUG_ERROR, ")\n"));
      SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
      return EFI_DEVICE_ERROR;
    }

    if (IntStatus & SDHCI_INT_RESPONSE) {
      //
      // Command complete!
      //
      SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_RESPONSE);

      //
      // Read response if needed
      //
      if (Response != NULL && (ResponseType & MMC_RSP_PRESENT)) {
        if (ResponseType & MMC_RSP_136) {
          // Long response (R2) - CRC is stripped so we need to do some shifting
          // Read in reverse order and shift by 8 bits (like Depthcharge does)
          Response[0] = SdhciReadl (Device, SDHCI_RESPONSE + 12) << 8;
          Response[1] = SdhciReadl (Device, SDHCI_RESPONSE + 8) << 8;
          Response[2] = SdhciReadl (Device, SDHCI_RESPONSE + 4) << 8;
          Response[3] = SdhciReadl (Device, SDHCI_RESPONSE + 0) << 8;
          // OR in the last byte from the previous register
          Response[0] |= (SdhciReadl (Device, SDHCI_RESPONSE + 8) >> 24) & 0xFF;
          Response[1] |= (SdhciReadl (Device, SDHCI_RESPONSE + 4) >> 24) & 0xFF;
          Response[2] |= (SdhciReadl (Device, SDHCI_RESPONSE + 0) >> 24) & 0xFF;
        } else {
          // Short response
          Response[0] = SdhciReadl (Device, SDHCI_RESPONSE);
        }
      }

      // DEBUG ((DEBUG_INFO, "SdMmcPciCb: CMD%d complete!\n", Cmd));

      //
      // If no data transfer, we're done
      //
      if (DataBuffer == NULL) {
        return EFI_SUCCESS;
      }

      //
      // ADMA Data Transfer - controller handles everything
      //
      return SdhciCompleteAdma (Device, 10000);
    }

    gBS->Stall (10);
    Timeout -= 10;
  }

  DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Command timeout!\n"));
  return EFI_TIMEOUT;
}

/**
  Initialize SDHCI controller.

  @param[in] Device  Device context

  @retval EFI_SUCCESS  Initialization successful
**/
/**
  Setup ADMA descriptor table for data transfer (from Depthcharge)

  @param[in]  Device      Pointer to device context
  @param[in]  DataBuffer  Data buffer address
  @param[in]  TotalBytes  Total bytes to transfer
  @param[in]  IsRead      TRUE for read, FALSE for write

  @retval EFI_SUCCESS     ADMA descriptors set up successfully
  @retval Others          Error occurred
**/
EFI_STATUS
SdhciSetupAdma (
  IN SD_MMC_CB_DEVICE  *Device,
  IN VOID              *DataBuffer,
  IN UINT32            TotalBytes,
  IN BOOLEAN           IsRead
  )
{
  UINT32                NeedDescriptors;
  UINT32                Remaining;
  UINT32                i;
  UINT8                 *BufferPtr;
  UINT16                Attributes;
  UINT32                DescLength;
  EFI_PHYSICAL_ADDRESS  DescTableAddr;

  if (TotalBytes == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: SdhciSetupAdma: Invalid TotalBytes=0\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate how many descriptors we need
  //
  NeedDescriptors = 1 + (TotalBytes / SDHCI_MAX_PER_DESCRIPTOR);

  //
  // Allocate or reallocate descriptor table if needed
  //
  if (Device->AdmaDescs == NULL || Device->AdmaDescCount < NeedDescriptors) {
    if (Device->AdmaDescs != NULL) {
      FreePool (Device->AdmaDescs);
      Device->AdmaDescs = NULL;
      Device->AdmaDescCount = 0;
    }

    Device->AdmaDescs = AllocatePool (NeedDescriptors * sizeof (SDHCI_ADMA_DESC));
    if (Device->AdmaDescs == NULL) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to allocate ADMA descriptors\n"));
      return EFI_OUT_OF_RESOURCES;
    }
    Device->AdmaDescCount = NeedDescriptors;
  }

  //
  // Build descriptor chain using direct physical addresses
  //
  BufferPtr = (UINT8 *)DataBuffer;
  Remaining = TotalBytes;

  for (i = 0; Remaining > 0; i++) {
    if (Remaining < SDHCI_MAX_PER_DESCRIPTOR) {
      DescLength = Remaining;
    } else {
      DescLength = SDHCI_MAX_PER_DESCRIPTOR;
    }
    Remaining -= DescLength;

    Attributes = SDHCI_ADMA_VALID | SDHCI_ACT_TRAN;
    if (Remaining == 0) {
      Attributes |= SDHCI_ADMA_END;
    }

    Device->AdmaDescs[i].Attributes = Attributes;
    Device->AdmaDescs[i].Length = (UINT16)(DescLength & 0xFFFF);
    Device->AdmaDescs[i].Address = (UINT32)(UINTN)BufferPtr;

    BufferPtr += DescLength;
  }

  //
  // Write descriptor table address to ADMA registers
  //
  DescTableAddr = (EFI_PHYSICAL_ADDRESS)(UINTN)Device->AdmaDescs;
  SdhciWritel (Device, SDHCI_ADMA_ADDRESS, (UINT32)DescTableAddr);
  if (Device->UseDma64) {
    SdhciWritel (Device, SDHCI_ADMA_ADDRESS_HI, (UINT32)(DescTableAddr >> 32));
  }

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: ADMA setup: %d descriptors, %d bytes\n",
          NeedDescriptors, TotalBytes));

  return EFI_SUCCESS;
}

/**
  Wait for ADMA transfer to complete (from Depthcharge)

  @param[in]  Device      Pointer to device context
  @param[in]  TimeoutMs   Timeout in milliseconds

  @retval EFI_SUCCESS     Transfer completed successfully
  @retval EFI_TIMEOUT     Transfer timed out
  @retval Others          Error occurred
**/
EFI_STATUS
SdhciCompleteAdma (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT32            TimeoutMs
  )
{
  UINT32  IntStatus;
  UINT32  Mask;
  UINT32  Retry;

  //
  // Wait for command response or data end
  // (On some controllers, DATA_END may arrive before RESPONSE for fast transfers)
  //
  Mask = SDHCI_INT_RESPONSE | SDHCI_INT_DATA_END | SDHCI_INT_ERROR;
  Retry = 10000;  // 10ms timeout for command

  while (Retry > 0) {
    IntStatus = SdhciReadl (Device, SDHCI_INT_STATUS);
    if ((IntStatus & Mask) != 0) {
      break;
    }
    gBS->Stall (1);  // 1 microsecond delay
    Retry--;
  }

  if (Retry == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: ADMA command timeout! IntStatus=0x%08x\n", IntStatus));
    SdhciReset (Device, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
    return EFI_TIMEOUT;
  }

  // Check for errors first
  if ((IntStatus & SDHCI_INT_ERROR) != 0) {
    UINT32 AdmaError = SdhciReadl (Device, SDHCI_ADMA_ERROR);
    UINT32 AdmaAddr = SdhciReadl (Device, SDHCI_ADMA_ADDRESS);
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: ADMA command error: IntStatus=0x%08x, AdmaError=0x%08x, AdmaAddr=0x%08x\n",
            IntStatus, AdmaError, AdmaAddr));

    // Reset controller
    SdhciReset (Device, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
    return EFI_DEVICE_ERROR;
  }

  // If we got DATA_END already, transfer is complete!
  if ((IntStatus & SDHCI_INT_DATA_END) != 0) {
    // DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: ADMA transfer complete (fast path)\n"));
    SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
    return EFI_SUCCESS;
  }

  // Otherwise, clear command response and wait for data
  SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_RESPONSE);
  // DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: ADMA command response received, waiting for data...\n"));

  //
  // Now wait for data transfer to complete
  //
  Mask = SDHCI_INT_DATA_END | SDHCI_INT_ERROR | SDHCI_INT_ADMA_ERROR;
  Retry = TimeoutMs * 1000;  // Convert to microseconds

  while (Retry > 0) {
    IntStatus = SdhciReadl (Device, SDHCI_INT_STATUS);
    if ((IntStatus & Mask) != 0) {
      break;
    }
    gBS->Stall (1);  // 1 microsecond delay
    Retry--;
  }

  // Clear all status bits
  SdhciWritel (Device, SDHCI_INT_STATUS, IntStatus);

  if (Retry == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: ADMA data timeout\n"));
    return EFI_TIMEOUT;
  }

  if ((IntStatus & (SDHCI_INT_ERROR | SDHCI_INT_ADMA_ERROR)) != 0) {
    UINT32  AdmaError = SdhciReadl (Device, SDHCI_ADMA_ERROR);
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: ADMA transfer error: IntStatus=0x%08x, AdmaError=0x%08x\n",
            IntStatus, AdmaError));

    // Reset data line
    SdhciReset (Device, SDHCI_RESET_DATA);
    return EFI_DEVICE_ERROR;
  }

  // DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: ADMA transfer complete\n"));
  return EFI_SUCCESS;
}

EFI_STATUS
SdhciInit (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Caps;

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: SdhciInit start\n"));

  //
  // Reset controller
  //
  Status = SdhciReset (Device, SDHCI_RESET_ALL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Power cycle to ensure clean state (important for warm boot)
  // On warm boot, card may be in high-speed mode (HS400/HS200/SDR50)
  //
  SdhciWriteb (Device, 0, SDHCI_POWER_CONTROL);
  gBS->Stall (10000);  // 10ms

  // Set appropriate voltage (1.8V for eMMC, will be adjusted for SD if needed)
  UINT8 PowerMode = SDHCI_POWER_ON;
  if (Device->IsEMMC) {
    PowerMode |= SDHCI_POWER_180;
  } else {
    PowerMode |= SDHCI_POWER_330;
  }
  SdhciWriteb (Device, PowerMode, SDHCI_POWER_CONTROL);
  gBS->Stall (10000);  // 10ms for power to stabilize

  //
  // Read capabilities to determine clock base and ADMA support
  //
  Caps = SdhciReadl (Device, SDHCI_CAPABILITIES);
  Device->ClockBase = ((Caps & SDHCI_CLOCK_V3_BASE_MASK) >> SDHCI_CLOCK_BASE_SHIFT) * 1000000;

  if (Device->ClockBase == 0) {
    // Fallback for older controllers
    Device->ClockBase = ((Caps & SDHCI_CLOCK_BASE_MASK) >> SDHCI_CLOCK_BASE_SHIFT) * 1000000;
  }

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Clock base = %d Hz\n", Device->ClockBase));

  //
  // Check for ADMA support and enable it
  // Force ADMA32 mode for compatibility (ADMA64 only needed for buffers >4GB)
  //
  if ((Caps & SDHCI_CAN_DO_ADMA2) != 0) {
    UINT8  HostControl;

    Device->UseDma64 = FALSE;  // Force ADMA32 for now

    HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
    HostControl &= ~SDHCI_CTRL_DMA_MASK;
    HostControl |= SDHCI_CTRL_ADMA32;
    SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Enabling ADMA32 mode\n"));
  } else {
    DEBUG ((DEBUG_WARN, "SdMmcPciCb: Controller doesn't support ADMA!\n"));
  }

  //
  // Set power to 3.3V
  //
  Status = SdhciSetPower (Device, SDHCI_POWER_330);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enable interrupts - Match Depthcharge EXACTLY
  // Enable only data and command interrupts for status polling
  //
  SdhciWritel (Device, SDHCI_INT_ENABLE, SDHCI_INT_DATA_MASK | SDHCI_INT_CMD_MASK);
  SdhciWritel (Device, SDHCI_SIGNAL_ENABLE, 0x0); // Depthcharge uses 0, not ALL_MASK!

  //
  // Set initial clock to 400 KHz for identification
  //
  Status = SdhciSetClock (Device, 400000);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // GL9763E-specific initialization
  //
  if (Device->IsGL9763E) {
    Status = Gl9763eInit (Device);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Device->Initialized = TRUE;
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: SdhciInit complete\n"));

  return EFI_SUCCESS;
}

