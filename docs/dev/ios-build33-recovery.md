# Build 33 recovery

Continues main 4b006fd and preserves Build 32 and later source changes.

The original shared UI state has separate queued-output (+0x44) and coordinate-compensation (+0x45) bytes. Native ownership must follow compensation, including sprites recorded in the sorted queue. Records captures showed 16:9 body glyphs and 4:3 footer glyphs sharing tag 0x83; the footer made the entire body switch canvas. The common sprite and immediate hooks now retain the actual compensation ownership before enqueueing. No screen-position or text-hash heuristic is used.

The Shop Item CPU sample identified 818F9028 repeatedly copying and scanning the 44-entry category table to recover a category ordinal. A bounded preceding-category count replaces that inner search for valid indices. Original unlock and purchase queries execute afterward, and invalid indices retain the original path. Private probe builds compare the optimized ordinal with the actual original loop on the first observed call for each index and abort on disagreement. All-vertex motion correspondence checks remain intact.

Garage captured ordinary text includes intentional horizontal scaling as well as normal compensated glyphs. Those intentional scales are retained. The shared fix applies to immediate and queued text, panels and icons throughout the game. Structural source coverage is distinct from reached routes and physical-device Metal verification.

Validation: pending Build 33 macOS device workflow; no local builds.
