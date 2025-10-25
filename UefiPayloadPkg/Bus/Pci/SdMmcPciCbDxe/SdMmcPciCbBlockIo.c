/** @file
  BlockIo Protocol Implementation

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciCbDxe.h"

/**
  Check if media is present and update media status.
  For removable media (SD cards), checks PRESENT_STATE register.
  For eMMC, always returns TRUE.

  @param[in] Device  Device context

  @retval EFI_SUCCESS       Media is present
  @retval EFI_NO_MEDIA      No media in device
  @retval EFI_MEDIA_CHANGED Media was changed
**/
STATIC
EFI_STATUS
CheckMediaPresence (
  IN SD_MMC_CB_DEVICE  *Device
  )
{
  BOOLEAN  CurrentPresent;
  BOOLEAN  PreviousPresent;

  //
  // eMMC is always present (embedded)
  //
  if (Device->IsEMMC) {
    return EFI_SUCCESS;
  }

  //
  // For SD cards, check PRESENT_STATE register
  //
  UINT32  PresentState = SdhciReadl (Device, SDHCI_PRESENT_STATE);
  CurrentPresent = (PresentState & SDHCI_CARD_PRESENT) != 0;
  PreviousPresent = Device->BlockIoMedia.MediaPresent;

  //
  // Check if media state changed
  //
  if (CurrentPresent != PreviousPresent) {
    Device->BlockIoMedia.MediaPresent = CurrentPresent;
    Device->BlockIoMedia.MediaId++;  // Increment to indicate media change

    if (CurrentPresent) {
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: Media status: inserted\n"));
    } else {
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: Media status: removed\n"));
    }

    return EFI_MEDIA_CHANGED;
  }

  //
  // No change, but check current state
  //
  if (!CurrentPresent) {
    return EFI_NO_MEDIA;
  }

  return EFI_SUCCESS;
}

/**
  Reset the block device.

  @param[in] This                 Pointer to the BLOCK_IO_PROTOCOL instance.
  @param[in] ExtendedVerification Indicates extended verification is requested.

  @retval EFI_SUCCESS             The device was reset.
  @retval EFI_DEVICE_ERROR        The device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SdMmcDcBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  )
{
  SD_MMC_CB_DEVICE  *Device;
  EFI_STATUS        Status;

  Device = SD_MMC_CB_DEVICE_FROM_BLOCK_IO (This);

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: BlockIoReset\n"));

  //
  // Check media presence for removable media
  //
  Status = CheckMediaPresence (Device);
  if (Status == EFI_NO_MEDIA) {
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Reset: No media present\n"));
    return EFI_SUCCESS;  // Reset succeeded, but no media
  }

  //
  // Reset the controller
  //
  return SdhciReset (Device, SDHCI_RESET_ALL);
}

/**
  Read blocks from the block device.

  @param[in]  This       Pointer to the BLOCK_IO_PROTOCOL instance.
  @param[in]  MediaId    Media ID that the read request is for.
  @param[in]  Lba        The starting logical block address to read from.
  @param[in]  BufferSize The size of the buffer in bytes.
  @param[out] Buffer     Pointer to the destination buffer.

  @retval EFI_SUCCESS           The data was read correctly.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The buffer size is not a multiple of the block size.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid.

**/
EFI_STATUS
EFIAPI
SdMmcDcBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  SD_MMC_CB_DEVICE  *Device;
  EFI_STATUS        Status;
  UINT32            BlockCount;
  UINT32            Response[4];

  Device = SD_MMC_CB_DEVICE_FROM_BLOCK_IO (This);

  //
  // Check media presence (for removable media like SD cards)
  //
  Status = CheckMediaPresence (Device);
  if (Status == EFI_MEDIA_CHANGED) {
    return EFI_MEDIA_CHANGED;
  }
  if (Status == EFI_NO_MEDIA) {
    return EFI_NO_MEDIA;
  }

  //
  // Validate MediaId
  //
  if (MediaId != Device->BlockIoMedia.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  //
  // Validate parameters
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if (BufferSize % Device->BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba + (BufferSize / Device->BlockSize) > Device->TotalBlocks) {
    return EFI_INVALID_PARAMETER;
  }

  BlockCount = BufferSize / Device->BlockSize;

  // Only log large transfers to reduce serial output overhead
  if (BlockCount > 8) {
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: ReadBlocks LBA=%ld Count=%d Size=%d\n",
            Lba, BlockCount, BufferSize));
  }

  //
  // Calculate command argument based on addressing mode
  // High capacity: block addressing (LBA)
  // Low capacity: byte addressing (LBA * block size)
  //
  UINT32 CmdArg;
  if (Device->HighCapacity) {
    CmdArg = (UINT32)Lba;
  } else {
    CmdArg = (UINT32)(Lba * Device->BlockSize);
  }

  //
  // Use CMD17 (single block) or CMD18 (multiple blocks)
  //
  if (BlockCount == 1) {
    Status = SdhciSendCommand (
               Device,
               MMC_CMD_READ_SINGLE_BLOCK,
               CmdArg,
               MMC_RSP_R1,
               Response,
               Buffer,
               Device->BlockSize,
               1,
               TRUE
               );
  } else {
    Status = SdhciSendCommand (
               Device,
               MMC_CMD_READ_MULTIPLE_BLOCK,
               CmdArg,
               MMC_RSP_R1,
               Response,
               Buffer,
               Device->BlockSize,
               BlockCount,
               TRUE
               );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Read failed: %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  // DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Read successful\n"));
  return EFI_SUCCESS;
}

/**
  Write blocks to the block device.

  @param[in] This       Pointer to the BLOCK_IO_PROTOCOL instance.
  @param[in] MediaId    Media ID that the write request is for.
  @param[in] Lba        The starting logical block address to write to.
  @param[in] BufferSize The size of the buffer in bytes.
  @param[in] Buffer     Pointer to the source buffer.

  @retval EFI_SUCCESS           The data was written correctly.
  @retval EFI_DEVICE_ERROR      The device reported an error.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The buffer size is not a multiple of the block size.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid.
  @retval EFI_WRITE_PROTECTED   The device cannot be written to.

**/
EFI_STATUS
EFIAPI
SdMmcDcBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  )
{
  SD_MMC_CB_DEVICE  *Device;
  EFI_STATUS        Status;
  UINT32            BlockCount;
  UINT32            Response[4];

  Device = SD_MMC_CB_DEVICE_FROM_BLOCK_IO (This);

  //
  // Check media presence (for removable media like SD cards)
  //
  Status = CheckMediaPresence (Device);
  if (Status == EFI_MEDIA_CHANGED) {
    return EFI_MEDIA_CHANGED;
  }
  if (Status == EFI_NO_MEDIA) {
    return EFI_NO_MEDIA;
  }

  //
  // Validate MediaId
  //
  if (MediaId != Device->BlockIoMedia.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  //
  // Validate parameters
  //
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if (BufferSize % Device->BlockSize != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (Lba + (BufferSize / Device->BlockSize) > Device->TotalBlocks) {
    return EFI_INVALID_PARAMETER;
  }

  BlockCount = BufferSize / Device->BlockSize;

  // Only log large transfers to reduce serial output overhead
  if (BlockCount > 8) {
    DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: WriteBlocks LBA=%ld Count=%d Size=%d\n",
            Lba, BlockCount, BufferSize));
  }

  //
  // Calculate command argument based on addressing mode
  // High capacity: block addressing (LBA)
  // Low capacity: byte addressing (LBA * block size)
  //
  UINT32 CmdArg;
  if (Device->HighCapacity) {
    CmdArg = (UINT32)Lba;
  } else {
    CmdArg = (UINT32)(Lba * Device->BlockSize);
  }

  //
  // Use CMD24 (single block) or CMD25 (multiple blocks)
  //
  if (BlockCount == 1) {
    Status = SdhciSendCommand (
               Device,
               MMC_CMD_WRITE_SINGLE_BLOCK,
               CmdArg,
               MMC_RSP_R1,
               Response,
               Buffer,
               Device->BlockSize,
               1,
               FALSE
               );
  } else {
    Status = SdhciSendCommand (
               Device,
               MMC_CMD_WRITE_MULTIPLE_BLOCK,
               CmdArg,
               MMC_RSP_R1,
               Response,
               Buffer,
               Device->BlockSize,
               BlockCount,
               FALSE
               );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Write failed: %r\n", Status));
    return EFI_DEVICE_ERROR;
  }

  // DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Write successful\n"));
  return EFI_SUCCESS;
}

/**
  Flush the block device.

  @param[in] This Pointer to the BLOCK_IO_PROTOCOL instance.

  @retval EFI_SUCCESS      All outstanding data was written to the device.
  @retval EFI_DEVICE_ERROR The device reported an error.
  @retval EFI_NO_MEDIA     There is no media in the device.

**/
EFI_STATUS
EFIAPI
SdMmcDcBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  )
{
  //
  // eMMC doesn't have a flush command, data is written immediately
  //
  return EFI_SUCCESS;
}
