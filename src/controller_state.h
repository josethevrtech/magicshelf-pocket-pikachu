#pragma once
#include <algorithm>
#include <cmath>
struct PocketControllerState {
    struct Sample {
        bool focused=false,held=false,axis=false,depthAxis=false,grip=false,gripAvailable=false;
        float x=0,y=0,depthY=0;
        bool confirm=false,back=false,friendship=false,settings=false,home=false;
        bool left=false,right=false,up=false,down=false;
    } previous;
    struct Result {
        bool confirm=false,back=false,friendship=false,settings=false,home=false,depth=false,depthAxis=false,grip=false,gripPressed=false,gripAvailable=false;
        float depthY=0;int x=0,y=0;
    };
    bool neutralRequired=true;int direction=0;float repeat=0;
    static float deflection(float value){
        if(!std::isfinite(value)||std::abs(value)<=.25f)return 0;
        return std::copysign((std::min(std::abs(value),1.f)-.25f)/.75f,value);
    }
    Result update(const Sample& s,float dt) {
        Result out;
        if(!s.focused){previous=s;neutralRequired=true;direction=0;repeat=0;return out;}
        out.grip=s.grip;out.gripAvailable=s.gripAvailable;
        if(previous.focused){
            out.gripPressed=s.gripAvailable&&s.grip&&!previous.grip;
            out.confirm=s.confirm&&!previous.confirm;out.back=s.back&&!previous.back;
            out.friendship=s.friendship&&!previous.friendship;out.settings=s.settings&&!previous.settings;
            out.home=s.home&&!previous.home;
        }
        // The holding hand's stick always moves distance, matching the other
        // creations. Navigation is sampled separately from the free hand.
        out.depth=s.held;out.depthAxis=s.held&&s.depthAxis;
        out.depthY=out.depthAxis?deflection(s.depthY):0;
        if(!previous.focused||previous.held!=s.held){neutralRequired=true;direction=0;}
        bool neutral=std::abs(s.x)<=.25f&&std::abs(s.y)<=.25f;
        if(neutral)neutralRequired=false;
        int next=0;
        if(s.left!=s.right)next=s.right?1:-1;
        else if(s.up!=s.down)next=s.up?2:-2;
        else if(s.axis&&!neutralRequired&&std::max(std::abs(s.x),std::abs(s.y))>.55f)
            next=std::abs(s.x)>std::abs(s.y)?(s.x>0?1:-1):(s.y>0?2:-2);
        bool fire=false;
        if(next!=direction){direction=next;repeat=.4f;fire=next!=0&&previous.focused;}
        else if(next){repeat-=std::clamp(dt,0.f,.1f);if(repeat<=0){fire=true;repeat=.12f;}}
        if(fire){if(std::abs(next)==1)out.x=next;else out.y=next/2;}
        previous=s;return out;
    }
};
