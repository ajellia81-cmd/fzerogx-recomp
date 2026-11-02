#import <GameController/GameController.h>
#import <CoreHaptics/CoreHaptics.h>
#import <QuartzCore/QuartzCore.h>
#include "rumble.h"
#include <atomic>
#include <cstdio>

namespace {
dispatch_queue_t queue() {
    static dispatch_queue_t q=dispatch_queue_create("fzerogx.rumble",DISPATCH_QUEUE_SERIAL);
    return q;
}
CHHapticEngine* engine;
id<CHHapticAdvancedPatternPlayer> player;
std::atomic<bool> desired{false}, ready{false}, pending{false};
std::atomic<int> route{0}; // 1 controller, 2 phone/system passthrough, 3 unsupported
bool playing=false;
double testUntil=0, retryAt=0; // main thread

void apply_motor() {
    if(!player)return;
    const bool on=desired.load();
    if(on==playing)return;
    NSError* error=nil;
    BOOL ok=on ? [player startAtTime:CHHapticTimeImmediate error:&error]
               : [player stopAtTime:CHHapticTimeImmediate error:&error];
    if(ok)playing=on;
    else {ready=false;fprintf(stderr,"haptics: motor command failed (%ld)\n",(long)error.code);}
}

void prepare(GCDeviceHaptics* provider) {
    if(player)[player stopAtTime:CHHapticTimeImmediate error:nil];
    CHHapticEngine* old=engine;
    player=nil;engine=nil;playing=false;
    old.stoppedHandler=nil;old.resetHandler=nil;
    [old stopWithCompletionHandler:nil];
    engine=provider ? [provider createEngineWithLocality:GCHapticsLocalityDefault] : nil;
    int target=1;
    NSError* error=nil;
    if(!engine) {
        target=2;
        if(CHHapticEngine.capabilitiesForHardware.supportsHaptics)
            engine=[[CHHapticEngine alloc] initAndReturnError:&error];
    }
    if(!engine){route=3;ready=true;pending=false;return;}
    engine.playsHapticsOnly=YES;
    engine.autoShutdownEnabled=NO;
    __weak CHHapticEngine* weak=engine;
    engine.resetHandler=^{ dispatch_async(queue(),^{ if(engine==weak)ready=false; }); };
    engine.stoppedHandler=^(CHHapticEngineStoppedReason reason) {
        dispatch_async(queue(),^{ if(engine==weak)ready=false; });
    };
    if(![engine startAndReturnError:&error]) {
        route=0;ready=false;pending=false;
        fprintf(stderr,"haptics: engine start failed (%ld)\n",(long)error.code);return;
    }
    CHHapticEventParameter* intensity=[[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity value:.65f];
    CHHapticEventParameter* sharpness=[[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:.25f];
    CHHapticEvent* event=[[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticContinuous parameters:@[intensity,sharpness] relativeTime:0 duration:1];
    CHHapticPattern* pattern=[[CHHapticPattern alloc] initWithEvents:@[event] parameters:@[] error:&error];
    player=pattern ? [engine createAdvancedPlayerWithPattern:pattern error:&error] : nil;
    if(!player){route=0;ready=false;pending=false;return;}
    player.loopEnabled=YES;
    route=target;ready=true;pending=false;
    fprintf(stderr,"haptics: prepared %s\n",target==1?"controller":"phone / system passthrough");
    apply_motor();
}
}

void ios_rumble_update(bool motor) {
    GCController* controller=GCController.current;
    if(!controller.extendedGamepad) {
        controller=nil;
        for(GCController* candidate in GCController.controllers)
            if(candidate.extendedGamepad){controller=candidate;break;}
    }
    static GCController* last=nil;
    static bool initialized=false;
    const bool changed=!initialized || last!=controller;
    if(changed){last=controller;initialized=true;ready=false;retryAt=0;}
    bool on=motor || CACurrentMediaTime()<testUntil;
    const bool edge=desired.exchange(on)!=on;
    if(!ready && !pending && CACurrentMediaTime()>=retryAt) {
        pending=true;retryAt=CACurrentMediaTime()+2;
        GCDeviceHaptics* provider=controller.haptics;
        dispatch_async(queue(),^{prepare(provider);});
    } else if(edge && ready) dispatch_async(queue(),^{apply_motor();});
}
void ios_rumble_test(){testUntil=CACurrentMediaTime()+.3;ios_rumble_update(false);}
const char* ios_rumble_route(){
    switch(route.load()) {case 1:return "패드";case 2:return "아이폰/패스스루";case 3:return "미지원";default:return "준비 중";}
}
