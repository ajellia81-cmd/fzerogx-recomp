> Superseded by build38: build37 introduced silent foreground audio by calling audio restoration before unpausing. See [build38 correction](ios-build38-audio-resume.md). Keep event-driven haptic preparation, but restore audio only after the lifecycle gate is released.

# Build37 foreground haptic preparation

Owner confirmed build36 Item FPS fixed and Records badge size normal.
Preserve those fixes.

Source8b493f468c45028fbbf591b763761f83e2670486 changes foreground recovery.
ios_rumble_resume directly requests preparation in the activation call path
before the lifecycle gate releases gameplay. foregroundChanged also requests
preparation when a Recomp menu stays open; motor permission remains muted.
No display-link, controller input, or in-game motor event is needed to initiate
preparation. The shared helper coalesces ready/pending requests and retains
controller-change/generation checks. Suspension cancels foreground work.
CoreHaptics engine construction/start remains on the serial haptic queue;
there is no synchronous main-thread wait for hardware readiness.

macOS runtime run44445103750 and device build44445103785 succeeded.
Since only frontend haptics/lifecycle ordering changed, unchanged Item and
game-wide capture routes were skipped. Common policy checks and full device
compilation/package validation were retained. No local build was run.

Delivered version37:14974989 bytes, seven files, ARM64 unsigned Mach-O,
ZIP CRC valid, no ROM/save/key/log/capture payloads.
SHA256:5f4ed07e26af41a948cd9aca65f9fcd0ac42e12fc811c867c0c0b1a3166d3f77.
Library:libfile_1ad5cfd69b7081919efca35cf21c25c0.

Source-level activation timing is verified. Physical haptic readiness latency
and any perceived foreground stutter still require an owner device check.
