/** @file
  Shared boot splash BMP validation and ESP import helpers.

  Copyright (c) 2026
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/BootSplashConfig.h>
#include <Guid/Gpt.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/BootSplashLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FileHandleLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/SimpleFileSystem.h>

/**
  Return TRUE if two device paths are byte-identical.
**/
STATIC
BOOLEAN
BootSplashDevicePathsEqual (
  IN EFI_DEVICE_PATH_PROTOCOL  *PathA,
  IN EFI_DEVICE_PATH_PROTOCOL  *PathB
  )
{
  UINTN  SizeA;
  UINTN  SizeB;

  if ((PathA == NULL) || (PathB == NULL)) {
    return FALSE;
  }

  SizeA = GetDevicePathSize (PathA);
  SizeB = GetDevicePathSize (PathB);
  if ((SizeA == 0) || (SizeA != SizeB)) {
    return FALSE;
  }

  return (BOOLEAN)(CompareMem (PathA, PathB, SizeA) == 0);
}

/**
  Ensure DirectoryName exists under Parent and return an open handle to it.
**/
STATIC
EFI_STATUS
BootSplashEnsureDirectory (
  IN  EFI_FILE_PROTOCOL  *Parent,
  IN  CHAR16             *DirectoryName,
  OUT EFI_FILE_PROTOCOL  **Directory
  )
{
  EFI_STATUS  Status;

  if ((Parent == NULL) || (DirectoryName == NULL) || (Directory == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Directory = NULL;
  Status     = Parent->Open (
                         Parent,
                         Directory,
                         DirectoryName,
                         EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                         0
                         );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  return Parent->Open (
                   Parent,
                   Directory,
                   DirectoryName,
                   EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                   EFI_FILE_DIRECTORY
                   );
}

/**
  Write BmpBuffer to BOOT_SPLASH_ESP_FILE_PATH on EspHandle.
**/
STATIC
EFI_STATUS
BootSplashWriteEspFile (
  IN EFI_HANDLE  EspHandle,
  IN VOID        *BmpBuffer,
  IN UINTN       BmpSize
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *Fs;
  EFI_FILE_PROTOCOL                *Root;
  EFI_FILE_PROTOCOL                *EfiDir;
  EFI_FILE_PROTOCOL                *SplashDir;
  EFI_FILE_PROTOCOL                *File;
  UINTN                            WriteSize;

  Status = gBS->HandleProtocol (
                  EspHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&Fs
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  EfiDir    = NULL;
  SplashDir = NULL;
  File      = NULL;

  Status = BootSplashEnsureDirectory (Root, BOOT_SPLASH_ESP_DIR_EFI, &EfiDir);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = BootSplashEnsureDirectory (EfiDir, BOOT_SPLASH_ESP_DIR_NAME, &SplashDir);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = SplashDir->Open (
                        SplashDir,
                        &File,
                        BOOT_SPLASH_ESP_FILE_NAME,
                        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                        0
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = File->SetPosition (File, 0);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  WriteSize = BmpSize;
  Status    = File->Write (File, &WriteSize, BmpBuffer);
  if (EFI_ERROR (Status) || (WriteSize != BmpSize)) {
    Status = EFI_ERROR (Status) ? Status : EFI_DEVICE_ERROR;
    goto Done;
  }

  //
  // Truncate if a previous larger logo left trailing bytes.
  //
  Status = FileHandleSetSize (File, BmpSize);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  File->Flush (File);

Done:
  if (File != NULL) {
    File->Close (File);
  }

  if (SplashDir != NULL) {
    SplashDir->Close (SplashDir);
  }

  if (EfiDir != NULL) {
    EfiDir->Close (EfiDir);
  }

  Root->Close (Root);
  return Status;
}

/**
  Decode and validate a BMP buffer for use as a boot splash.
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
  )
{
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *LocalBlt;
  UINTN                          BltSize;
  UINTN                          LocalWidth;
  UINTN                          LocalHeight;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  UINT32                         SizeOfX;
  UINT32                         SizeOfY;
  INTN                           LocalDestX;
  INTN                           LocalDestY;

  if (Blt != NULL) {
    *Blt = NULL;
  }

  if (Width != NULL) {
    *Width = 0;
  }

  if (Height != NULL) {
    *Height = 0;
  }

  if (DestX != NULL) {
    *DestX = 0;
  }

  if (DestY != NULL) {
    *DestY = 0;
  }

  if ((BmpBuffer == NULL) || (BmpSize == 0) || (BmpSize > BOOT_SPLASH_MAX_FILE_SIZE)) {
    return EFI_UNSUPPORTED;
  }

  LocalBlt    = NULL;
  BltSize     = 0;
  LocalWidth  = 0;
  LocalHeight = 0;
  Status      = TranslateBmpToGopBlt (
                  BmpBuffer,
                  BmpSize,
                  &LocalBlt,
                  &BltSize,
                  &LocalHeight,
                  &LocalWidth
                  );
  if (EFI_ERROR (Status) || (LocalBlt == NULL) || (LocalWidth == 0) || (LocalHeight == 0)) {
    if (LocalBlt != NULL) {
      FreePool (LocalBlt);
    }

    return EFI_UNSUPPORTED;
  }

  Status = gBS->HandleProtocol (
                  gST->ConsoleOutHandle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput
                  );
  if (EFI_ERROR (Status)) {
    FreePool (LocalBlt);
    return Status;
  }

  SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
  SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;
  if ((LocalWidth > SizeOfX) || (LocalHeight > SizeOfY)) {
    FreePool (LocalBlt);
    return EFI_UNSUPPORTED;
  }

  LocalDestX = (INTN)(SizeOfX - LocalWidth) / 2;
  if (FixedPcdGetBool (PcdFollowBGRTSpecification)) {
    LocalDestY = (INTN)(SizeOfY * 382) / 1000 - (INTN)LocalHeight / 2;
  } else {
    LocalDestY = (INTN)(SizeOfY - LocalHeight) / 2;
  }

  if ((LocalDestX < 0) || (LocalDestY < 0)) {
    FreePool (LocalBlt);
    return EFI_UNSUPPORTED;
  }

  if (Width != NULL) {
    *Width = LocalWidth;
  }

  if (Height != NULL) {
    *Height = LocalHeight;
  }

  if (DestX != NULL) {
    *DestX = LocalDestX;
  }

  if (DestY != NULL) {
    *DestY = LocalDestY;
  }

  if (Blt != NULL) {
    *Blt = LocalBlt;
  } else {
    FreePool (LocalBlt);
  }

  return EFI_SUCCESS;
}

/**
  Read a file into a newly allocated buffer.
**/
EFI_STATUS
EFIAPI
BootSplashReadFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  OUT VOID                      **FileBuffer,
  OUT UINTN                     *FileSize
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *FilePathCopy;
  EFI_DEVICE_PATH_PROTOCOL  *FilePathWalk;
  EFI_FILE_HANDLE           FileHandle;
  UINT64                    FileSize64;
  UINTN                     Size;
  VOID                      *Buffer;

  if ((FilePath == NULL) || (FileBuffer == NULL) || (FileSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *FileBuffer = NULL;
  *FileSize   = 0;

  FilePathCopy = DuplicateDevicePath (FilePath);
  if (FilePathCopy == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FileHandle   = NULL;
  FilePathWalk = FilePathCopy;
  Status       = EfiOpenFileByDevicePath (
                   &FilePathWalk,
                   &FileHandle,
                   EFI_FILE_MODE_READ,
                   0
                   );
  FreePool (FilePathCopy);
  if (EFI_ERROR (Status) || (FileHandle == NULL)) {
    return EFI_ERROR (Status) ? Status : EFI_NOT_FOUND;
  }

  Status = FileHandleGetSize (FileHandle, &FileSize64);
  if (EFI_ERROR (Status) || (FileSize64 == 0) || (FileSize64 > BOOT_SPLASH_MAX_FILE_SIZE)) {
    FileHandleClose (FileHandle);
    return EFI_UNSUPPORTED;
  }

  Size   = (UINTN)FileSize64;
  Buffer = AllocatePool (Size);
  if (Buffer == NULL) {
    FileHandleClose (FileHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = FileHandleRead (FileHandle, &Size, Buffer);
  FileHandleClose (FileHandle);
  if (EFI_ERROR (Status) || (Size != (UINTN)FileSize64)) {
    FreePool (Buffer);
    return EFI_ERROR (Status) ? Status : EFI_DEVICE_ERROR;
  }

  *FileBuffer = Buffer;
  *FileSize   = Size;
  return EFI_SUCCESS;
}

/**
  Locate the first EFI System Partition handle.
**/
EFI_STATUS
EFIAPI
BootSplashFindFirstEsp (
  OUT EFI_HANDLE  *EspHandle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       HandleCount;

  if (EspHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *EspHandle  = NULL;
  Handles     = NULL;
  HandleCount = 0;
  Status      = gBS->LocateHandleBuffer (
                       ByProtocol,
                       &gEfiPartTypeSystemPartGuid,
                       NULL,
                       &HandleCount,
                       &Handles
                       );
  if (EFI_ERROR (Status) || (HandleCount == 0) || (Handles == NULL)) {
    return EFI_NOT_FOUND;
  }

  *EspHandle = Handles[0];
  FreePool (Handles);
  return EFI_SUCCESS;
}

/**
  Validate a source BMP and copy it to a fixed path on the first ESP.
**/
EFI_STATUS
EFIAPI
BootSplashImportBmpToEsp (
  IN  EFI_DEVICE_PATH_PROTOCOL  *SourcePath,
  OUT EFI_DEVICE_PATH_PROTOCOL  **DestPath
  )
{
  EFI_STATUS                Status;
  VOID                      *BmpBuffer;
  UINTN                     BmpSize;
  EFI_HANDLE                EspHandle;
  EFI_DEVICE_PATH_PROTOCOL  *EspFilePath;

  if ((SourcePath == NULL) || (DestPath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *DestPath = NULL;
  BmpBuffer = NULL;
  BmpSize   = 0;

  Status = BootSplashReadFile (SourcePath, &BmpBuffer, &BmpSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BootSplashDecodeBmp (BmpBuffer, BmpSize, NULL, NULL, NULL, NULL, NULL);
  if (EFI_ERROR (Status)) {
    FreePool (BmpBuffer);
    return Status;
  }

  Status = BootSplashFindFirstEsp (&EspHandle);
  if (EFI_ERROR (Status)) {
    FreePool (BmpBuffer);
    return Status;
  }

  EspFilePath = FileDevicePath (EspHandle, BOOT_SPLASH_ESP_FILE_PATH);
  if (EspFilePath == NULL) {
    FreePool (BmpBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  if (BootSplashDevicePathsEqual (SourcePath, EspFilePath)) {
    FreePool (BmpBuffer);
    *DestPath = EspFilePath;
    return EFI_SUCCESS;
  }

  Status = BootSplashWriteEspFile (EspHandle, BmpBuffer, BmpSize);
  FreePool (BmpBuffer);
  if (EFI_ERROR (Status)) {
    FreePool (EspFilePath);
    return Status;
  }

  *DestPath = EspFilePath;
  return EFI_SUCCESS;
}
