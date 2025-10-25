/** @file
  Depthcharge-based SD/MMC PCI Host Controller Driver
  Main driver entry point and PCI binding logic

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SdMmcPciCbDxe.h"

//
// PCI Class codes
//
#define PCI_CLASS_SYSTEM_PERIPHERAL  0x08
#define PCI_SUBCLASS_SD_HOST_CONTROLLER  0x05
#define PCI_BASE_ADDRESSREG_OFFSET  0x10

EFI_DRIVER_BINDING_PROTOCOL gSdMmcPciCbDriverBinding = {
  SdMmcPciCbDriverBindingSupported,
  SdMmcPciCbDriverBindingStart,
  SdMmcPciCbDriverBindingStop,
  0x10,
  NULL,  // ImageHandle - will be set in entry point
  NULL   // DriverBindingHandle - will be set in entry point
};

/**
  Timer callback to check for SD card insertion/removal.

  @param[in] Event    The timer event
  @param[in] Context  Pointer to SD_MMC_CB_DEVICE

**/
VOID
EFIAPI
MediaChangeTimerCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  SD_MMC_CB_DEVICE  *Device;
  UINT32            PresentState;
  BOOLEAN           CurrentPresent;
  BOOLEAN           PreviousPresent;

  Device = (SD_MMC_CB_DEVICE *)Context;

  //
  // Only check for removable media (SD cards)
  //
  if (Device->IsEMMC) {
    return;
  }

  //
  // Check if card is present
  //
  PresentState = SdhciReadl (Device, SDHCI_PRESENT_STATE);
  CurrentPresent = (PresentState & SDHCI_CARD_PRESENT) != 0;
  PreviousPresent = Device->BlockIoMedia.MediaPresent;

  //
  // If media state changed, update and trigger reconnection
  //
  if (CurrentPresent != PreviousPresent) {
    Device->BlockIoMedia.MediaPresent = CurrentPresent;
    Device->BlockIoMedia.MediaId++;

    if (CurrentPresent) {
      DEBUG ((DEBUG_INFO, "SdMmcCb: SD card inserted! MediaId=%d\n", Device->BlockIoMedia.MediaId));

      //
      // Re-initialize the SD card to get its capacity and parameters
      // First, reset the controller to clear any stale state from previous card
      //
      SdhciReset (Device, SDHCI_RESET_ALL);
      gBS->Stall (10000);  // 10ms for reset to complete

      // Re-initialize the controller for the new card
      EFI_STATUS Status = SdhciInit (Device);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "SdMmcCb: SdhciInit failed during hot-plug: %r\n", Status));
        Device->BlockIoMedia.MediaPresent = FALSE;
        Device->BlockIoMedia.LastBlock = 0;
      } else {
        // Now initialize the card
        Status = SdStartup (Device);
        if (!EFI_ERROR (Status)) {
          // Update BlockIo media with new card's capacity
          Device->BlockIoMedia.BlockSize = Device->BlockSize;
          Device->BlockIoMedia.LastBlock = Device->TotalBlocks > 0 ? Device->TotalBlocks - 1 : 0;
          DEBUG ((DEBUG_INFO, "SdMmcCb: SD card initialized: %lld blocks\n", Device->TotalBlocks));
        } else {
          DEBUG ((DEBUG_ERROR, "SdMmcCb: SD card initialization failed: %r\n", Status));
          // Set media as not present if init fails
          Device->BlockIoMedia.MediaPresent = FALSE;
          Device->BlockIoMedia.LastBlock = 0;
        }
      }
    } else {
      DEBUG ((DEBUG_INFO, "SdMmcCb: SD card removed! MediaId=%d\n", Device->BlockIoMedia.MediaId));
      // Reset capacity when card is removed
      Device->TotalBlocks = 0;
      Device->BlockIoMedia.LastBlock = 0;
    }

    //
    // Reinstall BlockIo protocol to trigger BDS reconnection
    // This makes the boot menu refresh automatically
    //
    gBS->ReinstallProtocolInterface (
           Device->ChildHandle,
           &gEfiBlockIoProtocolGuid,
           &Device->BlockIo,
           &Device->BlockIo
           );
  }
}

/**
  Test to see if this driver supports ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver supports this device.
  @retval EFI_ALREADY_STARTED  This driver is already running on this device.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SdMmcPciCbDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           PciData;

  PciIo            = NULL;

  //DEBUG ((DEBUG_INFO, "SdMmcPciCb: Supported() called\n"));

  //
  // Open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    //DEBUG ((DEBUG_WARN, "SdMmcPciCb: OpenProtocol failed: %r\n", Status));
    return Status;
  }

  //DEBUG ((DEBUG_INFO, "SdMmcPciCb: OpenProtocol succeeded, reading PCI config...\n"));

  //
  // Read PCI configuration space
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (PciData) / sizeof (UINT32),
                        &PciData
                        );
  if (EFI_ERROR (Status)) {
    //DEBUG ((DEBUG_ERROR, "SdMmcPciCb: PCI read failed: %r\n", Status));
    goto Done;
  }

  //
  // Debug: Print every device we check
  //
  DEBUG ((DEBUG_VERBOSE, "SdMmcPciCb: Checking device %04x:%04x\n",
          PciData.Hdr.VendorId, PciData.Hdr.DeviceId));

  //
  // Check if this is an SD Host Controller (Class 08, SubClass 05)
  // This matches Depthcharge's approach: support ALL SDHCI controllers
  //
  if (PciData.Hdr.ClassCode[2] != PCI_CLASS_SYSTEM_PERIPHERAL ||
      PciData.Hdr.ClassCode[1] != PCI_SUBCLASS_SD_HOST_CONTROLLER) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  //
  // This is an SDHCI controller - we support it!
  // Same universal approach as Depthcharge's pci_sdhci.c
  //
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Found SDHCI controller %04x:%04x (Class=%02x SubClass=%02x)\n",
          PciData.Hdr.VendorId, PciData.Hdr.DeviceId,
          PciData.Hdr.ClassCode[2], PciData.Hdr.ClassCode[1]));

  //
  // Identify known controllers for informational purposes
  //
  if (PciData.Hdr.VendorId == 0x17a0) {
    // Genesys Logic
    if (PciData.Hdr.DeviceId == 0xe763) {
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: -> Genesys Logic GL9763E (eMMC)\n"));
    } else if (PciData.Hdr.DeviceId == 0x9750) {
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: -> Genesys Logic GL9750 (SD)\n"));
    }
  } else if (PciData.Hdr.VendorId == 0x8086) {
    // Intel integrated SDHCI
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: -> Intel integrated SDHCI\n"));
  } else if (PciData.Hdr.VendorId == 0x1022) {
    // AMD integrated SDHCI
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: -> AMD integrated SDHCI\n"));
  } else if (PciData.Hdr.VendorId == 0x1217) {
    // O2 Micro / BayHub (BH720 family)
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: -> BayHub/O2Micro SDHCI (generic mode)\n"));
  }

  //
  // Accept all SDHCI controllers (vendor quirks applied during Start)
  //
  Status = EFI_SUCCESS;

Done:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Start this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to bind driver to.
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
SdMmcPciCbDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  SD_MMC_CB_DEVICE      *Device;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT64                Supports;
  UINTN                 MmioBase;

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: DriverBindingStart called\n"));

  Device = NULL;

  //
  // Open PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate device context
  //
  Device = AllocateZeroPool (sizeof (SD_MMC_CB_DEVICE));
  if (Device == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Device->Signature = SD_MMC_CB_SIGNATURE;
  Device->ControllerHandle = Controller;
  Device->PciIo = PciIo;

  //
  // Detect controller type
  //
  PCI_TYPE00  PciData;
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (PciData) / sizeof (UINT32),
                        &PciData
                        );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Detect controller type and set quirk flags
  //
  Device->IsGL9763E = FALSE;
  Device->IsGL9750 = FALSE;

  if (PciData.Hdr.VendorId == 0x17a0) {
    // Genesys Logic controllers
    if (PciData.Hdr.DeviceId == 0xe763) {
      Device->IsGL9763E = TRUE;
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: Genesys Logic GL9763E detected (will apply L1 delay quirk)\n"));
    } else if (PciData.Hdr.DeviceId == 0x9750) {
      Device->IsGL9750 = TRUE;
      DEBUG ((DEBUG_INFO, "SdMmcPciCb: Genesys Logic GL9750 detected\n"));
    }
  }

  //
  // Auto-detect eMMC vs SD card after controller initialization
  // For now, assume eMMC for GL9763E, SD for GL9750, and try eMMC first for unknown
  //
  if (Device->IsGL9763E) {
    Device->IsEMMC = TRUE;  // GL9763E is always eMMC
  } else if (Device->IsGL9750) {
    Device->IsEMMC = FALSE;  // GL9750 is always SD
  } else {
    // Unknown controller - try eMMC first (most common in Chromebooks)
    // If eMMC fails, could fall back to SD detection in future
    Device->IsEMMC = TRUE;
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Unknown SDHCI controller, assuming eMMC slot\n"));
  }

  //
  // Get device path from controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&Device->DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "SdMmcPciCb: Failed to get DevicePath: %r\n", Status));
    Device->DevicePath = NULL;
  }

  //
  // Enable PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &Supports
                    );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    Supports,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  //
  // Get BAR0 (MMIO base) - Read directly from PCI config space
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        PCI_BASE_ADDRESSREG_OFFSET,
                        1,
                        &MmioBase
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to read BAR0: %r\n", Status));
    goto Error;
  }

  //
  // Clear lower bits (memory type flags - bit 0=Memory Space Indicator, bits 1-2=Type, bit 3=Prefetchable)
  //
  MmioBase &= ~0xFULL;

  if (MmioBase == 0) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: BAR0 is not assigned!\n"));
    Status = EFI_DEVICE_ERROR;
    goto Error;
  }

  Device->MmioBase = (VOID *)MmioBase;
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: MMIO Base = 0x%016lx\n", (UINT64)MmioBase));

  //
  // Read controller version and capabilities
  //
  Device->Version = SdhciReadw (Device, SDHCI_HOST_VERSION);
  Device->Capabilities = SdhciReadl (Device, SDHCI_CAPABILITIES);

  // DEBUG ((DEBUG_INFO, "SdMmcPciCb: GL9763E Controller initialized!\n"));
  // DEBUG ((DEBUG_INFO, "SdMmcPciCb: Version = 0x%04x\n", Device->Version));
  // DEBUG ((DEBUG_INFO, "SdMmcPciCb: Capabilities = 0x%08x\n", Device->Capabilities));

  //
  // Phase 2: Initialize SDHCI controller
  //
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Initializing SDHCI controller...\n"));
  Status = SdhciInit (Device);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: SdhciInit failed: %r\n", Status));
    goto Error;
  }

  //
  // Phase 3: Initialize card (eMMC or SD)
  //
  if (Device->IsEMMC) {
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Initializing eMMC device...\n"));
    Status = MmcStartup (Device);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: MmcStartup failed: %r\n", Status));
      goto Error;
    }
  } else {
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Initializing SD card...\n"));
    Status = SdStartup (Device);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: SdStartup failed: %r\n", Status));
      // SD card might not be inserted - this is OK for removable media
      // Set defaults and continue (BlockIo will report no media)
      Device->BlockSize = 512;
      Device->TotalBlocks = 0;
      Device->HighCapacity = TRUE;
      Device->BlockIoMedia.MediaPresent = FALSE;
      DEBUG ((DEBUG_WARN, "SdMmcPciCb: No SD card present or initialization failed\n"));
    }
  }

  //
  // Phase 4: Install BlockIo Protocol
  //
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Installing BlockIo protocol...\n"));

  //
  // Initialize BlockIo Media
  //
  Device->BlockIoMedia.MediaId = 0;
  Device->BlockIoMedia.RemovableMedia = !Device->IsEMMC;  // SD cards are removable
  Device->BlockIoMedia.MediaPresent = TRUE;  // Assume present (updated during init if failed)
  Device->BlockIoMedia.LogicalPartition = FALSE;
  Device->BlockIoMedia.ReadOnly = FALSE;
  Device->BlockIoMedia.WriteCaching = FALSE;
  Device->BlockIoMedia.BlockSize = Device->BlockSize;
  Device->BlockIoMedia.IoAlign = 4; // 32-bit aligned
  Device->BlockIoMedia.LastBlock = Device->TotalBlocks > 0 ? Device->TotalBlocks - 1 : 0;

  //
  // Initialize BlockIo Protocol
  //
  Device->BlockIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION3;
  Device->BlockIo.Media = &Device->BlockIoMedia;
  Device->BlockIo.Reset = SdMmcDcBlockIoReset;
  Device->BlockIo.ReadBlocks = SdMmcDcBlockIoReadBlocks;
  Device->BlockIo.WriteBlocks = SdMmcDcBlockIoWriteBlocks;
  Device->BlockIo.FlushBlocks = SdMmcDcBlockIoFlushBlocks;

  //
  // Initialize DiskInfo Protocol
  //
  if (Device->IsEMMC) {
    CopyGuid (&Device->DiskInfo.Interface, &gEfiDiskInfoSdMmcInterfaceGuid);
  } else {
    CopyGuid (&Device->DiskInfo.Interface, &gEfiDiskInfoSdMmcInterfaceGuid);
  }
  Device->DiskInfo.Inquiry = SdMmcDcDiskInfoInquiry;
  Device->DiskInfo.Identify = SdMmcDcDiskInfoIdentify;
  Device->DiskInfo.SenseData = SdMmcDcDiskInfoSenseData;
  Device->DiskInfo.WhichIde = SdMmcDcDiskInfoWhichIde;

  //
  // Create device path for the card device
  // The parent controller should have a device path; we append an SD/eMMC node to it
  //
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath = NULL;
  if (Device->DevicePath != NULL) {
    if (Device->IsEMMC) {
      // Use MSG_EMMC_DP for eMMC to get "Internal eMMC" naming in BDS
      EMMC_DEVICE_PATH  EmmcNode;

      EmmcNode.Header.Type = MESSAGING_DEVICE_PATH;
      EmmcNode.Header.SubType = MSG_EMMC_DP;
      SetDevicePathNodeLength (&EmmcNode.Header, sizeof (EMMC_DEVICE_PATH));
      EmmcNode.SlotNumber = 0;  // Single eMMC device on this controller

      NewDevicePath = AppendDevicePathNode (Device->DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&EmmcNode);
    } else {
      // Use MSG_SD_DP for SD card
      SD_DEVICE_PATH  SdNode;

      SdNode.Header.Type = MESSAGING_DEVICE_PATH;
      SdNode.Header.SubType = MSG_SD_DP;
      SetDevicePathNodeLength (&SdNode.Header, sizeof (SD_DEVICE_PATH));
      SdNode.SlotNumber = 0;

      NewDevicePath = AppendDevicePathNode (Device->DevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&SdNode);
    }

    if (NewDevicePath == NULL) {
      DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to create device path\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }
  }

  //
  // Install BlockIo, DiskInfo, and DevicePath Protocols on child handle
  //
  Device->ChildHandle = NULL;
  if (NewDevicePath != NULL) {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Device->ChildHandle,
                    &gEfiDevicePathProtocolGuid,
                    NewDevicePath,
                    &gEfiBlockIoProtocolGuid,
                    &Device->BlockIo,
                    &gEfiDiskInfoProtocolGuid,
                    &Device->DiskInfo,
                    NULL
                    );
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Device->ChildHandle,
                    &gEfiBlockIoProtocolGuid,
                    &Device->BlockIo,
                    &gEfiDiskInfoProtocolGuid,
                    &Device->DiskInfo,
                    NULL
                    );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to install BlockIo: %r\n", Status));
    if (NewDevicePath != NULL) {
      FreePool (NewDevicePath);
    }
    goto Error;
  }

  //
  // Open PciIo protocol BY_CHILD_CONTROLLER
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Device->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdMmcPciCb: Failed to open PciIo BY_CHILD: %r\n", Status));
    gBS->UninstallMultipleProtocolInterfaces (
           Device->ChildHandle,
           &gEfiBlockIoProtocolGuid,
           &Device->BlockIo,
           &gEfiDiskInfoProtocolGuid,
           &Device->DiskInfo,
           NULL
           );
    goto Error;
  }

  if (Device->IsEMMC) {
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Driver start complete! eMMC initialized successfully!\n"));
  } else {
    DEBUG ((DEBUG_INFO, "SdMmcPciCb: Driver start complete! SD card reader ready!\n"));

    //
    // For removable media (SD cards), create a timer to poll for media changes
    // This allows automatic boot menu refresh when cards are inserted/removed
    //
    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    MediaChangeTimerCallback,
                    Device,  // Pass device context to callback
                    &Device->MediaChangeEvent
                    );
    if (!EFI_ERROR (Status)) {
      // Poll every 100ms (same as standard driver)
      Status = gBS->SetTimer (
                      Device->MediaChangeEvent,
                      TimerPeriodic,
                      1000000  // 100ms in 100ns units
                      );
      if (EFI_ERROR (Status)) {
        gBS->CloseEvent (Device->MediaChangeEvent);
        Device->MediaChangeEvent = NULL;
      } else {
        DEBUG ((DEBUG_INFO, "SdMmcPciCb: Media change polling enabled (100ms)\n"));
      }
    }
  }
  DEBUG ((DEBUG_INFO, "SdMmcPciCb: BlockIo installed on handle %p\n", Device->ChildHandle));

  return EFI_SUCCESS;

Error:
  if (Device != NULL) {
    FreePool (Device);
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Stop this driver on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  Controller        Handle of device to stop driver on.
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle.
  @retval other             This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
SdMmcPciCbDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  EFI_BLOCK_IO_PROTOCOL     *BlockIo;
  SD_MMC_CB_DEVICE          *Device;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  if (NumberOfChildren == 0) {
    //
    // Close PciIo protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return EFI_SUCCESS;
  }

  //
  // Get BlockIo protocol from child handle
  //
  Status = gBS->OpenProtocol (
                  ChildHandleBuffer[0],
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Device = SD_MMC_CB_DEVICE_FROM_BLOCK_IO (BlockIo);

  //
  // Get device path to free it later
  //
  Status = gBS->OpenProtocol (
                  ChildHandleBuffer[0],
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  //
  // Close PciIo protocol opened BY_CHILD_CONTROLLER
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ChildHandleBuffer[0]
         );

  //
  // Uninstall protocols from child handle
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandleBuffer[0],
                  &gEfiBlockIoProtocolGuid,
                  &Device->BlockIo,
                  &gEfiDiskInfoProtocolGuid,
                  &Device->DiskInfo,
                  &gEfiDevicePathProtocolGuid,
                  DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Free device path
  //
  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  //
  // Stop and close media change timer if it exists (for SD cards)
  //
  if (Device->MediaChangeEvent != NULL) {
    gBS->SetTimer (Device->MediaChangeEvent, TimerCancel, 0);
    gBS->CloseEvent (Device->MediaChangeEvent);
    Device->MediaChangeEvent = NULL;
  }

  //
  // Free ADMA descriptors if allocated
  //
  if (Device->AdmaDescs != NULL) {
    FreePool (Device->AdmaDescs);
  }
  if (Device->Adma64Descs != NULL) {
    FreePool (Device->Adma64Descs);
  }

  //
  // Free device structure
  //
  FreePool (Device);

  return EFI_SUCCESS;
}

/**
  Driver entry point.

  @param  ImageHandle  The firmware allocated handle for the EFI image.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The driver was initialized successfully.

**/
EFI_STATUS
EFIAPI
InitializeSdMmcPciCbDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Depthcharge-based SDHCI Driver Loading...\n"));

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gSdMmcPciCbDriverBinding,
             ImageHandle,
             &gSdMmcPciCbComponentName,
             &gSdMmcPciCbComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "SdMmcPciCb: Driver installed, Status = %r\n", Status));

  return Status;
}

