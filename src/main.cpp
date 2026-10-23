#include "emulator.h"
#include "magicshelf_sdk.h"
#include "lcd.h"
#include "controls.h"
#include "controller_nav.h"
#include "audio.h"
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QTemporaryDir>
#include <QJsonArray>
#include <chrono>
#include <csignal>
#include <thread>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
namespace fs=std::filesystem;
using namespace vr;
static volatile sig_atomic_t running=1,showRequested=0,dockRequested=0;
static void stop(int){running=0;}static void show(int){showRequested=1;}static void home(int){dockRequested=1;}
static M fromvr(HmdMatrix34_t p){M r=identity();for(int i=0;i<3;i++)for(int j=0;j<4;j++)r[i+j*4]=p.m[i][j];return r;}
static void check(EVROverlayError e){if(e)throw std::runtime_error(VROverlay()->GetOverlayErrorNameFromEnum(e));}
static M rotated(float yaw,float pitch){M p=identity();p[5]=cosf(pitch);p[6]=sinf(pitch);p[9]=-sinf(pitch);p[10]=cosf(pitch);return mm(yawPose(yaw),p);}

class Desktop : public QWidget {
    Emulator& game;Renderer& renderer;Canvas lcd{ScreenWidth,ScreenHeight};QTimer timer;
    QPoint drag;bool dragging=false;float yaw=-.15f,pitch=.06f;int saves=0;
public:
    Desktop(Emulator& g,Renderer& r):game(g),renderer(r){
        setWindowTitle("Pocket Pikachu · MagicShelf");resize(720,790);setMinimumSize(480,550);setFocusPolicy(Qt::StrongFocus);
        connect(&timer,&QTimer::timeout,this,[&]{try{game.tick(33);if(++saves%30==0)game.save();update();if(!running)close();}catch(const std::exception&e){std::cerr<<e.what()<<"\n";timer.stop();close();}});timer.start(33);
    }
    void paintEvent(QPaintEvent*)override{
        eglMakeCurrent(renderer.display,renderer.surface,renderer.surface,renderer.context);
        drawLCD(lcd,game.snapshot());renderer.screen(lcd);
        auto*raw=renderer.draw(identity(),rotated(yaw,pitch),{0,0,.3f},{0,0,.3f},.115f,1);
        QPainter p(this);p.fillRect(rect(),QColor("#eee8d8"));
        int side=std::min(width(),height()-100);QImage image(raw,renderer.W,renderer.H,renderer.W*4,QImage::Format_RGBA8888);
        p.drawImage(QRect((width()-side)/2,20,side,side),image,QRect(0,0,renderer.W/2,renderer.H));
        p.setPen(QColor("#514a38"));p.setFont(QFont("sans",12));
        p.drawText(QRect(12,height()-77,width()-24,24),Qt::AlignCenter,"Arrow keys: D-pad    Enter: A    Backspace: B");
        p.drawText(QRect(12,height()-48,width()-24,24),Qt::AlignCenter,"F: friendship    M: settings    Space: shake    Drag: rotate");
    }
    void keyPressEvent(QKeyEvent* e)override {
        if(e->isAutoRepeat())return;QString button;
        switch(e->key()){
            case Qt::Key_Return:case Qt::Key_Enter:button="enter";break;case Qt::Key_Backspace:case Qt::Key_Escape:button="back";break;
            case Qt::Key_Left:button="left";break;case Qt::Key_Right:button="right";break;case Qt::Key_Up:button="top";break;case Qt::Key_Down:button="bottom";break;
            case Qt::Key_F:button="state";break;case Qt::Key_M:button="menu";break;case Qt::Key_Space:button="shake";break;
            case Qt::Key_R:if(e->modifiers()&Qt::ShiftModifier)button="reset";break;
        }if(!button.isEmpty()){game.press(button);game.save();update();}
    }
    void mousePressEvent(QMouseEvent* e)override{
        int side=std::min(width(),height()-100);V eye{0,0,.3f};
        V target{float((e->position().x()-(width()-side)/2)/side-.5)*.115f,float(.5-(e->position().y()-20)/side)*.115f,0};
        M inv=inverse(rotated(yaw,pitch));V a=point(inv,eye),b=point(inv,target),d=sub(b,a);
        auto button=buttonRay(a,d);if(!button.empty()){game.press(QString::fromStdString(button));game.save();return;}
        dragging=true;drag=e->position().toPoint();
    }
    void mouseMoveEvent(QMouseEvent* e)override{if(dragging){auto delta=e->position().toPoint()-drag;drag=e->position().toPoint();yaw+=delta.x()*.012f;pitch=std::clamp(pitch+delta.y()*.012f,-1.2f,1.2f);update();}}
    void mouseReleaseEvent(QMouseEvent*)override{dragging=false;}
};

static int runVR(Emulator& game,Renderer& renderer,HandheldAudio& audio,const std::string& package,const std::string& data,int lock){
    EVRInitError err;VR_Init(&err,VRApplication_Overlay);if(err)throw std::runtime_error(VR_GetVRInitErrorAsEnglishDescription(err));
    auto*ov=VROverlay();VROverlayHandle_t device=0;
    check(ov->CreateOverlay(getenv("MAGICSHELF_MODULE_KEY")?getenv("MAGICSHELF_MODULE_KEY"):DeviceKey,AppTitle,&device));
    for(auto flag:{VROverlayFlags_SideBySide_Parallel,VROverlayFlags_VisibleInDashboard,VROverlayFlags_MakeOverlaysInteractiveIfVisible,VROverlayFlags_EnableClickStabilization,VROverlayFlags_SendVRSmoothScrollEvents})check(ov->SetOverlayFlag(device,flag,true));
    check(ov->SetOverlayInputMethod(device,VROverlayInputMethod_Mouse));HmdVector2_t mouse{{512,512}};check(ov->SetOverlayMouseScale(device,&mouse));
    ObjectDock dock(ov,data);ControllerNav nav(package);RoomGrab grab;HeadsetPedometer pedometer;
    Canvas lcd(ScreenWidth,ScreenHeight);M world=identity(),lastPlane=identity(),lastModel=identity();
    float size=DefaultSize,lastSize=1,lastWidth=InteractionPlaneWidth;bool docked=true,placed=false,ready=false,candidate=false;
    uint32_t pointer=k_unTrackedDeviceIndexInvalid;bool overHandle=false;auto now=std::chrono::steady_clock::now(),last=now,lastSave=now,lastLCD=now-std::chrono::seconds(1);
    unsigned long motionSamples=0,trackedSamples=0,motionSteps=0,controllerInputs=0,depthMoves=0;
    bool repositioning=false;
    auto settings=data+"/placement.json";
    if(auto*j=json_object_from_file(settings.c_str())){
        json_object*v=nullptr;
        if(json_object_object_get_ex(j,"size",&v)){float value=json_object_get_double(v);if(std::isfinite(value))size=std::clamp(value,.6f,3.f);}
        if(json_object_object_get_ex(j,"docked",&v))docked=json_object_get_boolean(v);
        if(json_object_object_get_ex(j,"pose",&v)&&json_object_is_type(v,json_type_array)&&json_object_array_length(v)==16){for(int i=0;i<16;i++)world[i]=json_object_get_double(json_object_array_get_idx(v,i));placed=validRoomPose(world);}
        json_object_put(j);
    }
    auto savePlacement=[&]{QJsonArray pose;for(float v:world)pose.append(v);QJsonObject j{{"size",size},{"docked",docked},{"pose",pose}};QSaveFile f(QString::fromStdString(settings));if(!f.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot save placement");f.write(QJsonDocument(j).toJson());if(!f.commit())throw std::runtime_error("Cannot save placement");};
    auto toDock=[&]{showRequested=0;grab.end();docked=true;placed=false;candidate=false;savePlacement();};
    auto release=[&]{if(dock.canSnap(world,size,candidate))toDock();else{grab.end();candidate=false;savePlacement();}};
    while(running){
        now=std::chrono::steady_clock::now();float dt=std::chrono::duration<float>(now-last).count();last=now;
        game.tick(std::min(250.f,dt*1000));
        TrackedDevicePose_t poses[k_unMaxTrackedDeviceCount];VRSystem()->GetDeviceToAbsoluteTrackingPose(TrackingUniverseStanding,0,poses,k_unMaxTrackedDeviceCount);
        M head=fromvr(poses[0].mDeviceToAbsoluteTracking);dock.refresh();
        if(dockRequested||dock.takeHomeRequest()){dockRequested=0;toDock();}
        if(poses[0].bPoseIsValid&&(showRequested||(!placed&&!docked))){
            showRequested=0;grab.end();docked=false;placed=true;V p=point(head,{-.08f,-.10f,-.50f}),h=point(head,{0,0,0}),z=norm(sub(h,p)),x=norm(cross({0,1,0},z));world=pose(x,cross(z,x),z,p);savePlacement();
        }
        bool resetTracking=false;
        VREvent_t e;while(VRSystem()->PollNextEvent(&e,sizeof(e))){if(e.eventType==VREvent_Quit){VRSystem()->AcknowledgeQuit_Exiting();running=0;}if(dockResetEvent(e.eventType)){toDock();resetTracking=true;}}
        auto activity=VRSystem()->GetTrackedDeviceActivityLevel(0);
        bool worn=activity==k_EDeviceActivityLevel_UserInteraction||activity==k_EDeviceActivityLevel_UserInteraction_Timeout;
        if(resetTracking)pedometer.reset();
        // Sampling is independent of dashboard/creation visibility or grabbing.
        bool tracked=!resetTracking&&worn&&poses[0].bPoseIsValid&&poses[0].bDeviceIsConnected;
        ++motionSamples;if(tracked)++trackedSamples;
        if(pedometer.update(point(head,{0,0,0}),std::chrono::duration<double>(now.time_since_epoch()).count(),tracked)){game.press("shake");++motionSteps;}
        if(now-lastSave>std::chrono::seconds(1)){
            game.save();lastSave=now;auto frame=game.snapshot();
            QJsonObject status{{"steps",frame.property("steps").toInt()},{"watts",frame.property("watts").toInt()},{"status",frame.property("status").toString()},
                {"motion_samples",double(motionSamples)},{"tracked_samples",double(trackedSamples)},{"motion_steps",double(motionSteps)},{"headset_worn",worn},{"headset_tracked",poses[0].bPoseIsValid},{"docked",docked},{"visible",ov->IsOverlayVisible(device)},
                {"friendship",frame.property("friendship").toInt()},{"audio_connected",audio.ready()},{"audio_cues",int(audio.count())},
                {"held",grab.active},{"grab_distance",grab.distance},{"depth_moves",double(depthMoves)},{"repositioning",repositioning},{"controller_inputs",double(controllerInputs)},
                {"sound_enabled",frame.property("soundEnabled").toBool()},{"device_ui",frame.property("deviceUi").toString()},{"alarm_enabled",frame.property("alarmEnabled").toBool()},{"alarm_ringing",frame.property("alarmRinging").toBool()}};
            QSaveFile f(QString::fromStdString(data+"/runtime.json"));if(f.open(QIODevice::WriteOnly)){f.write(QJsonDocument(status).toJson());f.commit();}
        }
        if(grab.active){
            if(!poses[grab.controller].bPoseIsValid||!poses[0].bPoseIsValid)release();
            else if(grab.watchTriggerRelease){VRControllerState_t state{};if(VRSystem()->GetControllerState(grab.controller,&state,sizeof(state))&&!(state.ulButtonPressed&ButtonMaskFromId(k_EButton_SteamVR_Trigger)))release();}
        }
        while(ov->PollNextOverlayEvent(device,&e,sizeof(e))){
            if(dockResetEvent(e.eventType)){toDock();pedometer.reset();continue;}
            if(e.eventType==VREvent_ScrollSmooth&&grab.active){
                auto source=e.trackedDeviceIndex;
                if(source==0||source==k_unTrackedDeviceIndexInvalid)source=ov->GetPrimaryDashboardDevice();
                grab.queueScrollDepth(source,e.data.scroll.ydelta,e.eventAgeSeconds);continue;
            }
            if(e.eventType==VREvent_MouseButtonUp&&grab.active&&(e.trackedDeviceIndex==grab.controller||e.trackedDeviceIndex==k_unTrackedDeviceIndexInvalid||e.trackedDeviceIndex==0)){release();continue;}
            if(!poses[0].bPoseIsValid||(e.eventType!=VREvent_MouseMove&&e.eventType!=VREvent_MouseButtonDown))continue;
            if(grab.active&&e.trackedDeviceIndex==grab.controller)continue;
            if(e.eventType==VREvent_MouseButtonDown&&e.data.mouse.button!=VRMouseButton_Left)continue;
            auto index=e.trackedDeviceIndex;if(index==0||index>=k_unMaxTrackedDeviceCount||!poses[index].bPoseIsValid)index=ov->GetPrimaryDashboardDevice();if(index>=k_unMaxTrackedDeviceCount||!poses[index].bPoseIsValid)continue;
            V origin=point(fromvr(poses[index].mDeviceToAbsoluteTracking),{0,0,0});
            V target=point(lastPlane,{(e.data.mouse.x/512-.5f)*lastWidth,(e.data.mouse.y/512-.5f)*lastWidth,0});
            M inv=inverse(lastModel);V a=mul(point(inv,origin),1/lastSize),b=mul(point(inv,target),1/lastSize),delta=sub(b,a);
            if(fabsf(delta[2])<.0001f)continue;float t=(ScreenZ-a[2])/delta[2];if(t<0)continue;V hit=add(a,mul(delta,t));
            bool front=a[2]>ScreenZ;pointer=index;overHandle=onGrabHandle(hit);
            if(e.eventType!=VREvent_MouseButtonDown)continue;
            auto button=buttonRay(a,delta);
            if(!button.empty()){game.press(QString::fromStdString(button));game.save();ov->TriggerLaserMouseHapticVibration(device,.025f,80,.18f);continue;}
            if(grab.active||(front&&onGameScreen(hit)))continue;
            if(overHandle||onPhoneBody(hit)){
                V grabbed=point(lastModel,mul(hit,lastSize));
                if(docked){world=lastModel;V restored=point(world,mul(hit,size));for(int i=0;i<3;i++)world[12+i]+=grabbed[i]-restored[i];docked=false;placed=true;}
                VRControllerState_t state{};bool watch=VRSystem()->GetControllerState(index,&state,sizeof(state))&&(state.ulButtonPressed&ButtonMaskFromId(k_EButton_SteamVR_Trigger));
                grab.beginAt(index,fromvr(poses[index].mDeviceToAbsoluteTracking),world,grabbed,watch);ov->TriggerLaserMouseHapticVibration(device,.03f,80,.2f);
            }
        }
        uint32_t controller=grab.active?grab.controller:pointer;
        bool focused=controller<k_unMaxTrackedDeviceCount&&poses[controller].bPoseIsValid&&poses[0].bPoseIsValid&&(grab.active||ov->IsHoverTargetOverlay(device));
        int role=focused?int(VRSystem()->GetControllerRoleForTrackedDeviceIndex(controller)):0;
        auto action=nav.update(focused,role,dt,grab.active,controller);repositioning=false;
        if(grab.active){float before=grab.distance;grab.updateDepth(action.depthY,dt,action.depthAxis);repositioning=std::abs(grab.distance-before)>.00001f;if(repositioning)++depthMoves;world=grab.follow(fromvr(poses[controller].mDeviceToAbsoluteTracking));candidate=dock.canSnap(world,size,candidate);}
        if(focused){
            auto press=[&](const char* button){game.press(button);++controllerInputs;};
            if(action.confirm)press("enter");if(action.back)press("back");if(action.friendship)press("state");if(action.settings)press("menu");
            if(action.home)toDock();if(action.x)press(action.x>0?"right":"left");if(action.y)press(action.y>0?"top":"bottom");
        }
        bool visible=poses[0].bPoseIsValid&&(docked?dock.available:placed);
        bool capture=fs::exists(data+"/capture.request");
        if((visible&&now-lastLCD>std::chrono::milliseconds(66))||capture){drawLCD(lcd,game.snapshot());renderer.screen(lcd);lastLCD=now;}
        if(capture){renderer.showGrabHandle=false;renderer.draw(identity(),rotated(-.18f,.05f),{0,0,.3f},{0,0,.3f},.115f,1);renderer.save(data+"/capture.png");fs::remove(data+"/capture.request");}
        renderer.showGrabHandle=true;renderer.highlightGrabHandle=grab.active||overHandle;renderer.dockGrabHandle=candidate;
        if(poses[0].bPoseIsValid&&(docked?dock.available:placed)){
            M model=docked?dock.model(size):world;float effective=docked?dock.size(size):size;
            V p=point(model,{0,0,ScreenZ*effective}),h=point(head,{0,0,0}),z=norm(sub(h,p)),x=norm(cross({0,1,0},z));M plane=pose(x,cross(z,x),z,p);float width=InteractionPlaneWidth*effective;
            if(dot(sub(h,p),sub(h,p))>.015f){
                V eyes[2];for(int i=0;i<2;i++)eyes[i]=point(mm(head,fromvr(VRSystem()->GetEyeToHeadTransform((EVREye)i))),{0,0,0});renderer.draw(plane,model,eyes[0],eyes[1],width,effective,false);glFinish();Texture_t texture{(void*)(uintptr_t)renderer.tex,TextureType_OpenGL,ColorSpace_Gamma};check(ov->SetOverlayTexture(device,&texture));check(ov->SetOverlayWidthInMeters(device,width));auto transform=magicshelfTransform(plane);check(ov->SetOverlayTransformAbsolute(device,TrackingUniverseStanding,&transform));check(ov->ShowOverlay(device));lastPlane=plane;lastModel=model;lastSize=effective;lastWidth=width;
            }else ov->HideOverlay(device);
        }else ov->HideOverlay(device);
        if(!ready){std::string pid=std::to_string(getpid());ftruncate(lock,0);pwrite(lock,pid.data(),pid.size(),0);game.save();if(getenv("MAGICSHELF_SESSION"))magicshelfReady();ready=true;std::cerr<<"Pocket Pikachu ready (native JavaScript, no browser)\n";}
        std::this_thread::sleep_for(std::chrono::milliseconds(22));
    }
    game.save();savePlacement();ov->DestroyOverlay(device);VR_Shutdown();return 0;
}
int main(int argc,char**argv){try{
    const std::string mode=argc>1?argv[1]:"";std::unique_ptr<QCoreApplication> app;
    if(mode=="--desktop")app=std::make_unique<QApplication>(argc,argv);else app=std::make_unique<QCoreApplication>(argc,argv);
    if(mode=="--sound-test"){
        HandheldAudio audio;audio.button("enter","pokeball");
        for(int i=0;i<100&&!audio.count();i++)std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if(!audio.count())throw std::runtime_error("Sound test did not play");
        std::cerr<<"Pocket Pikachu audio played through the default headset output\n";return 0;
    }
    std::string package=fs::canonical("/proc/self/exe").parent_path().parent_path().string();
    std::string data=getenv("MAGICSHELF_DATA_DIR")?getenv("MAGICSHELF_DATA_DIR"):(QDir::homePath()+"/.local/share/magicshelf-pocket-pikachu").toStdString();
    QTemporaryDir previewData;if(mode=="--preview")data=previewData.path().toStdString();fs::create_directories(data);
    int lock=open((data+"/instance.lock").c_str(),O_CREAT|O_RDWR|O_CLOEXEC,0600);if(lock<0)throw std::runtime_error("Cannot open instance lock");bool held=flock(lock,LOCK_EX|LOCK_NB)!=0;
    if(mode=="--show"||mode=="--dock"){if(!held)return 1;char text[32]{};pread(lock,text,31,0);pid_t pid=atoi(text);return pid>1&&kill(pid,mode=="--dock"?SIGUSR2:SIGUSR1)==0?0:1;}
    if(held)throw std::runtime_error("Pocket Pikachu is already running with this save");
    std::signal(SIGINT,stop);std::signal(SIGTERM,stop);std::signal(SIGUSR1,show);std::signal(SIGUSR2,home);
    Emulator game(QString::fromStdString(package),QString::fromStdString(data));Renderer renderer(package);
    if(mode=="--preview"){if(argc<3)throw std::runtime_error("--preview requires an output PNG path");game.tick(3500);game.press("enter");game.tick(6000);game.press("enter");game.tick(100);Canvas lcd(ScreenWidth,ScreenHeight);drawLCD(lcd,game.snapshot());renderer.screen(lcd);float yaw=argc>3?std::stof(argv[3]):-.18f;renderer.draw(identity(),rotated(yaw,.05f),{0,0,.3f},{0,0,.3f},.115f,1);renderer.save(argv[2]);return 0;}
    HandheldAudio audio;game.buttonSound=[&](const QString& button,const QString& state){audio.button(button.toStdString(),state.toStdString());};
    if(mode=="--desktop"){Desktop desktop(game,renderer);desktop.show();int code=app->exec();game.save();return code;}
    if(!mode.empty())throw std::runtime_error("Unknown option");return runVR(game,renderer,audio,package,data,lock);
}catch(const std::exception&e){std::cerr<<"Pocket Pikachu: "<<e.what()<<"\n";return 1;}}
