/** @file
  Load payload assets from coreboot CBFS with firmware-volume fallback, and
  install PE32/RAW images as in-memory firmware volumes.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CbfsAssetLib.h>
#include <Library/CbfsLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#pragma pack(1)
typedef struct {
  UINT8       Push;
  EFI_GUID    Guid;
  UINT8       End;
} CBFS_ASSET_DEPEX;
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
  Build an in-memory FV containing a single FFS file and install it.

  @param[in]  Payload         File contents to wrap in a section.
  @param[in]  PayloadSize     Size of Payload in bytes.
  @param[in]  SectionType     Section type for Payload.
  @param[in]  FileGuid        FFS file name GUID.
  @param[in]  FileType        FFS file type.
  @param[in]  DepexProtocol   Optional DXE DEPEX protocol GUID.
  @param[in]  UiName          Optional UI string.
  @param[out] FvHandle        Optional installed FV handle.

  @retval EFI_SUCCESS  The FV was installed.
  @retval Others       The FV could not be built or installed.
**/
STATIC
EFI_STATUS
InstallSingleFileFv (
  IN  CONST VOID       *Payload,
  IN  UINTN            PayloadSize,
  IN  EFI_SECTION_TYPE SectionType,
  IN  CONST EFI_GUID   *FileGuid,
  IN  EFI_FV_FILETYPE  FileType,
  IN  CONST EFI_GUID   *DepexProtocol OPTIONAL,
  IN  CONST CHAR16     *UiName OPTIONAL,
  OUT EFI_HANDLE       *FvHandle OPTIONAL
  )
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;
  EFI_FFS_FILE_HEADER         *File;
  CBFS_ASSET_DEPEX            Depex;
  UINTN                       FvHeaderLength;
  UINTN                       FvSize;
  UINTN                       UiSize;
  UINTN                       DepexSize;
  UINT8                       *Cursor;
  EFI_HANDLE                  LocalFvHandle;
  UINTN                       SectionCount;

  if ((Payload == NULL) || (PayloadSize == 0) || (FileGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (PayloadSize > CBFS_ASSET_MAX_FILE_SIZE) {
    return EFI_UNSUPPORTED;
  }

  UiSize    = 0;
  DepexSize = 0;
  SectionCount = 1;
  if (DepexProtocol != NULL) {
    Depex.Push = EFI_DEP_PUSH;
    CopyGuid (&Depex.Guid, DepexProtocol);
    Depex.End = EFI_DEP_END;
    DepexSize = sizeof (Depex);
    SectionCount++;
  }

  if (UiName != NULL) {
    UiSize = StrSize (UiName);
    SectionCount++;
  }

  //
  // The header includes the terminating block map entry. The size estimate
  // includes worst-case file and section alignment padding.
  //
  FvHeaderLength = sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY);
  FvSize         = FvHeaderLength +
                   sizeof (EFI_FFS_FILE_HEADER) + 8 +
                   SectionCount * (sizeof (EFI_COMMON_SECTION_HEADER) + 4) +
                   DepexSize + PayloadSize + UiSize;
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
  if (DepexProtocol != NULL) {
    Cursor = AppendSection (Cursor, EFI_SECTION_DXE_DEPEX, &Depex, DepexSize);
  }

  Cursor = AppendSection (Cursor, SectionType, Payload, PayloadSize);
  if (UiName != NULL) {
    Cursor = AppendSection (Cursor, EFI_SECTION_USER_INTERFACE, UiName, UiSize);
  }

  FinishFfsFile (File, FileGuid, FileType, Cursor);

  ASSERT (Cursor <= (UINT8 *)Fv + FvSize);

  LocalFvHandle = NULL;
  Status = gDS->ProcessFirmwareVolume (Fv, FvSize, &LocalFvHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CbfsAssetLib: ProcessFirmwareVolume failed: %r\n", Status));
    FreePages (Fv, EFI_SIZE_TO_PAGES (FvSize));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "CbfsAssetLib: installed FV at %p size 0x%lx for %g\n",
    Fv,
    (UINT64)FvSize,
    FileGuid
    ));

  if (FvHandle != NULL) {
    *FvHandle = LocalFvHandle;
  }

  return EFI_SUCCESS;
}

/**
  Load a file from CBFS (trying each name in order), then fall back to a
  firmware-volume section.
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
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  VOID        *Data;
  UINTN       DataSize;

  if ((Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((CbfsNameCount > 0) && (CbfsNames == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Buffer     = NULL;
  *BufferSize = 0;
  if (LoadedFromCbfs != NULL) {
    *LoadedFromCbfs = FALSE;
  }

  for (Index = 0; Index < CbfsNameCount; Index++) {
    CONST CHAR8  *Name;
    CHAR8        EfiName[256];
    UINTN        NameLen;
    UINTN        Pass;

    Name = CbfsNames[Index];
    if (Name == NULL) {
      continue;
    }

    //
    // Prefer efi/<name> when the caller passed a root-relative name.
    //
    for (Pass = 0; Pass < 2; Pass++) {
      CONST CHAR8  *TryName;

      if (Pass == 0) {
        if (AsciiStrnCmp (Name, "efi/", 4) == 0) {
          continue;
        }

        NameLen = AsciiStrLen (Name);
        if (NameLen + 4 >= sizeof (EfiName)) {
          continue;
        }

        AsciiStrCpyS (EfiName, sizeof (EfiName), "efi/");
        AsciiStrCatS (EfiName, sizeof (EfiName), Name);
        TryName = EfiName;
      } else {
        TryName = Name;
      }

      Status = CbfsLoadFile (TryName, &Data, &DataSize);
      if (!EFI_ERROR (Status)) {
        *Buffer     = Data;
        *BufferSize = DataSize;
        if (LoadedFromCbfs != NULL) {
          *LoadedFromCbfs = TRUE;
        }

        DEBUG ((DEBUG_INFO, "CbfsAssetLib: loaded %a from CBFS (%lu bytes)\n", TryName, (UINT64)DataSize));
        return EFI_SUCCESS;
      }
    }
  }

  if (FvFileGuid == NULL) {
    return EFI_NOT_FOUND;
  }

  Data     = NULL;
  DataSize = 0;
  Status   = GetSectionFromAnyFv (
               FvFileGuid,
               SectionType,
               0,
               &Data,
               &DataSize
               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *Buffer     = Data;
  *BufferSize = DataSize;
  DEBUG ((DEBUG_INFO, "CbfsAssetLib: loaded %g from FV (%lu bytes)\n", FvFileGuid, (UINT64)DataSize));
  return EFI_SUCCESS;
}

/**
  Wrap a PE32 image in an in-memory firmware volume and hand it to DXE.
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
  )
{
  return InstallSingleFileFv (
           Pe32,
           Pe32Size,
           EFI_SECTION_PE32,
           FileGuid,
           FileType,
           DepexProtocol,
           UiName,
           FvHandle
           );
}

/**
  Wrap a RAW payload in an in-memory firmware volume and hand it to DXE.
**/
EFI_STATUS
EFIAPI
CbfsAssetInstallRawFv (
  IN  CONST VOID      *Raw,
  IN  UINTN           RawSize,
  IN  CONST EFI_GUID  *FileGuid,
  OUT EFI_HANDLE      *FvHandle OPTIONAL
  )
{
  return InstallSingleFileFv (
           Raw,
           RawSize,
           EFI_SECTION_RAW,
           FileGuid,
           EFI_FV_FILETYPE_FREEFORM,
           NULL,
           NULL,
           FvHandle
           );
}
