# iOS build 8 validation

Source: `53d9d1ca10799ff8f09f9ea4016bf95c857d9d7a`.
Runtime compile check: GitHub Actions `41544733167` (success, final source).
Preceding shader-cache check `41543472312` also passed.
Final device build: GitHub Actions `41544817408` (success, 9m22s total).
Intermediate device run `41543595406` compiled, linked and packaged successfully
but is superseded to reset stale history when Smooth motion is turned off.
Earlier device run `41542706567` was intentionally cancelled before packaging to
include the driver-specific shader-key normalization. It is not a failed compile
or a delivered IPA.

## Session and settings

The UIKit startup chooser remains for first setup or recovery. Successful boot
sets a persistent flag. Later launches validate the disc header and the standard
512KiB US memory-card metadata before booting automatically. Existing installs
with both files migrate. A missing card stops automatic launch; explicit Start
can create a new blank card. Invalid existing cards are preserved and require a
valid reimport. This does not validate the game's internal save payload.

Recomp settings open with Guide/Home where iOS delivers it, with a top-right
on-screen fallback. Smooth motion, FPS visibility and controller assignments are
saved in user defaults. Turning Smooth motion off clears old interpolation history;
turning it back on starts with a fresh native frame. There is no duplicate startup 120fps choice. Default
physical layout: A accelerate, B brake, X boost, LB side attack, RB spin; Y is
unassigned. Triggers retain their analog behavior. Opening Recomp parks the game
and prevents menu input from reaching it.

Background exit has been replaced by an iOS-only cooperative gate, GPU drain,
audio/rumble suspension and paused-time host-clock accounting. Resume retains
guest memory and queued renderer/writeback state. No shared desktop/VR core
files or submodule revisions changed.

## Aspect evidence and limits

The renderer formerly consulted the latest guest atomics while presenting older
queued frames. Presentation context now travels in the existing ordered FIFO
draw tags. Known startup materials and fades retain their flat canvas even when
legacy metadata is stale; zero-count draws no longer make a logo look like 3D.

A private software-GLES replay deliberately applied newer Selection metadata to
a saved startup AV frame. On a 1434x660 target, the previous build's white logo
footprint was 387x290; the new build's was 290x290. The current renderer returned
GL_NO_ERROR. This reproduces the aspect-classification vulnerability; it does
not establish the exact timing of the user's single-frame SEGA transition.

Metadata policies were checked on 68 saved renderer snapshots, including 15
startup-art and 11 movie frames. Representative vehicle selection, race pause
and movie frames were replayed privately. Existing menu 16:9, world fullscreen,
HUD, modal and original-movie aspect policies remain in place. Captures and
rendered game images are private, not committed or bundled.

The finish-rank `1st` artwork was not captured or compared to an original-device
reference. Its intended proportions remain unconfirmed. No speculative global
HUD correction was added for it.

## First-use stalls

Sampled course/world frames had 8-13 shader keys absent from the shipped list
when dual-source blending was unavailable. Every such absent key was the
fallback of a shipped dst_alpha=2 key. Cache loading now normalizes both disk
and shipped keys to actual driver capability before deduplication. Stored binary
source hashes still invalidate incompatible binaries. After normalization, all
68 saved snapshots' base shader keys were covered by the shipped list.

The shipped list is also loaded when the driver has no program-binary support.
Precompilation runs in stepped startup work with visible progress and suspend
checkpoints. Slow render frames log shader/program-state timing. First Metal
pipeline creation and uploads may still hitch; no device performance claim is
made from shader-key coverage alone.

## Validation boundary

A software-GLES ON -> OFF -> ON replay of a saved course frame returned
"first frame ready" after re-enabling motion with GL_NO_ERROR, confirming that
old interpolation history is discarded. This is not a physical-device test.

ROM-free fixtures cover queued context vs newer globals, startup material
classification, empty draws, shader capability normalization, modal groups,
movie aspect, lifecycle drain/resume/clock accounting and controller defaults.
ARM64 compile validation passed. Neither cloud software-GLES replay nor a
successful IPA build proves application launch, background restoration, Guide
button delivery, gameplay or 120Hz physical display presentation on a phone.


## Final IPA audit

- File: `FZeroGX-iOS-build8-unsigned.ipa`, 14,920,727 bytes.
- SHA256: `a8cff2681c3075b1ca301d0590ef1da1ad9cbca91c95f7a9b45a645d16635e6a`.
- Seven bundle files: executable, ANGLE dylib, plist/PkgInfo and approved icons.
- Both Mach-O images: thin ARM64, iPhoneOS, no code-signature load command.
- No disc images, memory cards, save payloads, generated C, diagnostic captures,
  embedded provisioning profile or signature directory.
- Bundle version 8, identifier `com.ajellia81.fzerogx`.
- The executable deployment target is iOS 17; the embedded ANGLE minimum is
  iOS 26.2, so the effective requirement remains iOS 26.2 or later.
- Final compilation succeeded directly; no internal-Clang retry was needed.
- Device application startup, gameplay, resume, controller Guide behavior and
  physical 120Hz output remain unverified for this build.
