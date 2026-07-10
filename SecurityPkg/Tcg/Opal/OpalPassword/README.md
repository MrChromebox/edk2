# OpalPassword UI modes

This driver supports two setup UI modes for TCG disk encryption management:

- **Advanced** (default): full technical OPAL form with BlockSID options, checkboxes, and deferred request processing.
- **Standard**: simplified consumer-oriented form with direct password actions and immediate reboot.

## Runtime selection

UI mode is stored in the non-volatile `TcgUiConfig` UEFI variable and exposed from the **TCG Disk Encryption** setup form.

A reboot is required after changing the mode because the HII form package is selected at driver initialization.

## First-boot defaults

When `TcgUiConfig` is absent, platform PCDs seed the initial value:

- `gEfiSecurityPkgTokenSpaceGuid.PcdTcgStorageSimpleUi` (BOOLEAN, default `FALSE` = Advanced)

Optional build-time default override:

```
-D TCG_STORAGE_SIMPLE_UI=ON
```

## Standard mode behavior

Standard mode only exposes:

- Set password
- Change password
- Remove password (non-destructive; requires current password)
- Erase & Reset (Forgotten password) (destructive; does not require current password)

Erase & Reset requires strong confirmation at execution time (typing `ERASE` and a final `Y/N` confirmation), plus the 32-character reset key printed on the drive label.
