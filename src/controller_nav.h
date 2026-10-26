#pragma once
#include "openvr.h"
#include "controller_state.h"
#include "frame_input.h"
#include <string>
#include <iostream>
struct ControllerNav {
    vr::VRActionSetHandle_t set=0;
    vr::VRActionHandle_t stick=0,adjust=0,confirm=0,back=0,friendship=0,settings=0,home=0,left=0,right=0,up=0,down=0,grip=0;
    vr::VRInputValueHandle_t hand[2]{};
    bool available=false;int lastRole=0;PocketControllerState state;FrameInput frame;
    bool focusedNow=false;int roleNow=0,inputError=0,digitalError=0;unsigned activeMask=0,pressedMask=0;
    unsigned long samples=0,focusChanges=0,buttonEdges=0;float axisX=0,axisY=0;bool adjusting=false,usingFallback=false;

    explicit ControllerNav(const std::string& base){
        auto* input=vr::VRInput();auto error=input->SetActionManifestPath((base+"/assets/input/actions.json").c_str());
        if(error){std::cerr<<"Pocket Pikachu action manifest error "<<error<<"\n";return;}
        error=input->GetActionSetHandle("/actions/pocket",&set);
        auto handle=[&](const char* name,vr::VRActionHandle_t& result){auto e=input->GetActionHandle((std::string("/actions/pocket/in/")+name).c_str(),&result);if(e)error=e;};
        handle("scroll",stick);handle("adjust",adjust);handle("click",confirm);handle("back",back);handle("friendship",friendship);handle("settings",settings);handle("home",home);
        handle("grip",grip);handle("left",left);handle("right",right);handle("up",up);handle("down",down);
        input->GetInputSourceHandle("/user/hand/left",&hand[0]);input->GetInputSourceHandle("/user/hand/right",&hand[1]);
        available=error==vr::VRInputError_None;std::cerr<<"Pocket Pikachu controller actions ready="<<available<<"\n";
    }
    PocketControllerState::Result update(bool focused,int role,float dt,bool held,uint32_t controller){
        PocketControllerState::Sample sample;
        ++samples;focusedNow=focused;roleNow=role;activeMask=pressedMask=0;digitalError=0;axisX=axisY=0;adjusting=usingFallback=false;
        focused=focused&&available&&role>=1&&role<=2;
        if(!available)return state.update(sample,dt);
        auto* input=vr::VRInput();vr::VRActiveActionSet_t active{};active.ulActionSet=set;
        // Either hand can operate a held handheld; hovering uses its pointer only.
        auto source=focused&&!held?hand[role-1]:vr::k_ulInvalidInputValueHandle;
        active.ulRestrictedToDevice=source;// Named actions keep the usual overlay priority; no global setting changes.
        active.nPriority=vr::k_nActionSetOverlayGlobalPriorityMin+1;
        auto error=input->UpdateActionState(focused?&active:nullptr,sizeof(active),focused?1:0);
        inputError=error;
        if(error||!focused){if(lastRole)++focusChanges;lastRole=0;return state.update(sample,dt);}
        int focusRole=held?3:role;if(lastRole!=focusRole){++focusChanges;state.update({},dt);}lastRole=focusRole;
        unsigned bit=0;
        auto downAction=[&](vr::VRActionHandle_t action,vr::VRInputValueHandle_t device){
            vr::InputDigitalActionData_t value{};auto err=input->GetDigitalActionData(action,&value,sizeof(value),device);
            if(err!=vr::VRInputError_None)digitalError=err;
            unsigned mask=1u<<bit++;if(err==vr::VRInputError_None&&value.bActive){activeMask|=mask;if(value.bState)pressedMask|=mask;}
            if(err==vr::VRInputError_None&&value.bActive)return value.bState;
            usingFallback=true;
            auto button=[&](int r,const char* name){if(!held&&r!=role)return false;return frame.pressed(r,hand[r-1],name);};
            if(action==confirm)return button(2,"button_a");
            if(action==back)return button(2,"button_b");
            if(action==friendship)return button(2,"button_x");
            if(action==settings)return button(2,"button_y");
            if(action==home)return button(1,"button_view")||button(2,"button_menu");
            if(action==adjust)return button(role,"thumbstick");
            if(held||role==1){float x=0,y=0;if(frame.axis(1,hand[0],x,y,true)){
                if(action==left)return x<-.5f;if(action==right)return x>.5f;
                if(action==up)return y>.5f;if(action==down)return y<-.5f;
            }}return false;
        };
        sample.focused=true;sample.held=held;
        vr::InputDigitalActionData_t gripData{};sample.gripAvailable=input->GetDigitalActionData(grip,&gripData,sizeof(gripData),hand[role-1])==vr::VRInputError_None&&gripData.bActive;sample.grip=sample.gripAvailable&&gripData.bState;
        if(!sample.gripAvailable){vr::RenderModel_ComponentState_t value{};sample.gripAvailable=frame.component(role,hand[role-1],"button_grip",value);sample.grip=sample.gripAvailable&&(value.uProperties&vr::VRComponentProperty_IsPressed);}
        sample.confirm=downAction(confirm,source);sample.back=downAction(back,source);sample.friendship=downAction(friendship,source);sample.settings=downAction(settings,source);sample.home=downAction(home,source);
        sample.left=downAction(left,source);sample.right=downAction(right,source);sample.up=downAction(up,source);sample.down=downAction(down,source);
        auto axis=[&](vr::VRInputValueHandle_t device,float& x,float& y){
            vr::InputAnalogActionData_t value{};
            if(input->GetAnalogActionData(stick,&value,sizeof(value),device)!=vr::VRInputError_None||!value.bActive)return false;
            x=value.x;y=value.y;return true;
        };
        // Prefer the holding stick when deflected; otherwise use the free stick.
        // Dashboard-suppressed actions fall back to the same device
        // visualization state, never the unavailable legacy input API.
        auto readStick=[&](int r,float& x,float& y){
            if(axis(hand[r-1],x,y))return true;
            usingFallback=true;return frame.axis(r,hand[r-1],x,y);
        };
        sample.axis=readStick(role,sample.x,sample.y);
        sample.adjust=held&&downAction(adjust,hand[role-1]);
        sample.depthAxis=held&&sample.axis;sample.depthY=sample.y;
        if(held&&!sample.adjust&&std::max(std::abs(sample.x),std::abs(sample.y))<=.25f){
            float x=0,y=0;if(readStick(3-role,x,y)){sample.axis=true;sample.x=x;sample.y=y;}
        }
        axisX=sample.x;axisY=sample.y;adjusting=sample.adjust;
        auto result=state.update(sample,dt);buttonEdges+=result.confirm+result.back+result.friendship+result.settings;return result;
    }
};
