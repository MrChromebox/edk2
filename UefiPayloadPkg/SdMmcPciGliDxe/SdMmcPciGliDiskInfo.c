/** @file
  Implements EFI Disk Info Protocol for SD/MMC devices.

  Based on MdeModulePkg/Bus/Sd/EmmcDxe/EmmcDiskInfo.c
  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciGliDxe.h"

/**
  Provides inquiry information for the controller type.

  For SD/MMC, this returns the CID (Card Identification) data.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] InquiryData       Pointer to a buffer for the inquiry data.
  @param[in, out] InquiryDataSize   Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough.
  @retval EFI_INVALID_PARAMETER  InquiryDataSize is NULL.

**/
EFI_STATUS
EFIAPI
SdMmcDcDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *InquiryData,
  IN OUT UINT32                  *InquiryDataSize
  )
{
  EFI_STATUS        Status;
  SD_MMC_CB_DEVICE  *Device;
  UINT32            RequiredSize;

  Device = SD_MMC_CB_DEVICE_FROM_DISK_INFO (This);

  if (Device->IsEMMC) {
    RequiredSize = sizeof (EMMC_CID);
  } else {
    RequiredSize = sizeof (SD_CID);
  }

  if (*InquiryDataSize >= RequiredSize) {
    Status = EFI_SUCCESS;
    if (Device->IsEMMC) {
      CopyMem (InquiryData, &Device->Cid.EmmcCid, sizeof (EMMC_CID));
    } else {
      CopyMem (InquiryData, &Device->Cid.SdCid, sizeof (SD_CID));
    }
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  *InquiryDataSize = RequiredSize;

  return Status;
}

/**
  Provides identify information for the controller type.

  Not used for SD/MMC.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] IdentifyData      Pointer to a buffer for the identify data.
  @param[in, out] IdentifyDataSize  Pointer to the value for the identify data size.

  @retval EFI_NOT_FOUND   The device does not support this data class.

**/
EFI_STATUS
EFIAPI
SdMmcDcDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *IdentifyData,
  IN OUT UINT32                  *IdentifyDataSize
  )
{
  return EFI_NOT_FOUND;
}

/**
  Provides sense data information for the controller type.

  Not used for SD/MMC.

  @param[in]  This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[out] SenseData         Pointer to the SenseData.
  @param[in, out] SenseDataSize      Size of SenseData in bytes.
  @param[out] SenseDataNumber   Number of items in SenseData.

  @retval EFI_NOT_FOUND   The device does not support this data class.

**/
EFI_STATUS
EFIAPI
SdMmcDcDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *SenseData,
  IN OUT UINT32                  *SenseDataSize,
  OUT    UINT8                   *SenseDataNumber
  )
{
  return EFI_NOT_FOUND;
}

/**
  Provides IDE channel and device information for the IDE/ATAPI controller.

  Not used for SD/MMC.

  @param[in]  This        Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[out] IdeChannel  Pointer to the IdeChannel.
  @param[out] IdeDevice   Pointer to the IdeDevice.

  @retval EFI_UNSUPPORTED This is not an IDE device.

**/
EFI_STATUS
EFIAPI
SdMmcDcDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL  *This,
  OUT UINT32                  *IdeChannel,
  OUT UINT32                  *IdeDevice
  )
{
  return EFI_UNSUPPORTED;
}
