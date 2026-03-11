# OpalPassword “Simple UI” mode

This driver supports an optional “Simple UI” mode intended for non-enterprise users. When enabled, the HII UI only exposes:

- Set password
- Change password
- Remove password (non-destructive; requires current password)
- Erase & Reset (Forgotten password) (destructive; does not require current password; requires strong local confirmation)

When disabled (default), the existing full/advanced UI is unchanged.

## Enabling Simple UI

The code uses the PCD as the single source of truth:

- `gEfiSecurityPkgTokenSpaceGuid.PcdTcgStorageSimpleUi` (BOOLEAN, default `FALSE`)

To enable via a platform DSC / `build --pcd`, set:

```
gEfiSecurityPkgTokenSpaceGuid.PcdTcgStorageSimpleUi|TRUE
```

If building `SecurityPkg/SecurityPkg.dsc` directly, you can also pass a build define to set the default PCD value:

```
-D TCG_STORAGE_SIMPLE_UI=ON
```

## Erase & Reset behavior

In Simple UI mode, “Erase & Reset (Forgotten Password)”:

- Requires strong confirmation at execution time (typing `ERASE` and a final `Y/N` confirmation).
- Requires entering the 32-character reset key printed on the drive label.
- Permanently deletes all data on the disk.
