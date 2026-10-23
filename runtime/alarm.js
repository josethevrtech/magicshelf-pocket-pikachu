// Local daily alarm extension. The pinned upstream clock only draws ALARM OFF.
var __alarm = {enabled:false,hour:7,minute:0,lastDay:''};
try {
    var __alarmSaved = JSON.parse(localStorage.getItem('nativeAlarm') || '{}');
    if (typeof __alarmSaved.enabled === 'boolean' && Number.isInteger(__alarmSaved.hour) && __alarmSaved.hour >= 0 && __alarmSaved.hour < 24 && Number.isInteger(__alarmSaved.minute) && __alarmSaved.minute >= 0 && __alarmSaved.minute < 60) {
        __alarm = {enabled:__alarmSaved.enabled,hour:__alarmSaved.hour,minute:__alarmSaved.minute,lastDay:typeof __alarmSaved.lastDay === 'string' ? __alarmSaved.lastDay : ''};
    }
} catch (_) {}
var __alarmSelection=0;
var __alarmDraft=null, __alarmField=0, __alarmUntil=0, __alarmNextSound=0, __alarmSound=false, __alarmLastWall=0;
function __alarmNow(){return __deviceNow();}
function __alarmStore(){localStorage.setItem('nativeAlarm',JSON.stringify(__alarm));}
function __alarmReset(){__alarm={enabled:false,hour:7,minute:0,lastDay:''};__alarmDraft=null;__alarmSelection=0;__alarmUntil=0;__alarmSound=false;}
function __alarmTick(now){
    if(now<__alarmLastWall){__alarmUntil=0;__alarmSound=false;}
    __alarmLastWall=now;
    const date=new Date(now), day=date.getFullYear()+'-'+(date.getMonth()+1)+'-'+date.getDate();
    if(__alarm.enabled && date.getHours()===__alarm.hour && date.getMinutes()===__alarm.minute && __alarm.lastDay!==day){
        __alarm.lastDay=day;__alarmStore();__alarmUntil=now+30000;__alarmNextSound=now;
    }
    if(__alarmUntil && now>=__alarmUntil){__alarmUntil=0;__alarmSound=false;}
    if(__alarmUntil && now>=__alarmNextSound){__alarmSound=true;__alarmNextSound=now+1000;}
}
function __takeAlarmSound(){const sound=__alarmSound;__alarmSound=false;return sound;}
function __alarmPress(button){
    if(button==='reset'){__alarmReset();return false;}
    if(button==='shake')return false;
    if(__alarmUntil){__alarmUntil=0;__alarmSound=false;return true;}
    if(__alarmDraft){
        if(button==='back'){__alarmDraft=null;return true;}
        if(button==='enter'){
            if(__alarm.hour!==__alarmDraft.hour || __alarm.minute!==__alarmDraft.minute || __alarm.enabled!==__alarmDraft.enabled)__alarm.lastDay='';
            Object.assign(__alarm,__alarmDraft);__alarmDraft=null;__alarmStore();return true;
        }
        if(button==='left'||button==='right'){__alarmField=1-__alarmField;return true;}
        if(button==='top'||button==='bottom'){
            const change=button==='top'?1:-1;
            if(__alarmField===0)__alarmDraft.hour=(__alarmDraft.hour+change+24)%24;
            else __alarmDraft.minute=(__alarmDraft.minute+change+60)%60;
            return true;
        }
        return true;
    }
    if(animStatus==='clock'){
        if(button==='enter'){
            if(__alarmSelection===0){__alarm.enabled=!__alarm.enabled;__alarmStore();}
            else{__alarmDraft={enabled:__alarm.enabled,hour:__alarm.hour,minute:__alarm.minute};__alarmField=0;}
            return true;
        }
        if(button==='top'||button==='bottom'){__alarmSelection=__alarm.enabled&&button==='bottom'?1:0;return true;}
        if(button==='back')__alarmSelection=0;
    }
    return false;
}
var __alarmFont={
 '0':['111','101','101','101','111'],'1':['010','110','010','010','111'],'2':['111','001','111','100','111'],
 '3':['111','001','111','001','111'],'4':['101','101','111','001','001'],'5':['111','100','111','001','111'],
 '6':['111','100','111','101','111'],'7':['111','001','010','010','010'],'8':['111','101','111','101','111'],
 '9':['111','101','111','001','111'],'A':['010','101','111','101','101'],'L':['100','100','100','100','111'],
 'R':['110','101','110','101','101'],'M':['101','111','111','101','101'],'P':['110','101','110','100','100'],
 'O':['111','101','101','101','111'],'N':['101','111','111','111','101'],'F':['111','100','110','100','100'],
 'S':['111','100','111','001','111'],'E':['111','100','110','100','111'],'T':['111','010','010','010','010'],
 'I':['111','010','010','010','111'],'U':['101','101','101','101','111'],'D':['110','101','101','101','110'],
 'C':['111','100','100','100','111'],'>':['100','110','111','110','100'],
 ':':['0','1','0','1','0'],' ':['00','00','00','00','00']
};
function __alarmText(pixels,text,x,y,scale){
    for(const character of text){const glyph=__alarmFont[character]||__alarmFont[' '];
        for(let row=0;row<5;row++)for(let col=0;col<glyph[row].length;col++)if(glyph[row][col]==='1')
            for(let dy=0;dy<scale;dy++)for(let dx=0;dx<scale;dx++){
                const xx=x+col*scale+dx,yy=y+row*scale+dy;if(xx>=0&&xx<36&&yy>=0&&yy<30)pixels[yy*36+xx]=2;
            }
        x+=(glyph[0].length+1)*scale;
    }
}
function __alarmTime(value){return String(value.hour%12||12).padStart(2,'0')+':'+String(value.minute).padStart(2,'0');}
function __alarmDisplay(pixels){
    const now=__alarmNow();
    if(__alarmDraft||__alarmUntil){
        const value=__alarmDraft||__alarm;pixels=new Array(1080).fill(0);
        __alarmText(pixels,'ALARM',1,0,1);__alarmText(pixels,value.hour>=12?'PM':'AM',27,0,1);
        __alarmText(pixels,__alarmTime(value),0,8,2);
        if(__alarmDraft)__alarmText(pixels,value.enabled?'ON':'OFF',value.enabled?14:12,24,1);
        else __alarmText(pixels,'STOP',15,24,1);
        if(__alarmDraft){
            const start=__alarmField===0?0:__alarmField===1?20:11,end=__alarmField===0?13:__alarmField===1?33:24,row=__alarmField===2?29:19;
            for(let x=start;x<=end;x++)pixels[row*36+x]=2;
        }else if(Math.floor(now/300)%2===0){
            for(const pixel of Anims.clock.alarmOff){const n=Number(pixel.slice(4));if(n%36<17)pixels[n+9*36-7]=2;}
        }
    }else if(animStatus==='clock'&&!__deviceUi){
        if(__alarm.enabled){
            for(let y=14;y<=21;y++)for(let x=17;x<36;x++)pixels[y*36+x]=0;
            __alarmText(pixels,'ON',19,15,1);
            __alarmText(pixels,__alarmTime(__alarm)+' '+(__alarm.hour>=12?'PM':'AM'),3,24,1);
        }
        __alarmText(pixels,'>',0,__alarmSelection===0?15:24,1);
    }
    return pixels;
}
