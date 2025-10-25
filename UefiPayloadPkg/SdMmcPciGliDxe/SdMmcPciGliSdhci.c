/** @file
  SDHCI Core Functions (ported from Depthcharge sdhci.c)

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciGliDxe.h"

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

  // DEBUG ((DEBUG_INFO, "SdMmcPciGli: SdhciReset(0x%02x)\n", Mask));

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
      DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Reset complete\n"));
      return EFI_SUCCESS;
    }
    gBS->Stall (10);
    Timeout -= 10;
  }

  DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Reset timeout!\n"));
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
  // DEBUG ((DEBUG_INFO, "SdMmcPciGli: SdhciSetPower(0x%02x)\n", PowerMode));

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
  UINT32   Div;
  UINT16   ClkReg;
  UINT32   Timeout;
  UINT32   ActualClock;
  BOOLEAN  PllProgrammed;
  UINT16   HostControl2;
  UINT8    UhsMode;
  BOOLEAN  IsSdr104;

  // DEBUG ((DEBUG_INFO, "SdMmcPciGli: SdhciSetClock(%d Hz)\n", Clock));

  //
  // For GL9750/GL9755, disable SSC PLL before changing clock
  //
  if (Device->IsGL9750) {
    Gl9750DisableSscPll (Device);
  } else if (Device->IsGL9755) {
    Gl9755DisableSscPll (Device);
  }

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
  // Note: Calculate BEFORE PLL programming (Linux does this)
  //
  Div = 0;
  ActualClock = Clock;
  PllProgrammed = FALSE;

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

    // Calculate actual clock (before PLL programming)
    if (Div == 0) {
      ActualClock = Device->ClockBase;
    } else {
      ActualClock = Device->ClockBase / (2 * Div);
    }
  }

  //
  // For GL9750/GL9755, program PLL for specific frequencies AFTER calculating divisor
  // The PLL effectively becomes the new base clock
  // Per Linux: Only use 205MHz PLL for SDR104 mode specifically
  //
  if (Device->IsGL9750 || Device->IsGL9755) {
    // Check if we're in SDR104 mode by checking HOST_CONTROL2 register
    HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
    UhsMode = HostControl2 & 0x0007;  // UHS mode bits [2:0]
    IsSdr104 = (UhsMode == 0x0003);  // SDR104 = 0x0003

    if (Clock == 200000000 && IsSdr104) {
      // SDR104: Use 205MHz PLL
      if (Device->IsGL9750) {
        Gl9750SetSscPll205Mhz (Device);
      } else {
        Gl9755SetSscPll205Mhz (Device);
      }
      ActualClock = 205000000;
      Div = 0;  // No division with PLL
      PllProgrammed = TRUE;
    } else if (Clock == 100000000) {
      // SDR50/DDR50: Use 100MHz PLL
      if (Device->IsGL9750) {
        Gl9750SetSscPll100Mhz (Device);
      } else {
        Gl9755SetSscPll100Mhz (Device);
      }
      ActualClock = 100000000;
      Div = 0;
      PllProgrammed = TRUE;
    } else if (Clock == 50000000) {
      // High Speed: Use 50MHz PLL
      if (Device->IsGL9750) {
        Gl9750SetSscPll50Mhz (Device);
      } else {
        Gl9755SetSscPll50Mhz (Device);
      }
      ActualClock = 50000000;
      Div = 0;
      PllProgrammed = TRUE;
    }
  }

  if (PllProgrammed) {
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Clock: requested=%u, divider=%u, actual=%u Hz (PLL)\n",
            Clock, Div, ActualClock));
  } else {
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Clock: requested=%u, divider=%u, actual=%u Hz\n",
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
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Internal clock never stabilized\n"));
    return EFI_TIMEOUT;
  }

  //
  // Enable SD clock
  //
  ClkReg |= SDHCI_CLOCK_CARD_EN;
  SdhciWritew (Device, SDHCI_CLOCK_CONTROL, ClkReg);

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Clock enabled\n"));

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
  UINT32      PresentState;
  UINT32      Timeout;
  UINT32      IntStatus;
  UINT16      CmdReg;
  UINT16      TransferMode;
  UINT8       Flags;
  UINT32      InhibitMask;

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: CMD%d arg=0x%08x%s\n",
          Cmd, Arg, (DataBuffer != NULL) ? " [DATA]" : ""));

  //
  // Wait for CMD and DAT lines to be ready
  // For R1B responses (busy), we need to wait for DATA_INHIBIT to clear as well
  //
  Timeout = 10000;
  InhibitMask = SDHCI_CMD_INHIBIT;
  if (DataBuffer != NULL || (ResponseType & MMC_RSP_BUSY)) {
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
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD%d line not ready! PresentState=0x%08x (need 0x%08x clear)\n",
            Cmd, PresentState, InhibitMask));
    return EFI_TIMEOUT;
  }

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: CMD%d ready, PresentState=0x%08x\n", Cmd, PresentState));

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
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to setup ADMA: %r\n", Status));
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
  // Send command
  //
  SdhciWritew (Device, SDHCI_COMMAND, CmdReg);

  //
  // Poll for command complete
  //
  Timeout = 100000; // 100ms
  while (Timeout > 0) {
    IntStatus = SdhciReadl (Device, SDHCI_INT_STATUS);

    if (IntStatus & SDHCI_INT_ERROR) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: CMD%d error! IntStatus=0x%08x (", Cmd, IntStatus));
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

      // DEBUG ((DEBUG_INFO, "SdMmcPciGli: CMD%d complete!\n", Cmd));

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

  DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Command timeout!\n"));
  return EFI_TIMEOUT;
}

/**
  Setup ADMA descriptor table for data transfer (from Depthcharge)

  @param[in]  Device      Pointer to device context
  @param[in]  DataBuffer  Data buffer address (may be above 4GB)
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
  UINTN                 BufferAddress;

  if (TotalBytes == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: SdhciSetupAdma: Invalid TotalBytes=0\n"));
    return EFI_INVALID_PARAMETER;
  }

  BufferAddress = (UINTN)DataBuffer;

  //
  // ADMA32 can only address 32-bit memory (below 4GB)
  //
  if (BufferAddress > 0xFFFFFFFF) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Buffer at 0x%lx is above 4GB, ADMA32 cannot access it\n", BufferAddress));
    return EFI_UNSUPPORTED;
  }

  //
  // Calculate how many descriptors we need
  //
  NeedDescriptors = TotalBytes / SDHCI_MAX_PER_DESCRIPTOR;
  if (TotalBytes % SDHCI_MAX_PER_DESCRIPTOR != 0) {
    NeedDescriptors++;
  }

  //
  // Allocate descriptor table, growing it only if needed
  // This avoids constant reallocation and pool fragmentation
  //
  if (Device->AdmaDescs == NULL || Device->AdmaDescCount < NeedDescriptors) {
    // Free old allocation if we need to grow
    if (Device->AdmaDescs != NULL) {
      FreePool (Device->AdmaDescs);
    }

    // Allocate with some headroom to avoid frequent reallocations
    Device->AdmaDescCount = NeedDescriptors + 4;  // Add 4 extra descriptors (~256KB headroom)
    Device->AdmaDescs = AllocateZeroPool (Device->AdmaDescCount * sizeof (SDHCI_ADMA_DESC));
    if (Device->AdmaDescs == NULL) {
      Device->AdmaDescCount = 0;
      DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Failed to allocate %d ADMA descriptors\n", Device->AdmaDescCount));
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    // Reuse existing allocation, just zero it
    ZeroMem (Device->AdmaDescs, NeedDescriptors * sizeof (SDHCI_ADMA_DESC));
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

  //
  // Verify descriptor table is below 4GB (ADMA32 limitation)
  //
  if (DescTableAddr > 0xFFFFFFFF) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: Descriptor table at 0x%lx is above 4GB!\n", DescTableAddr));
    return EFI_UNSUPPORTED;
  }

  SdhciWritel (Device, SDHCI_ADMA_ADDRESS, (UINT32)DescTableAddr);
  if (Device->UseDma64) {
    SdhciWritel (Device, SDHCI_ADMA_ADDRESS_HI, (UINT32)(DescTableAddr >> 32));
  }

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: ADMA setup: %d descriptors, %d bytes, buffer=0x%lx\n",
          NeedDescriptors, TotalBytes, BufferAddress));

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

  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Starting ADMA transfer wait\n"));

  while (Retry > 0) {
    IntStatus = SdhciReadl (Device, SDHCI_INT_STATUS);
    if ((IntStatus & Mask) != 0) {
      break;
    }
    gBS->Stall (1);  // 1 microsecond delay
    Retry--;
  }

  if (Retry == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: ADMA command timeout! IntStatus=0x%08x\n", IntStatus));
    SdhciReset (Device, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
    return EFI_TIMEOUT;
  }

  // Check for errors first
  if ((IntStatus & SDHCI_INT_ERROR) != 0) {
    UINT32 AdmaError = SdhciReadl (Device, SDHCI_ADMA_ERROR);
    UINT32 AdmaAddr = SdhciReadl (Device, SDHCI_ADMA_ADDRESS);
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: ADMA command error: IntStatus=0x%08x, AdmaError=0x%08x, AdmaAddr=0x%08x\n",
            IntStatus, AdmaError, AdmaAddr));

    // Reset controller
    SdhciReset (Device, SDHCI_RESET_CMD | SDHCI_RESET_DATA);
    return EFI_DEVICE_ERROR;
  }

  // If we got DATA_END already, transfer is complete!
  if ((IntStatus & SDHCI_INT_DATA_END) != 0) {
    // DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: ADMA transfer complete (fast path)\n"));
    SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_ALL_MASK);
    return EFI_SUCCESS;
  }

  // Otherwise, clear command response and wait for data
  SdhciWritel (Device, SDHCI_INT_STATUS, SDHCI_INT_RESPONSE);
  // DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: ADMA command response received, waiting for data...\n"));

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
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: ADMA data timeout\n"));
    return EFI_TIMEOUT;
  }

  if ((IntStatus & (SDHCI_INT_ERROR | SDHCI_INT_ADMA_ERROR)) != 0) {
    UINT32  AdmaError = SdhciReadl (Device, SDHCI_ADMA_ERROR);
    DEBUG ((DEBUG_ERROR, "SdMmcPciGli: ADMA transfer error: IntStatus=0x%08x, AdmaError=0x%08x\n",
            IntStatus, AdmaError));

    // Reset data line
    SdhciReset (Device, SDHCI_RESET_DATA);
    return EFI_DEVICE_ERROR;
  }

  // DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: ADMA transfer complete\n"));
  return EFI_SUCCESS;
}

EFI_STATUS
SdhciInit (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  EFI_STATUS  Status;
  UINT32      Caps;
  UINT8       PowerMode;
  UINT16      HostControl2;
  UINT8       HostControl;

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: SdhciInit start\n"));

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
  SdhciWriteb (Device, SDHCI_POWER_CONTROL, 0);
  gBS->Stall (10000);  // 10ms

  // Set appropriate voltage (1.8V for eMMC, will be adjusted for SD if needed)
  PowerMode = SDHCI_POWER_ON;
  if (Device->IsEMMC) {
    PowerMode |= SDHCI_POWER_180;
  } else {
    PowerMode |= SDHCI_POWER_330;
  }
  SdhciWriteb (Device, SDHCI_POWER_CONTROL, PowerMode);
  gBS->Stall (10000);  // 10ms for power to stabilize

  //
  // For eMMC, enable 1.8V signaling in HOST_CONTROL2 (matches depthcharge)
  // eMMC VCCQ (I/O Signaling) is typically hard wired, no voltage switching protocol
  //
  if (Device->IsEMMC) {
    HostControl2 = SdhciReadw (Device, SDHCI_HOST_CONTROL2);
    HostControl2 |= SDHCI_CTRL_180V_SIGNALING_ENABLE;
    SdhciWritew (Device, SDHCI_HOST_CONTROL2, HostControl2);
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Enabled 1.8V signaling for eMMC\n"));
  }

  //
  // Read capabilities to determine clock base and ADMA support
  //
  Caps = SdhciReadl (Device, SDHCI_CAPABILITIES);
  Device->ClockBase = ((Caps & SDHCI_CLOCK_V3_BASE_MASK) >> SDHCI_CLOCK_BASE_SHIFT) * 1000000;

  if (Device->ClockBase == 0) {
    // Fallback for older controllers
    Device->ClockBase = ((Caps & SDHCI_CLOCK_BASE_MASK) >> SDHCI_CLOCK_BASE_SHIFT) * 1000000;
  }

  DEBUG ((DEBUG_INFO, "SdMmcPciGli: Clock base = %d Hz\n", Device->ClockBase));

  //
  // Check for ADMA support and enable it
  // Force ADMA32 mode for compatibility (ADMA64 only needed for buffers >4GB)
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciGli: Checking ADMA support: Caps=0x%08x, ADMA2 bit=%d\n",
          Caps, (Caps & SDHCI_CAN_DO_ADMA2) ? 1 : 0));

  if ((Caps & SDHCI_CAN_DO_ADMA2) != 0) {
    Device->UseDma64 = FALSE;  // Force ADMA32 for now

    HostControl = SdhciReadb (Device, SDHCI_HOST_CONTROL);
    // Clear timing/bus width bits (like depthcharge MMC_TIMING_INITIALIZATION)
    HostControl &= ~(SDHCI_CTRL_DMA_MASK | SDHCI_CTRL_HISPD | SDHCI_CTRL_4BITBUS | SDHCI_CTRL_8BITBUS);
    HostControl |= SDHCI_CTRL_ADMA32;
    SdhciWriteb (Device, SDHCI_HOST_CONTROL, HostControl);
    DEBUG ((DEBUG_INFO, "SdMmcPciGli: Enabling ADMA32 mode, cleared timing/bus bits\n"));
  } else {
    DEBUG ((DEBUG_WARN, "SdMmcPciGli: Controller doesn't support ADMA - will use PIO mode\n"));
    Device->UseDma64 = FALSE;
    // Leave DMA mode disabled in HOST_CONTROL
  }


  //
  // Enable interrupts - Match Depthcharge EXACTLY
  // Enable only data and command interrupts for status polling
  //
  SdhciWritel (Device, SDHCI_INT_ENABLE, SDHCI_INT_DATA_MASK | SDHCI_INT_CMD_MASK);
  SdhciWritel (Device, SDHCI_SIGNAL_ENABLE, 0x0); // Depthcharge uses 0, not ALL_MASK!

  //
  // Set initial clock to 400 KHz for identification
  // This must be done BEFORE vendor init (GL9750/GL9755 need clock for PLL programming)
  //
  Status = SdhciSetClock (Device, 400000);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Genesys Logic vendor-specific initialization (GL9750, GL9755)
  //
  if (Device->IsGL9750 || Device->IsGL9755) {
    Status = Gl975xInit (Device);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Set timeout to maximum (matches depthcharge)
  // Must be AFTER vendor init in case it gets reset
  //
  SdhciWriteb (Device, SDHCI_TIMEOUT_CONTROL, 0xe);

  Device->Initialized = TRUE;
  DEBUG ((DEBUG_INFO, "SdMmcPciGli: SdhciInit complete\n"));

  return EFI_SUCCESS;
}

