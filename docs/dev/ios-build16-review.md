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

## Completed build and limits (2026-11-04)

- Functional source: `dcc6df30c0f73088e6feb04c932b20fdf40a135d`.
- Full Actions run: [41763672937](https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/41763672937), success.
- ARM64 launcher/runtime, renderer regression and detached-thread checks:
  [41763672938](https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/41763672938), success.
- Real guest logo policy: PASS, SEGA hold established; AV 120 updates / 12 fade updates.
- Final private capture policy checks: 66 frames, all menu-owner assertions pass.
  Title includes flat logo, menu and world frames; preview/card/shop frames remain Menu.
- Captured metadata: 217 base keys + 15 additional dual-source/fallback keys;
  81 observed blend recipes. Metadata is generated privately on the runner before compilation.
- GLES fallback smoke: 191 normalized programs, 2,342 warmup draws, no GL error,
  original framebuffer restored.
- GLES dual-source smoke: 232 programs, 2,865 warmup draws, no GL error,
  original framebuffer restored. This is Mesa GLES, not physical iOS/Metal.
- Offline inspection of representative Replay file list, Profile detail, Garage
  query and Shop merchandise frames: no black preview rectangles; zero non-black
  pixels outside the centered 16:9 menu canvas. Shop merchandise is rendered by
  Customize; linking `pilotpoint` is not required to recognize that screen.
- Exact existing-card Replay access/progress/no-data views are covered by the
  owner/transient regression policy, but were not visually reproduced from the
  owner's existing memory card. Saved race replay, all story/interview variants,
  hardware menu operation and physical refresh remain unverified for this IPA.
- No device evidence yet establishes elimination of every stutter. New phase
  diagnostics remain available for stalls without shader compilation.

Package: `FZeroGX-iOS-build16-unsigned.ipa`, 14,936,341 bytes.
SHA-256: `174c3c33aec9d40473e961fe5e992195222ec66caeacf116d1827032dd968e12`.
Seven bundle files; app and ANGLE are unsigned thin ARM64 / iPhoneOS Mach-O.
No disc/save/generated source/private key/capture/provisioning is included.
Effective minimum iOS remains 26.2 due to ANGLE (app itself declares 17.0).
Compilation/link/package success is separate from app launch, gameplay and
measured 120Hz presentation, which require physical-device verification.
