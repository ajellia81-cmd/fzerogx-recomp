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

The original project has no app-icon asset. A preview using the original game
logo awaits the user's explicit approval. No follow-up IPA build may be started
before that approval. The uploaded personal save and its converted raw card are
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
