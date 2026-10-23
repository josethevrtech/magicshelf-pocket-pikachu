#include "controls.h"
#include <iostream>
#include <stdexcept>
void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
int main(){try{
 HeadsetPedometer steps;int count=0;double t=10;
 for(int i=0;i<900;i++,t+=1./90)count+=steps.update({0,1.7f+.002f*sinf(i*2.f),0},t);
 require(count==0,"Stationary tracking jitter must not award steps");
 steps.reset();count=0;
 for(int i=0;i<900;i++,t+=1./90)count+=steps.update({0,1.7f+.025f*sinf(i*2*M_PI*2/90),float(i)*.004f},t);
 require(count>=18&&count<=20,"Two-Hz walking must award approximately 20 steps in 10 seconds");
 std::cout<<"Walking trace: "<<count<<" / 20 bobs\n";
 steps.reset();count=0;
 for(int i=0;i<900;i++,t+=1./90)count+=steps.update({0,1.7f+.04f*sinf(i*2*M_PI*3/90),0},t);
 require(count>=28&&count<=30,"Three-Hz running cadence");
 std::cout<<"Running trace: "<<count<<" / 30 bobs\n";
 for(int rate:{30,45}){
  steps.reset();count=0;
  for(int i=0;i<rate*10;i++,t+=1./rate)count+=steps.update({0,1.7f+.025f*sinf(i*2*M_PI*2/rate),0},t);
  require(count>=18&&count<=20,"Walking must also work at the native overlay sampling rates");
  steps.reset();count=0;
  for(int i=0;i<rate*10;i++,t+=1./rate)count+=steps.update({0,1.7f+.04f*sinf(i*2*M_PI*3/rate),0},t);
  require(count>=28&&count<=30,"Running must also work at the native overlay sampling rates");
 }
 for(int i=0;i<100;i++,t+=1./90)require(!steps.update({0,1.7f,0},t,false),"Untracked/unworn headset");
 steps.reset();for(int i=0;i<100;i++,t+=1./90)require(!steps.update({float(i%2)*5,1.7f,0},t),"Tracking teleport");
 steps.reset();for(int i=0;i<900;i++,t+=1./90)require(!steps.update({0,1.3f+float(i)*.0005f,0},t),"Slow posture change");
 std::cout<<"Headset motion: jitter, cadence, tracking loss, teleports and posture checks passed\n";
 return 0;
}catch(std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
