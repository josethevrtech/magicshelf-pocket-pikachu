// Original yellow handheld options. Clock changes are private to this pet.
var __device={sound:true,offset:0},__deviceUi='',__deviceSelection=1,__clockDraft=null,__clockField=0;
var __needsFirstClock=localStorage.getItem('InitTamagotchi')!=='true';
try{
    const saved=JSON.parse(localStorage.getItem('nativeDevice')||'{}');
    if(typeof saved.sound==='boolean')__device.sound=saved.sound;
    if(Number.isFinite(saved.offset)&&Math.abs(saved.offset)<=86400000)__device.offset=saved.offset;
}catch(_){}
function __deviceNow(){return Date.now()+__device.offset;}
function __deviceStore(){localStorage.setItem('nativeDevice',JSON.stringify(__device));}
function __soundEnabled(){return __device.sound;}
function __deviceClock(first){
    const now=new Date(__deviceNow());__clockDraft={hour:now.getHours(),minute:now.getMinutes()};
    __clockField=0;__deviceUi=first?'first-clock':'time';
}
function __deviceReset(){
    __device={sound:true,offset:0};__deviceUi='';__clockDraft=null;__deviceSelection=1;__needsFirstClock=true;
    hasReach150=hasReach300=hasReach450=false;
    __refreshSchedule();
}
function __refreshSchedule(){
    pokeStatus.eatingtHours=hasReach300?[12,18]:[10,12,18];
    pokeStatus.tvHours=hasReach300?[18,19,20]:[18,19];
    pokeStatus.greetingHours=hasReach300?[8,12,18,19]:[8,12,18];
    pokeStatus.bathHours=hasReach300?[20]:[19];pokeStatus.sleepHour=hasReach300?21:20;
}
function __deviceTick(){
    const now=new Date(),previous=__savedDate(pokeStatus.lastConected),days=__calendarDays(previous,now);
    if(Number.isFinite(days)&&days>0&&localStorage.getItem('InitTamagotchi')==='true'){
        if(settings.relSelected==='on'){
            pokeStatus.friendshipLevel=Math.max(-1500,pokeStatus.friendshipLevel-days*100);
            localStorage.setItem('friendshipLevel',pokeStatus.friendshipLevel);
        }
        pokeStatus.lastConected=now.toDateString();localStorage.setItem('lastConected',pokeStatus.lastConected);
    }
    if(__needsFirstClock&&localStorage.getItem('InitTamagotchi')==='true'){
        __needsFirstClock=false;__deviceClock(true);
    }
}
function __deviceOpen(){
    __deviceUi='settings';__deviceSelection=1;__clockDraft=null;
}
function __devicePress(button){
    if(button==='reset'){__deviceReset();return false;}
    if(button==='shake')return false;
    if(!__deviceUi)return false;
    if(__clockDraft){
        if(button==='back'){
            const first=__deviceUi==='first-clock';__clockDraft=null;__deviceUi=first?'':'settings';return true;
        }
        if(button==='enter'){
            const now=Date.now(),chosen=new Date(now);
            chosen.setHours(__clockDraft.hour,__clockDraft.minute,0,0);
            __device.offset=chosen.getTime()-now;__deviceStore();
            // A clock adjustment must not replay an already dismissed alarm.
            __alarmUntil=0;__alarmSound=false;__alarmLastWall=__deviceNow();
            const first=__deviceUi==='first-clock';__clockDraft=null;__deviceUi=first?'':'settings';
            return true;
        }
        if(button==='left'||button==='right'){__clockField=1-__clockField;return true;}
        if(button==='top'||button==='bottom'){
            const change=button==='top'?1:-1;
            if(__clockField===0)__clockDraft.hour=(__clockDraft.hour+change+24)%24;
            else __clockDraft.minute=(__clockDraft.minute+change+60)%60;
        }
        return true;
    }
    if(button==='back'||button==='menu'){__deviceUi='';__dispatch('back');return true;}
    if(button==='top')__deviceSelection=Math.max(0,__deviceSelection-1);
    if(button==='bottom')__deviceSelection=Math.min(2,__deviceSelection+1);
    if(button==='enter'){
        if(__deviceSelection===0){pokeStatus.steps=0;localStorage.setItem('steps',0);document.querySelector('.walkCounter').innerHTML='0';}
        if(__deviceSelection===1){__device.sound=!__device.sound;__deviceStore();}
        if(__deviceSelection===2)__deviceClock(false);
    }
    return true;
}
function __deviceDisplay(pixels){
    if(!__deviceUi)return pixels;
    pixels=new Array(1080).fill(0);
    if(__clockDraft){
        __alarmText(pixels,'TIME',1,0,1);__alarmText(pixels,__clockDraft.hour>=12?'PM':'AM',27,0,1);
        __alarmText(pixels,__alarmTime(__clockDraft),0,8,2);__alarmText(pixels,'A SET',9,24,1);
        for(let x=__clockField===0?0:20;x<=(__clockField===0?13:33);x++)pixels[19*36+x]=2;
    }else{
        __alarmText(pixels,'RESET',5,1,1);__alarmText(pixels,'SOUND',5,11,1);
        __alarmText(pixels,__device.sound?'ON':'OFF',__device.sound?27:25,11,1);
        __alarmText(pixels,'TIME SET',5,21,1);
        __alarmText(pixels,'>',0,1+__deviceSelection*10,1);
    }
    return pixels;
}
