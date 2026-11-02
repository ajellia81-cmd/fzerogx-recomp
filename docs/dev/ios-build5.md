# iOS build 5

Device feedback on build 4 showed the live submission counter reaching
60 native + 60 intermediate frames, but also menu side strips, edge remnants,
stretched 2D overlays and an intermittent opening classified as a menu.
Those screenshots establish submission rates, not measured panel scanout.

## Changes

- Interpolation is requested independently of fullscreen classification. A
  temporary menu classification no longer clears 3D motion history. Orthographic
  video is preserved; unmatched geometry, static scenes, the first frame and
  unsupported incomplete XFB frames do not become artificial intermediate frames.
- Guest-thread snapshots of linked mode modules disambiguate selection/options
  from the title/replay. No UI/render thread reads mutable guest memory.
- Private diagnostic run 41398069863 confirmed that the title module is linked
  during the opening, selection replaces it at the menu, and race state 41 remains
  stale during selection. The GP route observed 37/38 before flyover 39, grid 40
  and racing 41. Selection metadata therefore takes priority over stale race phase.
- World projection uses its camera aspect and the physical output aspect. Text,
  logos and dialog overlays retain their authored projection proportions within
  the selected canvas. Menus keep a 16:9 canvas. Pure 2D/video keeps the original
  presentation. Full-canvas backgrounds, fades, frame-copy composites and offscreen
  passes retain their coverage/coordinate system.
- Offscreen menu-preview draw suffixes use a private color/depth target, even
  when a menu background precedes the first preview without a copy. Only their
  source rectangle is copied, and capture/clear does not modify the visible EFB.
- UI scissor bounds follow the UI correction. Opaque solid-black vertical side
  masks on selection menus are excluded conservatively; full-screen fades,
  textured draws, small panels and offscreen passes are retained.
- Frame/copy textures clamp at their edges. Menu presentation uses a cleared
  letterbox outside its canvas, avoiding exposure of expanded working regions.
- Resolution selection is removed. The visible EFB source rectangle maps to the
  native drawable in both axes, including copy/readback/scissor coordinates.
  On a 2868x1320 screen with a 640x480 display source this allocates 2868x1452 EFB
  storage: 2868x1320 visible pixels plus the console's extra working rows.

## Checks before device build

Cloud software GLES3 checks passed for square UI proportions on native and 16:9
canvases, preservation of fullscreen fades/composites, selection-mask filtering,
native framebuffer dimensions, isolated color/depth copies and pooled targets.
Motion correspondence and mode-transition fixtures also passed. These do not
verify Metal, actual game appearance, physical haptics or sustained 120fps.

The complete macOS Actions game build and iPhone execution are separate gates.
Every reported visual case still needs comparison on the new device build.

## Completed artifact and limits

- Complete build source: `4bc085e3c8e8c9c09493002774d6eacdb54a503c`.
- Final full game build: Actions run `41401217861`, success (16m 17s).
- Independent ARM64 runtime compilation: run `41401119044`, success.
- Title route: 4,688 batches, 3,920 complete-frame moving geometry matches.
- Selection-to-GP route: 6,959 batches, 5,875 complete-frame moving geometry
  matches, including menu geometry now independent of fullscreen eligibility.
- Neither route encountered partial/multi-present frames or a prerendered movie.
- The solid-black side-mask candidate count was zero on these routes. This does
  not support solid mask geometry as the cause of the reported strips; the main
  fix is isolation of menu preview drawing and clears from the visible EFB.
  Actual game pixels and device appearance have not been verified here.
- `FZeroGX-iOS-build5-unsigned.ipa`: 14,904,552 bytes, bundle version 5.
- SHA256: `c6e717cfb9123c4756bac7e314947d0bb58e83a774a9d71c35f69ebc311327ba`.
- Independent ZIP/Mach-O audit passed: arm64 iPhoneOS, signatures/provisioning
  absent, icon registered, no ISO/CISO/DOL/REL/save or generated source payloads.
- Build 5 app launch, gameplay, visual fixes, physical haptics and sustained
  120fps have not been tested on an iPhone. Build 4's user screenshot showed
  120 render submissions, not a physical display measurement of this build.

