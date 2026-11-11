# Build 32: bitmap-only canvas evidence and authored menu cameras

Opening/attract bitmap captions were already 16:9-compensated by the guest but retained the port's default 4:3 canvas because they contain no ordinary glyphs. The source-canvas policy now uses positive, guest-authored full-view perspective aspect when ordinary glyph evidence is absent. The original 4:3/16:9 projection is inspected before device FOV correction, with native window families and offscreen passes excluded. Ordinary glyph evidence keeps priority. Mixed menu camera groups prefer a positively observed 16:9 foreground over the original 4:3 finite backdrop. Frames with neither kind of evidence use the guest's original display-mode snapshot. This occupies the existing high tag bit only on regular draws (native windows retain their independent family meaning), preserves all 40 scene/live contexts, and is retained per sorted queue record. Fixed queue slots are overwritten and reused rather than erased/reallocated every frame. The GPU command-metadata buffer size is unchanged. Capture version 3 identifies the extended regular-tag meaning; readers preserve versions 1/2 without inventing authored-canvas snapshots.

Menu output has a fixed authored 16:9 composition. Its full-view perspective camera groups now retain their original projection instead of inheriting real-world FOV expansion. This keeps finite backdrop/decorative geometry covering the frame. World and split-screen camera expansion, original window families and native preview targets retain their existing paths.

New asset-free fixtures check bitmap-only 4:3 and 16:9 source compositions at several target aspects, excluded ownership groups, mixed authored cameras, and finite background coverage. Private captured-frame oracles assert the two reported attract-caption transforms using their known original-art hashes (test-only) and assert that menu camera projections stay unchanged. The device workflow requires actual captured attract captions as well as native window glyphs.

Verification is performed by GitHub Actions macOS. The full-game function/call-path inventory is structural; captured owners and device visual confirmation are reported separately. Save Replay remains covered by its existing independent window-family and real-card-path checks. No arbitrary bitmap aspect is inferred from texture dimensions in production.

## Completed verification

Final application source: c16be4a3493d3461bdd3b45de7c01e196aab3f01.
Device build: https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/43034989436 (success).
ARM64/runtime/fixture check: https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/43034989089 (success).

The final private run validated 337 captures, 127,636 draw commands, 25 reported attract bitmap draws, 35,742 authored menu camera draws, and 2,697 ordinary native-window glyphs (550 gameplay-family 4:3, 2,147 card/window-family 16:9). The structural inventory remains 7,377 functions and 27,356 direct call edges, with 602 functions containing indirect dispatch. Observed capture owner masks are 0, 1, 2, 4, 8, 32, 64, 256, 512; other declared owners remain source-path coverage, not observed screen coverage.

Final package: FZeroGX-iOS-build32-unsigned.ipa, 14,974,076 bytes.
SHA256: aa47c9a3b2c01465132f08c21b6c6c4f2bdec52de3543927183c60e64589623e.
Package verification passed ZIP CRC, embedded build 32, iPhoneOS ARM64 main/ANGLE binaries, absent code signatures, and absence of ROM/DOL/REL/raw save/capture/key/source/log files. The package contains seven bundle files. Device Metal pixel confirmation is separate from these source and capture checks.
