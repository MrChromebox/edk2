/** @file
 *
 *  Non-discoverable SDHCI driver for non-PCI controllers.
 *
 *  Copyright (c) 2026, Matt DeVillier <matt.devillier@gmail.com>
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#ifndef SDHCI_NONPCI_DXE_H_
#define SDHCI_NONPCI_DXE_H_

#include <Uefi.h>

#define SDHCI_CAPABILITIES      0x40
#define SDHCI_HOST_VERSION      0xFE
#define SDHCI_NONPCI_MMIO_SIZE  0x1000

typedef enum {
  RemovableSlot,
  EmbeddedSlot,
  SharedBusSlot,
  UnknownSlot
} EFI_SD_MMC_SLOT_TYPE;

typedef struct {
  UINT32    TimeoutFreq   : 6;
  UINT32    Reserved      : 1;
  UINT32    TimeoutUnit   : 1;
  UINT32    BaseClkFreq   : 8;
  UINT32    MaxBlkLen     : 2;
  UINT32    BusWidth8     : 1;
  UINT32    Adma2         : 1;
  UINT32    Reserved2     : 1;
  UINT32    HighSpeed     : 1;
  UINT32    Sdma          : 1;
  UINT32    SuspRes       : 1;
  UINT32    Voltage33     : 1;
  UINT32    Voltage30     : 1;
  UINT32    Voltage18     : 1;
  UINT32    SysBus64V4    : 1;
  UINT32    SysBus64V3    : 1;
  UINT32    AsyncInt      : 1;
  UINT32    SlotType      : 2;
  UINT32    Sdr50         : 1;
  UINT32    Sdr104        : 1;
  UINT32    Ddr50         : 1;
  UINT32    Reserved3     : 1;
  UINT32    DriverTypeA   : 1;
  UINT32    DriverTypeC   : 1;
  UINT32    DriverTypeD   : 1;
  UINT32    DriverType4   : 1;
  UINT32    TimerCount    : 4;
  UINT32    Reserved4     : 1;
  UINT32    TuningSDR50   : 1;
  UINT32    RetuningMod   : 2;
  UINT32    ClkMultiplier : 8;
  UINT32    Reserved5     : 7;
  UINT32    Hs400         : 1;
} SD_MMC_HC_SLOT_CAP;

#endif
