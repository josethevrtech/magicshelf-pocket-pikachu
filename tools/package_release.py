#!/usr/bin/env python3
"""Package the native build and its public Discover image as one release."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'josethevrtech/magicshelf-pocket-pikachu'


def main():
    package = ROOT / 'build/package'
    meta = json.loads((package / 'module.json').read_text())
    source = json.loads((ROOT / 'module.json').read_text())
    if meta['version'] != source['version'] or meta['screenshots'] != source['screenshots']:
        raise ValueError('Rebuild this version before packaging it')
    image_url = f'https://github.com/{REPOSITORY}/releases/download/v{meta["version"]}/pocket-pikachu.png'
    if meta['screenshots'][0]['url'] != image_url:
        raise ValueError('The preview must belong to this release')
    subprocess.run(['python3', str(ROOT / 'vendor/magicshelf/tools/package_creation.py'), str(package),
                    '--repository', REPOSITORY, '--output', str(ROOT / 'dist')], check=True)
    out = ROOT / 'dist'
    shutil.copy2(package / 'media/pocket-pikachu.png', out / 'pocket-pikachu.png')
    name = f'{meta["id"]}-{meta["version"]}-linux-{meta["architecture"]}.magicshelf'
    assets = [out / name, out / 'catalog.json', out / 'pocket-pikachu.png']
    (out / 'SHA256SUMS').write_text(''.join(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name+'\n' for p in assets))


if __name__ == '__main__':
    main()
