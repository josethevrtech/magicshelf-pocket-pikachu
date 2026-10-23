#pragma once
#include "openvr.h"
#include "controller_state.h"
#include <string>
#include <iostream>
struct ControllerNav {
    vr::VRActionSetHandle_t set=0;
    vr::VRActionHandle_t stick=0,confirm=0,back=0,friendship=0,settings=0,home=0,left=0,right=0,up=0,down=0;
    vr::VRInputValueHandle_t hand[2]{};
    bool available=false;int lastRole=0;PocketControllerState state;
    explicit ControllerNav(const std::string& base){
        auto* input=vr::VRInput();auto error=input->SetActionManifestPath((base+"/assets/input/actions.json").c_str());
        if(error){std::cerr<<"Pocket Pikachu action manifest error "<<error<<"\n";return;}
        error=input->GetActionSetHandle("/actions/pocket",&set);
        auto handle=[&](const char* name,vr::VRActionHandle_t& result){auto e=input->GetActionHandle((std::string("/actions/pocket/in/")+name).c_str(),&result);if(e)error=e;};
        handle("scroll",stick);handle("click",confirm);handle("back",back);handle("friendship",friendship);handle("settings",settings);handle("home",home);
        handle("left",left);handle("right",right);handle("up",up);handle("down",down);
        input->GetInputSourceHandle("/user/hand/left",&hand[0]);input->GetInputSourceHandle("/user/hand/right",&hand[1]);
        available=error==vr::VRInputError_None;std::cerr<<"Pocket Pikachu controller actions ready="<<available<<"\n";
    }
    PocketControllerState::Result update(bool focused,int role,float dt,bool held,uint32_t controller){
        PocketControllerState::Sample sample;
        focused=focused&&available&&role>=1&&role<=2;
        if(!available)return state.update(sample,dt);
        auto* input=vr::VRInput();vr::VRActiveActionSet_t active{};active.ulActionSet=set;
        // Either hand can operate a held handheld; hovering uses its pointer only.
        auto source=focused&&!held?hand[role-1]:vr::k_ulInvalidInputValueHandle;
        active.ulRestrictedToDevice=source;active.nPriority=vr::k_nActionSetOverlayGlobalPriorityMin+1;
        auto error=input->UpdateActionState(focused?&active:nullptr,sizeof(active),focused?1:0);
        if(error||!focused){lastRole=0;return state.update(sample,dt);}
        int focusRole=held?3:role;if(lastRole!=focusRole)state.update({},dt);lastRole=focusRole;
        auto downAction=[&](vr::VRActionHandle_t action,vr::VRInputValueHandle_t device){vr::InputDigitalActionData_t value{};return input->GetDigitalActionData(action,&value,sizeof(value),device)==vr::VRInputError_None&&value.bActive&&value.bState;};
        sample.focused=true;sample.held=held;
        sample.confirm=downAction(confirm,source);sample.back=downAction(back,source);sample.friendship=downAction(friendship,source);sample.settings=downAction(settings,source);sample.home=downAction(home,source);
        sample.left=downAction(left,source);sample.right=downAction(right,source);sample.up=downAction(up,source);sample.down=downAction(down,source);
        auto axis=[&](vr::VRInputValueHandle_t device,float& x,float& y){
            vr::InputAnalogActionData_t value{};
            if(input->GetAnalogActionData(stick,&value,sizeof(value),device)!=vr::VRInputError_None||!value.bActive)return false;
            x=value.x;y=value.y;return true;
        };
        // Never merge the sticks while held: the owner controls placement,
        // the other hand controls the pet. Digital D-pad/buttons work on both.
        sample.axis=axis(held?hand[2-role]:source,sample.x,sample.y);
        if(held){
            float unused=0;sample.depthAxis=axis(hand[role-1],unused,sample.depthY);
            if(!sample.depthAxis&&controller<vr::k_unMaxTrackedDeviceCount){
                vr::VRControllerState_t raw{};
                if(vr::VRSystem()->GetControllerState(controller,&raw,sizeof(raw)))for(int n=0;n<5;n++)
                    if(vr::VRSystem()->GetInt32TrackedDeviceProperty(controller,(vr::ETrackedDeviceProperty)(vr::Prop_Axis0Type_Int32+n))==vr::k_eControllerAxis_Joystick){sample.depthAxis=true;sample.depthY=raw.rAxis[n].y;break;}
            }
        }
        return state.update(sample,dt);
    }
};
