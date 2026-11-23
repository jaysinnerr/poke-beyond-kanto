#!/usr/bin/env python3
import sys
from pathlib import Path

EOF_MARKER = 0x454F46
MAX_RECORD = 0xFFFF

def make_ips(base: bytes, mod: bytes) -> bytes:
    if len(base) != len(mod):
        raise SystemExit("IPS builder expects equal-size ROMs")

    out = bytearray(b"PATCH")
    i = 0
    n = len(base)

    while i < n:
        if base[i] == mod[i]:
            i += 1
            continue

        start = i
        while i < n and base[i] != mod[i]:
            i += 1
        end = i

        # Avoid a record offset equal to the IPS EOF marker.
        if start == EOF_MARKER and start > 0:
            start -= 1

        pos = start
        while pos < end:
            size = min(MAX_RECORD, end - pos)

            # Never begin a record at the reserved EOF marker.
            if pos == EOF_MARKER and pos > 0:
                pos -= 1
                size = min(MAX_RECORD, end - pos)

            out.extend(pos.to_bytes(3, "big"))
            out.extend(size.to_bytes(2, "big"))
            out.extend(mod[pos:pos + size])
            pos += size

    out.extend(b"EOF")
    return bytes(out)

def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: make_ips.py BASE.gba MOD.gba OUTPUT.ips")

    base_path, mod_path, out_path = map(Path, sys.argv[1:])
    base = base_path.read_bytes()
    mod = mod_path.read_bytes()
    patch = make_ips(base, mod)
    out_path.write_bytes(patch)

    print(f"Wrote {out_path} ({len(patch):,} bytes)")

if __name__ == "__main__":
    main()
