#include "emulator.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <iostream>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void clockMenu(Emulator& game){game.press("enter");game.tick(1000);require(game.snapshot().property("status").toString()=="clock","Clock opens with A");}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 QTemporaryDir data;int rings=0;
 {
  Emulator game(argv[1],data.path());game.evaluate("var __testTime=new Date(2026,9,21,6,59,58).getTime();__alarmNow=()=>__testTime;");
  game.buttonSound=[&](const QString& button,const QString&){if(button=="alarm")++rings;};
  game.tick(3500);game.press("enter");game.tick(4000);game.press("enter");clockMenu(game);
  game.press("enter");require(game.snapshot().property("alarmEnabled").toBool(),"A toggles alarm ON directly from the clock");
  game.press("bottom");game.press("enter");require(game.snapshot().property("alarmEditing").toBool(),"Down then A edits the alarm time");
  game.press("bottom");game.press("right");game.press("bottom");game.press("back");
  require(game.snapshot().property("alarmHour").toInt()==7&&game.snapshot().property("alarmMinute").toInt()==0,"B cancels time edits");
  game.tick(10);require(rings==0,"Alarm does not ring early");
  game.evaluate("__testTime+=2000");game.tick(10);require(rings==1&&game.snapshot().property("alarmRinging").toBool(),"Alarm rings at its chosen time");
  game.evaluate("__testTime+=1000");game.tick(10);require(rings==2,"Alarm repeats its cue");
  game.press("shake");require(game.snapshot().property("alarmRinging").toBool(),"Steps do not accidentally dismiss the alarm");
  game.press("back");require(!game.snapshot().property("alarmRinging").toBool(),"A physical button dismisses ringing without navigating");game.save();
 }
 {
  Emulator game(argv[1],data.path());game.evaluate("var __testTime=new Date(2026,9,21,7,0,5).getTime();__alarmNow=()=>__testTime;");
  game.buttonSound=[&](const QString& button,const QString&){if(button=="alarm")++rings;};
  game.tick(1000);require(rings==2&&game.snapshot().property("alarmEnabled").toBool(),"Restart preserves alarm without ringing twice");
  game.evaluate("__testTime=new Date(2026,9,22,7,0,0).getTime()");game.tick(10);require(rings==3,"Next day rings outside the clock view");
  game.evaluate("__testTime+=30000");game.tick(10);require(!game.snapshot().property("alarmRinging").toBool(),"Ringing expires after thirty seconds");
  game.evaluate("__testTime=new Date(2026,9,23,7,0,0).getTime()");game.tick(10);require(rings==4,"Alarm fires after a previous expiry");
  game.evaluate("__testTime-=60000");game.tick(10);require(!game.snapshot().property("alarmRinging").toBool(),"Clock moving backward cannot extend ringing");
  game.press("menu");game.press("enter");game.press("back");game.tick(1000);
  game.evaluate("__testTime=new Date(2026,9,24,7,0,0).getTime()");game.tick(10);
  require(rings==4&&game.snapshot().property("alarmRinging").toBool(),"Sound OFF leaves a silent visual alarm");
  game.press("back");clockMenu(game);game.press("enter");
  require(!game.snapshot().property("alarmEnabled").toBool(),"A toggles the alarm OFF");
  game.evaluate("__testTime=new Date(2026,9,25,7,0,0).getTime()");game.tick(10);require(!game.snapshot().property("alarmRinging").toBool(),"Disabled alarm stays off");
  game.press("enter");game.press("bottom");game.press("enter");
  for(int i=0;i<8;i++)game.press("bottom");game.press("right");game.press("bottom");game.press("enter");
  require(game.snapshot().property("alarmHour").toInt()==23&&game.snapshot().property("alarmMinute").toInt()==59,"Hours and minutes wrap across midnight");
  game.press("reset");require(!game.snapshot().property("alarmEnabled").toBool()&&!game.snapshot().property("alarmEditing").toBool(),"Full reset clears the alarm");
 }
 std::cout<<"Original clock controls, alarm editing, daily ring, mute, cancel, expiry and persistence passed\n";return 0;
}catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
