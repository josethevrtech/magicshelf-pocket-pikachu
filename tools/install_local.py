#!/usr/bin/env python3
"""Install a verified local development package through MagicShelf's manager.

Run on the Frame as its desktop user. Only the manager is briefly stopped to
serialize registration; SteamVR, the storefront and other creations keep running.
"""
import argparse, hashlib, json, shutil, subprocess, sys, tempfile, time
from pathlib import Path

def system(*args):
    return subprocess.run(['systemctl','--user',*args],check=True,capture_output=True,text=True).stdout.strip()

def main():
    p=argparse.ArgumentParser();p.add_argument('archive',type=Path);p.add_argument('--sha256',required=True);a=p.parse_args()
    root=Path.home()/'.local/share/magicshelf'
    sys.path.insert(0,str(root/'current'))
    from magicshelf.packages import unpack
    from magicshelf.manager import Manager
    from magicshelf.client import request
    archive=a.archive.resolve()
    if hashlib.sha256(archive.read_bytes()).hexdigest()!=a.sha256: raise ValueError('Package checksum mismatch')
    before=request('snapshot')
    keep={m['id']:(m.get('version'),m.get('enabled'),m.get('subscribed')) for m in before['modules'] if m['id']!='org.magicshelf.pocket-pikachu'}
    steam=system('show','steamvr.service','-p','MainPID','--value')
    dashboard=system('show','magicshelf-vr.service','-p','MainPID','--value')
    stamp=str(time.time_ns());record=root/'development-installs'/stamp;record.mkdir(parents=True)
    (record/'before.json').write_text(json.dumps(before,indent=2))
    with tempfile.TemporaryDirectory(prefix='pocket-pikachu-install-',dir=root) as temp:
        stage=Path(temp)/'package';meta=unpack(archive,stage,a.sha256)
        if meta['id']!='org.magicshelf.pocket-pikachu': raise ValueError('This installer only handles Pocket Pikachu')
        # Local development artwork uses a reserved, non-network cache identity.
        # Seed the existing storefront cache from verified package bytes, so
        # neither publishing assets nor changing the shared framework is needed.
        from magicshelf.storefront import image_key, valid_png
        for picture in meta.get('screenshots',[]):
            if picture.get('package_path'):
                if not picture['url'].startswith('https://local.magicshelf.invalid/'+meta['id']+'/'+meta['version']+'/'):
                    raise ValueError('Unexpected local preview identity')
                path=(stage/picture['package_path']).resolve()
                if not path.is_relative_to(stage.resolve()) or path.stat().st_size>6*1024*1024:
                    raise ValueError('Invalid packaged preview')
                valid_png(path);cache=root/'previews';cache.mkdir(parents=True,exist_ok=True)
                target=cache/image_key(picture['url']);temporary=target.with_suffix('.part')
                shutil.copy2(path,temporary);temporary.replace(target)
        installed=next((m for m in before['modules'] if m['id']==meta['id']),None)
        if installed:
            from magicshelf.contracts import version
            if version(meta['version'])<=version(installed['version']):
                raise ValueError('An update must have a newer version')
            request('module.toggle',{'id':meta['id'],'enabled':False})
        system('stop','magicshelf.service')
        try:
            manager=Manager()
            with manager.transaction():manager.commit_package(meta['id'],meta['version'],stage)
        finally:system('start','magicshelf.service')
    for attempt in range(50):
        try:request('snapshot');break
        except (OSError,RuntimeError):time.sleep(.1)
    after=request('module.toggle',{'id':meta['id'],'enabled':True})
    current=next(m for m in after['modules'] if m['id']==meta['id'])
    if current.get('version')!=meta['version'] or not current.get('enabled'):
        raise RuntimeError('The requested update did not remain enabled; check MagicShelf recovery status')
    for item in after['modules']:
        if item['id'] in keep:
            assert keep[item['id']]==(item.get('version'),item.get('enabled'),item.get('subscribed')),item['id']
    assert system('show','steamvr.service','-p','MainPID','--value')==steam
    assert system('show','magicshelf-vr.service','-p','MainPID','--value')==dashboard
    (record/'after.json').write_text(json.dumps(after,indent=2))
    print(json.dumps({'installed':meta['id'],'version':meta['version'],'steamvr_pid':steam,'storefront_pid':dashboard,'other_creations_preserved':True,'record':str(record)},indent=2))

if __name__=='__main__':main()
