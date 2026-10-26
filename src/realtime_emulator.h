#pragma once
#include "emulator.h"
#include <QVariantMap>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
#include <chrono>

// QJSEngine, timers and durable saves live exclusively on the worker. Only
// QVariant snapshots cross threads; the renderer's QJSValues belong to its own
// engine. Tracking never waits for animation, input processing or disk writes.
class RealtimeEmulator {
    QJSEngine displayEngine;
    QJSValue displayed;
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<QString> pending;
    QVariantMap latest;
    QString failure;
    bool stopping=false,ready=false,saveRequested=false;
    uint64_t generation=0,displayedGeneration=0;
    std::atomic<double> workMs{0},peakMs{0};
    std::atomic<unsigned long> processed{0},dropped{0};
    std::thread worker;
    void run(QString base,QString data,std::function<void(const QString&,const QString&)> sound){
        try {
            Emulator game(base,data);game.buttonSound=std::move(sound);
            game.save();
            auto publish=[&]{auto frame=game.snapshot().toVariant().toMap();std::lock_guard<std::mutex> lock(mutex);latest=std::move(frame);++generation;};
            publish();{std::lock_guard<std::mutex> lock(mutex);ready=true;}changed.notify_all();
            auto last=std::chrono::steady_clock::now(),lastSave=last,lastFrame=last;
            for(;;){
                std::deque<QString> inputs;bool stop=false,save=false;
                {std::unique_lock<std::mutex> lock(mutex);
                    changed.wait_for(lock,std::chrono::milliseconds(10),[&]{return stopping||saveRequested||!pending.empty();});
                    inputs.swap(pending);stop=stopping;save=saveRequested;saveRequested=false;
                }
                auto start=std::chrono::steady_clock::now();
                for(const auto& input:inputs){game.press(input);++processed;}
                if(stop){game.save();break;}
                double elapsed=std::chrono::duration<double,std::milli>(start-last).count();last=start;
                game.tick(std::min(250.,elapsed));
                if(start-lastFrame>=std::chrono::milliseconds(66)||!inputs.empty()){publish();lastFrame=start;}
                if(save||start-lastSave>=std::chrono::seconds(1)){game.save();lastSave=start;}
                double duration=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
                workMs=duration;peakMs=std::max(peakMs.load(),duration);
            }
        }catch(const std::exception& e){std::lock_guard<std::mutex> lock(mutex);failure=QString::fromUtf8(e.what());ready=true;changed.notify_all();}
    }
public:
    RealtimeEmulator(QString base,QString data,std::function<void(const QString&,const QString&)> sound={}){
        worker=std::thread([this,base,data,sound]{run(base,data,sound);});
        std::unique_lock<std::mutex> lock(mutex);changed.wait(lock,[&]{return ready;});
        if(!failure.isEmpty()){auto message=failure.toStdString();lock.unlock();worker.join();throw std::runtime_error(message);}
    }
    ~RealtimeEmulator(){ {std::lock_guard<std::mutex> lock(mutex);stopping=true;}changed.notify_one();if(worker.joinable())worker.join(); }
    RealtimeEmulator(const RealtimeEmulator&)=delete;
    void tick(double){std::lock_guard<std::mutex> lock(mutex);if(!failure.isEmpty())throw std::runtime_error(failure.toStdString());}
    void press(const QString& input){ {std::lock_guard<std::mutex> lock(mutex);if(pending.size()<128)pending.push_back(input);else ++dropped;}changed.notify_one(); }
    void save(){ {std::lock_guard<std::mutex> lock(mutex);saveRequested=true;}changed.notify_one(); }
    QJSValue snapshot(){
        QVariantMap frame;uint64_t version;
        {std::lock_guard<std::mutex> lock(mutex);version=generation;if(version==displayedGeneration)return displayed;frame=latest;}
        displayed=displayEngine.toScriptValue(frame);displayedGeneration=version;return displayed;
    }
    double lastWorkMs()const{return workMs.load();}
    double peakWorkMs()const{return peakMs.load();}
    unsigned long processedInputs()const{return processed.load();}
    unsigned long droppedInputs()const{return dropped.load();}
};
