#!/usr/bin/env python3
"""Exercise Pocket Pikachu's own On/Off controls without touching other creations."""
import json,subprocess,sys,time
from pathlib import Path
root=Path.home()/'.local/share/magicshelf';sys.path.insert(0,str(root/'current'))
from magicshelf.client import request
key='org.magicshelf.pocket-pikachu';data=root/'module-data'/key
def pid(unit):return subprocess.check_output(['systemctl','--user','show',unit,'-p','MainPID','--value'],text=True).strip()
def others(state):return {m['id']:(m.get('version'),m.get('enabled'),m.get('subscribed')) for m in state['modules'] if m['id']!=key}
before=request('snapshot');steam=pid('steamvr.service');dashboard=pid('magicshelf-vr.service')
save=json.loads((data/'save.json').read_text());status=json.loads((data/'runtime.json').read_text())
request('module.toggle',{'id':key,'enabled':False})
assert pid('magicshelf-module-'+key+'.service')=='0'
stopped=json.loads((data/'save.json').read_text())
for field in ('steps','totalSteps','watts','friendshipLevel','InitTamagotchi'):
    assert save['storage'].get(field)==stopped['storage'].get(field),(field,'stop')
request('module.toggle',{'id':key,'enabled':True})
assert pid('magicshelf-module-'+key+'.service')!='0'
time.sleep(2)
restarted=json.loads((data/'save.json').read_text());after=request('snapshot')
for field in ('steps','totalSteps','watts','friendshipLevel','InitTamagotchi'):
    assert stopped['storage'].get(field)==restarted['storage'].get(field),(field,'restart')
assert others(before)==others(after)
assert pid('steamvr.service')==steam and pid('magicshelf-vr.service')==dashboard
print(json.dumps({'off_stops_process':True,'on_reports_ready':True,'progress_preserved':True,'other_creations_preserved':True,'steamvr_pid':steam,'storefront_pid':dashboard,'before_runtime':status,'after_runtime':json.loads((data/'runtime.json').read_text())},indent=2))
