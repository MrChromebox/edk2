/** @file
  Read-only access to files in coreboot's CBFS.

  LZMA-compressed files are decoded through ExtractGuidedSectionLib, so modules
  using this library must also link LzmaCustomDecompressLib as a NULL library.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi/UefiBaseType.h>

/**
  Load a file from the active CBFS into a newly allocated buffer, decompressing
  it if needed.

  @param[in]  Name        NUL-terminated CBFS file name.
  @param[out] Buffer      Receives the file contents. Free with FreePool().
  @param[out] BufferSize  Receives the size of Buffer in bytes.

  @retval EFI_SUCCESS            The file was loaded.
  @retval EFI_INVALID_PARAMETER  A parameter is NULL.
  @retval EFI_NOT_FOUND          CBFS or the file could not be found.
  @retval EFI_UNSUPPORTED        CBFS is not memory-mapped, or the file uses an
                                 unsupported compression algorithm.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval EFI_VOLUME_CORRUPTED   The file could not be decompressed.
**/
EFI_STATUS
EFIAPI
CbfsLoadFile (
  IN  CONST CHAR8  *Name,
  OUT VOID         **Buffer,
  OUT UINTN        *BufferSize
  );
