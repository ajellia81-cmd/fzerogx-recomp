#!/usr/bin/env python3
"""Generate suspend-aware iOS variants without changing the pinned shared core."""
from pathlib import Path
import sys

root,out=map(Path,sys.argv[1:])
out.mkdir(parents=True,exist_ok=True)
def variant(source,name,replacements):
    text=(root/source).read_text()
    for old,new in replacements:
        if text.count(old)!=1:
            raise SystemExit(f"Pinned runtime changed: {source}: {old[:70]!r}")
        text=text.replace(old,new)
    (out/name).write_text(text)

# The runtime never joins these fire-and-forget host threads. POSIX defaults
# to joinable, retaining each finished thread's 64 MB stack until pthread_join.
# Select detached state BEFORE creation, so even immediate exit is reclaimed.
variant('runtime/platform.cpp','platform_ios.cpp',[
 ('    pthread_t th;\n    int rc = pthread_create(&th, &attr, fn, arg);',
  '    int detach_rc=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);\n'
  '    if(detach_rc){pthread_attr_destroy(&attr);return false;}\n'
  '    pthread_t th;\n    int rc = pthread_create(&th, &attr, fn, arg);')])
variant('runtime/threads.cpp','threads_ios.cpp',[
 ('#include "runtime.h"','#include "runtime.h"\n#include "lifecycle.h"'),
 ('extern "C" void irq_poll(CPU* c) {','extern "C" void irq_poll(CPU* c) {\n    ios_lifecycle::guest_checkpoint();')])
variant('runtime/cpu.cpp','cpu_ios.cpp',[
 ('#include "runtime.h"','#include "runtime.h"\n#include "lifecycle.h"'),
 ('void timing_init() {','void timing_init() {\n    ios_lifecycle::gate().reset_clock_baseline();'),
 ('return (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();',
  'const uint64_t elapsed=std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();\n    const uint64_t held=ios_lifecycle::clock_hold_ns();\n    return elapsed>held?elapsed-held:0;')])
variant('visionos/audio_coreaudio.mm','audio_ios.mm',[
 ('#include "runtime.h"','#include "runtime.h"\n#include "lifecycle.h"'),
 ('static void resume_after_interruption(int tries) {',
  'static void resume_after_interruption(int tries) {\n    if(ios_lifecycle::gate().paused() || !g_unit)return;'),
 ('bool audio_open() {',
  'void ios_audio_suspend(){if(g_unit)AudioOutputUnitStop(g_unit);[AVAudioSession.sharedInstance setActive:NO error:nil];}\n'
  'void ios_audio_resume(){resume_after_interruption(0);}\n\nbool audio_open() {')])

# Original timing policy only; game instructions/data are not reproduced.
p=out/'cpu_ios.cpp'
p.write_text(p.read_text()+'''
#include "logo_timing.h"
static void ios_logo_advance(CPU* c,uint64_t ticks){
    const uint64_t end=now_ticks()+ticks;
    while(now_ticks()<end){
        uint64_t part=std::min<uint64_t>(end-now_ticks(),TB_FREQ/120);
        g_vcount=g_vcount+(part+g_ticks_per_edge-1)/g_ticks_per_edge;
        irq_poll(c);
    }
}
extern "C" void fzgx_ios_logo_delay(CPU* c){
    const uint32_t caller=c->lr;
    const uint64_t ticks=ios_logo::delay_ticks(caller);
    if(ticks){ios_logo::note_submission(host_ns());ios_logo_advance(c,ticks);return;}
    if(caller!=ios_logo::title_tick)return;
    // This caller has already installed the relocated title-state pointer.
    const uint32_t state=c->r[30];
    const unsigned phase=LD8(state+576);
    static const bool trace=std::getenv("FZGX_LOGO_TRACE")!=nullptr;
    static unsigned previous=~0u;
    if(trace && previous!=phase){
        fprintf(stderr,"iOS logo phase=%u counter=%u host=%.6f virtual=%.6f deadline=%.6f\\n",phase,unsigned(LD16(state+574)),host_ns()/1e9,now_ticks()/double(TB_FREQ),ios_logo::hold_deadline()/1e9);
        previous=phase;
    }
    static bool held=false;
    if(!held && phase==1){
        held=true;
        // Guest/renderer cooperation and pause-adjusted host time keep a slow
        // loader or a foreground transition from shortening the visible hold.
        for(uint64_t deadline=ios_logo::hold_deadline();deadline && host_ns()<deadline;){
            ios_logo_advance(c,TB_FREQ/120);
            deadline=ios_logo::hold_deadline();
        }
    }
    if(held && ios_logo::hold_deadline() && host_ns()>=ios_logo::hold_deadline())ios_logo::hold_complete=true;
    if(phase>2 && ios_logo::av_updates.load())ios_logo::av_complete=true;
    static ios_logo::AVTimer timer;
    const uint16_t before=LD16(state+574);
    const uint16_t after=timer.remaining(phase,before);
    if(phase==2)ios_logo::note_av_update(before);
    if(trace && phase==2 && (before>138 || before<=30))fprintf(stderr,"iOS AV timer before=%u policy=%u host=%.6f\\n",unsigned(before),unsigned(after),host_ns()/1e9);
    if(after!=before)ST16(state+574,after);
}
''')
