/** @file
  Load payload assets from coreboot CBFS with firmware-volume fallback, and
  install PE32/RAW images as in-memory firmware volumes.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PiDxe.h>

#define CBFS_ASSET_MAX_FILE_SIZE  SIZE_8MB

/**
  Load a file from CBFS (trying each name in order), then fall back to a
  firmware-volume section.

  For each CBFS name that is not already under efi/, try efi/<name> first,
  then the bare name at the CBFS root.

  @param[in]  CbfsNames       Array of CBFS file names to try.
  @param[in]  CbfsNameCount   Number of entries in CbfsNames.
  @param[in]  FvFileGuid      FV file GUID for fallback, or NULL to skip FV.
  @param[in]  SectionType     FV section type used for the fallback.
  @param[out] Buffer          Receives the file contents. Free with FreePool().
  @param[out] BufferSize      Receives the size of Buffer in bytes.
  @param[out] LoadedFromCbfs  Optional. TRUE if the file came from CBFS.

  @retval EFI_SUCCESS            The file was loaded.
  @retval EFI_INVALID_PARAMETER  A required parameter is NULL/invalid.
  @retval EFI_NOT_FOUND          Neither CBFS nor the FV contained the file.
  @retval Others                 Propagation from CBFS or FV helpers.
**/
EFI_STATUS
EFIAPI
CbfsAssetLoadPreferCbfs (
  IN  CONST CHAR8 *CONST  *CbfsNames,
  IN  UINTN               CbfsNameCount,
  IN  CONST EFI_GUID      *FvFileGuid OPTIONAL,
  IN  EFI_SECTION_TYPE    SectionType,
  OUT VOID                **Buffer,
  OUT UINTN               *BufferSize,
  OUT BOOLEAN             *LoadedFromCbfs OPTIONAL
  );

/**
  Wrap a PE32 image in an in-memory firmware volume and hand it to the DXE
  dispatcher.

  @param[in]  Pe32            PE32 image bytes.
  @param[in]  Pe32Size        Size of Pe32 in bytes.
  @param[in]  FileGuid        FFS file name GUID.
  @param[in]  FileType        FFS file type (typically DRIVER or FREEFORM).
  @param[in]  DepexProtocol   Optional protocol GUID for a DXE DEPEX.
  @param[in]  UiName          Optional UI section string.
  @param[out] FvHandle        Optional receives the installed FV handle.

  @retval EFI_SUCCESS            The firmware volume was installed.
  @retval EFI_INVALID_PARAMETER  A required parameter is NULL/invalid.
  @retval EFI_UNSUPPORTED        Pe32Size exceeds CBFS_ASSET_MAX_FILE_SIZE.
  @retval Others                 Allocation or ProcessFirmwareVolume failure.
**/
EFI_STATUS
EFIAPI
CbfsAssetInstallPe32Fv (
  IN  CONST VOID       *Pe32,
  IN  UINTN            Pe32Size,
  IN  CONST EFI_GUID   *FileGuid,
  IN  EFI_FV_FILETYPE  FileType,
  IN  CONST EFI_GUID   *DepexProtocol OPTIONAL,
  IN  CONST CHAR16     *UiName OPTIONAL,
  OUT EFI_HANDLE       *FvHandle OPTIONAL
  );

/**
  Wrap a RAW payload in an in-memory firmware volume and hand it to DXE.

  @param[in]  Raw       RAW section bytes.
  @param[in]  RawSize   Size of Raw in bytes.
  @param[in]  FileGuid  FFS file name GUID.
  @param[out] FvHandle  Optional receives the installed FV handle.

  @retval EFI_SUCCESS            The firmware volume was installed.
  @retval EFI_INVALID_PARAMETER  A required parameter is NULL/invalid.
  @retval EFI_UNSUPPORTED        RawSize exceeds CBFS_ASSET_MAX_FILE_SIZE.
  @retval Others                 Allocation or ProcessFirmwareVolume failure.
**/
EFI_STATUS
EFIAPI
CbfsAssetInstallRawFv (
  IN  CONST VOID      *Raw,
  IN  UINTN           RawSize,
  IN  CONST EFI_GUID  *FileGuid,
  OUT EFI_HANDLE      *FvHandle OPTIONAL
  );
