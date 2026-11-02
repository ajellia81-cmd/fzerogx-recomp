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

## Completed build

- Source commit: `79b29c02bb165542cdda8543bfb25654512f9312`.
- Runtime ARM64 compile check: Actions run `41390042949`, success.
- Complete private game build: Actions run `41390148671`, success (12m 15s).
- Headless title/attract route: 5,867 batches; 4,672 classified world and 1,195
  menu. Of the world batches, 4,631 had moving matched geometry in a complete
  single-present frame (99.1%). No partial or multi-present world batches.
- No movie was encountered on this route. Movie detection was checked with a
  synthetic three-plane fixture only; original-video appearance remains unverified.
- Software GLES3 checks passed for projection/HUD aspect correction, copy-composite
  preservation, framebuffer pooling and isolated color/depth copies. These were
  Linux software-renderer checks, not Metal or device execution.
- IPA: `FZeroGX-iOS-build4-unsigned.ipa`, 14,902,214 bytes, bundle version 4.
- IPA SHA256: `682058ae078430cee5284e114401604553298b82ce4d71454d6ec2d4b675962b`.
- Independent archive/Mach-O audit: arm64 iPhoneOS, unsigned, app icon registered,
  no disc images, saves, raw/generated source or provisioning/signature payloads.
- Build 4 has not been launched on an iPhone here. App execution, gameplay,
  scene-by-scene appearance, physical haptics and actual 120fps remain unverified.
