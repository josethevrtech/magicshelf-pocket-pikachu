#pragma once
#include "openvr.h"
#include <array>
#include <vector>
#include <string>
#include <limits>
#include <cmath>

// The Frame dashboard can suppress action/legacy input while the runtime still
// exposes controller visualization state. Read that documented OpenVR state only
// for the focused creation, and only when its named action is unavailable.
struct FrameInput {
    struct PoseSample {vr::HmdMatrix34_t matrix{};float x=0,y=0;};
    struct Hand {uint32_t device=vr::k_unTrackedDeviceIndexInvalid;std::string model;
        std::vector<PoseSample> stick,dpad;} hands[2];
    static float rotationError(const vr::HmdMatrix34_t& a,const vr::HmdMatrix34_t& b){
        float sum=0;for(int r=0;r<3;r++)for(int c=0;c<3;c++){float d=a.m[r][c]-b.m[r][c];sum+=d*d;}return sum;
    }
    static bool decode(const std::vector<PoseSample>& samples,const vr::HmdMatrix34_t& matrix,float& x,float& y){
        float error=std::numeric_limits<float>::infinity();const PoseSample* nearest=nullptr;
        for(const auto& sample:samples){float d=rotationError(sample.matrix,matrix);if(d<error){error=d;nearest=&sample;}}
        // Reject an unexpected/custom model rather than inventing a direction.
        if(!nearest||!std::isfinite(error)||error>.003f)return false;
        x=nearest->x;y=nearest->y;return true;
    }
    bool prepare(int role){
        if(role<1||role>2)return false;
        auto& h=hands[role-1];auto index=vr::VRSystem()->GetTrackedDeviceIndexForControllerRole((vr::ETrackedControllerRole)role);
        if(index>=vr::k_unMaxTrackedDeviceCount)return false;
        if(index==h.device&&!h.model.empty())return true;
        h={};char model[256]{};vr::VRSystem()->GetStringTrackedDeviceProperty(index,vr::Prop_RenderModelName_String,model,sizeof(model));
        if(std::string(model).find("frame_controller")==std::string::npos)return false;
        h.model=model;h.device=index;
        auto* render=vr::VRRenderModels();vr::RenderModel_ControllerMode_State_t mode{};
        auto add=[&](std::vector<PoseSample>& out,float x,float y,float scale){
            vr::VRControllerState_t synthetic{};synthetic.rAxis[0]={x*scale,y*scale};vr::RenderModel_ComponentState_t state{};
            if(render->GetComponentState(model,"thumbstick",&synthetic,&mode,&state))out.push_back({state.mTrackingToComponentRenderModel,x,y});
        };
        // Generate reference orientations from the installed driver, not a
        // measured neutral pose that might already have its stick deflected.
        for(int y=-20;y<=20;y++)for(int x=-20;x<=20;x++)add(h.stick,x/20.f,y/20.f,1);
        // Frame's D-pad shares the stick's pivot axes, with 7 vs 20 degree tilt.
        if(role==1)for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)add(h.dpad,float(x),float(y),7.f/20.f);
        return !h.stick.empty();
    }
    bool component(int role,vr::VRInputValueHandle_t source,const char* name,vr::RenderModel_ComponentState_t& state){
        if(!prepare(role))return false;vr::RenderModel_ControllerMode_State_t mode{};
        return vr::VRRenderModels()->GetComponentStateForDevicePath(hands[role-1].model.c_str(),name,source,&mode,&state);
    }
    bool pressed(int role,vr::VRInputValueHandle_t source,const char* name){
        vr::RenderModel_ComponentState_t value{};return component(role,source,name,value)&&(value.uProperties&vr::VRComponentProperty_IsPressed);
    }
    bool axis(int role,vr::VRInputValueHandle_t source,float& x,float& y,bool dpad=false){
        vr::RenderModel_ComponentState_t value{};if(!component(role,source,dpad?"dpad":"thumbstick",value))return false;
        return decode(dpad?hands[role-1].dpad:hands[role-1].stick,value.mTrackingToComponentRenderModel,x,y);
    }
};
