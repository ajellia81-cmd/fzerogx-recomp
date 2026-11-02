# iOS build 4 validation

The old iOS frontend reused `wants_stereo` as a fullscreen and interpolation
condition. That hook intentionally enables headset stereo only during a live
race; its false answer does **not** mean the game is displaying a 2D menu.
The build-3 device screenshot showed the resulting 4:3 attract demo and about
59 native/output submissions with zero intermediate submissions.

The iOS frontend now classifies the actual visible draw batch. Full-view world
geometry selects a Hor+ presentation regardless of the player-race flag. Small
model previews and offscreen render targets do not by themselves select it.
The race hook remains a fallback. Full-view three-plane I8 video composition
takes priority, preserving the original 4:3 GameCube presentation and any
encoded letterbox, without changing the movie projection or cropping it.

| Situation | Intended presentation | Verification needed on iPhone |
| --- | --- | --- |
| Real-time title/attract demo and intro | Native screen, Hor+ 3D | Yes |
| Pre-race course flyover, grid and countdown | Native screen, Hor+ 3D | Yes |
| Race, spectator/demo race, time attack | Native screen, Hor+ 3D | Yes |
| Replay and finish camera | Native screen, Hor+ 3D | Yes |
| Story/winning scene with full-view real-time 3D | Native screen, Hor+ 3D | Yes |
| 2D menu and small machine preview | Centered 16:9; in-game widescreen option assumed | Yes |
| Prerendered movie | Original GameCube presentation; retain encoded bars | Yes |

Motion matching excludes unrelated BP state changes and unused texture-map
registers, and searches for corresponding indexed geometry across draw-order
changes. Matrix-local geometry, topology, material and displacement checks reject
unmatched objects, large cuts and ambiguous candidates. Lighting/UV animation is
not itself a reason to reject corresponding geometry. Intermediate rendering
continues to use isolated writable GPU targets and cannot advance guest ticks.

The optional host input probe runs the user's privately supplied game through
the original host harness. It reports scene counts and moving matched geometry
without saving vertices, pixels, generated code or game data. This is not a
renderer benchmark, an iOS execution test, or proof of 120 Hz scanout. The probe
requires a full-frame world batch with moving matched geometry before packaging.

The on-screen rate counts submitted native and intermediate renders. It also
shows the selected scene policy and the latest interpolation exclusion reason.
Physical screen FPS and appearance still require device validation. The game
simulation and physics remain at their original 60 Hz.
