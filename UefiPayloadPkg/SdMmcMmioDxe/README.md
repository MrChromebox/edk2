# SdMmcMmioDxe - Universal MMIO SDHCI Driver

## Overview

`SdMmcMmioDxe` is a universal UEFI driver that provides support for any MMIO-mapped SDHCI controller (eMMC or SD card). Originally designed for AMD Picasso platforms, it has been extended to support any platform with MMIO-accessible SDHCI controllers through configurable Platform Configuration Database (PCD) values.

## Key Features

- ✅ **Universal MMIO Support**: Works with any SDHCI-compatible controller
- ✅ **Dual Controller Support**: Separate eMMC and SD controller registration
- ✅ **Configurable via PCDs**: MMIO base addresses set through build-time configuration
- ✅ **Independent Enable/Disable**: Each controller can be enabled/disabled separately
- ✅ **Backward Compatible**: Default values maintain AMD Picasso support
- ✅ **Non-Discoverable Device Framework**: Integrates with EDK2's standard infrastructure

## Architecture

### Components

```
SdMmcMmioDxe/
├── SdMmcMmioDxe.dec    # PCD definitions and package declaration
├── SdMmcMmioDxe.h      # Header with type definitions
├── SdMmcMmioDxe.c      # Main driver implementation
└── SdMmcMmioDxe.inf    # Build configuration
```

### Design

The driver registers SDHCI controllers as non-discoverable MMIO devices, allowing standard EDK2 SDHCI drivers (`SdDxe.inf` / `EmmcDxe.inf`) to manage them. It provides an override protocol to customize controller capabilities (e.g., slot type).

## Configuration

### PCD Definitions

| PCD Name | Type | Default | Description |
|----------|------|---------|-------------|
| `PcdEmmcMmioBaseAddress` | UINT64 | 0xFEDD5000 | eMMC controller MMIO base address |
| `PcdSdMmioBaseAddress` | UINT64 | 0x0 | SD controller MMIO base address |

**Token Space GUID**: `gSdMmcMmioDxeTokenSpaceGuid`

### Setting PCDs

#### Method 1: DSC File (Recommended)
Edit `UefiPayloadPkg.dsc`:

```dsc
[PcdsFixedAtBuild]
  # Universal MMIO SDHCI Driver PCDs
  gSdMmcMmioDxeTokenSpaceGuid.PcdEmmcMmioBaseAddress|0xFEDD5000
  gSdMmcMmioDxeTokenSpaceGuid.PcdSdMmioBaseAddress|0x0
```

#### Method 2: Build Command Line
```bash
build -p UefiPayloadPkg/UefiPayloadPkg.dsc -t GCC -a X64 \
  --pcd gSdMmcMmioDxeTokenSpaceGuid.PcdEmmcMmioBaseAddress=0xFEDD5000 \
  --pcd gSdMmcMmioDxeTokenSpaceGuid.PcdSdMmioBaseAddress=0xFEDD6000
```

#### Method 3: Coreboot Integration
In `payloads/external/edk2/Makefile`:

```makefile
ifeq ($(CONFIG_EDK2_PCO_MMIO_EMMC),y)
BUILD_STR += -D USE_PCO_MMIO_EMMC=TRUE
BUILD_STR += --pcd gSdMmcMmioDxeTokenSpaceGuid.PcdEmmcMmioBaseAddress=0xFEDD5000
BUILD_STR += --pcd gSdMmcMmioDxeTokenSpaceGuid.PcdSdMmioBaseAddress=0x0
endif
```

## Usage Examples

### Example 1: AMD Picasso (Default)
```dsc
gSdMmcMmioDxeTokenSpaceGuid.PcdEmmcMmioBaseAddress|0xFEDD5000
gSdMmcMmioDxeTokenSpaceGuid.PcdSdMmioBaseAddress|0x0
```
**Result**: eMMC enabled at 0xFEDD5000, SD disabled

### Example 2: Both Controllers
```dsc
gSdMmcMmioDxeTokenSpaceGuid.PcdEmmcMmioBaseAddress|0xFEDD5000
gSdMmcMmioDxeTokenSpaceGuid.PcdSdMmioBaseAddress|0xFEDD6000
```
**Result**: Both controllers enabled

### Example 3: SD Only
```dsc
gSdMmcMmioDxeTokenSpaceGuid.PcdEmmcMmioBaseAddress|0x0
gSdMmcMmioDxeTokenSpaceGuid.PcdSdMmioBaseAddress|0xFEDD6000
```
**Result**: eMMC disabled, SD enabled

### Example 4: Custom Platform
```dsc
gSdMmcMmioDxeTokenSpaceGuid.PcdEmmcMmioBaseAddress|0xFE000000
gSdMmcMmioDxeTokenSpaceGuid.PcdSdMmioBaseAddress|0xFE001000
```
**Result**: Both controllers at custom addresses

## Driver Behavior

### Initialization Sequence

1. Read PCD values for eMMC and SD MMIO base addresses
2. If eMMC PCD ≠ 0: Register eMMC controller as non-discoverable device
3. If SD PCD ≠ 0: Register SD controller as non-discoverable device
4. If at least one controller registered: Install SD/MMC override protocol
5. Return success

### Controller Registration

Each controller is registered with:
- **Device Type**: `NonDiscoverableDeviceTypeSdhci`
- **DMA Type**: `NonDiscoverableDeviceDmaTypeCoherent`
- **MMIO Size**: 4KB (0x1000 bytes)

### Debug Output

Enable debug output with `-D BUILD_TARGETS=DEBUG` or `CONFIG_EDK2_DEBUG=y`:

```
SdMmcMmioDxe: eMMC MMIO base = 0xfedd5000
SdMmcMmioDxe: SD MMIO base = 0x0
SdMmcMmioDxe: eMMC controller registered at 0xfedd5000
SdMmcMmioDxe: Registered 1 controller(s)
```

## Integration with Coreboot

### Kconfig Option
```kconfig
config EDK2_PCO_MMIO_EMMC
    bool "Enable MMIO eMMC driver for AMD Picasso boards in edk2"
    depends on EDK2_REPO_MRCHROMEBOX
    default n
```

### Build System
The driver is included when `USE_PCO_MMIO_EMMC=TRUE` is set in the EDK2 build.

## Platform Support

### Tested Platforms
- ✅ AMD Picasso (eMMC @ 0xFEDD5000)

### Potential Platforms
- AMD Stoney Ridge
- Any platform with MMIO-accessible SDHCI controllers

## Technical Details

### Memory Layout
- Each controller requires 4KB of MMIO address space
- Controllers must be aligned to 4KB boundaries
- No overlap between eMMC and SD MMIO regions

### Slot Type
By default, all controllers are registered as `EmbeddedSlot`. This can be customized via the override protocol.

### DMA Coherency
The driver assumes DMA-coherent memory (`NonDiscoverableDeviceDmaTypeCoherent`), suitable for most x86 platforms.

## Troubleshooting

### Controller Not Detected
1. Verify PCD value is non-zero and correct for your platform
2. Check MMIO base address is valid (not reserved by other devices)
3. Ensure `USE_PCO_MMIO_EMMC=TRUE` is set in build
4. Review debug output for registration failures

### Build Errors
1. Ensure all file references updated (DSC, FDF, INF)
2. Verify PCD token space GUID matches across DEC and DSC
3. Check PcdLib is included in LibraryClasses

### Runtime Errors
1. Verify MMIO addresses don't conflict with other devices
2. Check platform firmware initializes SDHCI controller before UEFI
3. Ensure controller is not disabled in platform configuration

## Differences from PCI-based Driver

| Aspect | SdMmcMmioDxe (MMIO) | SdMmcPciCbDxe (PCI) |
|--------|---------------------|---------------------|
| **Discovery** | Static (PCD-based) | Dynamic (PCI enumeration) |
| **Configuration** | Build-time PCDs | Runtime PCI detection |
| **Use Case** | Fixed MMIO controllers | PCI/PCIe SDHCI devices |
| **Flexibility** | Per-platform build | Universal binary |

## Migration from AmdPcoSdhciDxe

The driver was renamed from `AmdPcoSdhciDxe` to `SdMmcMmioDxe` to reflect its universal nature:

| Old | New |
|-----|-----|
| `AmdPcoSdhciDxe` | `SdMmcMmioDxe` |
| `gAmdPcoSdhciDxeTokenSpaceGuid` | `gSdMmcMmioDxeTokenSpaceGuid` |
| `AmdPcoSdhciDxeInitialize` | `SdMmcMmioDxeInitialize` |

All functionality remains identical.

## License

SPDX-License-Identifier: BSD-2-Clause-Patent

## Credits

- Patrick Wildt <patrick@blueri.se>
- Mario Bălănică <mariobalanica02@gmail.com>
- CoolStar <coolstarorganization@gmail.com>
