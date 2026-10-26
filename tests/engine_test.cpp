#include "emulator.h"
#include "controls.h"
#include "audio.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
#include <map>
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
// Centers on the supplied handheld, in imported model coordinates. Exercise
// the physical hit mapping through the real emulator, rather than only IDs.
void physical(Emulator& game,const QString& name) {
    static const std::map<QString,V> centers{{"enter",{.021847f,-.014902f,.022905f}},{"back",{.009377f,-.020182f,.023191f}},
        {"state",{-.006903f,-.031418f,.022538f}},{"menu",{.007442f,-.031412f,.022538f}},{"reset",{.019418f,-.027554f,.019554f}},
        {"left",{-.0245f,-.0199f,.0228f}},{"right",{-.0125f,-.0199f,.0228f}},
        {"top",{-.0185f,-.0139f,.0228f}},{"bottom",{-.0185f,-.0259f,.0228f}}};
    auto target=centers.at(name);
    for(float x:{-.20f,0.f,.20f}){
        V origin{x,.04f,.30f};
        require(QString::fromStdString(buttonRay(origin,sub(target,origin)))==name,"Front and angled laser rays must hit the model's physical control");
    }
    V behind{0,0,-.3f};require(buttonRay(behind,sub(target,behind)).empty(),"Buttons cannot be pressed through the back");
    game.press(QString::fromStdString(buttonRay({0,0,.3f},sub(target,{0,0,.3f}))));
}
int main(int argc,char**argv) {
    QCoreApplication app(argc,argv);
    try {
        QTemporaryDir data;int steps=0;
        {
            Emulator game(argv[1],data.path());
            require(game.snapshot().property("pixels").property("length").toInt()==1080,"LCD must be 36 x 30 pixels");
            require(game.snapshot().property("status").toString()=="restart","Original power-on LCD test");
            game.tick(3500);require(game.snapshot().property("status").toString()=="pokeball","Original Pokeball startup");
            game.press("enter");game.tick(4000);
            require(game.snapshot().property("clockEditing").toBool(),"Hatching must offer the original time setting");
            game.press("enter");require(!game.snapshot().property("clockEditing").toBool(),"A saves the first clock setting");
            for(int i=0;i<20;i++){game.press("shake");game.tick(300);}
            game.tick(1500);steps=game.snapshot().property("steps").toInt();
            require(steps==20&&game.snapshot().property("watts").toInt()==51,"Every twenty steps earns exactly one watt");
            require(game.evaluate("__snapshot().pixelLevels===__snapshot().pixels.join('') && __snapshot().counter===document.querySelector('.walkCounter').innerHTML && JSON.stringify(__snapshot().icons)===JSON.stringify(['clockMenu','giftMenu','gameMenu'].map(id=>document.querySelector('#'+id).classList.contains('selected')))").toBool(),"Fast display snapshot agrees with pixel data, counter and selected icons");
            game.tick(65000);require(!game.snapshot().property("status").toString().isEmpty(),"The requested continuous LCD power survives idle time");
            require(game.evaluate("Number.isFinite(Number(calculatePastHours()))").toBool(),"Elapsed time parses saved hatch date");
            game.press("state");require(game.snapshot().property("status").toString()=="state","Friendship button");
            game.press("back");game.tick(1000);game.press("menu");
            require(game.snapshot().property("deviceUi").toString()=="settings","Start opens the original three-item menu");
            game.press("enter");require(!game.snapshot().property("soundEnabled").toBool(),"Middle menu item toggles sound off");
            game.evaluate("document.cookie='test_expiry=yes; expires=Wed, 01 Jan 2099 00:00:00 GMT'");
            game.evaluate("document.cookie='expired=no; expires=Thu, 01 Jan 1970 00:00:00 GMT'");game.save();
        }
        {
            Emulator restored(argv[1],data.path());
            require(restored.snapshot().property("steps").toInt()==steps,"Steps survive process restart");
            require(restored.snapshot().property("friendship").toInt()==0,"Same-day restart cannot make Pikachu leave");
            require(!restored.snapshot().property("soundEnabled").toBool(),"Sound setting survives restart");
            require(!restored.snapshot().property("clockEditing").toBool(),"Existing saves are not forced through first boot");
            require(restored.evaluate("document.cookie.includes('test_expiry=yes') && !document.cookie.includes('expired=no')").toBool(),"Cookie expiry and persistence");
            restored.tick(1000);restored.press("enter");restored.tick(1000);
            require(restored.snapshot().property("status").toString()=="clock","Clock menu");
            restored.press("back");restored.tick(1000);restored.press("right");restored.press("enter");restored.tick(1000);
            require(restored.snapshot().property("status").toString()=="gift","Gift menu");
            restored.press("back");restored.tick(1000);restored.press("right");restored.press("right");restored.press("enter");restored.tick(1000);
            require(restored.snapshot().property("status").toString()=="game","Slot machine menu");
            for(int i=0;i<4;i++){restored.press("enter");restored.tick(1500);}restored.save();
        }
        {
            Emulator game(argv[1],data.path());game.tick(1000);int audioCount=0;
            game.buttonSound=[&](const QString& button,const QString& state){
                auto wave=HandheldAudio::waveform(button.toStdString(),state.toStdString());
                require(!wave.empty(),"Audible button generates a cue");int peak=0;
                for(auto sample:wave)peak=std::max(peak,std::abs(int(sample)));
                require(peak>0&&peak<=2600,"Audio PCM level is bounded");++audioCount;
            };
            physical(game,"menu");require(audioCount==0,"Muted physical buttons stay silent");
            physical(game,"enter");require(audioCount==1,"Turning sound on produces feedback");
            physical(game,"top");physical(game,"enter");
            require(game.snapshot().property("steps").toInt()==0&&game.snapshot().property("totalSteps").toInt()==steps,"RESET clears only the visible counter");
            physical(game,"bottom");physical(game,"bottom");physical(game,"enter");
            require(game.snapshot().property("clockEditing").toBool(),"Third settings item opens the time editor");
            int hour=game.snapshot().property("clockHour").toInt();physical(game,"top");
            require(game.snapshot().property("clockHour").toInt()==(hour+1)%24,"Up adjusts the toy clock");
            physical(game,"right");physical(game,"bottom");physical(game,"left");physical(game,"enter");
            require(game.evaluate("new Date(__deviceNow()).getHours()").toInt()==(hour+1)%24,"Saving clock time changes the toy time");
            require(game.evaluate("Math.abs(__device.offset)<=86400000").toBool(),"Clock offset is bounded to one day");
            physical(game,"back");game.tick(1000);
            physical(game,"enter");physical(game,"state");game.tick(600);
            require(game.snapshot().property("status").toString()=="state","Stale A callback must not replace friendship view");
            physical(game,"reset");game.tick(3500);
            require(game.snapshot().property("status").toString()=="pokeball","Recessed reset returns to startup");
            require(game.evaluate("__device.sound && __device.offset===0 && !__snapshot().unlocks[1]").toBool(),"Full reset clears sound, clock and unlock state");
            physical(game,"enter");game.tick(4000);physical(game,"enter");
            int before=audioCount;game.press("shake");require(audioCount==before,"Headset steps remain silent");
        }
        QFile bad(data.path()+"/save.json");require(bad.open(QIODevice::WriteOnly),"Open test save");bad.write("bad save");bad.close();
        bool rejected=false;try{Emulator broken(argv[1],data.path());}catch(...){rejected=true;}
        require(rejected&&Emulator::read(data.path()+"/save.json")=="bad save","Corrupt saves must not be overwritten");
        std::cout<<"Native emulator: physical buttons, startup clock, sound, menus, steps and persistence passed\n";return 0;
    }catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}
}
