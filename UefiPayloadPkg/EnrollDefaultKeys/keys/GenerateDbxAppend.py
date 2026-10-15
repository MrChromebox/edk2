#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause-Patent
"""Generate the dbxDefault append list without changing signed DBX updates."""

from __future__ import annotations

from pathlib import Path
import struct
from typing import Iterable, Iterator, Tuple


SignatureList = Tuple[bytes, bytes, list]


def auth_descriptor_size(data: bytes) -> int:
    """Return the EFI_VARIABLE_AUTHENTICATION_2 descriptor length."""
    if len(data) < 20:
        raise ValueError('authenticated payload too small')
    descriptor_size = 16 + struct.unpack_from('<I', data, 16)[0]
    if not 40 <= descriptor_size < len(data):
        raise ValueError('invalid authentication descriptor')
    return descriptor_size


def signature_lists(data: bytes, *, authenticated: bool = True) -> Iterator[SignatureList]:
    """Yield (signature_type, header, entries) from an ESL or auth2+ESL blob."""
    offset = auth_descriptor_size(data) if authenticated else 0
    while offset < len(data):
        signature_type = data[offset:offset + 16]
        size, header_size, entry_size = struct.unpack_from('<III', data, offset + 16)
        start = offset + 28 + header_size
        end = offset + size
        if (entry_size < 16 or size < 28 + header_size or
                end > len(data) or (end - start) % entry_size):
            raise ValueError('invalid signature list')
        yield signature_type, data[offset:start], [
            data[pos:pos + entry_size] for pos in range(start, end, entry_size)
        ]
        offset = end


def signature_set(data: bytes, *, authenticated: bool = True) -> set:
    return {
        (signature_type, entry)
        for signature_type, _, entries in signature_lists(data, authenticated=authenticated)
        for entry in entries
    }


def build_append_esl(baseline: bytes, update: bytes) -> bytes:
    """
    Return unsigned ESL bytes containing update signatures absent from baseline.

    Matches AuthVariableLib append filtering: compare type and complete
    EFI_SIGNATURE_DATA (including owner), preserving update order.
    """
    baseline_entries = signature_set(baseline)
    result = bytearray()
    for signature_type, header, entries in signature_lists(update):
        added = b''.join(
            entry for entry in entries
            if (signature_type, entry) not in baseline_entries
        )
        if added:
            header = bytearray(header)
            struct.pack_into('<I', header, 16, len(header) + len(added))
            result += header + added
    return bytes(result)


def generate_append(
    baseline_path: Path,
    update_path: Path,
    output_path: Path,
) -> bytes:
    append = build_append_esl(baseline_path.read_bytes(), update_path.read_bytes())
    output_path.write_bytes(append)
    return append


def main(argv: Iterable[str] | None = None) -> int:
    directory = Path(__file__).resolve().parent
    generate_append(
        directory / 'dbx_microsoft_baseline.bin',
        directory / 'dbx_microsoft_update.bin',
        directory / 'dbx_microsoft_append.esl',
    )
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
