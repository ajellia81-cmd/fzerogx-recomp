# iOS Build12: first-use render stalls

User log, first course, smooth motion enabled: 52 ms frame with 47 ms in first program/blend draws; 151 ms frame with 130 ms shader compilation and 12 ms first draw. These explain two hitches, not all possible frame pacing issues.

The iOS renderer queues flat framebuffer-alpha and destination-alpha variants of shipped keys before boot. A startup-only step draws each prepared program into private 4x4 RGBA8/depth32 targets with common blend/write-mask combinations, then waits for completion. It does not skip visible geometry, alter simulation ticks, or present warmup frames. UI remains responsive between programs; individual driver calls cannot be preempted. This adds cold startup work and is not exhaustive pipeline coverage (alternate vertex layouts, topologies and less common blends may still compile later). Existing caches remain valid; clean-cache comparisons must remove shaders.bin on both builds.

Stall logging threshold is 20 ms for iOS only. Compare cold and second runs on the same course and smooth-motion setting. A host compile test does not establish iPhone hitch removal or real 120 Hz pacing.

## Validation and delivery (2026-11-03)

Source `d025aff5e118e43f635f8b3de94124c8d8484e3d`:

- ROM-free aspect/session regressions pass locally and on macOS.
- Linux Mesa surfaceless GLES smoke test compiled 191 distinct shipped programs, performed 2,292 pipeline draws, observed no GL error, and restored the original framebuffer. This is not an ANGLE Metal or iPhone performance measurement.
- iOS ARM64 runtime check run `41617486184`: success.
- Full macOS device run `41617485937`: success; real-guest logo probe PASS, 120 positive AV updates / 12 fade updates.
- IPA `FZeroGX-iOS-build12-unsigned.ipa`: 14,929,397 bytes; SHA256 `b231c91bf667f94fc778b39a0577e1208352018eb9f750970c8d8be991713c17`.
- Audit: seven bundle files; main binary and ANGLE are unsigned thin ARM64 iPhoneOS, no disc/save/generated sources or provisioning profiles. Effective minimum OS remains 26.2 due ANGLE. Approved PNG/asset catalog bytes match Build11.
- New build's physical-device launch, gameplay, 120 Hz presentation, warmup duration, and stutter reduction remain unverified. Compare clean shader-cache runs and second runs separately.
