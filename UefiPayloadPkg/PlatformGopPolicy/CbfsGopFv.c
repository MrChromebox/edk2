/** @file
  Load the Intel GOP driver and VBT from coreboot's CBFS.

  The GOP driver is wrapped in an in-memory firmware volume, so the DXE
  dispatcher and the image security policy handle it like any other driver
  in the payload.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CbfsLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "PlatformGopPolicy.h"

#define CBFS_GOP_DRIVER_NAME  "IntelGopDriver.efi"
#define CBFS_VBT_NAME         "vbt.bin"

#define CBFS_GOP_MAX_FILE_SIZE  SIZE_8MB

STATIC CONST EFI_GUID  mIntelGopDriverFileGuid = {
  0xFF0C8745, 0x3270, 0x4439, { 0xB7, 0x4F, 0x3E, 0x45, 0xF8, 0xC7, 0x70, 0x64 }
};

STATIC CONST CHAR16  mGopUiName[] = L"IntelGopDriver";

#pragma pack(1)
typedef struct {
  UINT8       Push;
  EFI_GUID    Guid;
  UINT8       End;
} GOP_DEPEX;
#pragma pack()

/**
  Append a leaf section to an FFS file under construction.

  @param[in] Cursor    Current end of the file.
  @param[in] Type      Section type.
  @param[in] Data      Section contents.
  @param[in] DataSize  Size of Data in bytes.

  @return The new end of the file.
**/
STATIC
UINT8 *
AppendSection (
  IN UINT8             *Cursor,
  IN EFI_SECTION_TYPE  Type,
  IN CONST VOID        *Data,
  IN UINTN             DataSize
  )
{
  EFI_COMMON_SECTION_HEADER  *Section;
  UINT32                     Size;

  Section          = ALIGN_POINTER (Cursor, 4);
  Size             = (UINT32)(sizeof (*Section) + DataSize);
  Section->Size[0] = (UINT8)Size;
  Section->Size[1] = (UINT8)(Size >> 8);
  Section->Size[2] = (UINT8)(Size >> 16);
  Section->Type    = Type;
  CopyMem (Section + 1, Data, DataSize);

  return (UINT8 *)Section + Size;
}

/**
  Fill in the header of an FFS file once all its sections have been appended.

  @param[in] File  The file header.
  @param[in] Name  The file name GUID.
  @param[in] Type  The file type.
  @param[in] End   The end of the file's last section.
**/
STATIC
VOID
FinishFfsFile (
  IN EFI_FFS_FILE_HEADER  *File,
  IN CONST EFI_GUID       *Name,
  IN EFI_FV_FILETYPE      Type,
  IN UINT8                *End
  )
{
  UINT32  Size;

  Size = (UINT32)(End - (UINT8 *)File);

  ZeroMem (File, sizeof (*File));
  CopyGuid (&File->Name, Name);
  File->Type    = Type;
  File->Size[0] = (UINT8)Size;
  File->Size[1] = (UINT8)(Size >> 8);
  File->Size[2] = (UINT8)(Size >> 16);

  //
  // The header checksum excludes State and the file checksum, so compute it
  // while both are still zero.
  //
  File->IntegrityCheck.Checksum.Header = CalculateCheckSum8 ((UINT8 *)File, sizeof (*File));
  File->IntegrityCheck.Checksum.File   = FFS_FIXED_CHECKSUM;

  //
  // The volume has EFI_FVB2_ERASE_POLARITY set, so state bits are inverted.
  //
  File->State = (EFI_FFS_FILE_STATE) ~(EFI_FILE_HEADER_CONSTRUCTION |
                                       EFI_FILE_HEADER_VALID |
                                       EFI_FILE_DATA_VALID);
}

/**
  Build an in-memory firmware volume holding the GOP driver, and hand it to
  the DXE dispatcher.

  @param[in] Gop      GOP driver PE32 image.
  @param[in] GopSize  Size of Gop in bytes.

  @retval EFI_SUCCESS  The firmware volume was installed.
  @retval Others       The firmware volume could not be built or installed.
**/
STATIC
EFI_STATUS
InstallGopFv (
  IN CONST VOID  *Gop,
  IN UINTN       GopSize
  )
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;
  EFI_FFS_FILE_HEADER         *File;
  GOP_DEPEX                   Depex;
  UINTN                       FvHeaderLength;
  UINTN                       FvSize;
  UINT8                       *Cursor;
  EFI_HANDLE                  FvHandle;

  if (GopSize > CBFS_GOP_MAX_FILE_SIZE) {
    return EFI_UNSUPPORTED;
  }

  Depex.Push = EFI_DEP_PUSH;
  CopyGuid (&Depex.Guid, &gPlatformGOPPolicyGuid);
  Depex.End = EFI_DEP_END;

  //
  // The header includes the terminating block map entry. The size estimate
  // includes worst-case file and section alignment padding.
  //
  FvHeaderLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  FvSize         = FvHeaderLength +
                   sizeof (EFI_FFS_FILE_HEADER) + 8 +
                   3 * (sizeof (EFI_COMMON_SECTION_HEADER) + 4) +
                   sizeof (Depex) + GopSize + sizeof (mGopUiName);
  FvSize = ALIGN_VALUE (FvSize, EFI_PAGE_SIZE);

  //
  // Boot services data that is never freed: the volume must stay valid for as
  // long as the DXE core may read from it.
  //
  Fv = AllocatePages (EFI_SIZE_TO_PAGES (FvSize));
  if (Fv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (Fv, FvSize, 0xFF);
  ZeroMem (Fv, FvHeaderLength);
  CopyGuid (&Fv->FileSystemGuid, &gEfiFirmwareFileSystem2Guid);
  Fv->FvLength   = FvSize;
  Fv->Signature  = EFI_FVH_SIGNATURE;
  Fv->Attributes = EFI_FVB2_READ_ENABLED_CAP | EFI_FVB2_READ_STATUS |
                   EFI_FVB2_MEMORY_MAPPED | EFI_FVB2_ERASE_POLARITY |
                   EFI_FVB2_ALIGNMENT_8;
  Fv->HeaderLength          = (UINT16)FvHeaderLength;
  Fv->Revision              = EFI_FVH_REVISION;
  Fv->BlockMap[0].NumBlocks = (UINT32)(FvSize / EFI_PAGE_SIZE);
  Fv->BlockMap[0].Length    = EFI_PAGE_SIZE;
  Fv->Checksum              = CalculateCheckSum16 ((UINT16 *)Fv, FvHeaderLength);

  Cursor = (UINT8 *)Fv + FvHeaderLength;

  File   = ALIGN_POINTER (Cursor, 8);
  Cursor = (UINT8 *)(File + 1);
  Cursor = AppendSection (Cursor, EFI_SECTION_DXE_DEPEX, &Depex, sizeof (Depex));
  Cursor = AppendSection (Cursor, EFI_SECTION_PE32, Gop, GopSize);
  Cursor = AppendSection (Cursor, EFI_SECTION_USER_INTERFACE, mGopUiName, sizeof (mGopUiName));
  FinishFfsFile (File, &mIntelGopDriverFileGuid, EFI_FV_FILETYPE_DRIVER, Cursor);

  ASSERT (Cursor <= (UINT8 *)Fv + FvSize);

  Status = gDS->ProcessFirmwareVolume (Fv, FvSize, &FvHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "PlatformGopPolicy: failed to install CBFS GOP FV: %r\n", Status));
    FreePages (Fv, EFI_SIZE_TO_PAGES (FvSize));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "PlatformGopPolicy: installed CBFS GOP FV at %p, size 0x%lx\n", Fv, (UINT64)FvSize));
  return EFI_SUCCESS;
}

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

  Status = CbfsLoadFile (CBFS_GOP_DRIVER_NAME, &Gop, &GopSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "PlatformGopPolicy: no GOP driver in CBFS: %r\n", Status));
    return Status;
  }

  Status = CbfsLoadFile (CBFS_VBT_NAME, &VbtData, &VbtDataSize);
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

  Status = InstallGopFv (Gop, GopSize);
  FreePool (Gop);
  if (EFI_ERROR (Status)) {
    FreePool (VbtData);
    return Status;
  }

  *Vbt     = VbtData;
  *VbtSize = VbtDataSize;
  return EFI_SUCCESS;
}
