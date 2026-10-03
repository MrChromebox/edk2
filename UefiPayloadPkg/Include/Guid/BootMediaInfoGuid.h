/** @file
  This file defines the hob structure for the bootloader's boot media (flash)
  layout, used to locate coreboot's CBFS.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

extern EFI_GUID  gEfiBootMediaInfoHobGuid;

///
/// All offsets are relative to the start of the boot media.
///
typedef struct {
  UINT64    FmapOffset;
  UINT64    CbfsOffset;
  UINT64    CbfsSize;
  UINT64    BootMediaSize;
} BOOT_MEDIA_INFO;
