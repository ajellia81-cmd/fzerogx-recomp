# Build38 foreground audio recovery

Owner log(7).txt confirms haptics recovered in build37 but game audio did not.
There are foreground-preparation and prepared-controller lines without an
audio-resumed line. Source inspection identifies a deterministic ordering bug:
build37 called ios_audio_resume while the lifecycle gate was still paused;
resume_after_interruption explicitly returns in that state. No retry is armed
by that early return, leaving the stopped output unit silent.

Fix: Gate::resume_devices unpauses first, restores audio, then requests haptics
in the SAME foreground callback. Duplicate activation callbacks do not repeat
device restoration. The explicit foreground haptic preparation path remains,
including silent warmup with a Recomp menu open; no input or motor event is
needed. Do not copy build37's audio-before-unpause ordering to another port.

The shared helper is exercised by30 foreground cycles in session_policy.cpp:
audio/haptics callbacks must observe an unpaused gate, audio precedes haptics,
and duplicate resume calls must invoke neither callback. Tests use the actual
helper called from UIKit; physical audio output remains an owner-device check.

Source6f464d14eae65efd717dbe44205551d498979eb4.
Runtime check44461767784 and private device build44461767720 succeeded on
macOS. No local build. No game/renderer/Item/badge policy changes were made.

Delivered version38 IPA:14974744 bytes, seven package files, ARM64 unsigned,
ZIP CRC valid, no ROM/save/key/log/capture payload.
SHA256:7617cbcdfea66f782bd8ea996d4d16bc2bdfaa6f1330443a2e189a73c43e847e.
Library:libfile_493d2fd9083881918c1a130bbfe90948.
Owner should verify foreground game audio and first post-resume rumble.
