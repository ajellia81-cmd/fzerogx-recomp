# iOS build 6 rendering follow-up

Device feedback on build 5 disproved the synthetic 4:3 UI assumption: menus,
HUD portraits/digits and movie captions were still horizontally compressed in
widescreen mode. Machine selection still had tall black rectangles and clipped
portraits; the blue menu background showed side remnants. Synthetic framebuffer
isolation alone did not establish that the real menu had been repaired.

The 16:9 game option already adjusts the 2D geometry, even when its orthographic
matrix retains a 640x480 coordinate range. The iOS correction now uses a 16:9
presentation canvas for that geometry, rather than deriving a 4:3 canvas from
matrix coefficients. Menu projections/scissors stay exactly unchanged at 16:9.
Fullscreen 3D uses a centered 16:9 UI overlay on the native drawable. Solid
full-frame fades and fullscreen EFB composites retain full coverage; textured
logos receive the same UI correction as other artwork.

The earlier movie detector was wrong: it required three I8 textures. Existing
upstream observations in `status.md` describe the actual guest-rendered movie as
an I8 luma texture plus a half-size IA8 chroma texture. The iOS path now detects
that pair and the linked movie modules. Encoded video geometry is left unchanged
on the original 4:3 display canvas, retaining encoded bars. Widescreen-adjusted
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
