/** @file
  Depthcharge-based SD/MMC PCI Host Controller Driver Header
  Register definitions and structures ported from Depthcharge

  Copyright (c) 2025, Matt DeVillier. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SD_MMC_PCI_DC_DXE_H_
#define _SD_MMC_PCI_DC_DXE_H_

#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DiskInfo.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Emmc.h>
#include <IndustryStandard/Sd.h>

extern EFI_DRIVER_BINDING_PROTOCOL   gSdMmcPciGliDriverBinding;

//
// SDHCI Register Definitions
//

#define SDHCI_BLOCK_SIZE            0x04
#define  SDHCI_MAKE_BLKSZ(dma, blksz) (((dma & 0x7) << 12) | (blksz & 0xFFF))

#define SDHCI_BLOCK_COUNT           0x06

#define SDHCI_ARGUMENT              0x08

#define SDHCI_TRANSFER_MODE         0x0C
#define  SDHCI_TRNS_DMA             0x01
#define  SDHCI_TRNS_BLK_CNT_EN      0x02
#define  SDHCI_TRNS_ACMD12          0x04
#define  SDHCI_TRNS_READ            0x10
#define  SDHCI_TRNS_MULTI           0x20

#define SDHCI_COMMAND               0x0E
#define  SDHCI_CMD_RESP_MASK        0x03
#define  SDHCI_CMD_CRC              0x08
#define  SDHCI_CMD_INDEX            0x10
#define  SDHCI_CMD_DATA             0x20

#define  SDHCI_CMD_RESP_NONE        0x00
#define  SDHCI_CMD_RESP_LONG        0x01
#define  SDHCI_CMD_RESP_SHORT       0x02
#define  SDHCI_CMD_RESP_SHORT_BUSY  0x03

#define SDHCI_MAKE_CMD(c, f) (((c & 0xff) << 8) | (f & 0xff))
#define SDHCI_GET_CMD(c) ((c>>8) & 0x3f)

#define SDHCI_RESPONSE              0x10

#define SDHCI_PRESENT_STATE         0x24
#define  SDHCI_CMD_INHIBIT          0x00000001
#define  SDHCI_DATA_INHIBIT         0x00000002
#define  SDHCI_CARD_PRESENT         0x00010000

#define SDHCI_HOST_CONTROL          0x28
#define  SDHCI_CTRL_4BITBUS         0x02
#define  SDHCI_CTRL_HISPD           0x04
#define  SDHCI_CTRL_DMA_MASK        0x18
#define   SDHCI_CTRL_ADMA32         0x10
#define  SDHCI_CTRL_8BITBUS         0x20

#define SDHCI_POWER_CONTROL         0x29
#define  SDHCI_POWER_ON             0x01
#define  SDHCI_POWER_180            0x0A
#define  SDHCI_POWER_300            0x0C
#define  SDHCI_POWER_330            0x0E

#define SDHCI_CLOCK_CONTROL         0x2C
#define  SDHCI_DIVIDER_SHIFT        8
#define  SDHCI_DIV_MASK             0xFF
#define  SDHCI_CLOCK_CARD_EN        0x0004
#define  SDHCI_CLOCK_INT_STABLE     0x0002
#define  SDHCI_CLOCK_INT_EN         0x0001

#define SDHCI_TIMEOUT_CONTROL       0x2E

#define SDHCI_SOFTWARE_RESET        0x2F
#define  SDHCI_RESET_ALL            0x01
#define  SDHCI_RESET_CMD            0x02
#define  SDHCI_RESET_DATA           0x04

#define SDHCI_INT_STATUS            0x30
#define SDHCI_INT_ENABLE            0x34
#define SDHCI_SIGNAL_ENABLE         0x38
#define  SDHCI_INT_RESPONSE         0x00000001
#define  SDHCI_INT_DATA_END         0x00000002
#define  SDHCI_INT_DMA_END          0x00000008
#define  SDHCI_INT_DATA_AVAIL       0x00000020
#define  SDHCI_INT_ERROR            0x00008000
#define  SDHCI_INT_TIMEOUT          0x00010000
#define  SDHCI_INT_CRC              0x00020000
#define  SDHCI_INT_END_BIT          0x00040000
#define  SDHCI_INT_INDEX            0x00080000
#define  SDHCI_INT_DATA_TIMEOUT     0x00100000
#define  SDHCI_INT_DATA_CRC         0x00200000
#define  SDHCI_INT_DATA_END_BIT     0x00400000
#define  SDHCI_INT_ADMA_ERROR       0x02000000

#define  SDHCI_INT_CMD_MASK         (SDHCI_INT_RESPONSE | SDHCI_INT_TIMEOUT | \
                                     SDHCI_INT_CRC | SDHCI_INT_END_BIT | SDHCI_INT_INDEX)
#define  SDHCI_INT_DATA_MASK        (SDHCI_INT_DATA_END | SDHCI_INT_DMA_END | \
                                     SDHCI_INT_DATA_AVAIL | \
                                     SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_DATA_CRC | \
                                     SDHCI_INT_DATA_END_BIT | SDHCI_INT_ADMA_ERROR)
#define SDHCI_INT_ALL_MASK          ((UINT32)-1)

#define SDHCI_HOST_CONTROL2         0x3E
#define  SDHCI_CTRL_UHS_MASK        0x0007
#define   SDHCI_CTRL_UHS_SDR25      0x0001
#define   SDHCI_CTRL_UHS_SDR50      0x0002
#define   SDHCI_CTRL_UHS_SDR104     0x0003
#define   SDHCI_CTRL_HS400          0x0005
#define  SDHCI_CTRL_180V_SIGNALING_ENABLE  0x0008
#define  SDHCI_CTRL_DRV_TYPE_SHIFT  4
#define  SDHCI_CTRL_DRV_TYPE_MASK   0x0030
#define   SDHCI_CTRL_DRV_TYPE_A     0x0010
#define  SDHCI_CTRL_EXEC_TUNING     0x0040
#define  SDHCI_CTRL_TUNED_CLK       0x0080

#define SDHCI_CAPABILITIES          0x40
#define  SDHCI_CLOCK_BASE_MASK      0x00003F00
#define  SDHCI_CLOCK_V3_BASE_MASK   0x0000FF00
#define  SDHCI_CLOCK_BASE_SHIFT     8
#define  SDHCI_CAN_DO_ADMA2         0x00080000
#define  SDHCI_SLOT_TYPE_MASK       0xC0000000  // bits 30:31
#define  SDHCI_SLOT_TYPE_SHIFT      30

#define SDHCI_CAPABILITIES_1        0x44
#define SDHCI_SUPPORT_SDR50         0x00000001
#define SDHCI_SUPPORT_SDR104        0x00000002
#define SDHCI_SUPPORT_DDR50         0x00000004
#define SDHCI_SUPPORT_HS400         0x80000000

#define SDHCI_ADMA_ERROR            0x54

#define SDHCI_ADMA_ADDRESS          0x58
#define SDHCI_ADMA_ADDRESS_HI       0x5C

//
// ADMA descriptor attributes (from Depthcharge)
//
#define SDHCI_ADMA_VALID            0x01
#define SDHCI_ADMA_END              0x02
#define SDHCI_ACT_TRAN              (2 << 4)
#define SDHCI_MAX_PER_DESCRIPTOR    0x10000  // 64KB max per descriptor

#define SDHCI_HOST_VERSION          0xFE

//
// Genesys Logic Vendor Registers (GL9750/GL9755/GL9763E)
//

// Common GL vendor config control (GL9750/GL9755)
#define GLI_CFG                     0x800  // Vendor config lock/unlock
#define  GLI_CFG_EN                 0x00000001  // Bit 0: Enable vendor config writes

// GL9750 vendor config registers
#define GL9750_CFG2                    0x848  // Vendor config register 2 (ASPM/power management)
#define  GL9750_CFG2_L0S_SUPPORT       0x00000040  // Bit 6: L0s ASPM support
#define  GL9750_CFG2_L1DLY_MASK        0x1FF80000  // Bits [28:19]: L1 entry delay
#define  GL9750_CFG2_L1DLY_SHIFT       19
#define  GL9750_CFG2_L1DLY_84US        0x54  // L1 delay value for 84 units

// GL9755 vendor config registers
#define GL9755_CFG2             0x48   // CFG2 at different offset than GL9750
#define  GL9755_CFG2_L1_LAT_MASK  0x00007038  // Bits [14:12] and [5:3]: L1 exit latency
#define  GL9755_CFG2_L1_LAT_64US  0x00006030  // 64us: (6 << 12) | (6 << 3)
#define  GL9755_CFG2_L0S_SUPPORT  0x00000040  // Bit 6: L0s support

#define GL9755_PECONF           0x44   // PECONF register for DMA configuration
#define  GL9755_PECONF_LFCLK_MASK  0x00007000  // Bits [14:12]: LFCLK
#define  GL9755_PECONF_DMACLK      0x20000000  // Bit 29: DMACLK

#define GL9755_LTR              0x5C   // LTR (Latency Tolerance Reporting)
#define  GL9755_LTR_SNOOP_VALUE    0x00000025     // Snoop latency value
#define  GL9755_LTR_SNOOP_SCALE    0x00000C00     // Snoop scale (bits [11:10])
#define  GL9755_LTR_NO_SNOOP_VALUE 0x00250000     // No-snoop latency value (bits [23:16])
#define  GL9755_LTR_NO_SNOOP_SCALE 0x0C000000     // No-snoop scale (bits [27:26])

#define GL9755_SERDES           0x70   // SerDes register for short circuit protection
#define  GL9755_SERDES_SCP_DIS     0x00080000  // Bit 19: SCP disable (clear to enable)

// GL9750 PLL/SSC registers for clock programming (MMIO)
#define GL9750_PLL              0x864
#define  GL9750_PLL_LDIV        0x000003FF  // Bits [9:0]: PLL loop divider
#define  GL9750_PLL_PDIV        0x00007000  // Bits [14:12]: PLL P divider
#define  GL9750_PLL_DIR         0x00008000  // Bit 15: PLL direction
#define  GL9750_PLLSSC_STEP     0x1F000000  // Bits [28:24]: SSC step
#define  GL9750_PLLSSC_EN       0x80000000  // Bit 31: SSC enable

#define GL9750_PLLSSC           0x86C
#define  GL9750_PLLSSC_PPM      0xFFFF0000  // Bits [31:16]: SSC PPM

#define GL9750_MISC             0x878
#define  GL9750_MISC_SSC_OFF    0x04000000  // Bit 26: SSC off

// GL9755 PLL/SSC registers for clock programming (PCI config space)
#define GL9755_PLL              0x64
#define  GL9755_PLL_LDIV        0x000003FF  // Bits [9:0]: PLL loop divider
#define  GL9755_PLL_PDIV        0x00007000  // Bits [14:12]: PLL P divider
#define  GL9755_PLL_DIR         0x00008000  // Bit 15: PLL direction
#define  GL9755_PLLSSC_STEP     0x1F000000  // Bits [28:24]: SSC step
#define  GL9755_PLLSSC_EN       0x80000000  // Bit 31: SSC enable

#define GL9755_PLLSSC           0x68
#define  GL9755_PLLSSC_PPM      0x0000FFFF  // Bits [15:0]: SSC PPM
#define  GL9755_PLLSSC_RTL      0x01000000  // Bit 24: RTL
#define  GL9755_PLLSSC_TRANS_PASS 0x08000000  // Bit 27: Trans pass
#define  GL9755_PLLSSC_RECV     0x30000000  // Bits [29:28]: Receive
#define  GL9755_PLLSSC_TRAN     0xC0000000  // Bits [31:30]: Transmit

#define GL9755_MISC             0x78
#define  GL9755_MISC_SSC_OFF    0x04000000  // Bit 26: SSC off

// GL9763E eMMC Control register for Enhanced Strobe
#define GL9763E_SDHC_EMMC_CTRL      0x52C
#define  GL9763E_ENHANCED_STROBE    0x00000100  // Bit 8

//
// PCI Configuration Space Registers (Common)
//
#define PCI_LINK_CONTROL_OFFSET     0x50  // PCI Link Control register
#define  PCI_ASPM_L0S_L1_MASK       0x0C  // Bits [3:2]: ASPM L0s and L1 support

//
// MMC Command Definitions
//

#define MMC_CMD_GO_IDLE_STATE       0
#define MMC_CMD_SEND_OP_COND        1
#define MMC_CMD_ALL_SEND_CID        2
#define MMC_CMD_SET_RELATIVE_ADDR   3
#define MMC_CMD_SWITCH              6
#define MMC_CMD_SELECT_CARD         7
#define MMC_CMD_SEND_EXT_CSD        8
#define MMC_CMD_SEND_STATUS         13
#define MMC_CMD_SET_BLOCKLEN        16
#define MMC_CMD_READ_SINGLE_BLOCK   17
#define MMC_CMD_READ_MULTIPLE_BLOCK 18
#define MMC_CMD_WRITE_SINGLE_BLOCK  24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK 25

//
// SD Command Definitions
//
#define SD_CMD_SEND_IF_COND         8   // CMD8
#define SD_CMD_SEND_CSD             9   // CMD9
#define SD_VOLTAGE_SWITCH           11  // CMD11 (UHS-I voltage switch)
#define SD_CMD_APP_CMD              55  // CMD55 (precedes ACMD)

// SD Application Commands (preceded by CMD55)
#define SD_ACMD_SET_BUS_WIDTH       6   // ACMD6
#define SD_ACMD_SEND_OP_COND        41  // ACMD41

// SD CMD8 argument
#define SD_CMD8_VHS_27_36           0x00000100  // 2.7-3.6V
#define SD_CMD8_CHECK_PATTERN       0x000000AA

// SD ACMD41 argument
#define SD_OCR_VDD_32_33            0x00200000
#define SD_OCR_HCS                  0x40000000  // High Capacity Support
#define SD_OCR_S18R                 0x01000000  // 1.8V switching request

// OCR (Operating Conditions Register) bits
#define OCR_BUSY                    0x80000000
#define OCR_HCS                     0x40000000  // High Capacity Support

// EXT_CSD fields (from Depthcharge)
#define EXT_CSD_BUS_WIDTH           183
#define EXT_CSD_STROBE_SUPPORT      184  // Enhanced Strobe support
#define EXT_CSD_HS_TIMING           185
#define EXT_CSD_CARD_TYPE           196
#define EXT_CSD_REV                 192
#define EXT_CSD_SEC_CNT             212
#define EXT_CSD_SIZE                512

// EXT_CSD BUS_WIDTH values
#define EXT_CSD_BUS_WIDTH_8         2

// EXT_CSD HS_TIMING values
#define EXT_CSD_TIMING_HS           1
#define EXT_CSD_TIMING_HS200        2
#define EXT_CSD_TIMING_HS400        3

// EXT_CSD BUS_WIDTH DDR values
#define EXT_CSD_DDR_BUS_WIDTH_8     6

// EXT_CSD BUS_WIDTH Enhanced Strobe bit (OR with DDR width)
#define EXT_CSD_BUS_WIDTH_STROBE    0x80

// EXT_CSD CARD_TYPE bits
#define EXT_CSD_CARD_TYPE_HS200_1_8V 0x10
#define EXT_CSD_CARD_TYPE_HS400_1_8V 0x40

// MMC_SWITCH mode
#define MMC_SWITCH_MODE_WRITE_BYTE  0x03

// Response types
#define MMC_RSP_PRESENT (1 << 0)
#define MMC_RSP_136     (1 << 1)
#define MMC_RSP_CRC     (1 << 2)
#define MMC_RSP_BUSY    (1 << 3)
#define MMC_RSP_OPCODE  (1 << 4)

#define MMC_RSP_NONE    (0)
#define MMC_RSP_R1      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)
#define MMC_RSP_R1B     (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE|MMC_RSP_BUSY)
#define MMC_RSP_R2      (MMC_RSP_PRESENT|MMC_RSP_136|MMC_RSP_CRC)
#define MMC_RSP_R3      (MMC_RSP_PRESENT)
#define MMC_RSP_R6      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)  // SD: Published RCA
#define MMC_RSP_R7      (MMC_RSP_PRESENT|MMC_RSP_CRC|MMC_RSP_OPCODE)  // SD: Card interface condition

//
// EDK2 Device Context
//

//
// ADMA descriptor structures (from Depthcharge)
//
#pragma pack(1)
typedef struct {
  UINT16  Attributes;
  UINT16  Length;
  UINT32  Address;
} SDHCI_ADMA_DESC;

typedef struct {
  UINT16  Attributes;
  UINT16  Length;
  UINT32  Address;
  UINT32  AddressHi;
} SDHCI_ADMA64_DESC;
#pragma pack()

#define SD_MMC_CB_SIGNATURE  SIGNATURE_32 ('S', 'D', 'D', 'C')

typedef struct {
  UINT32                      Signature;
  EFI_HANDLE                  ControllerHandle;
  EFI_HANDLE                  ChildHandle;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_BLOCK_IO_PROTOCOL       BlockIo;
  EFI_BLOCK_IO_MEDIA          BlockIoMedia;
  EFI_DISK_INFO_PROTOCOL      DiskInfo;

  // SDHCI controller info
  VOID                        *MmioBase;
  UINT32                      Capabilities;
  UINT32                      Version;
  UINT32                      ClockBase;
  UINT32                      CurrentClock;
  BOOLEAN                     Initialized;

  // eMMC/SD device info
  UINT32                      RelativeCardAddress;
  UINT32                      BlockSize;
  UINT64                      TotalBlocks;
  BOOLEAN                     HighCapacity;  // Block addressing vs byte addressing

  // Card Identification Data (structured, for DiskInfo)
  union {
    EMMC_CID                  EmmcCid;
    SD_CID                    SdCid;
  } Cid;

  // Vendor-specific flags
  BOOLEAN                     IsGL9763E;  // Genesys Logic GL9763E eMMC controller
  BOOLEAN                     IsGL9750;   // Genesys Logic GL9750 SD controller
  BOOLEAN                     IsGL9755;   // Genesys Logic GL9755 SD controller
  BOOLEAN                     IsEMMC;     // TRUE for eMMC, FALSE for SD card

  // Media change detection for removable media
  EFI_EVENT                   MediaChangeEvent;  // Timer for polling

  // ADMA support
  SDHCI_ADMA_DESC             *AdmaDescs;
  SDHCI_ADMA64_DESC           *Adma64Descs;
  UINT32                      AdmaDescCount;
  BOOLEAN                     UseDma64;
} SD_MMC_CB_DEVICE;

#define SD_MMC_CB_DEVICE_FROM_BLOCK_IO(a) \
  CR(a, SD_MMC_CB_DEVICE, BlockIo, SD_MMC_CB_SIGNATURE)

#define SD_MMC_CB_DEVICE_FROM_DISK_INFO(a) \
  CR(a, SD_MMC_CB_DEVICE, DiskInfo, SD_MMC_CB_SIGNATURE)

//
// Driver Binding Protocol
//

extern EFI_DRIVER_BINDING_PROTOCOL  gSdMmcPciGliDriverBinding;

//
// Function Prototypes
//

// From SdMmcPciGliDxe.c
EFI_STATUS
EFIAPI
SdMmcPciGliDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SdMmcPciGliDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
SdMmcPciGliDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

// From SdMmcPciGliSdhci.c
EFI_STATUS
SdhciReset (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT8             Mask
  );

EFI_STATUS
SdhciSetPower (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT8             PowerMode
  );

EFI_STATUS
SdhciSetClock (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT32            Clock
  );

EFI_STATUS
SdhciSendCommand (
  IN     SD_MMC_CB_DEVICE  *Device,
  IN     UINT32            Cmd,
  IN     UINT32            Arg,
  IN     UINT32            ResponseType,
  OUT    UINT32            *Response,
  IN     VOID              *DataBuffer    OPTIONAL,
  IN     UINT32            BlockSize      OPTIONAL,
  IN     UINT32            BlockCount     OPTIONAL,
  IN     BOOLEAN           IsRead         OPTIONAL
  );

EFI_STATUS
SdhciInit (
  IN SD_MMC_CB_DEVICE  *Device
  );

EFI_STATUS
SdhciSetupAdma (
  IN SD_MMC_CB_DEVICE  *Device,
  IN VOID              *DataBuffer,
  IN UINT32            TotalBytes,
  IN BOOLEAN           IsRead
  );

EFI_STATUS
SdhciCompleteAdma (
  IN SD_MMC_CB_DEVICE  *Device,
  IN UINT32            TimeoutMs
  );

// From SdMmcPciGliMmc.c
EFI_STATUS
MmcStartup (
  IN SD_MMC_CB_DEVICE  *Device
  );

// From SdMmcPciGliSd.c
EFI_STATUS
SdStartup (
  IN SD_MMC_CB_DEVICE  *Device
  );

// From SdMmcPciGliGli.c
EFI_STATUS
Gl975xInit (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9763eSetEnhancedStrobe (
  IN SD_MMC_CB_DEVICE  *Device,
  IN BOOLEAN           Enable
  );

VOID
Gl9750DisableSscPll (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9750SetSscPll205Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9750SetSscPll100Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9750SetSscPll50Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9750SetupTuning (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9755DisableSscPll (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9755SetSscPll205Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9755SetSscPll100Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  );

VOID
Gl9755SetSscPll50Mhz (
  IN SD_MMC_CB_DEVICE  *Device
  );

// From SdMmcPciGliBlockIo.c
EFI_STATUS
EFIAPI
SdMmcDcBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  );

EFI_STATUS
EFIAPI
SdMmcDcBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

EFI_STATUS
EFIAPI
SdMmcDcBlockIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  );

EFI_STATUS
EFIAPI
SdMmcDcBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  );

// Helper macros for register access
#define SdhciReadl(Device, Reg) \
  *(volatile UINT32 *)((UINTN)(Device)->MmioBase + (Reg))

#define SdhciWritel(Device, Reg, Val) \
  *(volatile UINT32 *)((UINTN)(Device)->MmioBase + (Reg)) = (Val)

#define SdhciReadw(Device, Reg) \
  *(volatile UINT16 *)((UINTN)(Device)->MmioBase + (Reg))

#define SdhciWritew(Device, Reg, Val) \
  *(volatile UINT16 *)((UINTN)(Device)->MmioBase + (Reg)) = (Val)

#define SdhciReadb(Device, Reg) \
  *(volatile UINT8 *)((UINTN)(Device)->MmioBase + (Reg))

#define SdhciWriteb(Device, Reg, Val) \
  *(volatile UINT8 *)((UINTN)(Device)->MmioBase + (Reg)) = (Val)

//
// DiskInfo Protocol functions
//
EFI_STATUS
EFIAPI
SdMmcDcDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *InquiryData,
  IN OUT UINT32                  *InquiryDataSize
  );

EFI_STATUS
EFIAPI
SdMmcDcDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *IdentifyData,
  IN OUT UINT32                  *IdentifyDataSize
  );

EFI_STATUS
EFIAPI
SdMmcDcDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *SenseData,
  IN OUT UINT32                  *SenseDataSize,
  OUT    UINT8                   *SenseDataNumber
  );

EFI_STATUS
EFIAPI
SdMmcDcDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL  *This,
  OUT UINT32                  *IdeChannel,
  OUT UINT32                  *IdeDevice
  );

#endif // _SD_MMC_PCI_DC_DXE_H_

