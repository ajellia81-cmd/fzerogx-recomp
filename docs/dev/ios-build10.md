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

## Completed build audit

- App source: `8cf10f01c282ad4a28c0c345f88956592c715353`.
- ARM64 runtime compile/regressions: Actions `41578129898`, success.
- Full real-game compile/link/package: Actions `41578327714`, success (13m47s).
- The manual dispatch appeared after a delay; duplicate push run `41578479332`
  was cancelled. The explicit request trigger remains available for future builds.
- No internal Clang abort/retry; the initial build log reports BUILD SUCCEEDED.
- `FZeroGX-iOS-build10-unsigned.ipa`: 14,924,066 bytes.
- SHA256: `3a2ff711363850d8d60c47ac46f523bcb9136e2b9866780e1be3aae9619b098d`.
- Seven bundle files, version 10; app and ANGLE thin ARM64 iPhoneOS, no code
  signatures, no provisioning, no disc/save/generated C/captures. Effective
  minimum iOS 26.2 from ANGLE. All three icon resources unchanged from build 9.
- Existing private startup captures 30/60/120/180/240 replay in software GLES
  at 1434x660 with original 2D aspect and GL_NO_ERROR, including deliberately
  mismatched newer Selection metadata. This is not iOS/Metal/device execution.
- New app launch, gameplay, physical 120Hz output, foreground restoration and
  exact logo timing/transition appearance remain device-unverified.
