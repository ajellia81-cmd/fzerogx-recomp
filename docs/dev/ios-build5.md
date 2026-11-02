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
  logos and dialog overlays retain their original 4:3 geometry proportions within
  the selected canvas. Menus keep a 16:9 canvas. Pure 2D/video keeps the original
  presentation. Full-canvas backgrounds, fades, frame-copy composites and offscreen
  passes retain their coverage/coordinate system.
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
