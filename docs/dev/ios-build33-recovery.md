# Build 33 recovery

Continues main 4b006fd, retaining Build 32 and later changes. User confirms Garage text matches the original game; it is excluded from correction.

## Records and shared 2D composition

Actual Records captures contain a 16:9 body followed by a 4:3 control footer. Mixing their ordinary glyph evidence changed the complete body canvas when the footer appeared. Private text-owner tracing identified the original shared text-and-panel message-bar compositor 81853C9C, including its complete backing and embedded controls. It and the related 81853E40/81853FE4 compositors now retain independent native ownership through the existing sorted sprite queue. General sprite, bitmap and ordinary string submissions retain their previous policy. No text-content, screen-position or texture-hash production heuristic is used.

Earlier broad sprite/string candidates were stopped by attract-bitmap and native-glyph aspect oracles before device compilation; they are superseded by these precise compositor scopes.

## SHOP Item performance

The actual Item CPU sample identified 818F9028 repeatedly copying and scanning its 44-entry category table to recover a category ordinal. A bounded preceding-category count replaces the inner search for valid indices. All original unlock/purchase queries execute afterward; invalid indices retain the original path. Private probes execute and compare the original search for the first observed call per index, aborting on disagreement. Forty indices were observed and matched in run 44020008618; 0,1,29,39 were not observed. This is host evidence, not measured iPhone/Metal performance. All-vertex Smooth Motion correspondence checks remain unchanged.

## Verification

Final macOS device/capture workflow passed: https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/44024358806 (source fc5f99682fc8206673cdd3e82d8da09d4c33770c). ARM64 runtime/fixture/thread-resource check passed: run 44024358668.

The final audit passed 455 captures / 191,278 draws, 3,783 physically checked native glyphs, 30 attract bitmap draws and 40,287 preserved authored menu camera draws. Native family 0 contained 2,286 4:3 glyphs; family 1 contained 1,497 16:9 glyphs. These are renderer-input checks, not physical-device pixels.

Package: FZeroGX-iOS-build33-unsigned.ipa, 14,973,997 bytes, seven bundle files. SHA256 715a735b602688979ef2b30426cfc91d9b62ab64cb8290acaa64361ba98d48f5. ZIP CRC passed; main and ANGLE are iPhoneOS ARM64 without LC_CODE_SIGNATURE. No ROM/DOL/REL/save/capture/key/source/log files in the IPA. Physical iPhone operation remains separate from structural source inventory, reached-route evidence and package verification. No local builds.
