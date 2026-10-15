#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""
Fetch Microsoft DBX artifacts and refresh the in-tree copies.

Microsoft publishes authenticated DBX *updates* meant for AppendWrite. Newer
signed packages are not always supersets of earlier complete lists, so this
tree keeps:

  dbx_microsoft_baseline.bin  - last known-good complete Microsoft-signed DBX
  dbx_microsoft_update.bin    - latest Microsoft-signed update (unmodified)
  dbx_microsoft_append.esl    - unsigned ESL of update-only entries for dbxDefault

Sources (https://github.com/microsoft/secureboot_objects):
  baseline candidate: PostSignedObjects/DBX/<arch>/DBXUpdate.bin
  latest update:      releases/*-signed  edk2-{2011|2023}-signed-secureboot-binaries

Examples:
  ./UpdateMicrosoftDbx.py
  ./UpdateMicrosoftDbx.py --kek 2023 --arch x64
  ./UpdateMicrosoftDbx.py --release v1.7.0-signed --rebase-baseline
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import ssl
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

from GenerateDbxAppend import auth_descriptor_size, generate_append, signature_set

REPO = 'microsoft/secureboot_objects'
API_RELEASES = f'https://api.github.com/repos/{REPO}/releases'
RAW_BASE = f'https://raw.githubusercontent.com/{REPO}'

ARCH_MAP = {
    'x64': {
        'postsigned': 'amd64',
        '2023_member': 'dbx_x64.efiauth2',
        '2011_member': 'dbx_x64_Legacy/dbx_x64_Legacy.efiauth2',
        'legacy_member': 'amd64/DBXUpdate.bin',
    },
    'ia32': {
        'postsigned': 'x86',
        '2023_member': 'dbx_ia32.efiauth2',
        '2011_member': 'dbx_ia32_Legacy/dbx_ia32_Legacy.efiauth2',
        'legacy_member': 'x86/DBXUpdate.bin',
    },
    'aarch64': {
        'postsigned': 'arm64',
        '2023_member': 'dbx_aarch64.efiauth2',
        '2011_member': 'dbx_aarch64_Legacy/dbx_aarch64_Legacy.efiauth2',
        'legacy_member': 'arm64/DBXUpdate.bin',
    },
    'arm': {
        'postsigned': 'arm',
        '2023_member': 'dbx_arm.efiauth2',
        '2011_member': 'dbx_arm_Legacy/dbx_arm_Legacy.efiauth2',
        'legacy_member': 'arm/DBXUpdate.bin',
    },
}

BASELINE_NAME = 'dbx_microsoft_baseline.bin'
UPDATE_NAME = 'dbx_microsoft_update.bin'
APPEND_NAME = 'dbx_microsoft_append.esl'
README_NAME = 'README.md'


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def http_get(url: str, timeout: int = 60) -> bytes:
    request = urllib.request.Request(
        url,
        headers={
            'User-Agent': 'UpdateMicrosoftDbx/1.0 (edk2 UefiPayloadPkg)',
            'Accept': 'application/octet-stream, application/json',
        },
    )
    context = ssl.create_default_context()
    try:
        with urllib.request.urlopen(request, timeout=timeout, context=context) as response:
            return response.read()
    except urllib.error.HTTPError as exc:
        raise SystemExit(f'HTTP {exc.code} fetching {url}') from exc
    except urllib.error.URLError as exc:
        raise SystemExit(f'Failed to fetch {url}: {exc.reason}') from exc


def http_get_json(url: str):
    return json.loads(http_get(url).decode())


def validate_auth2(data: bytes, label: str) -> None:
    try:
        auth_descriptor_size(data)
    except ValueError as exc:
        raise SystemExit(f'{label}: {exc}') from exc


def latest_signed_tag() -> str:
    for release in http_get_json(f'{API_RELEASES}?per_page=30'):
        tag = release.get('tag_name', '')
        if tag.endswith('-signed') and not release.get('draft') and not release.get('prerelease'):
            return tag
    raise SystemExit('No signed release found on microsoft/secureboot_objects')


def release_asset_url(tag: str, asset_name: str) -> str:
    release = http_get_json(f'https://api.github.com/repos/{REPO}/releases/tags/{tag}')
    for asset in release.get('assets', []):
        if asset.get('name') == asset_name:
            return asset['browser_download_url']
    raise SystemExit(f'Release {tag} has no asset named {asset_name}')


def extract_zip_member(archive: bytes, member_names: list[str]) -> tuple[str, bytes]:
    with zipfile.ZipFile(io.BytesIO(archive)) as zf:
        names = set(zf.namelist())
        for member in member_names:
            if member in names:
                return member, zf.read(member)
    raise SystemExit(
        'None of the expected members were found in the archive:\n  '
        + '\n  '.join(member_names)
    )


def fetch_postsigned_baseline(arch: str, ref: str) -> tuple[str, bytes]:
    post = ARCH_MAP[arch]['postsigned']
    rel_path = f'PostSignedObjects/DBX/{post}/DBXUpdate.bin'
    url = f'{RAW_BASE}/{ref}/{rel_path}'
    data = http_get(url)
    validate_auth2(data, rel_path)
    return url, data


def fetch_signed_update(tag: str, arch: str, kek: str) -> tuple[str, str, bytes]:
    arch_info = ARCH_MAP[arch]
    if kek == '2023':
        asset = 'edk2-2023-signed-secureboot-binaries.zip'
        members = [arch_info['2023_member'], arch_info['legacy_member']]
    else:
        asset = 'edk2-2011-signed-secureboot-binaries.zip'
        members = [arch_info['2011_member'], arch_info['legacy_member']]

    url = release_asset_url(tag, asset)
    member, data = extract_zip_member(http_get(url), members)
    validate_auth2(data, f'{asset}:{member}')
    return url, member, data


def choose_baseline(
    keys_dir: Path,
    postsigned: bytes,
    new_update: bytes,
    *,
    rebase: bool,
) -> tuple[bytes, str]:
    """
    Pick the retained complete Microsoft-signed DBX.

    Prefer an existing baseline. On first adoption, promote the in-tree update
    when it carries signatures the new update lacks; otherwise use PostSignedObjects.
    """
    baseline_path = keys_dir / BASELINE_NAME
    update_path = keys_dir / UPDATE_NAME

    if baseline_path.is_file() and not rebase:
        data = baseline_path.read_bytes()
        validate_auth2(data, baseline_path.name)
        return data, f'existing {baseline_path.name}'

    candidates: list[tuple[str, bytes]] = [('PostSignedObjects DBXUpdate.bin', postsigned)]
    if update_path.is_file():
        current = update_path.read_bytes()
        try:
            validate_auth2(current, update_path.name)
            candidates.append((f'existing {update_path.name}', current))
        except SystemExit:
            pass
    if baseline_path.is_file():
        current = baseline_path.read_bytes()
        validate_auth2(current, baseline_path.name)
        candidates.append((f'existing {baseline_path.name}', current))

    new_sigs = signature_set(new_update)
    best_label, best_data = candidates[0]
    best_only = len(signature_set(best_data) - new_sigs)
    best_count = len(signature_set(best_data))
    for label, data in candidates[1:]:
        only = len(signature_set(data) - new_sigs)
        count = len(signature_set(data))
        if only > best_only or (only == best_only and count > best_count):
            best_label, best_data = label, data
            best_only, best_count = only, count

    return best_data, best_label


def write_readme(
    keys_dir: Path,
    *,
    arch: str,
    kek: str,
    release: str,
    baseline: bytes,
    baseline_source: str,
    update: bytes,
    update_source: str,
    update_member: str,
    append: bytes,
) -> None:
    base_sigs = signature_set(baseline)
    upd_sigs = signature_set(update)
    only_base = len(base_sigs - upd_sigs)
    only_upd = len(upd_sigs - base_sigs)
    overlap = len(base_sigs & upd_sigs)

    text = f"""# Microsoft DBX

Artifacts are refreshed by `UpdateMicrosoftDbx.py` from
[microsoft/secureboot_objects](https://github.com/microsoft/secureboot_objects).

| File | Role |
|------|------|
| `{BASELINE_NAME}` | Retained complete Microsoft-signed DBX (factory write) |
| `{UPDATE_NAME}` | Latest Microsoft-signed update (unmodified; append) |
| `{APPEND_NAME}` | Unsigned ESL of update-only entries for `dbxDefault` |

## Current contents

- Arch: `{arch}`
- KEK package: `{kek}`
- Signed release: `{release}`
- Baseline source: {baseline_source}
- Update source: `{update_source}` (`{update_member}`)

| File | Bytes | SHA-256 | Signatures |
|------|------:|---------|------------|
| `{BASELINE_NAME}` | {len(baseline)} | `{sha256(baseline)}` | {len(base_sigs)} |
| `{UPDATE_NAME}` | {len(update)} | `{sha256(update)}` | {len(upd_sigs)} |
| `{APPEND_NAME}` | {len(append)} | `{sha256(append)}` | {only_upd} |

Signature set comparison: {overlap} shared, {only_upd} update-only, {only_base} baseline-only.

Setup Mode enrollment should write the baseline, then append the signed update.
`{APPEND_NAME}` is only for publishing a complete `dbxDefault`; it must not be
used as an authenticated OS update. Keep update order and signature owners
intact so fwupd can identify the DBX version from the last Microsoft entry.

## Refresh

```
python3 UpdateMicrosoftDbx.py
python3 UpdateMicrosoftDbx.py --release {release} --kek {kek} --arch {arch}
python3 UpdateMicrosoftDbx.py --rebase-baseline
```
"""
    (keys_dir / README_NAME).write_text(text)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description='Fetch Microsoft DBX artifacts and update the in-tree copies.',
    )
    parser.add_argument(
        '--keys-dir',
        type=Path,
        default=Path(__file__).resolve().parent,
        help='Directory containing dbx_microsoft_*.bin (default: this script\'s directory)',
    )
    parser.add_argument(
        '--arch',
        choices=sorted(ARCH_MAP),
        default='x64',
        help='UEFI architecture (default: x64)',
    )
    parser.add_argument(
        '--kek',
        choices=('2023', '2011'),
        default='2023',
        help='Which Microsoft KEK-signed package to take the latest update from (default: 2023)',
    )
    parser.add_argument(
        '--release',
        default='latest',
        help='Signed release tag (default: latest *-signed release)',
    )
    parser.add_argument(
        '--baseline-ref',
        default='main',
        help='Git ref for PostSignedObjects baseline candidate (default: main)',
    )
    parser.add_argument(
        '--rebase-baseline',
        action='store_true',
        help='Replace the retained baseline using PostSignedObjects / best candidate',
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Fetch and report without writing files',
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    keys_dir: Path = args.keys_dir.resolve()
    keys_dir.mkdir(parents=True, exist_ok=True)

    release = latest_signed_tag() if args.release == 'latest' else args.release
    print(f'Release:  {release}')
    print(f'Arch/KEK: {args.arch} / {args.kek}')

    postsigned_url, postsigned = fetch_postsigned_baseline(args.arch, args.baseline_ref)
    print(f'PostSignedObjects: {postsigned_url} ({len(postsigned)} bytes, {sha256(postsigned)})')

    update_url, update_member, new_update = fetch_signed_update(release, args.arch, args.kek)
    print(f'Update: {update_url} -> {update_member} ({len(new_update)} bytes, {sha256(new_update)})')

    baseline, baseline_source = choose_baseline(
        keys_dir,
        postsigned,
        new_update,
        rebase=args.rebase_baseline,
    )
    print(f'Baseline: {baseline_source} ({len(baseline)} bytes, {sha256(baseline)})')

    if baseline == new_update:
        print('Note: baseline and update are identical (update is a complete list).')

    with tempfile.TemporaryDirectory(prefix='dbx-update-') as tmp:
        tmp_dir = Path(tmp)
        baseline_path = tmp_dir / BASELINE_NAME
        update_path = tmp_dir / UPDATE_NAME
        append_path = tmp_dir / APPEND_NAME
        baseline_path.write_bytes(baseline)
        update_path.write_bytes(new_update)
        append = generate_append(baseline_path, update_path, append_path)
        print(f'Append ESL: {len(append)} bytes, {sha256(append)} '
              f'({len(signature_set(new_update) - signature_set(baseline))} new signatures)')

        if args.dry_run:
            print('Dry run: no files written.')
            return 0

        (keys_dir / BASELINE_NAME).write_bytes(baseline)
        (keys_dir / UPDATE_NAME).write_bytes(new_update)
        (keys_dir / APPEND_NAME).write_bytes(append)
        write_readme(
            keys_dir,
            arch=args.arch,
            kek=args.kek,
            release=release,
            baseline=baseline,
            baseline_source=baseline_source,
            update=new_update,
            update_source=update_url,
            update_member=update_member,
            append=append,
        )

    print(f'Updated files in {keys_dir}:')
    for name in (BASELINE_NAME, UPDATE_NAME, APPEND_NAME, README_NAME):
        path = keys_dir / name
        print(f'  {name} ({path.stat().st_size} bytes)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
