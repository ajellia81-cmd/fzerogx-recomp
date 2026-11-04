# Build18: game-wide 2D presentation review

## Scope and common correction

The scope is every in-game 2D presentation path, including menus, race HUD,
subtitles, finish/result dialogs, profile/shop/customization screens, movies,
startup/return transitions and transient effects. It is not limited to the
screens present in a particular capture folder.

Build17 corrected selected legacy letters while bypassing menu bitmaps.
Its text grouping additionally assumed command adjacency, although the guest
texture-sorts letters from unrelated words. Build18 replaces those per-fragment
adjustments with one coherent native-canvas map for the complete composition:
artwork, atlases, numbers, text, panels, borders and their clipping regions.
There is no texture/character whitelist for the ordinary 2D correction.

The original vertical size and internal layout are retained. Native 2D layout
can occupy a centered original-aspect area within a wider scene; independent
artwork is not widened to fill it. Real 3D cameras expand their frustum.
No generic image crop or image-size heuristic was introduced to fill backgrounds.
The user's closed pilot-profile size issue remains closed.

## Entire disc display-owner inventory

The inventory is checked against every REL metadata file by check_policy.py.
All owners route through the same GX batch/renderer policy.

| Owner | Presentation family | Review |
|---|---|---|
| fze.main | Race, HUD, pause, retirement, shared runtime | Shared ortho, perspective, clip and copy paths |
| fze.sample | Runtime support | No separate display-owner override |
| fze.title | Startup, title, attract/demo, return | Actual sampled logos, retained fade canvas, camera expansion |
| fze.sel | GP, VS, Practice, Time Attack, Records, selections | Shared complete 2D composition |
| fze.option | Controls, card, screen settings | Complete 2D composition and transient dialogs |
| fze.replay | Browser/card and course playback | Menu versus real course precedence |
| fze.story | Chapter/menu, movie and gameplay | Shared native 2D canvas and movie precedence |
| fze.winning | Finish/result presentations | Shared 2D policy; retained race phase is not a camera |
| fze.interview | Interviews/results overlay on 3D | Same native 2D canvas as other fullscreen scenes |
| fze.movie | Decoded films | Actually sampled Y/UV planes and original image aspect |
| fze.movie_module | Movie support | Same movie policy |
| fze.profile | Pilot details, artwork, text, previews | Native artwork/layout, full-size independent preview targets |
| fze.customize | Garage, parts, naming and dialogs | Composition, atlas and target boundaries |
| fze.pilotpoint | Shop and purchase dialogs | Composition, atlas and target boundaries |
| fze.car_colchg | Paint/color UI | Same material-independent 2D policy |

This is a source-path inventory. It does not assert that every unlocked asset
or every possible gameplay ending has been reached with controller inputs.

## Render-to-texture review

A 640x480 copy is not necessarily the final screen. Real Profile captures have
both an identity full-frame reader and a separately positioned preview reader
of different full-size targets. Treating both as postprocessing gives the
preview and its surrounding interface incompatible transforms.

The renderer now inspects the actual sampled consumers. Independent clearing
preview passes stay native even when their targets are full-sized; their
subsequent pictures join the same UI canvas and scissor transform. Genuine
final-screen feedback is not corrected twice. A separately placed reader of an
already-corrected frame conjugates placement while retaining picture scale.
Copy/clear/writeback commands and original vertex/UV/packed data are retained.

Untextured fades, cinematic edge mattes, fullscreen indirect effects, movies
and native intermediate targets are distinguished from ordinary pictures.
Retained but unsampled texture bindings cannot identify films or feedback.
The movie aspect lookup likewise considers only actual TEV inputs.

Orthographic geometry is excluded from motion interpolation by the existing
history policy. The canvas is applied once before native/intermediate output.

## Verification design

- Every menu owner is checked across retained phases and perspective previews.
- Native square/emblem/portrait aspect is checked on 4:3, 16:9, iPhone and 21:9.
- Texture-shuffled letters, panel seams and gauge-style adjacency use one map.
- Sub-viewports and scissors move together.
- Full-sized independent previews, final-screen feedback and translated readers
  have separate regression fixtures.
- Cinematic masks cover the drawable while their inner edges follow the UI.
- Unused movie/copy bindings cannot override presentation.
- Startup logos and decoded films retain their original presentation.
- Capture replay checks immutable vertices, packed data, matrices and copy
  commands; finite output states; protected native passes; and all sampled
  orthographic material formats.

## Private input coverage

The expanded harness declares 72 intended input routes: main game modes,
Records operations, option/card/controller screens, pause/retirement flows,
alternate course selections, garage entries, virtual two/three/four-player
inputs and thirty pilot-selection indices. Each route starts in a fresh private
harness. The index routes are not proof that thirty different pilots were
reached; observed owner/phase/material metadata must substantiate that.

Most captured frames retain geometry/state/hash metadata only. Frames 4500
and 5400 additionally retain decoded textures for private positional inspection.
Artifacts are encrypted, never committed. Game builds and compiled tests run
on GitHub Actions macOS; scratch is used for static capture inspection only.

## Current verification state

- Initial common-canvas source and edge-mask checks: macOS ARM64/regression
  Actions 41832471239, success.
- Reader-aware preview correction: Actions 41834677605, success.
- Final renderer/package request source:
  0ccf3a8c49a90b32b0a106a8265139f083ba3118.
- Final runtime/regression run: 41835457812, pending.
- Final complete build/capture run: 41835457932, pending.
- Earlier broad-capture run 41832471173 uses the preceding renderer source and
  is not the final delivery.

The package filename and embedded CFBundleVersion are both Build18.
No physical iPhone/Metal pixel verification or elimination of every possible
original-game visual defect is claimed by the source fixtures.
