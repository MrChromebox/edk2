# SdMmcPciGliDxe - Genesys Logic SDHCI Driver for EDK2

**Gli** = Genesys Logic - Specialized driver for GL9750/GL9755/GL9763E controllers

## Purpose
Optimized SDHCI driver specifically for Genesys Logic SD/MMC controllers.
Based on Depthcharge's SDHCI stack with vendor-specific initialization from coreboot.

## Supported Controllers
- **GL9763E** (0xe763) - eMMC controller with HS400-ES support
- **GL9750** (0x9750) - SD card controller with UHS-I support
- **GL9755** (0x9755) - SD card controller with UHS-I and LTR support

## Features
✅ **Vendor register programming** - Proper ASPM/L1/LTR configuration
✅ **UHS-I support** - SDR50/SDR104 for SD cards (up to 208 MHz)
✅ **HS400-ES support** - Enhanced Strobe for eMMC (up to 400 MHz effective)
✅ **Hot-plug detection** - SD card insertion/removal support
✅ **Warm boot support** - Survives system resets
✅ **Release build stable** - Works with all optimization levels

## Architecture
This driver uses a **monolithic BlockIo approach** (like Depthcharge):
- Directly implements BlockIo protocol
- Simple polling-based command execution
- No PassThru protocol layer
- Minimal dependencies

This allows it to coexist with the generic `SdMmcPciHcDxe` driver:
- **SdMmcPciGliDxe** handles Genesys Logic controllers (returns UNSUPPORTED for others)
- **SdMmcPciHcDxe** handles all other SDHCI controllers

## Files
- `SdMmcPciGliDxe.inf` - Build configuration
- `SdMmcPciGliDxe.h` - Register definitions and structures
- `SdMmcPciGliDxe.c` - Driver entry point and PCI binding
- `SdMmcPciGliSdhci.c` - SDHCI core functions (from Depthcharge)
- `SdMmcPciGliMmc.c` - eMMC initialization and commands
- `SdMmcPciGliSd.c` - SD card initialization and UHS-I support
- `SdMmcPciGli.c` - Genesys Logic vendor-specific initialization
- `SdMmcPciGliBlockIo.c` - EDK2 BlockIo protocol implementation
- `SdMmcPciGliDiskInfo.c` - DiskInfo protocol implementation

## Key Improvements Over Generic Driver
1. **Vendor register unlock/lock** - Proper CFG_EN sequence for GL controllers
2. **Optimized for release builds** - Critical timing delays for optimized code paths
3. **Voltage switch validation** - Proper 1.8V signaling with recovery
4. **GL-specific quirks** - Each controller variant properly configured
5. **Clean, focused codebase** - Only necessary registers/functions, proper encapsulation

## Code Quality
- **Minimal surface area** - Header exposes only cross-module interfaces
- **Proper scoping** - Internal helper functions marked STATIC
- **No dead code** - Unused register/command definitions removed
- **Well-documented** - Clear comments from Depthcharge/coreboot sources

## Status
✅ **Production Ready** - Tested on Intel Alderlake and AMD Picasso platforms
