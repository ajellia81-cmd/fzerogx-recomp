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
