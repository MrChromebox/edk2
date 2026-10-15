# Microsoft DBX

Artifacts are refreshed by `UpdateMicrosoftDbx.py` from
[microsoft/secureboot_objects](https://github.com/microsoft/secureboot_objects).

| File | Role |
|------|------|
| `dbx_microsoft_baseline.bin` | Retained complete Microsoft-signed DBX (factory write) |
| `dbx_microsoft_update.bin` | Latest Microsoft-signed update (unmodified; append) |
| `dbx_microsoft_append.esl` | Unsigned ESL of update-only entries for `dbxDefault` |

## Current contents

- Arch: `x64`
- KEK package: `2023`
- Signed release: `v1.7.0-signed`
- Baseline source: PostSignedObjects DBXUpdate.bin
- Update source: `https://github.com/microsoft/secureboot_objects/releases/download/v1.7.0-signed/edk2-2023-signed-secureboot-binaries.zip` (`dbx_x64.efiauth2`)

| File | Bytes | SHA-256 | Signatures |
|------|------:|---------|------------|
| `dbx_microsoft_baseline.bin` | 24629 | `2089e3125e611376cb44326b5765674255443b2484f88de97251939d18055f68` | 443 |
| `dbx_microsoft_update.bin` | 17256 | `524c796ade42db0048b656f6cf3863832174ae7f58cad6986d1ccae1e3d3ecd1` | 291 |
| `dbx_microsoft_append.esl` | 124 | `fda799a382c9089f4ad733bab643d73c0dff77b86fbb75a241a822e69ef7169d` | 2 |

Signature set comparison: 289 shared, 2 update-only, 154 baseline-only.

Setup Mode enrollment should write the baseline, then append the signed update.
`dbx_microsoft_append.esl` is only for publishing a complete `dbxDefault`; it must not be
used as an authenticated OS update. Keep update order and signature owners
intact so fwupd can identify the DBX version from the last Microsoft entry.

## Refresh

```
python3 UpdateMicrosoftDbx.py
python3 UpdateMicrosoftDbx.py --release v1.7.0-signed --kek 2023 --arch x64
python3 UpdateMicrosoftDbx.py --rebase-baseline
```
