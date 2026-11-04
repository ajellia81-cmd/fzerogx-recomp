# Build17 review

## Menu pause

In-app Recomp pauses no longer synchronously drain the render thread on UIKit.
The cooperative gate still parks guest execution and GPU submissions. Background
transitions retain the renderer drain before iOS suspension. The audio output unit
keeps pulling silence during an in-app pause without consuming the game's audio
ring or deactivating the session. Actual backgrounding still stops output and
deactivates the session. Interruption recovery remains pending through a menu pause.

Haptics stop asynchronously for an in-app menu and retain the prepared engine.
Backgrounding still invalidates, drains and retires the engine. Generation checks
keep queued work from operating on a newer engine.

## Source canvas

Legacy 24x24 I4 glyph runs now receive the same per-primitive compensation in Menu
as in World. Their run center is preserved. Already-compensated glyphs, general
bitmap atlases, preview cameras, copy targets, panel pieces and clipping layout
remain on the existing menu canvas; arbitrary bitmap aspect inference is excluded.

Known startup logo quads derive their canvas from the actually sampled texels and
geometry. Both 4:3 and already-compensated 16:9 layouts are accepted, including a
return from a menu. Fade-only frames retain that logo's canvas. Unrelated flat
artwork does not inherit the previous logo's canvas.

## Course and profile coverage

The private capture routes cover Practice, GP, Time Attack, return-to-logo,
Options, Story, Replay, Profile, Customize and Shop. Additional Profile routes
exercise X, Y and Z after opening details. They are ordinary input sequences;
their completion alone does not prove which artwork variant was reached.

Startup shader/blend collection now consumes every captured route rather than
only Replay/Profile/Customize/Shop. This reduces the missing configuration gap
at course entry; it is not evidence that CPU/IO stalls or every Metal pipeline
stall have been eliminated.

## Verification and delivery

- Functional code plus regression fixture: `e27e69bcc9bb6adda56b047e2e232e70332092cc`.
- ARM64 runtime/renderer/audio/controller compile, renderer and lifecycle fixtures,
  and detached-thread resource checks: Actions `41793968703`, success.
- Full package source/request: `83302c1992f0a607954bd0c8a9b856ed856c0d58`.
- Full delivery run: `41794101615` (in progress at this review).
- Private capture review run: `41793565818` (in progress at this review).

GameCube image-based mappings and upstream defaults remain present. Physical
menu latency, 2D pilot artwork size, existing-card Replay dialogs, all course
variants and measured 120Hz output still require coverage beyond ROM-free
fixtures. Do not describe these as completed device tests.
