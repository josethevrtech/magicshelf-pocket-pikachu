#include "emulator.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QDateTime>
#include <iostream>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void seed(const QString& path,int total,int watts,int remainder=0){
 QJsonObject storage{{"InitTamagotchi","true"},{"steps","123"},{"totalSteps",QString::number(total)},
  {"watts",QString::number(watts)},{"stepsToConvert",QString::number(remainder)},{"friendshipLevel","1000"},{"dificultyLevel","easy"}};
 QFile file(path+"/save.json");require(file.open(QIODevice::WriteOnly),"Seed temporary profile");
 file.write(QJsonDocument(QJsonObject{{"schema",1},{"storage",storage},{"cookies",QJsonObject{}}}).toJson());
}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 QTemporaryDir data;
 seed(data.path(),9999,500,19);
 {
  Emulator game(argv[1],data.path());game.tick(1000);game.evaluate("walk()");
  require(game.snapshot().property("totalSteps").toInt()==10000&&!game.snapshot().property("complete").toBool(),"Old easy saves continue past the fan emulator's 10K ending");
  require(game.snapshot().property("watts").toInt()==501,"Carried partial steps convert to watts");
  game.press("menu");game.press("top");game.press("enter");
  require(game.snapshot().property("steps").toInt()==0&&game.snapshot().property("watts").toInt()==501,"Counter reset preserves watts");
  game.save();
 }
 for(int threshold:{150000,300000,450000}){
  seed(data.path(),threshold-1,50);
  Emulator game(argv[1],data.path());game.tick(1000);game.evaluate("walk()");
  require(game.snapshot().property("unlocks").property(threshold/150000-1).toBool(),"Animation milestone unlocks in the same process at its boundary");
  if(threshold==300000)require(game.snapshot().property("sleepHour").toInt()==21,"New daily routine activates immediately with the milestone");
  game.save();
 }
 seed(data.path(),999998,9999,19);
 {
  Emulator game(argv[1],data.path());game.tick(1000);game.evaluate("walk()");
  require(game.snapshot().property("totalSteps").toInt()==999999&&game.snapshot().property("complete").toBool(),"Original lifetime limit is 999,999 steps");
  require(game.snapshot().property("watts").toInt()==9999,"Watt balance stays inside its four-digit LCD");
  game.evaluate("walk()");require(game.snapshot().property("totalSteps").toInt()==999999,"Further motion cannot overflow the total");
  game.press("enter");require(game.snapshot().property("pixels").property("length").toInt()==1080,"Completion record still renders");
 }
 seed(data.path(),200,800);
 {
  Emulator game(argv[1],data.path());game.tick(1000);
  game.evaluate("__device.offset=3600000;__deviceStore();__alarm.enabled=true;__alarm.hour=6;__alarm.minute=30;__alarmStore()");game.save();
 }
 {
  Emulator game(argv[1],data.path());
  require(game.evaluate("__device.offset").toInt()==3600000,"Toy clock adjustment survives restart");
  require(game.snapshot().property("alarmHour").toInt()==6&&game.snapshot().property("alarmMinute").toInt()==30,"Clock storage preserves existing daily alarm");
  require(game.snapshot().property("friendship").toInt()==1000&&game.snapshot().property("watts").toInt()==800,"Clock change does not erase or penalize progress");
  require(game.evaluate("__calendarDays(new Date(2026,2,8),new Date(2026,2,9))===1").toBool(),"Calendar days do not lose a day at DST");
 }
 std::cout<<"Legacy-save migration, watt conversion, live unlocks, original step limit and clock persistence passed\n";return 0;
}catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
