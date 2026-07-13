/** @file
  HOB describing non-PCI SDHCI controllers for the payload.

  Layout matches LB_TAG_SDHCI_NONPCI body (commonlib/bsd/sdhci_nonpci_info.h).
  Used for ACPI-mode PCI SDHCI (variable BAR) and fixed-MMIO SDHCI.

  Copyright (c) 2026, Matt DeVillier <matt.devillier@gmail.com>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

///
/// Coreboot non-PCI SDHCI info GUID
///
extern EFI_GUID  gUefiSdhciNonPciInfoGuid;

#define SDHCI_NONPCI_INFO_VERSION  1

#define SDHCI_NONPCI_SLOT_EMMC  0
#define SDHCI_NONPCI_SLOT_SDIO  1
#define SDHCI_NONPCI_SLOT_SD    2

#define SDHCI_NONPCI_FLAG_EMBEDDED  0x01

#define SDHCI_NONPCI_CTRL_MAX  8

typedef struct {
  UINT32    MmioBase;
  UINT32    MmioSize;
  UINT8     Slot;
  UINT8     Flags;
  UINT8     Reserved[2];
} SDHCI_NONPCI_CONTROL;

typedef struct {
  UINT32                Version;
  UINT32                Count;
  SDHCI_NONPCI_CONTROL  Ctrl[SDHCI_NONPCI_CTRL_MAX];
} SDHCI_NONPCI_INFO;
