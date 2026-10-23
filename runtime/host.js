// Minimal pixel-display host for Alberto-rp's unmodified emulator.
// No HTML renderer, browser, network, telemetry, or web server is involved.
var __storage = Object.assign(Object.create(null), __saved.storage || {});
var __cookies = Object.assign(Object.create(null), __saved.cookies || {});
// Qt does not parse the browser's toDateString() save format consistently.
// Parse those legacy local dates explicitly; ISO saves still use Date itself.
function __savedDate(value) {
    const m = /^(?:Mon|Tue|Wed|Thu|Fri|Sat|Sun) (Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) (\d{1,2}) (\d{4})(?: (\d{1,2}):(\d{2}))?$/.exec(String(value));
    if (m) return new Date(Number(m[3]), 'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec'.split(' ').indexOf(m[1]), Number(m[2]), Number(m[4] || 0), Number(m[5] || 0));
    return new Date(value);
}
function __calendarDays(before,after){
    return Math.floor((Date.UTC(after.getFullYear(),after.getMonth(),after.getDate())-Date.UTC(before.getFullYear(),before.getMonth(),before.getDate()))/86400000);
}
var localStorage = {
    getItem: k => Object.prototype.hasOwnProperty.call(__storage, k) ? __storage[k] : null,
    setItem: (k, v) => { __storage[k] = String(v); },
    removeItem: k => { delete __storage[k]; },
    clear: () => { __storage = Object.create(null); }
};
var __logs = [];
var console = {log: (...args) => { if (__logs.length >= 32) __logs.shift(); __logs.push(args.join(' ')); }};
console.error = console.warn = console.log;
var gtag = () => {}; // Upstream analytics hook intentionally has no transport.
var alert = console.log;
var __time = 0, __nextTimer = 1, __timers = new Map();
function __timer(fn, delay, repeat, args) {
    if (typeof fn !== 'function') throw Error('Only function timers are supported');
    const id = __nextTimer++, duration = Math.max(1, Number(delay) || 1);
    __timers.set(id, {fn, at: __time + duration, duration, repeat, args});
    return id;
}
var setTimeout = (fn, delay, ...args) => __timer(fn, delay, false, args);
var setInterval = (fn, delay, ...args) => __timer(fn, delay, true, args);
var clearTimeout = id => __timers.delete(id);
var clearInterval = clearTimeout;
// The original A action waits 500 ms. Track its timers so Back, Select,
// Start or reset cannot leave a queued action targeting a different screen.
var __enterTimers=[];
function __queueEnter(fn,delay){
    const id=setTimeout(()=>{__enterTimers=__enterTimers.filter(timer=>timer!==id);fn();},delay);
    __enterTimers.push(id);return id;
}
function __cancelEnter(){__enterTimers.forEach(clearTimeout);__enterTimers=[];}
function __advance(ms) {
    const end = __time + ms;
    let budget = 10000;
    while (budget-- > 0) {
        let next, id;
        for (const [key, timer] of __timers) {
            if (timer.at <= end && (!next || timer.at < next.at)) { next = timer; id = key; }
        }
        if (!next) { __time = end; __deviceTick(); __alarmTick(__alarmNow()); return; }
        __time = next.at;
        if (next.repeat) next.at += next.duration; else __timers.delete(id);
        next.fn(...next.args);
    }
    throw Error('Emulator timer budget exceeded');
}
class Classes extends Array {
    add(...names) { names.forEach(name => { if (!this.includes(name)) this.push(name); }); }
    remove(...names) { names.forEach(name => { const i = this.indexOf(name); if (i >= 0) this.splice(i, 1); }); }
    contains(name) { return this.includes(name); }
}
class Element {
    constructor(classes = '', id = '') {
        this.classList = new Classes(); this.classList.add(...classes.split(' ').filter(Boolean));
        this.id = id; this.children = []; this.style = {}; this.events = {}; this.html = '';
    }
    get innerHTML() { return this.html; }
    set innerHTML(value) { this.html = String(value); this.children = []; }
    get textContent() { return this.html; }
    set textContent(value) { this.innerHTML = value; }
    appendChild(child) { this.children.push(child); return child; }
    addEventListener(type, fn) { (this.events[type] || (this.events[type] = [])).push(fn); }
    removeEventListener(type, fn) { this.events[type] = (this.events[type] || []).filter(f => f !== fn); }
    dispatch(type) { for (const fn of (this.events[type] || []).slice()) fn({target: this}); }
    click() { this.dispatch('click'); }
    showModal() {} // Website release notes are not part of the handheld display.
    matches(selector) {
        if (selector.startsWith('#')) return this.id === selector.slice(1);
        return selector.startsWith('.') && selector.slice(1).split('.').every(c => this.classList.contains(c));
    }
    querySelectorAll(selector) {
        const result = [];
        for (const alternative of selector.split(',')) {
            const parts = alternative.trim().split(/\s+/);
            function visit(node, index) {
                for (const child of node.children) {
                    if (child.matches(parts[index])) {
                        if (index === parts.length - 1) result.push(child);
                        else visit(child, index + 1);
                    }
                    visit(child, index);
                }
            }
            visit(this, 0);
        }
        return [...new Set(result)];
    }
    querySelector(selector) {
        // All display rows are a fixed linear pixel array. Direct lookup keeps
        // slot animations and number drawing inexpensive on Frame's JS engine.
        if (/^\.num-\d+$/.test(selector)) return this.children[Number(selector.slice(5))] || null;
        return this.querySelectorAll(selector)[0] || null;
    }
}
var document = new Element();
document.createElement = () => new Element();
Object.defineProperty(document, 'cookie', {
    get() {
        return Object.entries(__cookies).filter(([k,v]) => v.expires > Date.now())
            .map(([k,v]) => k + '=' + v.value).join('; ');
    },
    set(value) {
        const parts = String(value).split(';'), eq = parts[0].indexOf('=');
        if (eq < 0) return;
        const key = parts[0].slice(0, eq).trim(), val = parts[0].slice(eq + 1);
        const expiry = parts.find(p => p.trim().toLowerCase().startsWith('expires='));
        const date = expiry ? Date.parse(expiry.trim().slice(8).replace(/^"|"$/g, '')) : Number.MAX_SAFE_INTEGER;
        if (date <= Date.now()) delete __cookies[key];
        else __cookies[key] = {value: val, expires: Number.isFinite(date) ? date : Number.MAX_SAFE_INTEGER};
    }
});
function element(parent, classes, id) { return parent.appendChild(new Element(classes, id)); }
var __screenContainer = element(document, 'screenContainer');
var __menu = element(__screenContainer, 'menuBar');
['clockMenu','giftMenu','gameMenu'].forEach(id => element(__menu, 'img-menu', id));
element(__menu, 'walkCounter');
var __screen = element(__screenContainer, 'screen');
var __petFrame=null;
function __readPixels(){
    return __screen.children.map(p=>p.classList.contains('clicked')?(p.classList.contains('halfClicked')?1:2):0);
}
function __rememberPetFrame(){__petFrame=__readPixels();}
function __restorePetFrame(){
    // Some pet animations draw their first frame on a later timer. Restore
    // the previous pet immediately instead of leaving a menu on the LCD.
    if(!__petFrame)return;
    __screen.innerHTML='';
    __petFrame.forEach((level,n)=>{
        const pixel=element(__screen,'pixel num-'+n);
        if(level)pixel.classList.add('clicked');
        if(level===1)pixel.classList.add('halfClicked');
    });
    __petFrame=null;
}
var __buttons = element(document, 'buttonsContainer');
['enter-button','back-button','state-button','menu-button','right-button','left-button','top-button','bottom-button','reset-button','shake','toggle-pedometer'].forEach(id => element(__buttons, '', id));
var __developer = element(document, 'developerScreen');
element(__developer, 'createScreen');
['saveAnimation','printInit','editPrint','eraser','startAnim','showGrid','showGridVer'].forEach(id => element(__developer, '', id));
var __modal = element(document, '', 'updateModal');
element(__modal, 'version').innerHTML = 'Ver.4.0.0';
element(__modal, '', 'closeModal');
element(document, 'preloader');
var navigator = {platform: 'SteamOS', userAgent: 'MagicShelf', vibrate: () => true, clipboard: {writeText: () => {}}};
var window = new Element(); window.navigator = navigator; window.location = {href:'magicshelf:local'};
// Resolve only the one bundled data resource. The unmodified upstream loader
// uses two synchronous callbacks; initialization is dispatched after loading.
function fetch(path) {
    if (path !== './anims.json') throw Error('Network access is unavailable: ' + path);
    function resolved(value) { return {then: fn => resolved(fn(value))}; }
    return resolved({json: () => __animations});
}
function __start() {
    document.dispatch('DOMContentLoaded'); window.dispatch('load');
    // A saved handheld powers up with its LCD awake. First boot still hatches.
    if (localStorage.getItem('InitTamagotchi') === 'true' && animStatus === '') __press('enter');
}
function __press(button) {
    if (!['enter','back','state','menu','right','left','top','bottom','reset','shake'].includes(button)) throw Error('Unknown button');
    // Do not cancel the initial wake-up for an otherwise ignored B/Select.
    if((animStatus!==''||button==='reset')&&['enter','back','state','menu','reset'].includes(button))__cancelEnter();
    if(button==='reset')__petFrame=null;
    if(__alarmUntil && button!=="shake" && button!=="reset"){__alarmPress(button);return;}
    if(__devicePress(button)||__alarmPress(button))return;
    __dispatch(button);
}
function __dispatch(button){
    document.querySelector('#' + (button === 'shake' ? button : button + '-button')).click();
}
function __snapshot() {
    return {
        pixels: __alarmDisplay(__deviceDisplay(__readPixels())),
        counter: document.querySelector('.walkCounter').innerHTML,
        icons: ['clockMenu','giftMenu','gameMenu'].map(id => document.querySelector('#'+id).classList.contains('selected')),
        status: animStatus, steps: pokeStatus.steps, totalSteps: pokeStatus.totalSteps, watts: pokeStatus.watts,
        friendship: pokeStatus.friendshipLevel,unlocks:[hasReach150,hasReach300,hasReach450],complete:pokeStatus.reachEnd,sleepHour:pokeStatus.sleepHour,timerCount: __timers.size,
        soundEnabled:__device.sound,deviceUi:__deviceUi,deviceSelection:__deviceSelection,clockEditing:!!__clockDraft,clockHour:__clockDraft?__clockDraft.hour:-1,clockMinute:__clockDraft?__clockDraft.minute:-1,
        alarmEnabled:__alarm.enabled,alarmHour:__alarm.hour,alarmMinute:__alarm.minute,alarmEditing:!!__alarmDraft,alarmField:__alarmField,alarmRinging:!!__alarmUntil
    };
}
function __save() { return JSON.stringify({schema: 1, storage: __storage, cookies: __cookies}); }
function __installPixelRenderer() {
    // These two upstream functions only manipulate display pixels. Replacing
    // their repeated linear searches with a Set preserves their output exactly.
    loadAnim = function(screen, anim, clear, full) {
        const lit = new Set(clear ? [] : anim || []);
        screen.innerHTML = '';
        for(let i=0;i<1080;i++){
            const pixel=new Element('pixel num-'+i);
            if(full||lit.has('num-'+i))pixel.classList.add('clicked');
            screen.appendChild(pixel);
        }
    };
    loadLocatedAnim = function(screen, anim) {
        for(const name of new Set(anim)){
            const pixel=screen.querySelector('.'+name);
            if(pixel)pixel.classList.add('clicked');
        }
    };
}
