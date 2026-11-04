# Menu 2D audit after Build17

## Findings

The TIME ATTACK Records footer spacing failure is caused by Build17's
command-adjacent glyph grouping. The guest sorts draw commands by texture.
Letters belonging to Clear, Sort and Pilot are interleaved with letters from
other labels. Giving each command-adjacent fragment its own center produces
different translations within one word. OPTIONS Return/Enter independently
reproduces the same failure in static capture overlays.

Commit 41baeaa1edc3f80061e93b79d532aee8da179153 recovers glyph groups from screen
position instead. Projection, viewport, scissor and render-pass signatures
separate incompatible draws; height, baseline and horizontal distance recover
individual labels irrespective of texture submission order. EFB-copy/present
boundaries remain separate. The correction applies to the shared legacy 24x24 I4
path in menus and fullscreen scenes. It does not resize pilot artwork.

GitHub macOS Actions run 41807960265 passed ARM64 runtime/renderer compilation,
renderer regression fixtures (including texture-shuffled multi-label text and
copy boundaries), lifecycle fixtures and thread-resource checks.
This change is not present in the previously delivered Build17 IPA.

## Full captured-frame inspection

390 raw frames from 17 input routes were inspected, including intermediate
transitions. The union contains 386 unique sampled 2D texture hashes. The
inspection covers sampled texture bindings, UV spans, quad geometry, legacy
glyph membership and old/new glyph anchors. Static overlays are analytical
reconstructions, not Metal/device screenshots.

| Route | Frames | Frames with changed glyph anchors |
|---|---:|---:|
| Practice | 22 | 19 |
| GP | 22 | 11 |
| Time Attack entry/course | 22 | 15 |
| Records | 19 | 19 |
| Records sort | 19 | 19 |
| Records pilot toggle | 19 | 19 |
| Alternate Time Attack selection | 19 | 17 |
| Options | 22 | 22 |
| Replay | 32 | 28 |
| Customize | 22 | 17 |
| Shop | 22 | 13 |
| Story | 22 | 5 |
| Profile | 40 | 26 |
| Profile X | 22 | 20 |
| Profile Y | 22 | 20 |
| Profile Z | 22 | 20 |
| Return to startup | 22 | 0 |

An anchor change indicates exposure to the old grouping logic, not proof that
every changed word was visibly corrupt. Profile inspection concerns shared
text only; the user verified the original large pilot portrait and closed its
size issue. The alternate Records-right route reaches machine selection; its
name must not be taken as proof of another Records page.

## Remaining bitmap canvas mismatch

The Records shield is a 92x116 texture sampled over its complete UV range.
Its submitted normalized quad spans about 0.16675 x 0.280333. The sampled
texel-to-quad ratio recovers a 4:3 source canvas. On the current 16:9 menu canvas,
its width relative to height is therefore 4/3 of the native texture ratio.
The shield is about 33.3 percent too wide under this presentation.

The neighboring metal backing uses an intentionally different quad scale;
the glow is another layer. Resizing each of these independently around its own
center is not a safe general fix for composite UI. Both side emblems must be
treated as coherent compositions.

The same broad remaining exposure exists for menu bitmap titles, button glyphs,
circular frames, ranking/time atlases and small machine icons: Menu currently
bypasses all non-legacy-glyph draws. Sampled captures include legacy 4:3 bitmap
geometry in these categories. This is distinct from the texture-sorted text
spacing bug and remains uncorrected by the glyph commit. Background images,
stretchable panel centers, nine-slice borders, EFB previews and movies must not
be classified as independent icons merely from texture aspect.

The white vertical line with a purple triangle in Records is part of the
original 12x28 setting-pointer texture. Its existence is not rendering damage.
It must remain aligned with the neighboring 36x12 setting gauge if their
composition is subsequently corrected.

## Limits and next implementation boundary

This is a complete scan of the available 390-frame capture set and shared
renderer policy, not a claim that every unlock, saved-card dialog, pilot,
course, paint submenu or physical-device state was exercised. Bitmap groups
need coordinated canvas correction before a new package can be described as
fixing all remaining 2D proportions. No new IPA has been produced as part of
this audit. No original asset bytes, captures, private keys or generated guest
code are stored in this report.
