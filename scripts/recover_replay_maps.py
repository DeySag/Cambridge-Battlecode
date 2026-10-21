#!/usr/bin/env python3
"""Recover exact published maps embedded in extracted organiser replays.

Requires the organiser's saved /api/v1/maps/pool JSON manifest. Only writes
bytes whose size and SHA-256 match a manifest entry; no network access.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())['maps']
    entries = {(int(m['size']), m['sha256']): m for m in manifest}
    sizes = sorted({size for size, _ in entries})
    args.output.mkdir(parents=True, exist_ok=True)
    rows = []
    for replay in sorted(args.input.rglob('*.repl26a')):
        data = replay.read_bytes()
        offset = data.find(b'w62pam')
        if offset < 0:
            raise ValueError(f'No embedded map header: {replay}')
        found = False
        for size in sizes:
            payload = data[offset:offset + size]
            digest = hashlib.sha256(payload).hexdigest()
            entry = entries.get((len(payload), digest))
            if entry is None:
                continue
            name = entry['name']
            if not re.fullmatch(r'[A-Za-z0-9_-]+', name):
                raise ValueError(f'Unsafe manifest map name: {name}')
            destination = args.output / f'{name}.map26a'
            if destination.exists() and destination.read_bytes() != payload:
                raise ValueError(f'Refusing to replace a different map: {destination}')
            destination.write_bytes(payload)
            rows.append({'replay': str(replay.relative_to(args.input)),
                         'map': name, 'size': size, 'sha256': digest})
            found = True
            break
        if not found:
            raise ValueError(f'Embedded map not in this manifest: {replay}')
    if not rows:
        raise ValueError('No extracted replay files found')
    (args.output / 'recovery.json').write_text(json.dumps(rows, indent=2) + '\n')
    print(f"Verified {len(rows)} replays / {len({r['map'] for r in rows})} maps")


if __name__ == '__main__':
    main()
