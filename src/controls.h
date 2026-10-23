#pragma once
#include "placement.h"
#include <string>
inline std::string buttonAt(V p) {
    auto circle=[&](float x,float y,float r){return (p[0]-x)*(p[0]-x)+(p[1]-y)*(p[1]-y)<r*r;};
    if(circle(.02185f,-.01490f,.0048f))return "enter";
    if(circle(.00938f,-.02018f,.0048f))return "back";
    if(circle(.01942f,-.02755f,.0017f))return "reset";
    if(fabsf(p[0]+.00690f)<.005f&&fabsf(p[1]+.03142f)<.0027f)return "state";
    if(fabsf(p[0]-.00744f)<.005f&&fabsf(p[1]+.03141f)<.0027f)return "menu";
    float dx=p[0]+.01845f,dy=p[1]+.01991f;
    if(fabsf(dx)<.0085f&&fabsf(dy)<.0085f){
        if(fabsf(dx)<.002f&&fabsf(dy)<.002f)return "";
        if(fabsf(dx)>fabsf(dy))return dx>0?"right":"left";
        return dy>0?"top":"bottom";
    }return "";
}
inline std::string buttonRay(V origin,V direction) {
    if(fabsf(direction[2])<.0001f)return "";
    auto atDepth=[&](float depth){
        float t=(depth-origin[2])/direction[2];
        return origin[2]>depth&&t>=0?buttonAt(add(origin,mul(direction,t))):std::string();
    };
    auto raised=atDepth(ButtonZ);
    if(!raised.empty()&&raised!="reset")return raised;
    // The pinhole is on the shell, below the raised button faces.
    return atDepth(.01955f)=="reset"?"reset":"";
}
// A cadence-qualified vertical head bob is one estimated walking/running step.
// It keeps sampling when the creation is hidden, without consuming game input.
struct HeadsetPedometer {
    V previous{};double last=0,lastPeak=0;float filtered=0,trough=0,peak=0;
    bool sampled=false,rising=false;
    void reset(){sampled=rising=false;lastPeak=0;}
    bool update(V p,double now,bool tracked=true) {
        if(!tracked||!std::isfinite(now)){reset();return false;}
        for(float v:p)if(!std::isfinite(v)){reset();return false;}
        if(!sampled){previous=p;last=now;filtered=trough=peak=p[1];sampled=true;return false;}
        double dt=now-last;V movement=sub(p,previous);float travel=sqrtf(dot(movement,movement));last=now;previous=p;
        if(dt<=0||dt>.15||travel>.20f||travel/dt>4){reset();return false;}
        filtered+=(p[1]-filtered)*float(dt/(.035+dt));
        if(!rising){trough=std::min(trough,filtered);if(filtered-trough>.008f){rising=true;peak=filtered;}return false;}
        peak=std::max(peak,filtered);
        if(peak-filtered<.007f)return false;
        float prominence=peak-trough;rising=false;trough=filtered;
        double cadence=now-lastPeak;
        if(prominence<.015f||prominence>.18f){lastPeak=0;return false;}
        lastPeak=now;
        // Requiring a preceding bob rejects an isolated posture change.
        return cadence>=.25&&cadence<=1.5;
    }
};
