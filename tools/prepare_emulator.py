#!/usr/bin/env python3
"""Prepare the pinned emulator for the native host with documented fixes."""
import argparse, hashlib, json, shutil, re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def prepare(source, destination):
    lock=json.loads((ROOT/'emulator.lock.json').read_text())
    destination.mkdir(parents=True,exist_ok=True)
    for name,digest in lock['files'].items():
        (destination/name).parent.mkdir(parents=True,exist_ok=True)
        raw=(source/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest: raise ValueError('Upstream pin differs: '+name)
        if name=='script.js':
            text=raw.decode()
            start=text.index('    /* PEDOMETER */')
            end=text.index('\n})\n',start)
            text=text[:start]+'    // Mobile browser pedometer replaced by native headset motion.\n'+text[end:]
            def replace_once(old,new,count=1):
                nonlocal text
                if text.count(old)!=count: raise ValueError('Unexpected upstream date logic: '+old)
                text=text.replace(old,new)
            replace_once('new Date(pokeStatus.lastConected)', '__savedDate(pokeStatus.lastConected)')
            replace_once('if(daysPassed != 0)', 'if(Number.isFinite(daysPassed) && daysPassed > 0)')
            replace_once('const Start = new Date(', 'const Start = __savedDate(')
            replace_once('% 12 || 0)', '% 12 || 12)')
            replace_once('(timeStart.getHours() > 12)?', '(timeStart.getHours() >= 12)?')
            # Keep animation durations and daily routines on the toy's clock;
            # cookie expiry and save/elapsed-playtime accounting use real time.
            text=text.replace('timeStart = new Date()', 'timeStart = new Date(__deviceNow())')
            text=text.replace('startTime = new Date()', 'startTime = new Date(__deviceNow())')
            text=text.replace('endTime = new Date()', 'endTime = new Date(__deviceNow())')
            text=text.replace('var time = new Date()', 'var time = new Date(__deviceNow())')
            replace_once('Math.trunc((today - lastDate) / (1000 * 3600 * 24))', '__calendarDays(lastDate,today)')
            # Pixel lists contain strings. Native includes preserves membership
            # without invoking a JS callback for every candidate of every pixel.
            pixel_test='.some(elem => elem == `num-${i}`)'
            if text.count(pixel_test)!=46: raise ValueError('Unexpected pixel membership sites')
            text=text.replace(pixel_test,'.includes(`num-${i}`)')
            # The yellow hardware stops at 999,999 lifetime steps. Remove the
            # fan emulator's shortened difficulty progression, retaining saves.
            replace_once('settings.dificultySelected = (localStorage.getItem("dificultyLevel") != null)? localStorage.getItem("dificultyLevel") : "medium";',
                         'settings.dificultySelected = "hard";',2)
            replace_once("settings.dificultySelected = 'medium';", "settings.dificultySelected = 'hard';")
            replace_once('"unlockAnims": [150000, 300000, 450000, 1000000]', '"unlockAnims": [150000, 300000, 450000, 999999]')
            replace_once('case 1000000:', 'case 999999:')
            for milestone in (150,300,450):
                text=text.replace(f'const hasReach{milestone} =',f'let hasReach{milestone} =')
                text=text.replace(f'pokeStatus.totalSteps > limit{milestone} &&',f'pokeStatus.totalSteps >= limit{milestone} &&')
                old=f'localStorage.setItem("hasReach{milestone}", 1);'
                replace_once(old,old+f' hasReach{milestone}=true; __refreshSchedule();')
            # A four-digit watt display must never overflow or lose winnings
            # if the creation is stopped part-way through a payout animation.
            replace_once('pokeStatus.watts++', 'pokeStatus.watts=Math.min(9999,pokeStatus.watts+1)')
            replace_once('pokeStatus.watts += 10;', 'pokeStatus.watts=Math.min(9999,pokeStatus.watts+10); localStorage.setItem("watts",pokeStatus.watts);')
            # An enabled VR companion has continuous power. Keep the upstream
            # sleep animations, but do not blank its LCD after sixty seconds.
            idle='screenOff = setInterval(switchOff, 60000)'
            if text.count(idle)!=3: raise ValueError('Expected three LCD idle timers')
            text=text.replace(idle,'screenOff = undefined; // Continuous native LCD power')
            # Start opens only the native Reset/Sound/Time menu. Never draw
            # the fan difficulty menu underneath it, even for one frame.
            replace_once('loadAnim(DisplayScreen, Anims.settings.diffLevel)', '__deviceOpen()',2)
            # Remember the current pet before every top-level menu. Back
            # restores it before a routine with a deferred first frame starts.
            replace_once("        if(allowedAnims.some(anim => anim == animStatus)){\n            cleanStates();",
                         "        if(allowedAnims.some(anim => anim == animStatus)){\n            __rememberPetFrame();\n            cleanStates();")
            replace_once("        if(allowedAnims.some(anim => anim == animStatus) && !pokeStatus.reachEnd) {\n            cleanStates();",
                         "        if(allowedAnims.some(anim => anim == animStatus) && !pokeStatus.reachEnd) {\n            __rememberPetFrame();\n            cleanStates();")
            replace_once('                        switch (menuSelected) {',
                         '                        __rememberPetFrame();\n                        switch (menuSelected) {')
            replace_once('            basicAnim(false, false, true, avoidEatAfterWalk);',
                         '            __restorePetFrame();\n            basicAnim(false, false, true, avoidEatAfterWalk);')
            # A's analog-style delay is cancellable. Navigation must not fire
            # later after B, Start or Select has changed the user's context.
            start=text.index('            // Init screen (Timeouts to emulate analogic)')
            end=text.index('            // APAGAR LA PANTALLA',start)
            pending=text[start:end]
            if pending.count('setTimeout(')!=3: raise ValueError('Expected three delayed A callbacks')
            text=text[:start]+'            __cancelEnter();\n'+pending.replace('setTimeout(', '__queueEnter(')+text[end:]
            replace_once('    clearTimeout(auxiliarTimeout2);\n}',
                         '    clearTimeout(auxiliarTimeout2);\n    clearTimeout(auxiliarTimeout3);\n    __cancelEnter();\n}')
            # A queued A-button callback can run after friendship/settings has
            # cleared menu selection. Ignore the stale action, as no menu is
            # selected anymore, instead of terminating the native process.
            query="let menuSelected = document.querySelector('.menuBar .selected').id"
            if text.count(query)!=1: raise ValueError('Expected one delayed menu selection')
            text=text.replace(query,"const selectedMenu = document.querySelector('.menuBar .selected');\n                        if (!selectedMenu) return;\n                        let menuSelected = selectedMenu.id")
            # QJSEngine does not hoist function declarations directly inside a
            # switch case like browser engines do. Explicit case blocks retain
            # the same logic while giving those declarations a lexical scope.
            start=text.index('switch (menuSelected) {')
            end=text.index('\n                        }',start)
            cases=text[start:end]
            cases=re.sub(r'case "(clockMenu|giftMenu|gameMenu)":',r'case "\1": {',cases)
            cases=cases.replace('break;','break;\n                            }')
            text=text[:start]+cases+text[end:]
            (destination/name).write_text(text)
        else: shutil.copy2(source/name,destination/name)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=ROOT/'vendor/emulator');p.add_argument('--output',type=Path,default=ROOT/'build/package/emulator')
    a=p.parse_args();prepare(a.source,a.output)
