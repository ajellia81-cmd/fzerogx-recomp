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
