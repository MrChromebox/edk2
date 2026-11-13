/** @file
  Load optional payload assets from CBFS with firmware-volume fallback.

  Prefers CBFS (efi/<name>, then root) and falls back to FV embeds so
  uefipayload_2608 can transition boards to CBFS without dropping the
  existing NetworkDrivers / logo FV content. Installs into in-memory FVs
  only when the bytes came from CBFS.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CbfsAssetLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

STATIC CONST CHAR8  *mLogoCbfsNames[] = {
  "logo.bmp",
  "bootsplash.bmp"
};

STATIC CONST CHAR8  *mIpxeCbfsNames[] = {
  "ipxe.efi"
};

typedef struct {
  CONST CHAR8         *CbfsName;
  CONST EFI_GUID      *FileGuid;
  CONST CHAR16        *UiName;
} CBFS_UNDI_ASSET;

STATIC CONST EFI_GUID  mRtkPciUndiGuid = {
  0xD30D5496, 0x6DF9, 0x462D, { 0x9B, 0x97, 0x0B, 0x93, 0xF6, 0x59, 0xD1, 0xA0 }
};
STATIC CONST EFI_GUID  mRtkUsbUndiGuid = {
  0x95BA3BCA, 0xF086, 0x4FDA, { 0xBB, 0xAC, 0x44, 0xF5, 0xAE, 0xE5, 0x17, 0x6B }
};
STATIC CONST EFI_GUID  mAx88179Guid = {
  0x27E5D3B6, 0x7839, 0x47C2, { 0x86, 0x18, 0x5D, 0x21, 0x90, 0x72, 0x9B, 0xC7 }
};
STATIC CONST EFI_GUID  mAx88772Guid = {
  0xB15239D6, 0x6A01, 0x4808, { 0xA0, 0xF7, 0xB7, 0xF2, 0x0F, 0x07, 0x35, 0x55 }
};

STATIC CONST CBFS_UNDI_ASSET  mUndiAssets[] = {
  { "RtkUndiDxe.efi",    &mRtkPciUndiGuid, L"RtkUndiDxe"    },
  { "RtkUsbUndiDxe.efi", &mRtkUsbUndiGuid, L"RtkUsbUndiDxe" },
  { "Ax88179.efi",       &mAx88179Guid,    L"Ax88179"       },
  { "Ax88772.efi",       &mAx88772Guid,    L"Ax88772"       },
};

/**
  Install a bootsplash BMP from CBFS when present.

  LogoDxe consumes PcdLogoFileGuid via GetSectionFromAnyFv. On CBFS miss the
  embedded HII logo (or FV RAW) remains the fallback.
**/
STATIC
VOID
LoadLogoAsset (
  VOID
  )
{
  EFI_STATUS     Status;
  VOID           *Buffer;
  UINTN          BufferSize;
  BOOLEAN        FromCbfs;
  CONST EFI_GUID *LogoGuid;

  LogoGuid = (CONST EFI_GUID *)PcdGetPtr (PcdLogoFileGuid);
  if ((LogoGuid == NULL) || (PcdGetSize (PcdLogoFileGuid) != sizeof (EFI_GUID)) || IsZeroGuid (LogoGuid)) {
    DEBUG ((DEBUG_INFO, "CbfsPayloadAssets: PcdLogoFileGuid unset, skip logo CBFS load\n"));
    return;
  }

  Buffer = NULL;
  Status = CbfsAssetLoadPreferCbfs (
             mLogoCbfsNames,
             ARRAY_SIZE (mLogoCbfsNames),
             LogoGuid,
             EFI_SECTION_RAW,
             &Buffer,
             &BufferSize,
             &FromCbfs
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "CbfsPayloadAssets: no logo in CBFS/FV: %r\n", Status));
    return;
  }

  if (FromCbfs) {
    Status = CbfsAssetInstallRawFv (Buffer, BufferSize, LogoGuid, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "CbfsPayloadAssets: failed to install CBFS logo FV: %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "CbfsPayloadAssets: installed CBFS logo (%lu bytes)\n", (UINT64)BufferSize));
    }
  }

  FreePool (Buffer);
}

/**
  Prefer ipxe.efi from CBFS; fall back to the FV FREEFORM embed (NETWORK_IPXE).
**/
STATIC
VOID
LoadIpxeAsset (
  VOID
  )
{
  EFI_STATUS     Status;
  VOID           *Buffer;
  UINTN          BufferSize;
  BOOLEAN        FromCbfs;
  CONST EFI_GUID *IpxeGuid;

  IpxeGuid = (CONST EFI_GUID *)PcdGetPtr (PcdiPXEFile);
  if ((IpxeGuid == NULL) || (PcdGetSize (PcdiPXEFile) != sizeof (EFI_GUID)) || IsZeroGuid (IpxeGuid)) {
    return;
  }

  Buffer = NULL;
  Status = CbfsAssetLoadPreferCbfs (
             mIpxeCbfsNames,
             ARRAY_SIZE (mIpxeCbfsNames),
             IpxeGuid,
             EFI_SECTION_PE32,
             &Buffer,
             &BufferSize,
             &FromCbfs
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "CbfsPayloadAssets: no iPXE in CBFS/FV: %r\n", Status));
    return;
  }

  if (FromCbfs) {
    Status = CbfsAssetInstallPe32Fv (
               Buffer,
               BufferSize,
               IpxeGuid,
               EFI_FV_FILETYPE_FREEFORM,
               NULL,
               L"ipxe",
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "CbfsPayloadAssets: failed to install CBFS iPXE FV: %r\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "CbfsPayloadAssets: installed CBFS iPXE\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "CbfsPayloadAssets: using FV-embedded iPXE\n"));
  }

  FreePool (Buffer);
}

/**
  Prefer each UNDI from CBFS; fall back to NetworkDrivers FV embeds.
**/
STATIC
VOID
LoadUndiAssets (
  VOID
  )
{
  EFI_STATUS       Status;
  UINTN            Index;
  VOID             *Buffer;
  UINTN            BufferSize;
  BOOLEAN          FromCbfs;
  CONST CHAR8      *Names[1];

  for (Index = 0; Index < ARRAY_SIZE (mUndiAssets); Index++) {
    Names[0] = mUndiAssets[Index].CbfsName;
    Buffer   = NULL;
    Status   = CbfsAssetLoadPreferCbfs (
                 Names,
                 1,
                 mUndiAssets[Index].FileGuid,
                 EFI_SECTION_PE32,
                 &Buffer,
                 &BufferSize,
                 &FromCbfs
                 );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_VERBOSE,
        "CbfsPayloadAssets: skip UNDI %a: %r\n",
        mUndiAssets[Index].CbfsName,
        Status
        ));
      continue;
    }

    if (FromCbfs) {
      Status = CbfsAssetInstallPe32Fv (
                 Buffer,
                 BufferSize,
                 mUndiAssets[Index].FileGuid,
                 EFI_FV_FILETYPE_DRIVER,
                 NULL,
                 mUndiAssets[Index].UiName,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "CbfsPayloadAssets: failed to install UNDI %a: %r\n",
          mUndiAssets[Index].CbfsName,
          Status
          ));
      } else {
        DEBUG ((DEBUG_INFO, "CbfsPayloadAssets: installed UNDI %a from CBFS\n", mUndiAssets[Index].CbfsName));
      }
    } else {
      DEBUG ((DEBUG_VERBOSE, "CbfsPayloadAssets: using FV-embedded UNDI %a\n", mUndiAssets[Index].CbfsName));
    }

    FreePool (Buffer);
  }
}

/**
  Entry point: publish optional CBFS/FV assets for later consumers.
**/
EFI_STATUS
EFIAPI
CbfsPayloadAssetsDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  LoadLogoAsset ();
  LoadIpxeAsset ();
  LoadUndiAssets ();
  return EFI_SUCCESS;
}
