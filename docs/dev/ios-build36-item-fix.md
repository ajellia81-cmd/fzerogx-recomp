# Build36 Item ownership scan fix

Source995e2f80596537e9a7456cc30a72fdb87bb56209.
The owner confirmed build34 badge sizes fixed and Item~30 original fps even
with Smooth Motion disabled. Preserve build34 visual fixes and GPU changes.

The successful bounded macOS source inspection44126850966 identified the
remaining sampled hotspot818F965C: a repeated inverse search for an item's
category ordinal. It copies all440 bytes of the44x10 table during every inner
candidate search and repeats the search for each preceding ordinal.

At818F9710, valid indices0..43 now count preceding entries with the same signed
16-bit category once. The final selected10-byte temporary record and440-byte
scratch table are recreated at the original guest addresses. Invalid input
retains the original path. The existing category-specific ownership/unlock
checks after818F97A8 remain unchanged; no ownership cache is introduced.

Private host probes execute the ORIGINAL search on the first observed call
for each index. At the convergence point they compare rank, selected index,
all10 selected-record bytes and all440 scratch-table bytes against the fast
path's expected values. Subsequent calls use the fast path. The build requires
at least40 actually observed indices; it does not claim all44 are reachable.

The iOS original clock speed and interpolation policies are unchanged.
Build35 diagnostic timing logs remain to confirm actual device behavior.
Only the Item headless route is repeated. Existing asset-free policy tests,
ARM64 compilation, unsigned packaging and payload audit remain required.

Device run44200431243 passed the Item original-code differential stage and is
compiling. Final artifact identity and exact observed coverage pending.
