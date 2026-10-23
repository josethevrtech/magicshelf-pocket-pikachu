#include "emulator.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <functional>
#include <vector>

void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void hatch(Emulator& game){
    game.tick(3500);game.press("enter");game.tick(4000);game.press("enter");
}
void noLegacyPixels(Emulator& game){
    require(game.evaluate("!__testLegacy.includes(JSON.stringify(__snapshot().pixels))").toBool(),
            "A legacy difficulty/relationship menu flashed on the LCD");
}
void inspectFrames(Emulator& game,int duration){
    noLegacyPixels(game);
    for(int ms=0;ms<duration;ms+=25){game.tick(25);noLegacyPixels(game);}
}
int main(int argc,char**argv){
    QCoreApplication app(argc,argv);int failures=0;
    using Check=std::pair<const char*,std::function<void(Emulator&)>>;
    const std::vector<Check> checks{
        {"Time editor and settings Back frames",[](Emulator& game){
            game.press("menu");game.press("bottom");game.press("enter");
            require(game.snapshot().property("clockEditing").toBool(),"Time editor must open");
            game.press("back");
            require(game.snapshot().property("deviceUi").toString()=="settings","B returns to native settings");
            inspectFrames(game,600);game.press("back");inspectFrames(game,1500);
        }},
        {"A then immediate Back cancels opening",[](Emulator& game){
            game.press("enter");game.tick(100);game.press("back");inspectFrames(game,1500);
            require(game.snapshot().property("status").toString()!="clock","Cancelled A must not reopen the clock");
        }},
        {"A cannot reopen after a settings round trip",[](Emulator& game){
            game.press("enter");game.tick(100);game.press("menu");game.press("back");
            inspectFrames(game,1500);
            require(game.snapshot().property("status").toString()!="clock","Stale A must not open clock after leaving settings");
        }},
        {"Clock Back restores pet immediately",[](Emulator& game){
            game.press("enter");game.tick(600);
            require(game.snapshot().property("status").toString()=="clock","A opens clock");
            const auto clock=game.evaluate("JSON.stringify(__snapshot().pixels)").toString();
            game.press("back");
            require(game.evaluate("JSON.stringify(__snapshot().pixels)").toString()!=clock&&
                    game.evaluate("__snapshot().pixels.some(level=>level!==0)").toBool(),
                    "Back must restore the pet before its next animation frame");
            inspectFrames(game,1500);
        }},
        {"Alarm editor Back and motion preserve settings",[](Emulator& game){
            game.press("enter");game.tick(600);game.press("enter");
            game.press("bottom");game.press("enter");
            require(game.snapshot().property("alarmEditing").toBool(),"Alarm editor opens");
            game.press("top");game.press("shake");game.tick(300);game.press("back");
            require(game.snapshot().property("status").toString()=="clock"&&
                    !game.snapshot().property("alarmEditing").toBool(),"B cancels editor and stays in clock");
            require(game.snapshot().property("alarmHour").toInt()==7,"Cancelling does not save draft time");
            game.press("back");inspectFrames(game,1300);
            game.press("menu");game.press("bottom");game.press("enter");
            game.press("shake");game.tick(300);game.press("back");game.press("back");inspectFrames(game,1300);
            require(game.snapshot().property("totalSteps").toInt()==2,"Steps continue during both editors");
            require(game.snapshot().property("alarmEnabled").toBool(),"Back preserves enabled alarm");
            require(game.snapshot().property("soundEnabled").toBool(),"Navigation preserves sound");
        }},
        {"Repeated clock and settings navigation",[](Emulator& game){
            for(int n=0;n<20;n++){
                game.press("menu");game.press("bottom");game.press("enter");
                game.press(n%2?"enter":"back");game.press("back");inspectFrames(game,1300);
                game.press("enter");game.tick(600);
                require(game.snapshot().property("status").toString()=="clock","Clock remains reachable");
                game.press("back");inspectFrames(game,1300);
                require(game.snapshot().property("status").toString()!="clock","B leaves clock");
            }
            require(game.snapshot().property("timerCount").toInt()<10,"Menu cycles must not accumulate timers");
            require(game.snapshot().property("watts").toInt()==50,"Navigation cannot spend watts");
            require(game.snapshot().property("totalSteps").toInt()==0,"Navigation cannot add steps");
        }}
    };
    for(const auto& check:checks){
        try{QTemporaryDir data;
            {Emulator fresh(argv[1],data.path());hatch(fresh);fresh.save();}
            Emulator game(argv[1],data.path());
            // Exercise a restored pet during a routine whose first animation
            // frame arrives on an interval, not the special first-hatch stand.
            game.evaluate(R"JS(
                __deviceNow=()=>new Date(2026,9,22,12,15,0).getTime();
                Math.random=()=>0.5;
                var __testLegacy=[];
                ['settings','settingsDiff','settingsRel'].forEach(group=>
                    Object.values(__animations[group]).forEach(frame=>{
                        const lit=new Set(frame);
                        __testLegacy.push(JSON.stringify(Array.from({length:1080},(_,n)=>lit.has('num-'+n)?2:0)));
                    }));
            )JS");
            game.tick(100);game.press("back");game.tick(1900);check.second(game);
            std::cout<<"PASS: "<<check.first<<"\n";
        }catch(const std::exception& e){++failures;std::cerr<<"FAIL: "<<check.first<<": "<<e.what()<<"\n";}
    }
    return failures?1:0;
}
