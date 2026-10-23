#!/usr/bin/env python3
"""Fetch only pinned emulator inputs; never execute upstream installation code."""
import hashlib,json,urllib.request
from pathlib import Path
root=Path(__file__).resolve().parents[1];lock=json.loads((root/'emulator.lock.json').read_text())
destination=root/'vendor/emulator'
for name,digest in lock['files'].items():
    path=destination/name
    if path.exists():
        if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:raise SystemExit('Changed upstream file: '+name)
        continue
    url='https://raw.githubusercontent.com/Alberto-rp/Pocket-pikachu-emulator/'+lock['revision']+'/'+name
    with urllib.request.urlopen(url,timeout=30) as response:raw=response.read(4*1024*1024+1)
    if len(raw)>4*1024*1024 or hashlib.sha256(raw).hexdigest()!=digest:raise SystemExit('Upstream checksum mismatch: '+name)
    path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(raw)
print('Emulator inputs verified at '+lock['revision'])
