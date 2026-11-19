# Build39 accelerated initial audio recovery

Owner confirms build38 foreground audio and haptics work, with a short delay
before audio is audible. Existing code retries a failed restart after1second;
the supplied device report alone does not prove this is the whole delay.

iOS recovery still attempts restart immediately. Failures now retry after
50,100,200,400ms, then1000ms intervals. Recovery state/session/output-unit
restart calls are serialized on the main queue. Duplicate pending requests
coalesce. Success and suspension invalidate delayed retries by generation.
Interruption-end notifications restart immediately and invalidate the older
polling request; missing-end fallback remains after1second. Obsolete fallback
notifications cannot restart an already recovered or newer session.

Audio buffers/latency cushion, playback format, guest timing, Item optimization,
rendering and haptic policies are unchanged. Build38's unpause-before-audio
ordering and immediate event-driven foreground haptics are retained.

The actual Recovery policy is exercised by session_policy.cpp for delay
sequence/backoff, request coalescing, cancellation, stale completion and
recovery reset. Existing foreground-order tests remain.
Runtime44464614404 and private macOS device run44464614414 succeeded.
Source9cf8e3ad77017f03b29be5e433bd40faca8d7165. No local build.

Delivered version39 IPA:14975066 bytes, seven package files, ARM64 unsigned,
ZIP CRC valid; no ROM/save/key/log/capture payloads.
SHA256:6a500fc9369ccaeeacfe001786a784c55eabcfcf1dfacdb157370c65c8ff6b32.
Library:libfile_688b479291688191a3200634e0b2d073.
Actual reduction of audible foreground delay needs owner-device verification;
hardware/session startup and buffer refill may still contribute.
