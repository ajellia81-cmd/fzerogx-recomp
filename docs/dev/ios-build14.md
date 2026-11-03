# iOS Build14: fullscreen-world overlay canvas audit

## Report and confirmed code gaps

User screenshots show GP interview dialogue, the broadcast lower third and question selection laid over a fullscreen 3D camera. Their horizontal text proportions differ from the intended fixed UI canvas. This is user-observed visual distortion, not a reproduced host capture of the interview.

The guest presentation registry omitted fze.interview.rel entirely. Consequently, FIFO draw tags had no interview context, and generic World UI chose the race HUD's already-compensated 16:9 canvas. Free-standing interview glyphs, labels and bespoke panels could not be covered by recognizing just the common panel material.

Add Interview to guest metadata, FIFO encode/decode and World classification. All orthographic interview UI uses a 4:3 canvas; the perspective camera still expands to the native drawable. Choose the UI canvas from each draw's stamped context, with the ordered batch context only as legacy fallback. The 4:3 choice follows the supplied distortion and the game's uncorrected UI family; exact interview pixel comparison still requires device verification.

A second, independent gap affected common popup runs in any World scene: modal_draws validated all retained texture bindings, including unsampled maps. An unrelated stale binding could reject a valid panel/font/title run. Validate only the maps sampled by the material. Actually sampled unrelated artwork still ends the run. A material-hash-only correction cannot solve all overlay types; interview captions therefore do not depend on hashes or glyph dimensions.

## Complete renderer decision-path audit

| Path | Canvas / exclusion | Validation scope |
| --- | --- | --- |
| Fullscreen perspective world: intro, course demonstration, race, replay, results camera | Native aspect from projection; no flat-video stretching | Existing code + synthetic fixtures; each gameplay sequence not replayed this turn |
| Interview subtitles, broadcast graphics, arbitrary character/name textures, question panels | Per-draw Interview context selects 4:3 for orthographic UI | New metadata, projection and mixed-context fixtures; user device retest pending |
| Race HUD and other compensated World overlays | Existing 16:9 canvas retained | Square-proportion fixtures for pre-compensated HUD coordinates |
| Common pause/results/quit/save/confirmation popup material runs | 4:3, including shadow and foreground glyphs | Existing boundary/cache fixtures + new stale-vs-sampled texture tests |
| Main/machine/course/options menus; name-entry/chapter selection | Existing Menu composition retained | Existing command/state/scissor fixtures |
| Startup logos and their transition draws | Existing Flat 4:3 policy | Existing logo fixtures and real-guest timer probe in workflow |
| Prerendered movie quads and subtitles | Existing encoded-video aspect and subtitle policy retained | Existing I8/IA8 metadata and movie canvas fixtures |
| Fullscreen texture composites | Excluded from a second correction | Existing samples_fullscreen_copy guard |
| Offscreen copies, portrait/preview viewports | Retained original pass geometry and composition | Existing mark_offscreen_passes / viewport guards |
| Solid fullscreen fades | Cover entire drawable | Existing full-bounds exception |
| UI scissor and corrected-state reuse | Move clip with projection; distinguish fixed vs compensated canvases | Existing shared-state modal fixture + new mixed tagged-context fixture |

All existing renderer branches were inspected. This does not establish that every character/cup/result/story/replay screen has been visually exercised. Unknown standalone uncompensated overlays outside the interview remain possible; neither a universal 4:3 override nor guessing from every ortho matrix is safe because those previously squeezed the working HUD and menus.

## Verification

ROM-free policy tests pass. New tests cover all registered module families at 4:3, 16:9, 2868:1320 and 21:9 targets, standalone fixed UI vs pre-compensated HUD squares, retained unsampled textures, sampled unrelated textures, interview tag round-trip/priority and per-draw mixed scene boundaries. These prove the policy/math and regression boundaries, not a rendered capture of every game scene.

Build13 thread-resource reclamation, per-frame autorelease pools, haptics queue draining, Build12 shader/pipeline warmup, icon and 60 Hz logic remain intact. No game data, decoded frames or generated game source is committed.
