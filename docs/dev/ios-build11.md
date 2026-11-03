# iOS build 11: separate SEGA and AV presentation timing

The user's reference recording shows SEGA remaining until about 2.4 s, overlap
until about 2.9 s, AV alone until about 4.8 s and black by 5.0 s. These are
approximate recording timestamps, not an assertion about all original consoles.

Build 10's uniform 800,000-tick delay did not solve the visible duration. Private
inspection confirmed three boot delay groups and a separate title-logo countdown.
Uniformly extending the shared loop also extends time before rendering and does
not account for loading/presentation. That approach is replaced.

Only the boot fade call sites receive elapsed virtual time, in event-sized chunks.
The SEGA hold uses the first actual presentation timestamp (submission fallback),
so intervening load time counts toward the target instead of being added to it.
The first title-logo update waits until that visible hold deadline. Title's own
60Hz overlap remains; its shared busy loop stays removed.

Only AV's countdown is adjusted: 108 steady updates plus 12 fade updates (2 s at
60Hz), following the initial approximately half-second overlap. The original fade
curve is traversed at alternating two/three counter units per update. Other title
phases, gameplay, physics, 60Hz VI and smooth-motion rendering are unchanged.
Relocated title state is obtained from the dedicated caller's already-installed
register; the return-site addresses and field offsets are layout metadata for the
pinned executable. No generated game instructions are included here.

Validation includes synthetic hold anchoring/AV countdown tests and a private real
guest host run of the same iOS timer hook before the full ARM64 build. Host traces
and inspection inputs are encrypted before upload. Exact physical presentation,
app launch/gameplay and real 120Hz output require device testing.
