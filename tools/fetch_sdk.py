#!/usr/bin/env python3
import json,pathlib,re,subprocess
root=pathlib.Path(__file__).resolve().parents[1]
lock=json.loads((root/'sdk.lock.json').read_text())
if lock['repository']!='https://github.com/josethevrtech/magicshelf.git' or not re.fullmatch(r'[0-9a-f]{40}',lock['revision']):
    raise SystemExit('Invalid SDK pin')
target=root/'vendor/magicshelf'
if target.exists():
    actual=subprocess.check_output(['git','-C',str(target),'rev-parse','HEAD'],text=True).strip()
    dirty=subprocess.check_output(['git','-C',str(target),'status','--porcelain'],text=True).strip()
    if actual!=lock['revision'] or dirty:raise SystemExit('Existing SDK differs from pin; move it aside before fetching')
else:
    target.mkdir(parents=True)
    subprocess.run(['git','init',str(target)],check=True)
    subprocess.run(['git','-C',str(target),'fetch','--depth=1',lock['repository'],lock['revision']],check=True)
    subprocess.run(['git','-C',str(target),'checkout','--detach',lock['revision']],check=True)
print('SDK pinned at',lock['revision'])
