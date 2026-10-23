// Optional on-device check using a temporary pet, real wall time and native audio.
#include "emulator.h"
#include "magicshelf_sdk.h"
#include "lcd.h"
#include "audio.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <chrono>
#include <thread>
#include <iostream>
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
    if(argc!=3)throw std::runtime_error("Usage: check_alarm PACKAGE OUTPUT_DIRECTORY");
    QTemporaryDir data;Emulator game(argv[1],data.path());Renderer renderer(argv[1]);
    std::filesystem::create_directories(argv[2]);
    auto capture=[&](const char* name){Canvas lcd(ScreenWidth,ScreenHeight);drawLCD(lcd,game.snapshot());renderer.screen(lcd);
        renderer.draw(identity(),yawPose(-.18f),{0,0,.3f},{0,0,.3f},.115f,1);renderer.save(std::string(argv[2])+"/"+name+".png");};
    game.tick(3500);game.press("enter");game.tick(4000);capture("first-clock");game.press("enter");
    game.press("menu");capture("settings-sound-on");game.press("enter");capture("settings-sound-off");
    game.press("bottom");game.press("enter");capture("time-setting");game.press("back");capture("time-back-settings");
    game.press("back");capture("settings-back-immediate");game.tick(1000);
    game.press("menu");game.press("enter");game.press("back");game.tick(1000);game.press("enter");game.tick(1000);
    capture("alarm-clock");game.press("enter");game.press("bottom");game.press("enter");capture("alarm-setup");game.press("back");
    capture("alarm-back-clock");game.press("back");capture("clock-back-immediate");game.tick(1000);
    HandheldAudio audio;
    game.buttonSound=[&](const QString& button,const QString& state){audio.button(button.toStdString(),state.toStdString());};
    game.evaluate("__alarm={enabled:true,hour:new Date(__deviceNow()).getHours(),minute:new Date(__deviceNow()).getMinutes(),lastDay:''};__alarmStore();");
    game.tick(25);
    if(!game.snapshot().property("alarmRinging").toBool())throw std::runtime_error("Real-time alarm did not ring");
    capture("alarm-ringing");
    for(int n=0;n<100&&!audio.count();++n)std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if(!audio.count())throw std::runtime_error("Native alarm audio did not play");
    game.press("back");game.save();
    if(game.snapshot().property("alarmRinging").toBool())throw std::runtime_error("Alarm did not dismiss");
    std::cout<<"Real wall-clock alarm, native rendering, audio playback and dismissal passed with an isolated save\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
