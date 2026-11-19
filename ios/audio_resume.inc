// iOS recovery only. The output unit, session calls and retry state are
// serialized on the main queue; stale delayed blocks are generation-checked.
#import <Foundation/Foundation.h>
#include "audio_recovery.h"
static ios_audio::Recovery g_ios_audio_recovery;
static void ios_try_audio_resume(uint64_t ticket){
    if(!g_ios_audio_recovery.current(ticket))return;
    if(ios_lifecycle::gate().paused() || !g_unit){g_ios_audio_recovery.cancel();return;}
    NSError* error=nil;
    const BOOL active=[AVAudioSession.sharedInstance setActive:YES error:&error];
    const OSStatus status=active?AudioOutputUnitStart(g_unit):noErr;
    if(!g_ios_audio_recovery.current(ticket))return;
    if(active && status==noErr){
        const unsigned retries=g_ios_audio_recovery.failures();
        g_ios_session_suspended=false;
        g_ios_audio_recovery.complete(ticket);
        fprintf(stderr,"audio: resumed (%u retries)\n",retries);
        return;
    }
    const unsigned delay=g_ios_audio_recovery.next_delay_ms(ticket);
    if(g_ios_audio_recovery.failures()==1)
        fprintf(stderr,"audio: cannot resume yet: session=%ld output=%d; retry in %ums\n",(long)error.code,(int)status,delay);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,int64_t(delay)*NSEC_PER_MSEC),dispatch_get_main_queue(),^{
        ios_try_audio_resume(ticket);
    });
}
static void resume_after_interruption(int tries){
    (void)tries;
    void (^request)(void)=^{
        if(!g_ios_session_suspended.load())return;
        if(ios_lifecycle::gate().paused() || !g_unit){g_ios_audio_recovery.cancel();return;}
        if(g_ios_audio_recovery.pending())return;
        ios_try_audio_resume(g_ios_audio_recovery.start());
    };
    if(NSThread.isMainThread)request();else dispatch_async(dispatch_get_main_queue(),request);
}
static void ios_audio_interrupted(bool began){
    void (^notify)(void)=^{
        g_ios_session_suspended=true;
        g_ios_audio_recovery.cancel();
        if(began){
            // Keep a fallback for missing end notifications. Manual foreground
            // recovery or a newer interruption invalidates this delayed request.
            const uint64_t ticket=g_ios_audio_recovery.generation();
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW,NSEC_PER_SEC),dispatch_get_main_queue(),^{
                if(g_ios_audio_recovery.generation()==ticket)resume_after_interruption(0);
            });
        }else resume_after_interruption(0);
    };
    if(NSThread.isMainThread)notify();else dispatch_async(dispatch_get_main_queue(),notify);
}
