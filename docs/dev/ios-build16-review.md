# Build16 review

## Display-owner inventory

All 15 supported REL names in `analysis/rels` are enumerated in
`ios_guest::owners`; `check_policy.py` fails on an omitted or newly added name.
This inventories source routes, not proof of every scene on a physical device.

| Owner | Presentation policy | Reviewed edge |
|---|---|---|
| main, sample | No display override; inspect the batch | Bootstrap / retained race phase |
| title | Flat startup artwork; perspective intro/demo World | Logo/fade transition keeps existing timing policy |
| sel, option | Menu | Preview geometry, incomplete header, stale phase |
| replay | Menu unless an actual live race takes precedence | Card access/progress/no-data must not depend on header strips |
| profile | Menu | Embedded pilot and machine previews |
| customize, pilotpoint, car_colchg | Menu | Garage/card prompts, shop/category/grid, paint previews |
| movie, movie_module | Movie | Preserve decoded image canvas and subtitles |
| story | Movie detection, menu frame, then actual geometry/phase | Orthographic-only stale race phase no longer forces World |
| winning | Menu frame, then actual geometry/phase | Orthographic-only stale finish phase no longer forces World |
| interview | World with per-draw modal/glyph correction | Keep expanded 3D and the previous fixed-text correction |

Every menu owner must also win over a retained Title module when the queued FIFO context
is encoded. World rendering remains available for genuine course/race/replay
playback. A saved replay playback route and every interview/story/ceremony
variant have not been visually verified on hardware. These are explicit coverage
gaps, not completed device tests.

## Controller menu

Restore frontend mapping using GameCube target buttons, represented by original
vector button artwork rasterized through UIKit. Twelve targets: A/B/X/Y/Z/L/R,
START and four D-pad directions. The assignment is to physical controller inputs,
including both analog triggers. Guide stays reserved for Recomp. A scrollable
list supports landscape screens and pad focus. Defaults match upstream; saved
mapping uses a new versioned preference key. Trigger remapping carries analog
L/R values to the assigned target, rather than leaking the original trigger.

## Stutter evidence and mitigation

The supplied log contains a 142ms frame with 121ms shader compilation, and
Customize 158ms/112ms frames with 110ms/93ms compilation. It also contains
20–44ms frames without new shaders or pipeline states; those cannot be explained
solely by compilation.

Both dual-source blending and fallback renderer configurations are collected,
independently of the host GPU capabilities. Startup frames accompany the Replay route.
Private host menu captures now contribute renderer shader keys and exact blend
recipes to startup preparation before the device build. Only renderer metadata
is emitted; decoded textures/geometry are encrypted private diagnostics. The
collector checks classification of every captured menu-owned frame. The shop
route enters its category, then selects its machine list. Early Replay captures
cover more transition frames. These scripts are ordinary controller input;
a route completing does not by itself prove it reached a particular dialog.

Slow-frame diagnostics separately measure intermediate/native submission,
presentation waits, and remaining preparation/history time. No claim is made
that all stutter is eliminated without a new device run.

## Verification

ROM-free tests cover menu owners with and without perspective previews, every
retained phase 37–43, incomplete/missing headers, queued Replay+Title context,
flat logos, fixed glyphs, movie canvases, lifecycle and analog remapping.
GLES startup warmup is smoke tested separately. GitHub Actions remains the
source of iOS ARM64 compile/link/package validation; this is distinct from
physical app launch, gameplay and measured display refresh.
