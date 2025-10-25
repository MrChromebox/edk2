# SdMmcPciCbDxe - Universal Chromebook SDHCI Driver for EDK2

**Cb** = Chromebook/Coreboot - Based on Depthcharge's proven SDHCI stack

## Purpose
Complete port of Depthcharge's SDHCI/MMC driver stack to EDK2.
Uses the proven simple polling approach from Depthcharge that works with GL9763E.

## Status
🚧 IN PROGRESS - Replacing the problematic generic SdMmcPciHcDxe driver

## Files
- SdMmcPciDcDxe.inf - Build configuration
- SdMmcPciDcDxe.h - Register definitions (from Depthcharge sdhci.h)
- SdMmcPciDcDxe.c - Driver entry point and PCI binding
- SdMmcPciDcSdhci.c - SDHCI core functions (from Depthcharge sdhci.c)
- SdMmcPciDcMmc.c - MMC/eMMC protocol (from Depthcharge mmc.c)
- SdMmcPciDcGli.c - GL9763E specific code (from Depthcharge sdhci_gli.c)
- SdMmcPciDcBlockIo.c - EDK2 BlockIo protocol implementation

## Advantages
1. ✅ Proven code - works in Depthcharge
2. ✅ Simple polling - no complex interrupt handling
3. ✅ GL9763E tested - known to work with this exact hardware
4. ✅ Clean slate - no legacy baggage from generic driver

## Next Steps
1. Complete basic driver structure
2. Port core SDHCI functions
3. Port eMMC initialization
4. Implement BlockIo for EDK2
5. Test and iterate

