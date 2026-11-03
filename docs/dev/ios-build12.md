# iOS Build12: first-use render stalls

User log, first course, smooth motion enabled: 52 ms frame with 47 ms in first program/blend draws; 151 ms frame with 130 ms shader compilation and 12 ms first draw. These explain two hitches, not all possible frame pacing issues.

The iOS renderer queues flat framebuffer-alpha and destination-alpha variants of shipped keys before boot. A startup-only step draws each prepared program into private 4x4 RGBA8/depth32 targets with common blend/write-mask combinations, then waits for completion. It does not skip visible geometry, alter simulation ticks, or present warmup frames. UI remains responsive between programs; individual driver calls cannot be preempted. This adds cold startup work and is not exhaustive pipeline coverage (alternate vertex layouts, topologies and less common blends may still compile later). Existing caches remain valid; clean-cache comparisons must remove shaders.bin on both builds.

Stall logging threshold is 20 ms for iOS only. Compare cold and second runs on the same course and smooth-motion setting. A host compile test does not establish iPhone hitch removal or real 120 Hz pacing.
