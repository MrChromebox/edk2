# UFS Platform Implementation Comparison and Requirements

## Executive Summary

This document compares the UFS (Universal Flash Storage) implementation between:
- **depthcharge** (`/home/mattd/dev/depthcharge/src/drivers/storage/ufs*.c`)
- **EDK2 MdeModulePkg** UFS drivers
- **Existing binary** `UfsPlatform.efi` (needs to be replaced)

The goal is to identify functionality missing from EDK2 that needs to be implemented in a new **UfsPlatformDxe** driver.

---

## Current State

### EDK2 UFS Architecture

EDK2 has a layered UFS architecture:

1. **UfsPciHcDxe** - PCI Host Controller driver
   - Manages PCI device enumeration
   - Provides `EDKII_UFS_HOST_CONTROLLER_PROTOCOL`
   - Handles PCI MMIO access, DMA operations

2. **UfsPassThruDxe** - Main UFS protocol stack
   - Implements `EFI_EXT_SCSI_PASS_THRU_PROTOCOL`
   - Handles UFS initialization sequence
   - Manages UPIU (UFS Protocol Information Unit) transactions
   - Consumes optional `EDKII_UFS_HC_PLATFORM_PROTOCOL`

3. **UfsBlockIoPei** - PEI phase support (not relevant for DXE)

4. **UfsPlatform.efi** (BINARY) - Platform-specific customization
   - Currently a binary blob in `UefiPayloadPkg/UfsPlatform/`
   - Referenced in `UefiPayloadPkg/UefiPayloadPkg.fdf` lines 379-380
   - **NEEDS TO BE REPLACED WITH SOURCE-BASED DRIVER**

---

## Platform Protocol Definition

### EDKII_UFS_HC_PLATFORM_PROTOCOL (EDK2)

Defined in `MdeModulePkg/Include/Protocol/UfsHostControllerPlatform.h`:

```c
struct _EDKII_UFS_HC_PLATFORM_PROTOCOL {
  UINT32                                    Version;              // Protocol version
  EDKII_UFS_HC_PLATFORM_OVERRIDE_HC_INFO    OverrideHcInfo;      // Override HC capabilities
  EDKII_UFS_HC_PLATFORM_CALLBACK            Callback;            // Phase-based callbacks
  EDKII_UFS_CARD_REF_CLK_FREQ_ATTRIBUTE     RefClkFreq;          // Reference clock frequency
  BOOLEAN                                   SkipHceReenable;     // Skip HC enable reset
  BOOLEAN                                   SkipLinkStartup;     // Skip link startup
};
```

#### Callback Phases (EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE)

1. **EdkiiUfsHcPreHce** - Before Host Controller Enable
   - Called before setting HCE bit
   - Platform can perform pre-initialization

2. **EdkiiUfsHcPostHce** - After Host Controller Enable
   - Called after HCE bit is set
   - Platform can configure controller

3. **EdkiiUfsHcPreLinkStartup** - Before UFS Link Startup
   - Called before DME_LINKSTARTUP command
   - **CRITICAL: Intel platforms need to disable PA_LOCAL_TX_LCC_ENABLE here**

4. **EdkiiUfsHcPostLinkStartup** - After Link Startup
   - Called after link is established
   - Platform can perform post-link configuration

#### Driver Interface (EDKII_UFS_HC_DRIVER_INTERFACE)

Provided to callbacks:
```c
struct _EDKII_UFS_HC_DRIVER_INTERFACE {
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL    *UfsHcProtocol;  // MMIO access
  EDKII_UFS_EXEC_UIC_COMMAND            UfsExecUicCommand; // Execute UIC commands
};
```

This allows platform code to:
- Read/write UFS HC registers
- Execute UIC (UniPro Interconnect Configuration) commands to set PHY attributes

---

## Depthcharge Implementation Analysis

### Hook Architecture

Depthcharge uses a simpler callback mechanism defined in `ufs.h`:

```c
typedef enum {
  UFS_OP_PRE_HCE,           // Before HC enable
  UFS_OP_PRE_LINK_STARTUP,  // Before link startup
  UFS_OP_POST_LINK_STARTUP, // After link startup
  UFS_OP_PRE_GEAR_SWITCH,   // Before gear change (performance mode)
  UFS_OP_POST_GEAR_SWITCH,  // After gear change
} UfsHookOp;

typedef int (*UFSHookFn)(UfsCtlr *ufs, UfsHookOp op, void *data);
```

### Intel-Specific Implementation (ufs_intel.c)

The Intel platform hook performs critical platform-specific configuration:

```c
static int intel_ufs_hook_fn(UfsCtlr *ufs, UfsHookOp op, void *data)
{
  switch (op) {
  case UFS_OP_PRE_LINK_STARTUP:
    // CRITICAL: Disable PA_LOCAL_TX_LCC_ENABLE before link startup
    return ufs_dme_set(ufs, PA_LOCAL_TX_LCC_ENABLE, 0);
  default:
    break;
  };
  return 0;
}
```

**Why this matters:**
- `PA_LOCAL_TX_LCC_ENABLE` (UniPro attribute 0x155E) controls Line Coding Control
- Intel platforms require this to be disabled (0) for proper link initialization
- Without this, UFS link may fail to establish or be unstable

### Other Platform Configuration

Intel platform also sets:
```c
intel_ufs->ufs.update_refclkfreq = true;
intel_ufs->ufs.refclkfreq = UFS_REFCLKFREQ_19_2;  // 19.2 MHz
pci_set_bus_master(dev);  // Enable PCI bus mastering
```

---

## Functionality Gap Analysis

### ✅ Already Supported in EDK2 Platform Protocol

| Feature | EDK2 Support | Notes |
|---------|-------------|-------|
| Pre-HCE callback | ✅ EdkiiUfsHcPreHce | Equivalent to UFS_OP_PRE_HCE |
| Post-HCE callback | ✅ EdkiiUfsHcPostHce | Not in depthcharge |
| Pre-LinkStartup callback | ✅ EdkiiUfsHcPreLinkStartup | **THIS IS WHERE PA_LOCAL_TX_LCC_ENABLE MUST BE SET** |
| Post-LinkStartup callback | ✅ EdkiiUfsHcPostLinkStartup | Equivalent to UFS_OP_POST_LINK_STARTUP |
| Reference clock config | ✅ RefClkFreq field | Maps to depthcharge's refclkfreq |
| UIC command execution | ✅ UfsExecUicCommand | Can set PA_* attributes |
| Override HC capabilities | ✅ OverrideHcInfo | Not needed by Intel |
| Skip HCE re-enable | ✅ SkipHceReenable | Not needed by Intel |
| Skip link startup | ✅ SkipLinkStartup | Not needed by Intel |

### ⚠️ Missing in EDK2 Platform Protocol

| Feature | Status | Workaround |
|---------|--------|-----------|
| Pre-gear-switch callback | ❌ Not in protocol | Not critical for basic operation |
| Post-gear-switch callback | ❌ Not in protocol | Not critical for basic operation |

**Note:** Gear switching (performance mode changes) callbacks are not in the EDK2 platform protocol but are not strictly necessary for Intel platforms. The UFS driver handles gear switching automatically, and platform-specific configuration can be done during earlier phases if needed.

### 🔍 Platform-Specific Attributes

The following UniPro/UFS attributes need platform configuration:

| Attribute | Address | Description | Intel Value |
|-----------|---------|-------------|-------------|
| PA_LOCAL_TX_LCC_ENABLE | 0x155E | Line Coding Control | 0 (disabled) |
| bRefClkFreq | Attribute 0x0A | Reference Clock Freq | 0 (19.2 MHz) |

---

## What Needs to be Implemented: UfsPlatformDxe Driver

### File Structure

```
UefiPayloadPkg/UfsPlatformDxe/
├── UfsPlatformDxe.inf         # Driver build file
├── UfsPlatformDxe.c           # Main driver implementation
└── UfsPlatformDxe.h           # Header file
```

### Required Protocol Implementation

The new driver must implement `EDKII_UFS_HC_PLATFORM_PROTOCOL`:

```c
GLOBAL_REMOVE_IF_UNREFERENCED
EDKII_UFS_HC_PLATFORM_PROTOCOL mUfsPlatform = {
  .Version           = EDKII_UFS_HC_PLATFORM_PROTOCOL_VERSION,  // 3
  .OverrideHcInfo    = NULL,  // Not needed for Intel
  .Callback          = UfsPlatformCallback,
  .RefClkFreq        = EdkiiUfsCardRefClkFreq19p2Mhz,  // 19.2 MHz
  .SkipHceReenable   = FALSE,
  .SkipLinkStartup   = FALSE,
};
```

### Critical Callback Implementation

```c
EFI_STATUS
EFIAPI
UfsPlatformCallback (
  IN     EFI_HANDLE                            ControllerHandle,
  IN     EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE  CallbackPhase,
  IN OUT VOID                                  *CallbackData
  )
{
  EFI_STATUS                      Status;
  EDKII_UFS_HC_DRIVER_INTERFACE   *UfsHcDriver;
  EDKII_UIC_COMMAND               UicCommand;

  if (CallbackData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UfsHcDriver = (EDKII_UFS_HC_DRIVER_INTERFACE *)CallbackData;

  switch (CallbackPhase) {
    case EdkiiUfsHcPreLinkStartup:
      //
      // CRITICAL: Disable PA_LOCAL_TX_LCC_ENABLE for Intel platforms
      // This must be done before DME_LINKSTARTUP command
      //
      UicCommand.Opcode = 0x02;  // DME_SET
      UicCommand.Arg1   = 0x155E0000;  // PA_LOCAL_TX_LCC_ENABLE (0x155E), GenSelectorIndex = 0
      UicCommand.Arg2   = 0x00000000;  // AttrSetType = Normal (0)
      UicCommand.Arg3   = 0x00000000;  // Value = 0 (disable)

      Status = UfsHcDriver->UfsExecUicCommand (UfsHcDriver, &UicCommand);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to disable PA_LOCAL_TX_LCC_ENABLE: %r\n", Status));
        return Status;
      }
      break;

    case EdkiiUfsHcPreHce:
    case EdkiiUfsHcPostHce:
    case EdkiiUfsHcPostLinkStartup:
      // No additional configuration needed for Intel platforms
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
```

### Driver Entry Point

```c
EFI_STATUS
EFIAPI
UfsPlatformDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install EDKII_UFS_HC_PLATFORM_PROTOCOL
  // This will be located by UfsPassThruDxe during initialization
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEdkiiUfsHcPlatformProtocolGuid,
                  &mUfsPlatform,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to install UFS Platform Protocol: %r\n", Status));
  }

  return Status;
}
```

### .inf File Content

```ini
[Defines]
  INF_VERSION                    = 0x00010005
  BASE_NAME                      = UfsPlatformDxe
  FILE_GUID                      = 9F3C6294-BF19-4755-B1D3-D5E2B5A4E5A5
  MODULE_TYPE                    = DXE_DRIVER
  VERSION_STRING                 = 1.0
  ENTRY_POINT                    = UfsPlatformDxeInitialize

[Sources]
  UfsPlatformDxe.c
  UfsPlatformDxe.h

[Packages]
  MdePkg/MdePkg.dec
  MdeModulePkg/MdeModulePkg.dec

[LibraryClasses]
  UefiDriverEntryPoint
  UefiBootServicesTableLib
  DebugLib
  BaseLib

[Protocols]
  gEdkiiUfsHcPlatformProtocolGuid    ## PRODUCES

[Depex]
  TRUE
```

---

## Integration Steps

### 1. Remove Binary UfsPlatform.efi

Edit `UefiPayloadPkg/UefiPayloadPkg.fdf`:
```diff
- FILE DRIVER = ... {
-   SECTION PE32 = UefiPayloadPkg/UfsPlatform/UfsPlatform.efi
-   SECTION UI = "UfsPlatform"
- }
```

### 2. Add New UfsPlatformDxe Driver

Edit `UefiPayloadPkg/UefiPayloadPkg.dsc`:
```ini
[Components]
  # ... existing components ...
  UefiPayloadPkg/UfsPlatformDxe/UfsPlatformDxe.inf
```

Edit `UefiPayloadPkg/UefiPayloadPkg.fdf`:
```ini
INF UefiPayloadPkg/UfsPlatformDxe/UfsPlatformDxe.inf
```

### 3. Delete Binary Directory

```bash
rm -rf UefiPayloadPkg/UfsPlatform/
```

---

## Testing Checklist

### Functional Tests

- [ ] UFS device detection succeeds
- [ ] Link startup completes without errors
- [ ] Device enumeration works
- [ ] Read/write operations perform correctly
- [ ] Boot from UFS device succeeds
- [ ] Reference clock frequency is set to 19.2 MHz
- [ ] PA_LOCAL_TX_LCC_ENABLE is disabled before link startup

### Debug Points

Key debug locations in EDK2 UfsPassThruDxe:

1. **UfsPassThruDriverBindingStart** (`UfsPassThru.c:897-902`)
   - Locates platform protocol
   - Add debug: `DEBUG ((DEBUG_INFO, "UfsHcPlatform protocol located: %p\n", mUfsHcPlatform));`

2. **UfsEnableHostController** (`UfsPassThruHci.c:1857-1863`)
   - Calls EdkiiUfsHcPreHce
   - Add debug to confirm callback execution

3. **UfsDeviceDetection** (`UfsPassThruHci.c:1944-1950`)
   - Calls EdkiiUfsHcPreLinkStartup
   - **CRITICAL: Verify PA_LOCAL_TX_LCC_ENABLE is set here**
   - Add debug: `DEBUG ((DEBUG_INFO, "PreLinkStartup callback returned: %r\n", Status));`

4. **Reference Clock Setup** (`UfsPassThru.c:936-958`)
   - Configures bRefClkFreq attribute
   - Verify 19.2 MHz is being set

---

## UniPro/UFS Attribute Reference

### PA Layer (Physical Adapter) Attributes

| Attribute Name | Address | Description | Type |
|----------------|---------|-------------|------|
| PA_LOCAL_TX_LCC_ENABLE | 0x155E | Line Coding Control Enable | RW |
| PA_ACTIVETXDATALANES | 0x1560 | Active TX data lanes | RW |
| PA_ACTIVERXDATALANES | 0x1580 | Active RX data lanes | RW |
| PA_CONNECTEDTXDATALANES | 0x1561 | Connected TX lanes | RO |
| PA_CONNECTEDRXDATALANES | 0x1581 | Connected RX lanes | RO |

### Device Attributes

| Attribute Name | IDN | Description | Type |
|----------------|-----|-------------|------|
| bRefClkFreq | 0x0A | Reference Clock Frequency | RW |

#### bRefClkFreq Values

| Value | Frequency |
|-------|-----------|
| 0 | 19.2 MHz |
| 1 | 26 MHz |
| 2 | 38.4 MHz |
| 3 | 52 MHz |

---

## Additional Considerations

### Performance Tuning (Future Enhancement)

If gear switching hooks are needed in the future, the protocol would need to be extended:

```c
typedef enum {
  EdkiiUfsHcPreHce,
  EdkiiUfsHcPostHce,
  EdkiiUfsHcPreLinkStartup,
  EdkiiUfsHcPostLinkStartup,
  EdkiiUfsHcPreGearSwitch,      // NEW
  EdkiiUfsHcPostGearSwitch      // NEW
} EDKII_UFS_HC_PLATFORM_CALLBACK_PHASE;
```

However, this is not required for basic Intel platform support.

### Multi-Platform Support

The driver could be enhanced to:
1. Detect PCI Device/Vendor IDs
2. Apply different configurations per platform
3. Read platform-specific settings from PCDs or ACPI

Current implementation is Intel-specific (19.2 MHz clock, PA_LOCAL_TX_LCC_ENABLE disabled).

---

## Summary of Required Changes

### New Files to Create

1. **UefiPayloadPkg/UfsPlatformDxe/UfsPlatformDxe.c**
   - Main driver implementation
   - Callback function with PA_LOCAL_TX_LCC_ENABLE configuration
   - Driver entry point

2. **UefiPayloadPkg/UfsPlatformDxe/UfsPlatformDxe.h**
   - Header with includes and definitions

3. **UefiPayloadPkg/UfsPlatformDxe/UfsPlatformDxe.inf**
   - Build configuration

### Files to Modify

1. **UefiPayloadPkg/UefiPayloadPkg.dsc**
   - Add UfsPlatformDxe.inf to [Components]

2. **UefiPayloadPkg/UefiPayloadPkg.fdf**
   - Remove binary UfsPlatform.efi reference
   - Add INF UefiPayloadPkg/UfsPlatformDxe/UfsPlatformDxe.inf

### Files/Directories to Delete

1. **UefiPayloadPkg/UfsPlatform/** (entire directory)
   - Contains binary UfsPlatform.efi

---

## Conclusion

The EDK2 `EDKII_UFS_HC_PLATFORM_PROTOCOL` already provides all necessary hooks to implement Intel UFS platform support equivalent to depthcharge. The key requirement is:

**Disable PA_LOCAL_TX_LCC_ENABLE (0x155E) during EdkiiUfsHcPreLinkStartup callback**

This single platform-specific configuration is what makes UFS work on Intel platforms. The new UfsPlatformDxe driver will be a simple, clean source-based implementation replacing the existing binary blob.

