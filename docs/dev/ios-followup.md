# Native iOS follow-up

The first unsigned IPA booted into the game's title/demo on the user's device.
The user has not provided measurements of sustained 60 fps or 120 fps.

The follow-up source adds:

- Main-view perspective projection expands horizontally for the drawable's aspect,
  preserving vertical field of view. HUD geometry keeps its proportions, with
  fullscreen 2D composites retaining coverage. Secondary/offscreen viewports are
  excluded. Game-side object culling and screen effects require device inspection.
- An on-screen analog stick, A/B/X/Y, L/R/Z and Start appear only without an
  extended gamepad. Connecting a pad clears touch state and hides the overlay;
  disconnecting it enables touch again. Backbone goes through GameController.
- Experimental motion interpolation keeps guest FPS=60 and step scale=1. It
  matches draw topology, material/attributes and source indices conservatively,
  blends view-space vertices (including camera motion), and rejects large jumps.
  Reordered, spawned, animated or unmatched geometry snaps to the current tick.
  Multi-strip XFB batches are excluded. This is not complete interpolation of
  every effect, and sustained 120 fps has not been demonstrated.
- Intermediate GPU passes use isolated EFB/copy targets, suppress guest RAM copy
  writeback and presentation counters, and restore canonical targets before the
  real frame. Texture preparation/cache aging occurs once per guest batch.
  Logs distinguish batch rate, swap attempts and generated intermediate rate;
  none is a measurement of frames actually displayed by the physical screen.
- Startup exposes a 120 fps interpolation switch on 120 Hz screens. The display
  link requests 120 Hz; normal render timing remains EGL swap interval 1.
- Startup imports validated 512 KiB ANSI raw memory cards with atomic replacement
  and a unique backup of an existing card. Direct GCI import is not implemented.
  A personal GCI can be migrated externally with Dolphin into a compatible raw
  card, then imported or placed in Documents as memcard_a.raw while the app is
  fully closed. Never put personal saves in the public repository or IPA.

The original project has no app-icon asset. The user supplied cover artwork,
approved its square crop on 2026-11-02, and authorized the follow-up IPA build.
Only that approved icon artwork is bundled; no disc assets or saves are bundled. The uploaded personal save and its converted raw card are
kept outside git. This follow-up chooses import rather than embedding the save.

## Verification before icon approval

- iOS ARM64 object compilation of runtime, generated iOS renderer, UIKit launcher,
  touch controls, audio and controller adapter passed on Xcode 26.3:
  https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/41368075896
  Source commit: ae302d6f3a5a36922a81391fa7ea3cf6807ca276.
- Cloud C++ tests covered midpoint vertex packing, material mismatch, camera-jump
  rejection and raw-card metadata validation. A software GLES3 context verified
  framebuffer color/depth copying and that drawing into the transient framebuffer
  leaves the canonical color target unchanged. This was not an iOS/Metal test.
- Icon generation reproduces the preview pixels and generates iPhone/iPad asset
  catalog entries. No follow-up IPA packaging or device execution has occurred.
- The first two compile checks exposed a generated-source include-path omission
  and missing ARC on the object-check target. Both were corrected in the passing run.

Full-game build 41369088303 compiled the game and icon assets but failed at the
final link: CGRect helpers in touch_controls.mm require explicit CoreGraphics
linkage. The framework was added for the next build. Upload of diagnostics in
an always-run step is not evidence that compilation or packaging succeeded.

## Build 2 delivery

- Full compile, link and IPA packaging succeeded on Xcode 26.3:
  https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/41370418909
  Built source commit: fbd325edeb3d409a6fa1804e4470a03617234435.
- FZeroGX-iOS-build2-unsigned.ipa: 14,890,077 bytes.
  SHA256: ddb287ae7eae7fb1f459103b99fcea36f14cd0e1139378e0f27eb3d1d41acef7.
- Archive audit confirmed thin ARM64 iPhoneOS executable and ANGLE dylib, no
  code-signature load commands or signing payload, registered AppIcon, and no
  disc image, memory card, REL/DOL input or generated source files.
- The app executable declares iOS 17, but the ANGLE dependency requires iOS
  26.2; that dependency sets the effective minimum for this build.
- The approved cover icon, expanded horizontal FOV, controller-aware touch
  overlay, raw memory-card import and experimental interpolation are included.
- No device execution, gameplay, personal-save loading or physical 120 fps
  display measurement of build 2 has been verified. Earlier user boot feedback
  applies only to the first IPA.

## Build 3 fixes after device feedback

The user reports no vibration, horizontally squeezed UI, corrupted vehicle
selection composites, and low apparent frame rate in build 2.

- Menu/vehicle-selection batches now keep original projections and are displayed
  at 4:3. Horizontal expansion and interpolation are limited to the existing
  guest-frame hook's live-race classification. Race HUD correction uses the main
  camera aspect, including the game's widescreen option. Fullscreen copy
  composites are excluded from a second aspect correction.
- Removed per-state full-batch bounding-box searches. Transient EFB/depth and
  texture-copy framebuffers are pooled and reused rather than allocated each
  frame. Copy-size transitions fall back to the native pass.
- Default internal rendering scale is 2x, with optional 3x at startup; physical
  drawable size is unchanged.
- An always-visible native overlay shows output, original and interpolated
  frame rates averaged over one second. These count completed renderer/swap
  submissions, not physical screen scanout.
- The guest motor state is atomically sampled on the guest-frame hook. The iOS
  adapter prepares controller CoreHaptics on a serial queue, falling back to
  phone/system haptics when no controller engine is provided. Startup offers a
  vibration test. Output hardware and perceptible vibration need device testing.
- ARM64 compile check 41386062300 passed the UIKit, CoreHaptics and renderer
  changes. A software GLES3 test additionally checked unchanged menu projections,
  expected race projection scaling, unscaled fullscreen composites, pooled FBO
  identity and isolated color/depth target copies. Neither is a device test.
