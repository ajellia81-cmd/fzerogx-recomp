# iOS build 10: startup logo follow-up

The user's build 9 device report disproves that its estimated boot delay fixed
SEGA timing, and shows the transient logo aspect regression still occurs.

## Timing

The iOS-only pre-module delay now consumes the documented 800,000 virtual ticks
per call instead of build 9's divided 266,666 estimate. Fifty calls consume
40,000,000 timebase ticks (about 0.99 seconds), versus 0.33 seconds in build 9.
This is an intentional startup timing adjustment, not a measured original-console
wall time. Once any mode module is linked the hook still returns immediately;
Title's frame-counted logo/fade and the 60Hz game logic are not slowed down.

## Aspect transitions

The previous whole-batch whitelist rejected a valid logo if ANY other draw or
retained texture binding was not also a recognized logo or full-screen fade.
Recognition now checks known startup material hashes only on texture maps that
the draw's enabled TEV stages actually sample, on an orthographic full-view draw.
Other draws cannot veto that recognition and unused old bindings cannot select it.
Logo canvas persists through solid/EFB-only transition batches; a newly drawn
non-logo texture, perspective scene or movie ends that transition. This avoids
changing canvas merely because the fading logo temporarily has no textured draw.
No timeout based on host/render frame counts is used. Menus, movies and 3D resume
the existing independent aspect policies.

## Verification

ROM-free regressions cover unused logo bindings, unrelated transition draws,
queued logo/fade/menu/world transitions, HUD/menu/modal/movie policies and session
timing. Private existing metadata captures: 68 frames, 15 startup logos and 11
movies retain classification. No private asset bytes or generated code are stored
in this repository. Native compile and package audit are recorded after the build.
Physical logo timing, transitions, application launch and real display rate remain
unverified until tested on the user's phone.

The workflow also accepts an explicit main-branch change to
`ios/private-build-request.txt`. This provides a narrowly scoped alternative to
manual dispatch when the GitHub web UI does not create a run. Ordinary source
pushes do not start the private build. The approved icon and Intel runner defaults
are fixed for this route, and already reviewed captures are not repeated.
