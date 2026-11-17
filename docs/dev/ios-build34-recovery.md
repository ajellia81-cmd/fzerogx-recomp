# iOS build 34: badge composition and Item GPU work

Source request follows the owner's build33 device report and log(5).txt.
Build33 Records body/footer fixes and the Shop ordinal optimization remain.
Garage text matches the original game and is unchanged.

## Device evidence

The supplied Item log shows original and interpolated rates about 29.2 each
(total58.5). The sampled slow frame spent2.4ms in prepare/history,2.7ms in
intermediate rendering and20.8ms in native rendering, with0.1ms swap/wait.
The stalled frame compiled0 programs and encountered0 new pipeline states.
This identifies the measured slow phase, not a definitive GPU timestamp.

## Badge correction

The private Records route trace identifies original helper8194108C as the
owner of the left/right92x116 badges,144x92 backing plates and112x112 glow.
Its glow helper81941360 is also called independently. Static direct-call
inventory covers three callers of8194108C, one wrapper caller and five
callers of81941360 (including the wrapper). Names of unvisited individual screens are not
inferred.

Both helpers establish a nestable pixel-art group. Only copied submission
records have scaleX multiplied by0.75 when the original display snapshot is
wide and an independent native canvas has not already corrected it. The
ordinary sorted queue and both immediate branches are covered. Positions,
scaleY, UVs and original persistent assets are unchanged. Background plates
retain their intentional relative scaling. No texture hash or screen position
is used to select production fixes.

## GPU work

Native EFB scaling uses the actual final letterboxed content extent: menus on
2868x1320 use2346x1320 visible pixels. World rendering retains2868x1320.
Extra EFB rows and independent render targets remain available.

Smooth Motion can replace its initial full color/depth copy with an identical
clear only when the preceding real pass ended in a full color/alpha/depth XFB
clear and the current batch has one contained Present and no independent EFB
copies. Scene changes, incomplete clears, trailing draws, multiple Presents
and out-of-bounds rectangles retain the copy path. This does not remove draws,
reduce visible resolution, change shaders or suppress interpolation.

## Validation status

No local compilation or execution of compiled tests is permitted.
macOS Actions private device build44085305066 and runtime check44085305090
are requested for434b945b822e59581a38a746a0205f67687bde97.
Focused real guest routes: records,records-page,shop-items,shop-items-right,
attract,gp-pause,replay-card. Existing canvas and actual-glyph checks remain;
new capture checks require actual shield aspect92/116 and terminal-clear
coverage. Asset-free fixtures exercise positive and negative copy-elision
conditions. Completion, final artifact identity and real-device performance
must be recorded after results are available. A fixed120fps is not claimed.
