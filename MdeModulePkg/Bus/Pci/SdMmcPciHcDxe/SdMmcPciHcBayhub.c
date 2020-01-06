/** @file
  Bayhub/O2 Micro eMMC controller quirk implementation.

  Copyright (c) 2020, 2025. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Pci.h>
#include "SdMmcPciHcDxe.h"
#include "SdMmcPciHcBayhub.h"

extern EFI_STATUS
EmmcSendTuningBlk (
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot,
  IN UINT8                          BusWidth
  );

//
// Cached Bayhub device ID (set by BhtHostPciSupport when binding).
//
STATIC UINT32  mBhtDeviceId = 0;

//
// BAR1 MMIO helpers (direct read/write of BAR1, not PCIR mapping).
//
STATIC
UINT32
BhtReadl (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset
  )
{
  UINT32  Value;

  PciIo->Mem.Read (PciIo, EfiPciIoWidthUint32, 1, Offset, 1, &Value);
  return Value;
}

STATIC
VOID
BhtWritel (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  PciIo->Mem.Write (PciIo, EfiPciIoWidthUint32, 1, Offset, 1, &Value);
}

BOOLEAN
BhtHostPciSupport (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  PCI_TYPE00  Pci;

  if (PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, 0, sizeof (Pci) / sizeof (UINT32), &Pci) != EFI_SUCCESS) {
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "SdMmcPciHc: check device %04x:%04x\n", Pci.Hdr.VendorId, Pci.Hdr.DeviceId));

  if (Pci.Hdr.VendorId != 0x1217) {
    return FALSE;
  }

  switch (Pci.Hdr.DeviceId) {
    case BHT_PCI_DEV_ID_SDS0:
    case BHT_PCI_DEV_ID_SDS1:
    case BHT_PCI_DEV_ID_FJ2:
    case BHT_PCI_DEV_ID_SB0:
    case BHT_PCI_DEV_ID_SB1:
      mBhtDeviceId = Pci.Hdr.DeviceId;
      return TRUE;
    default:
      break;
  }

  return FALSE;
}

UINT32
PciBhtRead32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset
  )
{
  UINT32  i;
  UINT32  Tmp[3] = { 0 };

  if ((mBhtDeviceId == BHT_PCI_DEV_ID_SDS0) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_SDS1) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_FJ2) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_SB0) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_SB1))
  {
    if ((mBhtDeviceId == BHT_PCI_DEV_ID_SDS0) ||
        (mBhtDeviceId == BHT_PCI_DEV_ID_FJ2) ||
        (mBhtDeviceId == BHT_PCI_DEV_ID_SB0) ||
        (mBhtDeviceId == BHT_PCI_DEV_ID_SB1))
    {
      i = 0;
      BhtWritel (PciIo, BHT_PCIRMappingEn, 0x40000000);
      while ((BhtReadl (PciIo, BHT_PCIRMappingEn) & 0x40000000) == 0) {
        if (i == 5) {
          goto RdDisMapping;
        }

        gBS->Stall (1000);
        i++;
        BhtWritel (PciIo, BHT_PCIRMappingEn, 0x40000000);
      }
    } else if (mBhtDeviceId == BHT_PCI_DEV_ID_SDS1) {
      i = 0;
      BhtWritel (PciIo, BHT_PCIRMappingEn, 0x20000000);
      while ((BhtReadl (PciIo, BHT_PCIRMappingEn) & 0x20000000) == 0) {
        if (i == 5) {
          goto RdDisMapping;
        }

        gBS->Stall (1000);
        i++;
        BhtWritel (PciIo, BHT_PCIRMappingEn, 0x20000000);
      }
    }

    i = 0;
    while (BhtReadl (PciIo, BHT_PCIRMappingCtl) & 0xc0000000) {
      if (i == 5) {
        goto RdDisMapping;
      }

      gBS->Stall (1000);
      i++;
    }

    Tmp[0] |= 0x40000000;
    Tmp[0] |= Offset;
    BhtWritel (PciIo, BHT_PCIRMappingCtl, Tmp[0]);

    i = 0;
    while (BhtReadl (PciIo, BHT_PCIRMappingCtl) & 0x40000000) {
      if (i == 5) {
        goto RdDisMapping;
      }

      gBS->Stall (1000);
      i++;
    }

    Tmp[1] = BhtReadl (PciIo, BHT_PCIRMappingVal);

RdDisMapping:
    BhtWritel (PciIo, BHT_PCIRMappingEn, 0x80000000);
    return Tmp[1];
  }

  return Tmp[0];
}

VOID
PciBhtWrite32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  UINT32  Tmp = 0;
  UINT32  i;

  if ((mBhtDeviceId == BHT_PCI_DEV_ID_SDS0) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_SDS1) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_FJ2) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_SB0) ||
      (mBhtDeviceId == BHT_PCI_DEV_ID_SB1))
  {
    if ((mBhtDeviceId == BHT_PCI_DEV_ID_SDS0) ||
        (mBhtDeviceId == BHT_PCI_DEV_ID_FJ2) ||
        (mBhtDeviceId == BHT_PCI_DEV_ID_SB0) ||
        (mBhtDeviceId == BHT_PCI_DEV_ID_SB1))
    {
      i = 0;
      BhtWritel (PciIo, BHT_PCIRMappingEn, 0x40000000);
      while ((BhtReadl (PciIo, BHT_PCIRMappingEn) & 0x40000000) == 0) {
        if (i == 5) {
          goto WrDisMapping;
        }

        gBS->Stall (1000);
        i++;
        BhtWritel (PciIo, BHT_PCIRMappingEn, 0x40000000);
      }
    } else if (mBhtDeviceId == BHT_PCI_DEV_ID_SDS1) {
      i = 0;
      BhtWritel (PciIo, BHT_PCIRMappingEn, 0x20000000);
      while ((BhtReadl (PciIo, BHT_PCIRMappingEn) & 0x20000000) == 0) {
        if (i == 5) {
          goto WrDisMapping;
        }

        gBS->Stall (1000);
        i++;
        BhtWritel (PciIo, BHT_PCIRMappingEn, 0x20000000);
      }
    }

    BhtWritel (PciIo, BHT_PCIRMappingVal, 0x80000000);
    BhtWritel (PciIo, BHT_PCIRMappingCtl, 0x800000D0);

    i = 0;
    while (BhtReadl (PciIo, BHT_PCIRMappingCtl) & 0xc0000000) {
      if (i == 5) {
        goto WrDisMapping;
      }

      gBS->Stall (1000);
      i++;
    }

    BhtWritel (PciIo, BHT_PCIRMappingVal, Value);
    Tmp |= 0x80000000;
    Tmp |= Offset;
    BhtWritel (PciIo, BHT_PCIRMappingCtl, Tmp);

    i = 0;
    while (BhtReadl (PciIo, BHT_PCIRMappingCtl) & 0x80000000) {
      if (i == 5) {
        goto WrDisMapping;
      }

      gBS->Stall (1000);
      i++;
    }

WrDisMapping:
    BhtWritel (PciIo, BHT_PCIRMappingVal, 0x80000001);
    BhtWritel (PciIo, BHT_PCIRMappingCtl, 0x800000D0);

    i = 0;
    while (BhtReadl (PciIo, BHT_PCIRMappingCtl) & 0xc0000000) {
      if (i == 5) {
        break;
      }

      gBS->Stall (1000);
      i++;
    }

    BhtWritel (PciIo, BHT_PCIRMappingEn, 0x80000000);
  }
}

VOID
PciBhtOr32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  UINT32  Arg;

  Arg = PciBhtRead32 (PciIo, Offset);
  PciBhtWrite32 (PciIo, Offset, Value | Arg);
}

VOID
PciBhtAnd32 (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT32               Value
  )
{
  UINT32  Arg;

  Arg = PciBhtRead32 (PciIo, Offset);
  PciBhtWrite32 (PciIo, Offset, Value & Arg);
}

EFI_STATUS
BayhubInitHost (
  IN SD_MMC_HC_PRIVATE_DATA  *Private,
  IN UINT8                   Slot
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;
  SD_MMC_HC_SLOT_CAP   Capability;
  UINT64               Cap;
  EFI_STATUS           Status;
  UINT32               Value32;
  UINT8                CardMode;
  UINT16               EmmcVar;
  UINTN                EmmcVarSize;

  PciIo      = Private->PciIo;
  Capability = Private->Capability[Slot];

#ifdef DISABLE_L1_2
  PciBhtAnd32 (PciIo, 0xd0, ~(BIT31));
  PciBhtAnd32 (PciIo, 0x90, ~(BIT1 | BIT0));

  Value32 = PciBhtRead32 (PciIo, 0xe0);
  Value32 &= ~(BIT31 | BIT30 | BIT29 | BIT28);
  Value32 |= (BIT29 | BIT28);
  PciBhtWrite32 (PciIo, 0xe0, Value32);

  Value32 = PciBhtRead32 (PciIo, 0xfc);
  Value32 &= ~(BIT19 | BIT18 | BIT17 | BIT16);
  Value32 |= BIT19;
  PciBhtWrite32 (PciIo, 0xfc, Value32);

  Value32 = PciBhtRead32 (PciIo, 0x3f4);
  Value32 &= ~(BIT3 | BIT2 | BIT1 | BIT0);
  Value32 |= (BIT3 | BIT1);
  PciBhtWrite32 (PciIo, 0x3f4, Value32);

  Value32 = PciBhtRead32 (PciIo, 0x248);
  Value32 &= ~(BIT3 | BIT2 | BIT1 | BIT0);
  Value32 |= (BIT3 | BIT1);
  PciBhtWrite32 (PciIo, 0x248, Value32);

  Value32 = PciBhtRead32 (PciIo, 0x90);
  Value32 &= ~(BIT1 | BIT0);
  Value32 |= BIT1;
  PciBhtWrite32 (PciIo, 0x90, Value32);
#endif

  PciBhtOr32 (PciIo, 0xEC, 0x3);
  PciBhtOr32 (PciIo, 0xD4, BIT6);
  PciBhtOr32 (PciIo, 0x308, BIT4);

  Value32 = PciBhtRead32 (PciIo, 0x304);
  Value32 &= 0x0000FFFF;
  Value32 |= 0x25100000;

  EmmcVarSize = sizeof (EmmcVar);
  Status      = gRT->GetVariable (
                      L"EMMC_CLK_DRIVER_STRENGTH",
                      &gEfiGenericVariableGuid,
                      NULL,
                      &EmmcVarSize,
                      &EmmcVar
                      );
  if (EFI_ERROR (Status)) {
    EmmcVar = HOST_CLK_DRIVE_STRENGTH;
  }

  Value32 &= 0xFFFFFF8F;
  Value32 |= ((EmmcVar & 0x7) << 4);

  EmmcVarSize = sizeof (EmmcVar);
  Status      = gRT->GetVariable (
                      L"EMMC_DATA_DRIVER_STRENGTH",
                      &gEfiGenericVariableGuid,
                      NULL,
                      &EmmcVarSize,
                      &EmmcVar
                      );
  if (EFI_ERROR (Status)) {
    EmmcVar = HOST_DAT_DRIVE_STRENGTH;
  }

  Value32 &= 0xFFFFFFF1;
  Value32 |= ((EmmcVar & 0x7) << 1);
  PciBhtWrite32 (PciIo, 0x304, Value32);
  PciBhtOr32 (PciIo, 0x3E4, BIT22);

  EmmcVarSize = sizeof (CardMode);
  Status      = gRT->GetVariable (
                      L"EMMC_FORCE_CARD_MODE",
                      &gEfiGenericVariableGuid,
                      NULL,
                      &EmmcVarSize,
                      &CardMode
                      );
  if (EFI_ERROR (Status) || (CardMode > 2)) {
    CardMode = 0;
  }

  if (CardMode == 1) {
    EmmcVarSize = sizeof (EmmcVar);
    Status      = gRT->GetVariable (
                        L"EMMC_HS100_ALLPASS_PHASE",
                        &gEfiGenericVariableGuid,
                        NULL,
                        &EmmcVarSize,
                        &EmmcVar
                        );
    if (EFI_ERROR (Status) || (EmmcVar > 10)) {
      EmmcVar = HS100_ALLPASS_PHASE;
    }
  } else if (CardMode == 2) {
    EmmcVarSize = sizeof (EmmcVar);
    Status      = gRT->GetVariable (
                        L"EMMC_HS200_ALLPASS_PHASE",
                        &gEfiGenericVariableGuid,
                        NULL,
                        &EmmcVarSize,
                        &EmmcVar
                        );
    if (EFI_ERROR (Status) || (EmmcVar > 10)) {
      EmmcVar = HS200_ALLPASS_PHASE;
    }
  } else {
    EmmcVar = 0;
  }

  Value32 = 0x21000033 | (EmmcVar << 20);
  PciBhtWrite32 (PciIo, 0x300, Value32);

  Value32 = BIT0;
  Status  = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_CLOCK_CTRL, sizeof (Value32), &Value32);

  Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
  Value32 |= BIT12;
  Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, FALSE, sizeof (Value32), &Value32);
  gBS->Stall (1);

  Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
  Value32 &= ~BIT12;
  Value32 |= BIT18;
  Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, FALSE, sizeof (Value32), &Value32);

  Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
  while (!(Value32 & BIT14)) {
    gBS->Stall (100);
    Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
  }

  if (Value32 & BIT18) {
    while (1) {
      Status = SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_PRESENT_STATE, TRUE, sizeof (Value32), &Value32);
      if (((Value32 >> 16) & 0x01) == ((Value32 >> 18) & 0x01)) {
        break;
      }
    }

    Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, TRUE, sizeof (Value32), &Value32);
    Value32 &= ~BIT18;
    Status = SdMmcHcRwMmio (PciIo, Slot, 0x1CC, FALSE, sizeof (Value32), &Value32);
  }

  Status = SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_CAP, TRUE, sizeof (Cap), &Cap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (&Capability, &Cap, sizeof (Capability));
  Status = SdMmcHcInitPowerVoltage (PciIo, Slot, Capability);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BayhubInitPowerVoltageExtra (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Slot
  )
{
  EFI_STATUS  Status;
  UINT8       HostCtrl2;

  HostCtrl2 = (UINT8)BIT3;
  Status   = SdMmcHcOrMmio (PciIo, Slot, SD_MMC_HC_HOST_CTRL2, sizeof (HostCtrl2), &HostCtrl2);
  gBS->Stall (5000);
  return Status;
}

VOID
BayhubEmmcIdentificationPre (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  UINT32  Val32;

  Val32 = PciBhtRead32 (PciIo, BHT_CLK_SRC_SWITCH);
  Val32 &= ~BHT_PCR_SD_SEL_DLL;
  PciBhtWrite32 (PciIo, BHT_CLK_SRC_SWITCH, Val32);
}

EFI_STATUS
BayhubEmmcTuningStart (
  IN EFI_PCI_IO_PROTOCOL            *PciIo,
  IN EFI_SD_MMC_PASS_THRU_PROTOCOL  *PassThru,
  IN UINT8                          Slot
  )
{
  EFI_STATUS  Status;
  UINT8       HostCtrl2;

  Status = SdMmcHcSetBusWidth (PciIo, Slot, 4);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  HostCtrl2 = (UINT8)(~0x10);
  Status    = SdMmcHcAndMmio (PciIo, Slot, 0x110, sizeof (HostCtrl2), &HostCtrl2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EmmcSendTuningBlk (PassThru, Slot, 4);
}

EFI_STATUS
BayhubEmmcTuningRestoreBusWidth (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Slot,
  IN UINT8                BusWidth
  )
{
  return SdMmcHcSetBusWidth (PciIo, Slot, BusWidth);
}

EFI_STATUS
BayhubEmmcSwitchToHS200Post (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                Slot
  )
{
  EFI_STATUS  Status;
  UINT32      Value32;

  Status = SdMmcHcWaitMmioSet (
             PciIo,
             Slot,
             0x1cc,
             sizeof (Value32),
             BIT14,
             BIT14,
             SD_MMC_HC_GENERIC_TIMEOUT
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  while (1) {
    Status = SdMmcHcRwMmio (PciIo, Slot, SD_MMC_HC_PRESENT_STATE, TRUE, sizeof (Value32), &Value32);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (((Value32 >> 18) & 0x01) == ((Value32 >> 16) & 0x01)) {
      break;
    }
  }

  Status = SdMmcHcWaitMmioSet (
             PciIo,
             Slot,
             0x1cc,
             sizeof (Value32),
             BIT11,
             BIT11,
             SD_MMC_CLOCK_STABLE_TIMEOUT
             );
  return Status;
}

VOID
BayhubEmmcSetBusModeFromVariable (
  IN OUT SD_MMC_BUS_SETTINGS  *BusMode
  )
{
  EFI_STATUS  Status;
  UINT8       EmmcVar;
  UINTN       EmmcVarSize;

  EmmcVarSize = sizeof (EmmcVar);
  Status      = gRT->GetVariable (
                      L"EMMC_FORCE_CARD_MODE",
                      &gEfiGenericVariableGuid,
                      NULL,
                      &EmmcVarSize,
                      &EmmcVar
                      );
  if (!EFI_ERROR (Status) && (EmmcVar <= 2)) {
    if (EmmcVar == 2) {
      BusMode->BusTiming  = SdMmcMmcHs200;
      BusMode->ClockFreq  = 200;
    } else if (EmmcVar == 1) {
      BusMode->BusTiming  = SdMmcMmcHs200;
      BusMode->ClockFreq  = 100;
    } else {
      BusMode->BusTiming  = SdMmcMmcHsSdr;
      BusMode->ClockFreq  = 52;
    }
  } else {
    BusMode->BusTiming  = SdMmcMmcHsSdr;
    BusMode->ClockFreq  = 52;
  }
}

EFI_STATUS
BayhubEmmcApplyHs100Phase (
  IN     EFI_PCI_IO_PROTOCOL  *PciIo,
  IN     UINT8                Slot,
  IN OUT SD_MMC_BUS_SETTINGS  *BusMode
  )
{
  EFI_STATUS  Status;
  UINT32      Val32;
  UINT16      EmmcVar;
  UINTN       EmmcVarSize;

  Val32 = PciBhtRead32 (PciIo, 0x300);
  Val32 &= 0xFF0FFFFF;
  EmmcVarSize = sizeof (EmmcVar);
  Status      = gRT->GetVariable (
                      L"EMMC_HS100_ALLPASS_PHASE",
                      &gEfiGenericVariableGuid,
                      NULL,
                      &EmmcVarSize,
                      &EmmcVar
                      );
  if (EFI_ERROR (Status) || (EmmcVar > 10)) {
    EmmcVar = HS100_ALLPASS_PHASE;
  }

  Val32 |= (EmmcVar << 20);
  PciBhtWrite32 (PciIo, 0x300, 0x21000033 | Val32);
  BusMode->ClockFreq = 100;

  SdMmcHcRwMmio (PciIo, Slot, 0x3C, TRUE, sizeof (Val32), &Val32);
  Val32 &= ~BIT22;
  SdMmcHcRwMmio (PciIo, Slot, 0x3C, FALSE, sizeof (Val32), &Val32);
  Val32 = (BIT26 | BIT25);
  SdMmcHcOrMmio (PciIo, Slot, 0x2C, sizeof (Val32), &Val32);

  return EFI_SUCCESS;
}

VOID
BayhubPostInitReads (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  UINT32  Value;
  UINT16  IntStatus;

  SdMmcHcRwMmio (PciIo, 0, 0x110, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, 0x114, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, 0x1a8, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, 0x1ac, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, 0x1B0, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, 0x1CC, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, 0x040, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, SD_MMC_HC_PRESENT_STATE, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, SD_MMC_HC_HOST_CTRL1, TRUE, sizeof (IntStatus), &IntStatus);
  SdMmcHcRwMmio (PciIo, 0, SD_MMC_HC_CLOCK_CTRL, TRUE, sizeof (IntStatus), &IntStatus);
  SdMmcHcRwMmio (PciIo, 0, SD_MMC_HC_TIMEOUT_CTRL, TRUE, sizeof (IntStatus), &IntStatus);
  SdMmcHcRwMmio (PciIo, 0, SD_MMC_HC_NOR_INT_STS, TRUE, sizeof (Value), &Value);
  SdMmcHcRwMmio (PciIo, 0, SD_MMC_HC_HOST_CTRL2, TRUE, sizeof (IntStatus), &IntStatus);
}
