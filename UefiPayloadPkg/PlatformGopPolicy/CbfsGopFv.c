/** @file
  Load the Intel GOP driver and VBT from coreboot's CBFS.

  The GOP driver is wrapped in an in-memory firmware volume, so the DXE
  dispatcher and the image security policy handle it like any other driver
  in the payload.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CbfsAssetLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include "PlatformGopPolicy.h"

#define CBFS_GOP_DRIVER_NAME  "IntelGopDriver.efi"
#define CBFS_VBT_NAME         "vbt.bin"

STATIC CONST EFI_GUID  mIntelGopDriverFileGuid = {
  0xFF0C8745, 0x3270, 0x4439, { 0xB7, 0x4F, 0x3E, 0x45, 0xF8, 0xC7, 0x70, 0x64 }
};

STATIC CONST CHAR16  mGopUiName[] = L"IntelGopDriver";

/**
  Load the GOP driver and VBT from CBFS, and hand the GOP driver to the DXE
  dispatcher.

  @param[out] Vbt      On success, the VBT. The caller owns the buffer.
  @param[out] VbtSize  On success, the size of Vbt in bytes.

  @retval EFI_SUCCESS  The GOP driver was installed and the VBT returned.
  @retval Others       The GOP driver or VBT could not be loaded.
**/
EFI_STATUS
LoadGopFromCbfs (
  OUT VOID   **Vbt,
  OUT UINTN  *VbtSize
  )
{
  EFI_STATUS  Status;
  VOID        *Gop;
  UINTN       GopSize;
  VOID        *VbtData;
  UINTN       VbtDataSize;
  CONST CHAR8 *GopNames[] = { CBFS_GOP_DRIVER_NAME };
  CONST CHAR8 *VbtNames[] = { CBFS_VBT_NAME };

  Status = CbfsAssetLoadPreferCbfs (
             GopNames,
             ARRAY_SIZE (GopNames),
             NULL,
             EFI_SECTION_PE32,
             &Gop,
             &GopSize,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "PlatformGopPolicy: no GOP driver in CBFS: %r\n", Status));
    return Status;
  }

  Status = CbfsAssetLoadPreferCbfs (
             VbtNames,
             ARRAY_SIZE (VbtNames),
             NULL,
             EFI_SECTION_RAW,
             &VbtData,
             &VbtDataSize,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PlatformGopPolicy: GOP driver in CBFS but no VBT: %r\n", Status));
    FreePool (Gop);
    return Status;
  }

  if ((VbtDataSize > MAX_UINT32) || (VbtDataSize < 4) || (CompareMem (VbtData, "$VBT", 4) != 0)) {
    DEBUG ((DEBUG_ERROR, "PlatformGopPolicy: CBFS %a is not a valid VBT\n", CBFS_VBT_NAME));
    FreePool (Gop);
    FreePool (VbtData);
    return EFI_VOLUME_CORRUPTED;
  }

  Status = CbfsAssetInstallPe32Fv (
             Gop,
             GopSize,
             &mIntelGopDriverFileGuid,
             EFI_FV_FILETYPE_DRIVER,
             &gPlatformGOPPolicyGuid,
             mGopUiName,
             NULL
             );
  FreePool (Gop);
  if (EFI_ERROR (Status)) {
    FreePool (VbtData);
    return Status;
  }

  *Vbt     = VbtData;
  *VbtSize = VbtDataSize;
  return EFI_SUCCESS;
}
