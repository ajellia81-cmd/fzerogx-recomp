# Item slowdown follow-up after build34

Owner confirms badge size fixed. Item still halves output; Smooth Motion OFF
also leaves original frame rate around30. Preserve all confirmed visual fixes.

Supplied log(6).txt shows ~29.9 batches/s and~59.7 swaps/s with interpolation
active. The build33 20.8ms native slow-phase sample is absent from this log;
absence is not proof of low renderer cost. Full pipeline and average-phase
timers are required before further renderer changes.

Build34 macOS CPU sample:
private build logs item-cpu-profile.txt, 2926 samples per thread.
Guest path: 8198C568 ->819838F8 ->818F965C.
818F965C has916 self samples (and further IRQ/scheduler child samples);
previously optimized818F9028 has6 self samples. The remaining hot function
is therefore different. Function metadata size0x22C.
Do not claim the previous ordinal optimization fixed Item.

Commit5bfb26d0d41763749e47f9895718a02c859ff3d9 prepares diagnostic version35:
5s pipeline wait/setup/widen/render averages; renderer prep/intermediate/native/
swap averages and initial-copy/clear counters; GCN_CLOCKLOG and GCN_STALLS.
No gameplay, display, interpolation or clock-speed change is introduced.
A single GPU-free shop-items route is selected instead of repeating the
unchanged seven-route visual corpus. Existing asset-free presentation tests
and device compilation remain required.

Device build44124250506 queued.
Runtime check44124250583 failed BEFORE any steps because GitHub annotation:
"The job was not acquired by Runner of type hosted even after multiple attempts"
(check-run134183578596). No compiler error or test failure was produced.

Bounded encrypted source review requests44124754642 (macos-15-intel) and
44126850966 (alternate macos-15) are queued. They generate source on macOS
and encrypt only818F965C,819838F8,818F9028 for review, no public guest source.
Latest source-review workflow commitbb4def22a098a88ffc085d0361e90390179fba65.

Next: obtain original818F965C semantics, inspect Item clock/work and queue
timings, then implement a result-equivalent fix with differential validation.
No Item fix or build35 IPA is claimed complete while runners are unavailable.
