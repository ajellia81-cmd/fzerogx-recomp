# F-Zero GX iOS port checkpoint — 2026-11-02

## Actual status

This is initial port infrastructure, not an executable game or an IPA.
No Apple SDK compilation, Actions run, device launch, gameplay test, or
120-fps measurement has occurred. The controller adapter is not yet connected
to a UIKit frontend. Fullscreen presentation and interpolation are not implemented.

## Source revisions inspected

- fzerogx-recomp: `5b201ed6066783f3faee2afd52ec764d4a4d182b`
- pinned gcn-recomp submodule: `9e93f1bb7cc4a370af53c895e971fdcab55ed3fc`
- zorrobyte/macos-on-ios-public: `c2562a333d4de4c08da4ceb4a4f1485bddb84e04`

## Selected implementation

Use a real iOS UIKit frontend, GameController, the portable GameCube runtime,
and the existing OpenGL ES 3.0 renderer through ANGLE's Metal backend. This is
Metal-backed rendering, not a newly written direct Metal GX renderer.

The desktop target uses SDL and desktop OpenGL 3.3. macos-on-ios retags Mach-O
and supplies macOS framework shims; its Metal support does not automatically
replace that desktop OpenGL renderer. In contrast, gcn-recomp already has a
visionOS ARM64 frontend, Core Audio output, GL ES 3.0 compatibility, and an
ANGLE build script using WebKit's pinned Xcode project. Reuse those portable
pieces, without visionOS CompositorServices, ARKit or the OpenXR immersive loop.

The isolated ios/CMakeLists.txt compiles portable runtime and renderer objects,
Core Audio, and a GameController input adapter without linking guest symbols.
It does not require ANGLE to compile renderer objects, because the vendored glad
headers provide the GL declarations. Actual launch needs an iPhoneOS build of
ANGLE and an EGL window surface on a CAMetalLayer.

## Next implementation stages

1. Run ios-runtime-check.yml on a macOS runner and fix actual SDK errors.
2. Add UIKit launcher, Files import, CAMetalLayer/EGL surface, audio lifecycle,
   controller-to-PadState translation and boot/render loops. Pause guest/audio
   on backgrounding. Start with the existing 60-Hz timing, step scale 1.0.
3. Render to the actual landscape drawable size; target the requested
   2868x1320. Keep internal render resolution separate from display size.
   Decide between aspect stretch and a game-specific widescreen projection;
   do not claim merely resizing the output is a correct widescreen patch.
4. Build and test 60-fps game operation before interpolation.
5. Retain consecutive immutable GX frame snapshots; establish stable identity
   for camera/object transforms. Interpolate translation and orientation at
   render time without modifying guest memory or executing extra guest ticks.
   Batch vertex/draw order alone is not a stable object identity. Handle spawn,
   disappearance, camera cuts, replays, menus, EFB feedback and particles with
   discontinuity fallback. Never replay resource creation/deletion twice.
6. Request up to 120-Hz presentation only after that path exists. Measure actual
   unique frame presentation and simulation tick rate separately on a device.
   A 120-Hz display callback or repeated 60-Hz frames is not 120-fps interpolation.

## Disc and private build procedure

Only the USA GFZE01 disc is supported; extracted main.dol must match SHA-1
421c88106697d3275a3fc26fb7a01bf6d816b271. The original disc must be dumped by
the user from their own copy. Do not obtain or download a replacement game.

Use public Actions only for code-free runtime compile checks. Do not place a
disc URL, extracted executable, generated C, runtime REL, or game-linked binary
in public Actions artifacts, logs, releases, caches or git history.

The complete game build must run in an access-controlled cloud workspace or
private Actions repository. Transfer the disc there via authenticated private
storage; do not pass signed URLs as public workflow inputs. Bootstrap the
headless host game on the macOS runner to capture fze.main, which is assembled
by the game at boot. Then cross-compile all generated game C and runtime RELs
to iOS ARM64. Cross-compilation alone cannot run that bootstrap executable.

The final unsigned IPA may omit the ISO/assets but still contains compiled
game-derived code. Treat it as a private personal deliverable, never a public
release asset. Remove all disc data from the .app and audit the package before
private delivery. Keep memory-card saves and user data in Documents.

## Current external blockers

- Connected GitHub tools can read/write existing repositories, but expose no
  repository-create/fork or workflow-dispatch operation. The requested
  ajellia81-cmd/fzerogx-recomp repository returned 404. The cloud shell's
  authenticated-user API probe returned 401; no usable shell GitHub identity
  was established. Browser fallback needs the user's explicit permission.
- No user-supplied GFZE01 disc image or private game-code build input exists
  in this task. Runtime compilation can proceed without it; a complete game
  link cannot.

## Verification gates

| Gate | Evidence required | Status |
|---|---|---|
| Runtime compile | Successful iPhoneOS ARM64 Actions job | Pending |
| Full game compile/link | Private build with actual generated guest code | Pending |
| IPA packaging | Audited Payload/app with device ARM64 executable | Pending |
| App launch | Actual device launch/log | Pending |
| Game operation at 60 fps | Device gameplay, audio, input and timing | Pending |
| 120-fps interpolation | Unique presented frames + 60-Hz guest ticks | Pending |

Sources: https://github.com/ethannicholas/fzerogx-recomp,
https://github.com/ethannicholas/gcn-recomp,
https://github.com/zorrobyte/macos-on-ios-public.
