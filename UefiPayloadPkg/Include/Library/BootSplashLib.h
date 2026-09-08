/** @file
  Shared boot splash BMP validation and ESP import helpers.

  Copyright (c) 2026
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BOOT_SPLASH_LIB_H_
#define BOOT_SPLASH_LIB_H_

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

/**
  Decode and validate a BMP buffer for use as a boot splash.

  Rejects empty / oversized files, undecodable BMPs, zero dimensions, and
  images that do not fit the current GOP mode (including BGRT Y placement).

  @param[in]  BmpBuffer  BMP file contents.
  @param[in]  BmpSize    Size of BmpBuffer in bytes.
  @param[out] Blt        Optional. On success, caller owns the pixel buffer.
                         If NULL, the decoded buffer is freed internally.
  @param[out] Width      Optional. Image width in pixels.
  @param[out] Height     Optional. Image height in pixels.
  @param[out] DestX      Optional. Centered X offset for the current GOP mode.
  @param[out] DestY      Optional. Y offset for the current GOP mode (BGRT-aware).

  @retval EFI_SUCCESS      BMP is valid for boot splash use.
  @retval EFI_UNSUPPORTED  BMP failed size, decode, or display-fit checks.
  @retval other            Propagated error (e.g. no GOP when fit-checking).
**/
EFI_STATUS
EFIAPI
BootSplashDecodeBmp (
  IN  VOID                           *BmpBuffer,
  IN  UINTN                          BmpSize,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  **Blt    OPTIONAL,
  OUT UINTN                          *Width   OPTIONAL,
  OUT UINTN                          *Height  OPTIONAL,
  OUT INTN                           *DestX   OPTIONAL,
  OUT INTN                           *DestY   OPTIONAL
  );

/**
  Read a file into a newly allocated buffer.

  @param[in]  FilePath    Device path to the file.
  @param[out] FileBuffer  On success, caller must FreePool.
  @param[out] FileSize    Size of FileBuffer in bytes.

  @retval EFI_SUCCESS  File read successfully.
  @retval other        Open/read failed or file empty/oversized.
**/
EFI_STATUS
EFIAPI
BootSplashReadFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT VOID                      **FileBuffer,
  OUT UINTN                     *FileSize
  );

/**
  Locate the first EFI System Partition handle.

  @param[out] EspHandle  Handle with Simple File System on an ESP.

  @retval EFI_SUCCESS    First ESP found.
  @retval EFI_NOT_FOUND  No ESP available.
**/
EFI_STATUS
EFIAPI
BootSplashFindFirstEsp (
  OUT EFI_HANDLE  *EspHandle
  );

/**
  Validate a source BMP and copy it to a fixed path on the first ESP.

  Always overwrites \\EFI\\BootSplash\\logo.bmp when the source is a different
  file. If SourcePath already refers to that ESP file, validation still runs
  and no write is performed.

  @param[in]  SourcePath  Device path of the selected BMP.
  @param[out] DestPath    On success, device path of the ESP logo file.
                          Caller must FreePool.

  @retval EFI_SUCCESS      Logo validated and present on the ESP.
  @retval EFI_UNSUPPORTED  Source failed BMP / display validation.
  @retval EFI_NOT_FOUND    No ESP found.
  @retval other            Read or write failure.
**/
EFI_STATUS
EFIAPI
BootSplashImportBmpToEsp (
  IN  EFI_DEVICE_PATH_PROTOCOL  *SourcePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DestPath
  );

#endif
