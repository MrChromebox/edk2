/** @file
  Apply boot splash settings from NVRAM (enable / type / custom BMP path).

  Copyright (c) 2026
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformBootManager.h"
#include <Guid/BootSplashConfig.h>
#include <Protocol/BootLogo.h>
#include <Protocol/BootLogo2.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PlatformLogo.h>
#include <Library/BmpSupportLib.h>
#include <Library/FileHandleLib.h>

/**
  Read a UINT8 boot splash NVRAM variable or return DefaultValue.
**/
STATIC
UINT8
BootSplashGetUint8Variable (
  IN CHAR16  *VariableName,
  IN UINT8   DefaultValue
  )
{
  EFI_STATUS  Status;
  UINTN       DataSize;
  UINT8       Value;

  DataSize = sizeof (Value);
  Status   = gRT->GetVariable (
                    VariableName,
                    &gUefiPayloadBootSplashGuid,
                    NULL,
                    &DataSize,
                    &Value
                    );
  if (EFI_ERROR (Status) || (DataSize != sizeof (Value))) {
    return DefaultValue;
  }

  return Value;
}

/**
  Display a custom BMP from BootSplashPath and register it with BootLogo.

  @retval EFI_SUCCESS   Custom logo displayed.
  @retval other         Failed; caller should fall back to default.
**/
STATIC
EFI_STATUS
BootSplashDisplayCustom (
  VOID
  )
{
  EFI_STATUS                             Status;
  UINTN                                  DataSize;
  EFI_DEVICE_PATH_PROTOCOL               *StoredPath;
  EFI_DEVICE_PATH_PROTOCOL               *FilePath;
  EFI_DEVICE_PATH_PROTOCOL               *FilePathWalk;
  EFI_FILE_HANDLE                        FileHandle;
  UINT64                                 FileSize64;
  UINTN                                  FileSize;
  VOID                                   *BmpBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL          *Blt;
  UINTN                                  BltSize;
  UINTN                                  Height;
  UINTN                                  Width;
  EFI_GRAPHICS_OUTPUT_PROTOCOL           *GraphicsOutput;
  EFI_BOOT_LOGO_PROTOCOL                 *BootLogo;
  EDKII_BOOT_LOGO2_PROTOCOL              *BootLogo2;
  INTN                                   DestX;
  INTN                                   DestY;
  UINT32                                 SizeOfX;
  UINT32                                 SizeOfY;

  StoredPath = AllocateZeroPool (BOOT_SPLASH_PATH_MAX_SIZE);
  if (StoredPath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DataSize = BOOT_SPLASH_PATH_MAX_SIZE;
  Status   = gRT->GetVariable (
                    BOOT_SPLASH_PATH_VARIABLE_NAME,
                    &gUefiPayloadBootSplashGuid,
                    NULL,
                    &DataSize,
                    StoredPath
                    );
  if (EFI_ERROR (Status) || (DataSize < sizeof (EFI_DEVICE_PATH_PROTOCOL)) ||
      !IsDevicePathValid (StoredPath, DataSize))
  {
    FreePool (StoredPath);
    return EFI_NOT_FOUND;
  }

  FilePath = DuplicateDevicePath (StoredPath);
  FreePool (StoredPath);
  if (FilePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FileHandle   = NULL;
  FilePathWalk = FilePath;
  Status       = EfiOpenFileByDevicePath (
                   &FilePathWalk,
                   &FileHandle,
                   EFI_FILE_MODE_READ,
                   0
                   );
  FreePool (FilePath);

  if (EFI_ERROR (Status) || (FileHandle == NULL)) {
    return EFI_ERROR (Status) ? Status : EFI_NOT_FOUND;
  }

  Status = FileHandleGetSize (FileHandle, &FileSize64);
  if (EFI_ERROR (Status) || (FileSize64 == 0) || (FileSize64 > MAX_UINTN)) {
    FileHandleClose (FileHandle);
    return EFI_UNSUPPORTED;
  }

  FileSize  = (UINTN)FileSize64;
  BmpBuffer = AllocatePool (FileSize);
  if (BmpBuffer == NULL) {
    FileHandleClose (FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FileHandleRead (FileHandle, &FileSize, BmpBuffer);
  FileHandleClose (FileHandle);
  if (EFI_ERROR (Status)) {
    FreePool (BmpBuffer);
    return Status;
  }

  Blt     = NULL;
  BltSize = 0;
  Height  = 0;
  Width   = 0;
  Status  = TranslateBmpToGopBlt (BmpBuffer, FileSize, &Blt, &BltSize, &Height, &Width);
  FreePool (BmpBuffer);
  if (EFI_ERROR (Status) || (Blt == NULL) || (Width == 0) || (Height == 0)) {
    if (Blt != NULL) {
      FreePool (Blt);
    }

    return EFI_UNSUPPORTED;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Blt);
    return Status;
  }

  SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
  SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;
  if ((Width > SizeOfX) || (Height > SizeOfY)) {
    FreePool (Blt);
    return EFI_UNSUPPORTED;
  }

  DestX = (INTN)(SizeOfX - Width) / 2;
  if (FixedPcdGetBool (PcdFollowBGRTSpecification)) {
    DestY = (INTN)(SizeOfY * 382) / 1000 - (INTN)Height / 2;
  } else {
    DestY = (INTN)(SizeOfY - Height) / 2;
  }

  if ((DestX < 0) || (DestY < 0)) {
    FreePool (Blt);
    return EFI_UNSUPPORTED;
  }

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  Status = GraphicsOutput->Blt (
                             GraphicsOutput,
                             Blt,
                             EfiBltBufferToVideo,
                             0,
                             0,
                             (UINTN)DestX,
                             (UINTN)DestY,
                             Width,
                             Height,
                             Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                             );
  if (EFI_ERROR (Status)) {
    FreePool (Blt);
    return Status;
  }

  BootLogo  = NULL;
  BootLogo2 = NULL;
  gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **)&BootLogo);
  gBS->LocateProtocol (&gEdkiiBootLogo2ProtocolGuid, NULL, (VOID **)&BootLogo2);

  Status = EFI_UNSUPPORTED;
  if (BootLogo2 != NULL) {
    Status = BootLogo2->SetBootLogo (BootLogo2, Blt, (UINTN)DestX, (UINTN)DestY, Width, Height);
  }

  if (EFI_ERROR (Status) && (BootLogo != NULL)) {
    Status = BootLogo->SetBootLogo (BootLogo, Blt, (UINTN)DestX, (UINTN)DestY, Width, Height);
  }

  FreePool (Blt);
  //
  // Display succeeded even if BootLogo registration fails.
  //
  return EFI_SUCCESS;
}

/**
  Display the embedded PlatformLogo default splash.
**/
STATIC
VOID
BootSplashDisplayDefault (
  VOID
  )
{
  EFI_STATUS                    Status;
  EDKII_PLATFORM_LOGO_PROTOCOL  *PlatformLogo;

  Status = gBS->LocateProtocol (&gEdkiiPlatformLogoProtocolGuid, NULL, (VOID **)&PlatformLogo);
  if (EFI_ERROR (Status)) {
    return;
  }

  if ((gST != NULL) && (gST->ConOut != NULL)) {
    gST->ConOut->ClearScreen (gST->ConOut);
  }

  BootLogoEnableLogo ();
}

/**
  TRUE if splash is enabled and type is Custom (needs a connected filesystem).
**/
BOOLEAN
BootSplashRequiresConnect (
  VOID
  )
{
  UINT8  Enable;
  UINT8  Type;

  Enable = BootSplashGetUint8Variable (
             BOOT_SPLASH_ENABLE_VARIABLE_NAME,
             BOOT_SPLASH_ENABLE_DEFAULT
             );
  if (Enable == 0) {
    return FALSE;
  }

  Type = BootSplashGetUint8Variable (
           BOOT_SPLASH_TYPE_VARIABLE_NAME,
           BOOT_SPLASH_TYPE_DEFAULT_VALUE
           );
  return (BOOLEAN)(Type == BOOT_SPLASH_TYPE_CUSTOM);
}

/**
  Apply boot splash policy from NVRAM variables.

  Enable=0 skips the logo. Type=Custom attempts BootSplashPath; on any failure
  falls back to the embedded default logo.
**/
VOID
BootSplashApply (
  VOID
  )
{
  UINT8       Enable;
  UINT8       Type;
  EFI_STATUS  Status;

  Enable = BootSplashGetUint8Variable (
             BOOT_SPLASH_ENABLE_VARIABLE_NAME,
             BOOT_SPLASH_ENABLE_DEFAULT
             );
  if (Enable == 0) {
    DEBUG ((DEBUG_INFO, "%a: boot splash disabled by NVRAM\n", __func__));
    return;
  }

  Type = BootSplashGetUint8Variable (
           BOOT_SPLASH_TYPE_VARIABLE_NAME,
           BOOT_SPLASH_TYPE_DEFAULT_VALUE
           );

  if (Type == BOOT_SPLASH_TYPE_CUSTOM) {
    if ((gST != NULL) && (gST->ConOut != NULL)) {
      gST->ConOut->ClearScreen (gST->ConOut);
    }

    Status = BootSplashDisplayCustom ();
    if (!EFI_ERROR (Status)) {
      return;
    }

    DEBUG ((
      DEBUG_WARN,
      "%a: custom bootsplash failed (%r); falling back to default\n",
      __func__,
      Status
      ));
  }

  BootSplashDisplayDefault ();
}
