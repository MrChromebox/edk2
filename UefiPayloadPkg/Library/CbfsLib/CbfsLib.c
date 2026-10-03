/** @file
  Read-only access to files in coreboot's CBFS.

  CBFS is read through the x86 memory-mapped flash window just below 4 GiB.
  The walk mirrors cbfs_walk() in coreboot's commonlib/bsd/cbfs_private.c.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/BootMediaInfoGuid.h>
#include <Guid/LzmaDecompress.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CbfsLib.h>
#include <Library/DebugLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>

#define CBFS_FILE_MAGIC          "LARCHIVE"
#define CBFS_ALIGNMENT           64
#define CBFS_METADATA_MAX_SIZE   256
#define CBFS_MMIO_WINDOW_SIZE    SIZE_16MB

#define CBFS_TYPE_DELETED  0x00000000
#define CBFS_TYPE_NULL     0xffffffff

#define CBFS_FILE_ATTR_TAG_COMPRESSION  0x42435a4c

#define CBFS_COMPRESS_NONE  0
#define CBFS_COMPRESS_LZMA  1

//
// All multi-byte CBFS metadata fields are big-endian.
//
#pragma pack(1)
typedef struct {
  CHAR8     Magic[8];
  UINT32    Len;
  UINT32    Type;
  UINT32    AttributesOffset;
  UINT32    Offset;
  // CHAR8  Filename[];
} CBFS_FILE_HEADER;

typedef struct {
  UINT32    Tag;
  UINT32    Len;
} CBFS_FILE_ATTRIBUTE;

typedef struct {
  UINT32    Tag;
  UINT32    Len;
  UINT32    Compression;
  UINT32    DecompressedSize;
} CBFS_FILE_ATTR_COMPRESSION;
#pragma pack()

typedef union {
  CBFS_FILE_HEADER    Header;
  UINT8               Raw[CBFS_METADATA_MAX_SIZE];
} CBFS_METADATA;

/**
  Locate the active CBFS in the memory-mapped flash window.

  @param[out] CbfsBase  Receives the host address of the start of CBFS.
  @param[out] CbfsSize  Receives the size of CBFS in bytes.

  @retval EFI_SUCCESS      CBFS was located.
  @retval EFI_NOT_FOUND    The bootloader did not report the boot media layout.
  @retval EFI_UNSUPPORTED  CBFS is not entirely inside the memory-mapped window.
**/
STATIC
EFI_STATUS
CbfsLocate (
  OUT CONST UINT8  **CbfsBase,
  OUT UINT32       *CbfsSize
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  BOOT_MEDIA_INFO    *Info;

  GuidHob = GetFirstGuidHob (&gEfiBootMediaInfoHobGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  Info = GET_GUID_HOB_DATA (GuidHob);
  if ((Info->BootMediaSize == 0) || (Info->CbfsSize == 0) ||
      (Info->CbfsOffset >= Info->BootMediaSize) ||
      (Info->CbfsSize > Info->BootMediaSize - Info->CbfsOffset))
  {
    DEBUG ((DEBUG_ERROR, "CBFS: invalid boot media layout\n"));
    return EFI_NOT_FOUND;
  }

  if (Info->BootMediaSize - Info->CbfsOffset > CBFS_MMIO_WINDOW_SIZE) {
    DEBUG ((
      DEBUG_ERROR,
      "CBFS: offset 0x%lx is outside the memory-mapped flash window\n",
      Info->CbfsOffset
      ));
    return EFI_UNSUPPORTED;
  }

  *CbfsBase = (CONST UINT8 *)(UINTN)(BASE_4GB - Info->BootMediaSize + Info->CbfsOffset);
  *CbfsSize = (UINT32)Info->CbfsSize;
  return EFI_SUCCESS;
}

/**
  Find a file in CBFS by name.

  @param[in]  CbfsBase  Host address of the start of CBFS.
  @param[in]  CbfsSize  Size of CBFS in bytes.
  @param[in]  Name      NUL-terminated file name.
  @param[out] Metadata  Receives a copy of the file's header, name and attributes.
  @param[out] Offset    Receives the offset of the file header within CBFS.

  @retval EFI_SUCCESS    The file was found.
  @retval EFI_NOT_FOUND  The file was not found.
**/
STATIC
EFI_STATUS
CbfsFindFile (
  IN  CONST UINT8    *CbfsBase,
  IN  UINT32         CbfsSize,
  IN  CONST CHAR8    *Name,
  OUT CBFS_METADATA  *Metadata,
  OUT UINT32         *Offset
  )
{
  UINT32  FileOffset;
  UINT32  AttrOffset;
  UINT32  DataOffset;
  UINT32  DataLength;
  UINT32  Type;
  UINT32  NameEnd;

  FileOffset = 0;
  while ((UINT64)FileOffset + sizeof (CBFS_FILE_HEADER) < CbfsSize) {
    CopyMem (&Metadata->Header, CbfsBase + FileOffset, sizeof (CBFS_FILE_HEADER));
    if (CompareMem (Metadata->Header.Magic, CBFS_FILE_MAGIC, sizeof (Metadata->Header.Magic)) != 0) {
      FileOffset += CBFS_ALIGNMENT;
      continue;
    }

    AttrOffset = SwapBytes32 (Metadata->Header.AttributesOffset);
    DataOffset = SwapBytes32 (Metadata->Header.Offset);
    DataLength = SwapBytes32 (Metadata->Header.Len);
    Type       = SwapBytes32 (Metadata->Header.Type);

    if ((DataOffset < sizeof (CBFS_FILE_HEADER)) || (DataOffset > CBFS_METADATA_MAX_SIZE) ||
        (DataLength > CbfsSize) || ((UINT64)FileOffset + DataOffset + DataLength > CbfsSize))
    {
      DEBUG ((DEBUG_WARN, "CBFS: file @0x%x has invalid header\n", FileOffset));
      FileOffset += CBFS_ALIGNMENT;
      continue;
    }

    NameEnd = (AttrOffset != 0) ? AttrOffset : DataOffset;
    if ((Type != CBFS_TYPE_DELETED) && (Type != CBFS_TYPE_NULL) &&
        (NameEnd > sizeof (CBFS_FILE_HEADER)) && (DataOffset >= AttrOffset))
    {
      CopyMem (
        Metadata->Raw + sizeof (CBFS_FILE_HEADER),
        CbfsBase + FileOffset + sizeof (CBFS_FILE_HEADER),
        DataOffset - sizeof (CBFS_FILE_HEADER)
        );
      Metadata->Raw[NameEnd - 1] = '\0';

      if (AsciiStrCmp ((CHAR8 *)Metadata->Raw + sizeof (CBFS_FILE_HEADER), Name) == 0) {
        *Offset = FileOffset;
        return EFI_SUCCESS;
      }
    }

    FileOffset = ALIGN_VALUE (FileOffset + DataOffset + DataLength, CBFS_ALIGNMENT);
  }

  return EFI_NOT_FOUND;
}

/**
  Get the compression algorithm and decompressed size of a CBFS file.

  @param[in]  Metadata          The file's metadata.
  @param[out] Compression       Receives the CBFS compression algorithm.
  @param[out] DecompressedSize  Receives the decompressed size, if compressed.
**/
STATIC
VOID
CbfsGetCompression (
  IN  CONST CBFS_METADATA  *Metadata,
  OUT UINT32               *Compression,
  OUT UINT32               *DecompressedSize
  )
{
  CONST CBFS_FILE_ATTRIBUTE         *Attr;
  CONST CBFS_FILE_ATTR_COMPRESSION  *CompAttr;
  UINT32                            Offset;
  UINT32                            End;
  UINT32                            Len;

  *Compression      = CBFS_COMPRESS_NONE;
  *DecompressedSize = 0;

  Offset = SwapBytes32 (Metadata->Header.AttributesOffset);
  End    = SwapBytes32 (Metadata->Header.Offset);
  if (Offset == 0) {
    return;
  }

  while (Offset + sizeof (CBFS_FILE_ATTRIBUTE) <= End) {
    Attr = (CONST CBFS_FILE_ATTRIBUTE *)(Metadata->Raw + Offset);
    Len  = SwapBytes32 (Attr->Len);
    if ((Len < sizeof (CBFS_FILE_ATTRIBUTE)) || (Len > End - Offset)) {
      return;
    }

    if ((SwapBytes32 (Attr->Tag) == CBFS_FILE_ATTR_TAG_COMPRESSION) &&
        (Len == sizeof (CBFS_FILE_ATTR_COMPRESSION)))
    {
      CompAttr          = (CONST CBFS_FILE_ATTR_COMPRESSION *)Attr;
      *Compression      = SwapBytes32 (CompAttr->Compression);
      *DecompressedSize = SwapBytes32 (CompAttr->DecompressedSize);
      return;
    }

    Offset += Len;
  }
}

/**
  Decompress CBFS LZMA data.

  coreboot's LZMA format (5 property bytes, 8-byte little-endian size, stream)
  matches edk2's, so the data is wrapped in an LZMA GUIDed section and decoded
  through ExtractGuidedSectionLib.

  @param[in]  Src               Compressed data.
  @param[in]  SrcSize           Size of Src in bytes.
  @param[in]  DecompressedSize  Expected decompressed size from the CBFS attribute.
  @param[out] Buffer            Receives the decompressed data.
  @param[out] BufferSize        Receives the size of Buffer in bytes.
**/
STATIC
EFI_STATUS
CbfsLzmaDecompress (
  IN  CONST UINT8  *Src,
  IN  UINT32       SrcSize,
  IN  UINT32       DecompressedSize,
  OUT VOID         **Buffer,
  OUT UINTN        *BufferSize
  )
{
  EFI_STATUS                 Status;
  EFI_GUID_DEFINED_SECTION2  *Section;
  UINTN                      SectionSize;
  UINT32                     OutputSize;
  UINT32                     ScratchSize;
  UINT16                     SectionAttribute;
  UINT32                     AuthenticationStatus;
  VOID                       *Output;
  VOID                       *Scratch;

  Output  = NULL;
  Scratch = NULL;

  SectionSize = sizeof (EFI_GUID_DEFINED_SECTION2) + SrcSize;
  Section     = AllocatePool (SectionSize);
  if (Section == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Section->CommonHeader.Size[0]      = 0xFF;
  Section->CommonHeader.Size[1]      = 0xFF;
  Section->CommonHeader.Size[2]      = 0xFF;
  Section->CommonHeader.Type         = EFI_SECTION_GUID_DEFINED;
  Section->CommonHeader.ExtendedSize = (UINT32)SectionSize;
  CopyGuid (&Section->SectionDefinitionGuid, &gLzmaCustomDecompressGuid);
  Section->DataOffset = (UINT16)sizeof (EFI_GUID_DEFINED_SECTION2);
  Section->Attributes = EFI_GUIDED_SECTION_PROCESSING_REQUIRED;
  CopyMem (Section + 1, Src, SrcSize);

  Status = ExtractGuidedSectionGetInfo (Section, &OutputSize, &ScratchSize, &SectionAttribute);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CBFS: LZMA decoder unavailable or bad header: %r\n", Status));
    Status = EFI_VOLUME_CORRUPTED;
    goto Done;
  }

  if ((DecompressedSize != 0) && (OutputSize != DecompressedSize)) {
    DEBUG ((DEBUG_ERROR, "CBFS: LZMA size 0x%x != attribute size 0x%x\n", OutputSize, DecompressedSize));
    Status = EFI_VOLUME_CORRUPTED;
    goto Done;
  }

  Output  = AllocatePool (OutputSize);
  Scratch = AllocatePool (ScratchSize);
  if ((Output == NULL) || (Scratch == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = ExtractGuidedSectionDecode (Section, &Output, Scratch, &AuthenticationStatus);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "CBFS: LZMA decode failed: %r\n", Status));
    Status = EFI_VOLUME_CORRUPTED;
    goto Done;
  }

  *Buffer     = Output;
  *BufferSize = OutputSize;
  Output      = NULL;

Done:
  if (Output != NULL) {
    FreePool (Output);
  }

  if (Scratch != NULL) {
    FreePool (Scratch);
  }

  FreePool (Section);
  return Status;
}

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
  )
{
  EFI_STATUS     Status;
  CONST UINT8    *CbfsBase;
  UINT32         CbfsSize;
  CBFS_METADATA  Metadata;
  UINT32         FileOffset;
  CONST UINT8    *Data;
  UINT32         DataLength;
  UINT32         Compression;
  UINT32         DecompressedSize;
  VOID           *Copy;

  if ((Name == NULL) || (Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = CbfsLocate (&CbfsBase, &CbfsSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = CbfsFindFile (CbfsBase, CbfsSize, Name, &Metadata, &FileOffset);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "CBFS: '%a' not found\n", Name));
    return Status;
  }

  Data       = CbfsBase + FileOffset + SwapBytes32 (Metadata.Header.Offset);
  DataLength = SwapBytes32 (Metadata.Header.Len);
  CbfsGetCompression (&Metadata, &Compression, &DecompressedSize);

  DEBUG ((
    DEBUG_INFO,
    "CBFS: '%a' @0x%x, size 0x%x, compression %u\n",
    Name,
    FileOffset,
    DataLength,
    Compression
    ));

  switch (Compression) {
    case CBFS_COMPRESS_NONE:
      Copy = AllocateCopyPool (DataLength, Data);
      if (Copy == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      *Buffer     = Copy;
      *BufferSize = DataLength;
      return EFI_SUCCESS;

    case CBFS_COMPRESS_LZMA:
      return CbfsLzmaDecompress (Data, DataLength, DecompressedSize, Buffer, BufferSize);

    default:
      DEBUG ((DEBUG_ERROR, "CBFS: '%a' uses unsupported compression %u\n", Name, Compression));
      return EFI_UNSUPPORTED;
  }
}
