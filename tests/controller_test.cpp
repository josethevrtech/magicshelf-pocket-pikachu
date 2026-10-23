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
 s.back=s.friendship=s.settings=false;s.depthY=1;r=state.update(s,.022f);
 require(r.depthAxis&&r.depthY==1&&!r.y,"Holding stick pushes distance without a modifier or navigating menus");
 s.depthY=-1;r=state.update(s,.022f);require(r.depthY==-1&&!r.y,"Holding stick pulls distance back");
 s.y=1;r=state.update(s,.022f);require(r.y==1&&r.depthY==-1,"Free stick navigates independently during a depth adjustment");
 require(!state.update(s,.1f).y,"Navigation waits before repeating");
 s.y=0;s.right=true;r=state.update(s,.022f);require(r.x==1,"Physical left-controller D-pad remains available while held");
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
 std::cout<<"Two-hand controls, direct push/pull, Frame scroll fallback and focus isolation passed\n";return 0;
}catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
