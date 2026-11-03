# iOS build 9 follow-up

Build 8 did not fix background restoration on the user's phone. The supplied
LiveContainer exception shows CoreHaptics `checkEngineRunning:` throwing from
FZeroGX's `ios_rumble_update` dispatch block. The previous assumption that GPU
suspension alone solved restoration is disproved by that device report.

## Haptic lifecycle

The old pause path called ordinary `ios_rumble_update(false)`. That could enqueue
preparation or a player stop after iOS/audio-session suspension had stopped the
engine. `prepare` also stopped the old player without knowing its engine state.

Suspension now first forbids haptic work and invalidates every queued generation,
then drains the haptic queue and retires the engine before audio deactivation.
Retirement discards old players without invoking their start/stop methods. Reset
and stopped callbacks immediately invalidate readiness; callbacks carry unique
preparation generations so late callbacks cannot invalidate a newer engine.
Foreground resume permits fresh engine/player preparation. Player and engine
operations catch Objective-C exceptions, including a stop racing Apple's
asynchronous stopped callback. A haptics failure must not terminate gameplay.
The renderer/guest suspension gate and paused host-clock accounting remain.

## Recomp touch access

The utility button moves to top center, away from the R/Z shoulder touch targets.
It follows the game touch-pad visibility: hidden while a hardware pad is
connected and visible again on disconnection. Guide/Home remains the controller
path, subject to iOS delivering the event. Existing settings and mappings persist.

## Initial logo timing

Upstream `recomp/patches.txt` replaces `fn_80005E0C`, a fixed busy-loop delay, with
an immediate return to avoid the documented AV-title 20fps slowdown. The initial
boot logo also relies on this delay in groups outside its frame-counted fades.
Removing it therefore shortens that initial screen; the earlier belief that all
logo duration was protected by retrace waits does not cover these groups.

Only iOS receives a build-directory copy of the public recomp metadata. Its hook
restores estimated elapsed virtual time for the initial boot delay and invokes
the runtime's existing event/pacing path instead of spinning. It is disabled as
soon as mode-module metadata is present, so it does not restore the AV title's
per-frame busy work. Estimate: 800,000 timebase ticks divided by the documented
~3x overcharge; 50 initial calls add about 0.33 seconds, excluding existing fades.
This is timing metadata, not game code or a protection/data decoder. Exact logo
wall time still requires a phone comparison. Desktop/VR patches are unchanged.

Guest timing initialization also moves after shader precompilation, so shader
warmup is not counted as guest boot elapsed time. The 60Hz VI, simulation and
physics rates and render-only smooth-motion path are unchanged.

## Validation boundary

ROM-free policy/session tests and generated CPU/thread syntax validation passed.
Native ARM64 compile, final IPA audit and real-device results are recorded after
completion. A successful compile does not establish that device suspension,
Guide delivery or the logo's exact duration is fixed.
