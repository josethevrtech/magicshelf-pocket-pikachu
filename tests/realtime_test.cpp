#include "realtime_emulator.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <atomic>
#include <iostream>
void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F> void waitFor(F condition){auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(!condition()){if(std::chrono::steady_clock::now()>until)throw std::runtime_error("Worker timeout");std::this_thread::sleep_for(std::chrono::milliseconds(2));}}
int main(int argc,char**argv){QCoreApplication app(argc,argv);try{
 QTemporaryDir data;
 {Emulator seed(argv[1],data.path());seed.tick(3500);seed.press("enter");seed.tick(4000);seed.press("enter");seed.save();}
 std::atomic<bool> stalled{false};int before=0;
 {
  RealtimeEmulator game(argv[1],data.path(),[&](const QString& button,const QString&){if(button=="menu"){stalled=true;std::this_thread::sleep_for(std::chrono::milliseconds(250));stalled=false;}});
  before=game.snapshot().property("steps").toInt();game.press("menu");waitFor([&]{return stalled.load();});
  auto start=std::chrono::steady_clock::now();
  game.press("shake");for(int i=0;i<50;i++){game.tick(11);game.save();require(game.snapshot().property("pixelLevels").toString().size()==1080,"Published frame stays readable during worker stall");}
  auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  require(ms<100&&stalled,"Tracking-facing calls must return while worker is still blocked");
  waitFor([&]{return game.processedInputs()>=2;});
  waitFor([&]{return game.snapshot().property("steps").toInt()==before+1;});
  require(game.peakWorkMs()>=200,"Injected stall occurred on worker");
  require(game.droppedInputs()==0,"Queued inputs survive a stalled worker");
  std::cout<<"50 tracking-facing updates during 250ms worker stall: "<<ms<<"ms\n";
 }
 {Emulator restored(argv[1],data.path());require(restored.snapshot().property("steps").toInt()==before+1,"Shutdown flushes queued steps and durable save");}
 QFile bad(data.path()+"/save.json");bad.open(QIODevice::WriteOnly);bad.write("bad");bad.close();bool rejected=false;
 try{RealtimeEmulator invalid(argv[1],data.path());}catch(...){rejected=true;}
 require(rejected&&Emulator::read(data.path()+"/save.json")=="bad","Worker startup rejects and preserves corrupt saves");
 std::cout<<"Worker isolation, input queue, snapshot transfer, shutdown and failure propagation passed\n";return 0;
}catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
