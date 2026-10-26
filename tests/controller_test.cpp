#include "controller_state.h"
#include "controls.h"
#include <iostream>
#include <stdexcept>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){try{
 PocketControllerState state;PocketControllerState::Sample s;s.focused=s.held=s.axis=s.depthAxis=true;
 s.confirm=true;s.y=1;auto r=state.update(s,.022f);require(!r.confirm&&!r.y,"Picking up with held buttons/free stick must not send gameplay input");
 s.confirm=false;s.y=0;state.update(s,.022f);s.confirm=true;r=state.update(s,.022f);require(r.confirm,"A must work while held");
 require(!state.update(s,.022f).confirm,"A sends one press per squeeze");
 s.confirm=false;s.back=s.friendship=s.settings=true;r=state.update(s,.022f);require(r.back&&r.friendship&&r.settings,"B/X/Y remain available while held");
 s.back=s.friendship=s.settings=false;s.depthY=1;s.y=1;r=state.update(s,.022f);
 require(!r.depthAxis&&r.depthY==0&&r.y==1,"Normal holding-stick direction plays without moving depth");
 s.adjust=true;r=state.update(s,.022f);require(r.depthAxis&&r.depthY==1&&!r.y,"Stick-click chord adjusts depth without gameplay");
 s.adjust=false;r=state.update(s,.022f);require(!r.y,"Leaving adjustment with a deflected stick cannot navigate");
 s.y=0;state.update(s,.022f);s.y=-1;r=state.update(s,.022f);require(r.y==-1,"Fresh direction works after neutral");
 s.y=0;s.right=true;r=state.update(s,.022f);require(r.x==1,"Physical left-controller D-pad remains available while held");
 s.held=false;r=state.update(s,.022f);require(!r.x,"Release cannot leak a held D-pad press");
 state.update({},.022f);s.held=true;s.confirm=true;r=state.update(s,.022f);require(!r.confirm&&!r.x,"Recovery clears buttons and D-pad until released");
 RoomGrab grab;M room=identity();room[14]=-1;grab.begin(2,identity(),room);
 float initial=grab.distance;
 grab.updateDepth(1,.022f,true);require(grab.distance>initial,"Forward stick actually moves the model farther away");
 grab.updateDepth(-1,.022f,true);require(std::abs(grab.distance-initial)<.00001f,"Backward stick restores model distance");
 require(grab.queueScrollDepth(2,1,.01f),"Frame dashboard scroll is accepted from the grabbing hand");
 grab.updateDepth(0,.022f,false);require(grab.distance>initial,"Dashboard scroll works when the analog action is unavailable");
 require(!grab.queueScrollDepth(3,-1,.01f)&&!grab.queueScrollDepth(2,-1,.5f),"Other-hand and stale scroll cannot move the creation");
 float after=grab.distance;grab.queueScrollDepth(2,1,.01f);grab.updateDepth(0,.022f,true);
 require(grab.distance==after&&grab.pendingScrollDepth==0,"Axis and scroll must never double-count the same input");
 grab.queueScrollDepth(2,1,.01f);grab.end();grab.updateDepth(1,.022f,true);require(grab.distance==after,"Releasing clears queued movement");
 s.focused=false;s.home=true;r=state.update(s,.022f);require(!r.x&&!r.home&&!r.confirm&&!r.depthAxis,"Unfocused creation releases input");
 PocketControllerState gripState;PocketControllerState::Sample grip;grip.focused=grip.gripAvailable=true;
 grip.grip=true;require(!gripState.update(grip,.02f).gripPressed,"Already-held grip cannot pick up on focus gain");
 grip.grip=false;gripState.update(grip,.02f);grip.grip=true;require(gripState.update(grip,.02f).gripPressed,"Fresh grip starts pickup");
 require(!gripState.update(grip,.02f).gripPressed,"Holding grip cannot restart pickup");
 grip.held=true;require(gripState.update(grip,.02f).grip,"Grip stays held after pickup");
 grip.grip=false;require(!gripState.update(grip,.02f).grip,"Releasing grip places the handheld");
 grip.focused=false;require(!gripState.update(grip,.02f).gripAvailable,"Tracking/focus loss releases the grip");
 std::cout<<"Gameplay directions, explicit depth chord, grip transitions, recovery and focus isolation passed\n";return 0;
}catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
