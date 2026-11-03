# iOS Build13: host-thread exhaustion and resource review

User Build12 log: roughly 210 seconds into the session, pthread_create returned EAGAIN with a 64 MB stack request; runtime fatal reported cumulative host-thread id 1015. This does not establish 1015 simultaneously running threads. The POSIX start API creates joinable threads, discards their handles, and never joins them; completed threads retain OS resources. The runtime does retire stale guest-context threads, but pthread_exit alone does not reclaim joinable-thread resources.

Generate an iOS-only platform.cpp variant selecting PTHREAD_CREATE_DETACHED before pthread_create. Preserve the 64 MB stack size (deep guest recursion still needs it) and the existing guest-context scheduling. Both the real device target and standalone ARM64 check use this variant. No Windows/desktop shared source change.

Regression: under Linux with 512 MB address-space limit, unchanged platform API fails after six sequential thread creations, while the modified actual platform implementation completes 1500 creations/exits at the same 64 MB stack size. macOS runs the same native stress test without Linux's address-space cap. These are synthetic resource tests, not second-stage gameplay verification.

## Broader resource audit

- Renderer thread had one autorelease pool spanning its entire session. Add a nested per-batch pool so temporary Objective-C/Metal objects need not accumulate until game exit.
- Serial haptics queue now drains autoreleased objects per work item. Existing generation guards, bounded motor updates and engine retirement remain intact.
- Ticker, sampler, shutdown helper and app-render std::threads already detach; no additional joinable-thread leak found in those call sites.
- HostThread CPU/debug records and context bindings remain in runtime containers after retirement; detached-thread reclamation does not delete those records. They are separate from OS stacks/handles and kept for existing pointer/debug semantics. This remains a long-session bookkeeping growth risk, not demonstrated as this crash's trigger. Avoid unsafe record reuse while saved contexts may still refer to them.
- Touch input uses one display link per game controller. Audio opens once per game session; interruption observer installation is once per open. No per-course recreation in the current launcher.
- Disc validation and shader cache FILE operations pair close calls. Disc backend lifetime follows the game session.
- Interpolation targets drop obsolete copy ids on eligible frames and reuse matching allocations; startup warmup deletes its private framebuffer, VAO, VBO and texture. No per-frame GL object creation leak identified in these paths; pipeline/program caches intentionally persist for reuse.

Build12 warmup, approved icon, 60 Hz simulation and presentation policy are unchanged. Physical-device second-stage stability, sustained memory, app launch and 120 Hz presentation need user verification.
