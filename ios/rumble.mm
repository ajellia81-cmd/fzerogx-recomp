#import <GameController/GameController.h>
#import <CoreHaptics/CoreHaptics.h>
#import <QuartzCore/QuartzCore.h>
#include "rumble.h"
#include <atomic>
#include <cstdio>

namespace {
dispatch_queue_t queue(){static auto q=dispatch_queue_create("fzerogx.rumble",DISPATCH_QUEUE_SERIAL);return q;}
// All engine/player references live on queue(). Callback threads only invalidate
// atomics. Each preparation gets a generation so old callbacks cannot tear down
// a newer engine, and suspension cancels work before draining the queue.
CHHapticEngine* engine;
id<CHHapticAdvancedPatternPlayer> player;
std::atomic<bool> desired{false},ready{false},pending{false},permitted{true},alive{false};
std::atomic<uint64_t> generation{1};
std::atomic<int> route{0};
bool playing=false;
double testUntil=0,retryAt=0; // main thread only
bool current(uint64_t ticket){return permitted.load() && generation.load()==ticket;}
void retire(){
    // Never stop a stale player: CoreHaptics can raise NSException, rather than
    // returning NSError, when iOS has already stopped its engine.
    player=nil;playing=false;alive=false;ready=false;
    CHHapticEngine* old=engine;engine=nil;
    @try{old.stoppedHandler=nil;old.resetHandler=nil;[old stopWithCompletionHandler:nil];}
    @catch(NSException* exception){fprintf(stderr,"haptics: engine teardown exception %s\n",exception.name.UTF8String);}
}
void finish(uint64_t ticket){if(generation.load()==ticket)pending=false;}
void apply_motor(uint64_t ticket){
    if(!current(ticket)||!ready||!alive||!player)return;
    bool on=desired.load();if(on==playing)return;
    @try{
        NSError* error=nil;
        BOOL ok=on?[player startAtTime:CHHapticTimeImmediate error:&error]:[player stopAtTime:CHHapticTimeImmediate error:&error];
        if(ok)playing=on;
        else{ready=false;alive=false;player=nil;playing=false;fprintf(stderr,"haptics: motor command failed (%ld)\n",(long)error.code);}
    }@catch(NSException* exception){
        // An external engine stop can race the check above. Haptics failure
        // must never terminate the game, including in LiveContainer.
        ready=false;alive=false;player=nil;playing=false;
        fprintf(stderr,"haptics: motor exception %s; engine will be recreated\n",exception.name.UTF8String);
    }
}
void prepare(GCDeviceHaptics* provider,uint64_t ticket){
    if(!current(ticket)){finish(ticket);return;}
    retire();
    @try{
        engine=provider?[provider createEngineWithLocality:GCHapticsLocalityDefault]:nil;
        int target=1;NSError* error=nil;
        if(!engine){target=2;if(CHHapticEngine.capabilitiesForHardware.supportsHaptics)engine=[[CHHapticEngine alloc] initAndReturnError:&error];}
        if(!engine){if(current(ticket)){route=3;ready=true;}finish(ticket);return;}
        engine.playsHapticsOnly=YES;engine.autoShutdownEnabled=NO;
        void (^invalidated)(void)=^{
            if(generation.load()!=ticket)return;
            alive=false;ready=false;
            dispatch_async(queue(),^{if(generation.load()==ticket)retire();});
        };
        engine.resetHandler=invalidated;
        engine.stoppedHandler=^(CHHapticEngineStoppedReason reason){invalidated();};
        if(![engine startAndReturnError:&error]){fprintf(stderr,"haptics: engine start failed (%ld)\n",(long)error.code);retire();finish(ticket);return;}
        if(!current(ticket)){retire();finish(ticket);return;}
        auto intensity=[[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:.65f];
        auto sharpness=[[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:.25f];
        auto event=[[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticContinuous parameters:@[intensity,sharpness] relativeTime:0 duration:1];
        auto pattern=[[CHHapticPattern alloc] initWithEvents:@[event] parameters:@[] error:&error];
        player=pattern?[engine createAdvancedPlayerWithPattern:pattern error:&error]:nil;
        if(!player){retire();finish(ticket);return;}
        player.loopEnabled=YES;
        if(!current(ticket)){retire();finish(ticket);return;}
        alive=true;route=target;ready=true;finish(ticket);
        fprintf(stderr,"haptics: prepared %s\n",target==1?"controller":"phone / system passthrough");
        apply_motor(ticket);
    }@catch(NSException* exception){
        fprintf(stderr,"haptics: preparation exception %s\n",exception.name.UTF8String);
        retire();finish(ticket);
    }
}
}
void ios_rumble_suspend(){
    // Main-thread API. Drain before audio-session deactivation, not afterwards.
    permitted=false;desired=false;++generation;ready=false;pending=false;alive=false;testUntil=0;retryAt=0;
    dispatch_sync(queue(),^{retire();});
}
void ios_rumble_resume(){permitted=true;retryAt=0;}
void ios_rumble_update(bool motor){
    if(!permitted)return;
    GCController* controller=GCController.current;
    if(!controller.extendedGamepad){controller=nil;for(GCController* candidate in GCController.controllers)if(candidate.extendedGamepad){controller=candidate;break;}}
    static GCController* last=nil;static bool initialized=false;
    if(!initialized||last!=controller){last=controller;initialized=true;++generation;ready=false;pending=false;alive=false;retryAt=0;}
    bool on=motor||CACurrentMediaTime()<testUntil;
    bool edge=desired.exchange(on)!=on;
    if(!ready&&!pending&&CACurrentMediaTime()>=retryAt){
        pending=true;retryAt=CACurrentMediaTime()+2;
        uint64_t ticket=++generation;GCDeviceHaptics* provider=controller.haptics;
        dispatch_async(queue(),^{prepare(provider,ticket);});
    }else if(edge&&ready){uint64_t ticket=generation.load();dispatch_async(queue(),^{apply_motor(ticket);});}
}
void ios_rumble_test(){if(!permitted)return;testUntil=CACurrentMediaTime()+.3;ios_rumble_update(false);}
const char* ios_rumble_route(){switch(route.load()){case 1:return "패드";case 2:return "아이폰/패스스루";case 3:return "미지원";default:return "준비 중";}}
