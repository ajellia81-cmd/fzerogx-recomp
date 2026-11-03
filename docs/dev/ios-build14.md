# iOS fullscreen-world overlay audit (Build14 candidate)

## Scope and evidence

The user's GP interview screenshots show horizontally stretched subtitles, a question panel and broadcast graphics over the expanded 3D camera. Audit the common rendering paths rather than applying an interview-only aspect override.

The private corpus contains 7,418 generated functions from the existing supported module build. Static indexing found 10 direct callers of projection upload `fn_80072270`, 18 calls in 16 functions to its matrix wrapper `fn_800737E4`, and 11 direct callers of the orthographic constructor `fn_80015EE8`. The union of these caller sets is 18 functions. All of these paths were inspected. This inventory covers direct calls in that corpus, not arbitrary indirect calls or proof of execution in every game mode. Generated code stays outside the public repository and is delivered only as an encrypted, short-lived owner artifact.

The common formatted-text entry `fn_8184AE0C` has 706 direct calls in 167 functions across 12 module families. Another shared text entry, `fn_8184A0D8`, has 275 calls in 91 functions. These counts establish broad shared-code exposure, not 167 independently observed visual bugs.

| Shared formatted-text family | Direct caller functions | Fullscreen-overlay relevance |
| --- | ---: | --- |
| Main | 49 | Race HUD, course/grid/result presentation, common dialogs |
| Title | 24 | Intro/title camera overlays |
| Customize | 23 | Mostly menu composition; leave existing Menu policy intact |
| Selection | 18 | Machine/course menus; leave existing Menu policy intact |
| Pilot point | 15 | Reward/status text; visual execution not captured |
| Option | 11 | Menu composition |
| Profile | 10 | Profile/status presentation; visual execution not captured |
| Winning | 4 | Victory/result presentation; visual execution not captured |
| Interview | 4 | User-reported captions and question UI |
| Replay | 4 | Replay text/dialogs over the 3D camera |
| Story | 3 | Story presentation and dialogs |
| Movie | 2 | Encoded-video subtitles; excluded from new World-only glyph correction |

## Findings and corrected policy

An orthographic projection does not identify the source UI canvas. Expanded World frames mix pre-compensated widescreen artwork with legacy fixed-width text and modal widgets. Selecting 16:9 for every orthographic draw stretches the latter. Selecting 4:3 for everything squeezes the former. This is why a single scene-aspect switch is insufficient.

The initial interview-wide 4:3 proposal was rejected: private call inspection found widescreen-aware interview paths (`fn_17_3410`, `fn_17_613C`, `fn_17_9314`). A second rejected experiment inferred the canvas of arbitrary bitmap quads from their texel aspect. Real-frame rendering separated HUD portrait faces from their borders and disturbed composite labels. Neither experiment is in the final candidate.

The candidate recognizes only the existing 24x24 I4 font path, using the actually sampled map, finite UV coordinates and projected quad dimensions. An uncorrected square mapping indicates the legacy 4:3 glyph canvas; a pre-compensated 18x24 mapping indicates 16:9 and is preserved. Outline-font coordinate sets may be translated but must have matching spans. Indirect stages, EFB textures, unrelated multi-map effects, malformed or intentionally distorted glyphs are excluded. Generic artwork is not inferred from its image dimensions.

Contiguous glyphs with matching projection, viewport, scissor, height and baseline are corrected as a line so that character spacing shrinks with glyph width. Each line keeps its former widescreen center. Corrected-state reuse cannot cross the fixed/compensated boundary or reuse another line's anchor. Progressive text/layout animation has not been visually exercised in the interview; the line bounds are frame-local, and this remains a device-test item rather than a proven stable alignment contract.

Common modal runs are recognized as a unit: shadow glyphs, panel pieces, foreground glyphs and title atlas. Recognition now checks maps actually sampled by TEV, rather than all retained bindings. A stale unused texture can no longer incorrectly terminate a run; a sampled unrelated material still does. This is a code-path gap demonstrated by fixtures; it changed no modal decisions in the available real-capture corpus.

`fze.interview.rel` now has an ordered FIFO presentation tag, appended without renumbering prior tags. It identifies a World scene even during stale menu metadata transitions, but does not select a blanket 4:3 UI canvas. Perspective expansion remains separate from flat overlay correction.

## Complete renderer branch review

| Branch | Result / safeguard |
| --- | --- |
| Intro, course flyover, race, replay, result/interview 3D | Perspective camera still expands to the native drawable |
| Shared uncorrected World glyphs | Per-line fixed-canvas correction, independent of the scene name |
| Already compensated HUD artwork and generic bitmaps | Existing 16:9 geometry policy preserved |
| Common pause/results/quit/save/confirmation/question panel family | Fixed-canvas material run; unrelated HUD ends the run |
| Main/machine/course/options and name-entry menus | Existing Menu composition retained; no new glyph inference |
| Startup logos and transitions | Existing Flat policy and timing retained |
| Prerendered movies and subtitles | Existing encoded-video policy retained; new glyph inference is World-only |
| Offscreen preview/copy passes | Excluded by existing pass/viewport guards |
| Fullscreen texture composites | Excluded from a second correction |
| Solid fullscreen fades | Continue to cover the drawable |
| UI scissor and shared PixelState | Clipping follows the appropriate canvas; per-line state has a separate anchor |

## Verification and limits

ROM-free renderer/session policy fixtures pass on the cloud Linux host. They cover sampled/stale textures, interview tag priority, compensated/uncompensated primitives sharing a state, outlined glyphs, indirect-stage exclusions, logos, movies, common panels and offscreen boundaries. The previous audit commit passed the macOS iOS ARM64 runtime check; the final candidate needs its own run.

The private replay corpus contains 68 real decoded frames: startup logos, menus, course/race/popup and story/movie samples. Fifteen frames contain startup logos and eleven classify as movies. Shared legacy glyph draws are present outside the interview: 135 in each of two course-intro samples and 11 in several race samples. Presence is not necessarily visible text: some course glyphs are transparent in the chosen frame.

Host GLES replay before/after comparison preserved HUD face/border composition after the generic-bitmap experiment was removed. Additional representative movie and menu replays are byte-identical before/after. Pause and confirmation popups retain their panel composition; only generic HUD font labels outside those runs change. Host rendering is not Metal/device validation. There is no actual interview, winning-screen, reward-screen or complete end-credit capture in this corpus. Bespoke broadcast artwork, nonstandard fonts, indirect presentation paths and every progressive subtitle animation are not proven covered. Do not describe this audit as every scene visually tested or all stretching eliminated. Additional per-material fixes require evidence rather than broad aspect guesses.

Build13 thread-resource reclamation, autorelease pools and haptics queue draining, Build12 pipeline warmup, the approved icon and 60 Hz game logic remain intact. No ROM, save data, decoded frame or generated game source is committed. This document describes a candidate; it is not an IPA build or device-performance claim.
