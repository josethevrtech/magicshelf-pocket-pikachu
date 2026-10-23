#pragma once
#include <pulse/simple.h>
#include <pulse/error.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Short piezo-style cues. The upstream project supplies no audio recordings.
// Playback never blocks emulation, tracking, or rendering.
class HandheldAudio {
    std::mutex mutex;
    std::condition_variable changed;
    std::deque<std::vector<int16_t>> queued;
    bool stopping=false;
    std::atomic<bool> connected{false};
    std::atomic<unsigned> played{0};
    std::thread worker;
    void run() {
        pa_simple* stream=nullptr;
        for(;;) {
            std::vector<int16_t> samples;
            {
                std::unique_lock<std::mutex> guard(mutex);
                changed.wait(guard,[&]{return stopping||!queued.empty();});
                if(stopping&&queued.empty())break;
                samples=std::move(queued.front());queued.pop_front();
            }
            int error=0;
            if(!stream) {
                pa_sample_spec spec{PA_SAMPLE_S16LE,48000,1};
                pa_buffer_attr buffer{48000,4800,960,960,uint32_t(-1)};
                stream=pa_simple_new(nullptr,"Pocket Pikachu",PA_STREAM_PLAYBACK,nullptr,"Handheld beeps",&spec,nullptr,&buffer,&error);
                connected=stream!=nullptr;
                if(!stream){std::cerr<<"Pocket Pikachu audio: "<<pa_strerror(error)<<"\n";continue;}
            }
            if(pa_simple_write(stream,samples.data(),samples.size()*sizeof(int16_t),&error)<0||pa_simple_drain(stream,&error)<0) {
                std::cerr<<"Pocket Pikachu audio: "<<pa_strerror(error)<<"\n";
                pa_simple_free(stream);stream=nullptr;connected=false;
            }else ++played;
        }
        if(stream)pa_simple_free(stream);
    }
public:
    HandheldAudio():worker([this]{run();}){}
    ~HandheldAudio(){ {std::lock_guard<std::mutex> guard(mutex);stopping=true;}changed.notify_one();worker.join(); }
    bool ready()const{return connected.load();}
    unsigned count()const{return played.load();}
    static std::vector<int16_t> waveform(const std::string& button,const std::string& state) {
        if(button=="shake")return {};
        std::vector<double> frequencies{1960};
        double duration=.035;
        if(button=="back")frequencies={1470};
        if(button=="left"||button=="right"||button=="top"||button=="bottom")frequencies={1760};
        if(button=="enter"&&state=="pokeball"){frequencies={1568,2093,2637};duration=.055;}
        if(button=="reset"){frequencies={1047,784};duration=.060;}
        if(button=="alarm"){frequencies={2093,2637,2093};duration=.070;}
        std::vector<int16_t> samples;
        for(double frequency:frequencies){
            int length=int(48000*duration);
            for(int i=0;i<length;i++){
                double envelope=std::min({1.,i/144.,(length-1-i)/144.});
                double square=std::sin(2*3.141592653589793*frequency*i/48000)>=0?1:-1;
                samples.push_back(int16_t(2600*square*envelope));
            }
            samples.insert(samples.end(),720,0);
        }
        return samples;
    }
    void button(const std::string& name,const std::string& state) {
        auto samples=waveform(name,state);if(samples.empty())return;
        {std::lock_guard<std::mutex> guard(mutex);if(queued.size()>=3)queued.pop_front();queued.push_back(std::move(samples));}
        changed.notify_one();
    }
};
