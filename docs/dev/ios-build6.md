# iOS build 6 rendering follow-up

Device feedback on build 5 disproved the synthetic 4:3 UI assumption: menus,
HUD portraits/digits and movie captions were still horizontally compressed in
widescreen mode. Machine selection still had tall black rectangles and clipped
portraits; the blue menu background showed side remnants. Synthetic framebuffer
isolation alone did not establish that the real menu had been repaired.

The 16:9 game option already adjusts the 2D geometry, even when its orthographic
matrix retains a 640x480 coordinate range. The iOS correction now uses a 16:9
presentation canvas for that geometry, rather than deriving a 4:3 canvas from
matrix coefficients. Every menu projection, scissor, EFB copy/clear and render target now stays exactly unchanged at 16:9.
Fullscreen 3D uses a centered 16:9 UI overlay on the native drawable. Solid
full-frame fades and fullscreen EFB composites retain full coverage; textured
logos receive the same UI correction as other artwork.

The earlier movie detector was wrong: it required three I8 textures. Existing
upstream observations in `status.md` describe the actual guest-rendered movie as
an I8 luma texture plus a half-size IA8 chroma texture. The iOS path now detects
that pair and the linked movie modules. Encoded video geometry is left unchanged
on a display canvas recovered from the guest movie quad and decoded texture dimensions, retaining encoded bars in either display option. Widescreen-adjusted
caption geometry is corrected independently. Encoded frames are not interpolated.
No disc/movie decoder has been added.

`python3 ios/tests/check_policy.py` runs original synthetic fixtures without a
ROM. They cover game-compensated 2D geometry, an unchanged menu scissor, fullscreen
HUD sizing and positive/negative I8/IA8 video identification. This does not verify
real game pixels, iOS execution, Metal output or physical display FPS.

The opt-in host diagnostic can serialize already-decoded renderer batches and
live textures for private pixel replay. Captures are never committed, cached or
uploaded in plaintext: only explicitly selected capture files are included in
the encrypted diagnostics archive. They contain game-derived data and must stay
private. Native/intermediate submission counts do not measure display scanout.

## Private pixel replay findings

The new diagnostics captured machine selection at batch 2700, a live race with
HUD at 5500, and the prologue movie at 3500 and 6000. The source display copy is
640x448 in these captures, not the previously assumed 640x480. The native storage
mapping already follows that actual source size.

A software GLES3 replay at 1434x660 (the requested device aspect, half resolution)
reproduced the black machine-selection rectangles. Both the menu perspective
widening and the build-5 private preview-FBO isolation independently broke the
composed menu. Disabling only one did not fix it. Keeping the complete menu batch
unchanged eliminated both rectangles and restored the machine/pilot portraits;
the whole resulting image matched the unmodified 16:9 composition pixel for pixel. In an interior left-side sample the near-black pixel fraction fell from 100 percent in build 5 to 0.45 percent. The offending
menu has a full 640x480 EFB texture composite, in addition to a 320x224 preview.
The prior inference that isolating preview drawing fixed the visible EFB was
therefore disproved by actual rendered pixels, not just synthetic framebuffer
checks. That isolation code and its generated-renderer injections are removed.

The actual movie batch contains I8 640x336 plus IA8 320x168. Its quad occupies
70 percent of the display height, so its recovered canvas is 4:3 and its image
aspect is 640/336. Canvas recovery, rather than a fixed assumption, also supports
the game's widescreen movie placement. The captured prologue frames contain no
separate subtitle draws: the user's chapter-specific subtitle case still needs
device verification. The blank-card captures have the initial four selectable
machines, not the user's fully unlocked card.

These are software replay checks of actual renderer inputs, not iPhone execution,
Metal validation or physical 120-Hz measurements. The final device IPA must be
rebuilt after the menu-isolation removal; the earlier candidate from run
41437251064 does not include that final correction and is not the delivery.
