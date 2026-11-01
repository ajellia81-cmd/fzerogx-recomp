# F-Zero GX iOS port — first unsigned device build

## Result (2026-11-02)

The complete iPhoneOS ARM64 game compiled, linked and packaged successfully in
Actions run 41330232233, from source commit
`12dee2e58e178f5a784e95cd0dd9169dd0f1434a`.

- IPA: `FZeroGX-iOS-60fps-unsigned.ipa`, 11,040,569 bytes.
- SHA-256: `8e4a31130f93bd080fd76d967c85b63d3da9aaddc53e738463e633f7770ba73a`.
- Executable and bundled ANGLE dylib: Mach-O ARM64, platform iPhoneOS, unsigned.
- ZIP integrity verified. The package contains only the executable, ANGLE dylib,
  Info.plist and PkgInfo; no disc, DOL, REL, generated C source or private key.
- Effective minimum OS is iOS 26.2 because of the bundled ANGLE build. The
  requested target device runs iOS 27. Device operation has not been tested.

## Implemented

- Native UIKit / UIWindowScene launcher and Files import into Documents.
- Original guest boot and GX rendering, with a native EGL window surface on a
  CAMetalLayer through ANGLE's Metal backend; no macOS binary retagging or JIT.
- Native landscape drawable size, including the requested 2868x1320 target.
  The final 4:3 image is stretched to fill the screen; no game-specific widescreen
  camera projection patch is claimed.
- Core Audio output; GameController extended gamepad input for Backbone and
  compatible controllers. Face buttons, analog ranges and deadzones follow
  the desktop port. Controller operation is not device-verified.
- Original 60-Hz guest logic and step scale 1.0. The app does not run game logic
  at 120 Hz. No motion interpolation is implemented in this first build.
- Saves and shader cache use Documents. This initial frontend ends the game
  process on backgrounding; it does not support guest suspend/resume.

## Verification gates

| Gate | Result |
|---|---|
| Isolated iOS ARM64 runtime compile | Passed: run 41323546923 |
| Host bootstrap / runtime fze.main generation | Passed on macOS runner |
| ANGLE iPhoneOS ARM64 build | Passed |
| Complete game/app compile and link | Passed: run 41330232233 |
| Unsigned device IPA packaging and static audit | Passed |
| App launch on iPhone | Not tested |
| Actual game operation, sound and controller input | Not tested |
| Measured 60-fps presentation | Not tested |
| 120-fps interpolation | Not implemented; requires a stable 60-fps baseline first |

## Build fixes

The standard ARM macOS runner was capacity-constrained, so the build uses a
standard Intel macOS host and cross-compiles the app to ARM64. Xcode 26.3 is
selected explicitly. ANGLE's pinned WebKit source triggers two mutex capability
static-analysis diagnostics with the public libc++ headers; only those two
warnings are not treated as errors. Actual mutex locking code is unchanged.

The iOS shader-key source has an independent custom command, avoiding an Xcode
new-build-system conflict with the host benchmark target. The upstream runtime
submodule remains pinned at `9e93f1bb7cc4a370af53c895e971fdcab55ed3fc`.

## Private inputs and delivery

The user supplied a 1,459,978,240-byte GFZE01 disc image. main.dol SHA-1 matched
`421c88106697d3275a3fc26fb7a01bf6d816b271`. Only original source and metadata are
committed. Disc images and generated game C are never uploaded or cached.

Actions receives the source share identifier through a repository secret.
Generated code stays in the ephemeral runner workspace. The IPA and diagnostics
are encrypted to `delivery-public.pem` before artifact upload. The private key
stays outside this repository and is never sent to Actions. Public dependency
caching is restricted to the ANGLE directory. A fork must supply its own delivery
certificate and retain its corresponding private key.

A private personal IPA still contains compiled game-derived code, despite not
containing the disc assets. It is not published as an unencrypted release asset.
The user must import their disc image into the installed app to play.

Keep the original image for device import and subsequent rebuilds. The attempted
persistent source-image backup did not succeed; a temporary working copy is not
claimed to be durable.

## Next stage

Install the first candidate on the actual device and establish correct boot,
rendering, audio, input, saves and stable 60-Hz game behavior. Then implement
render-only interpolation from stable camera/object identities, with cuts,
spawn/despawn and effect discontinuities handled explicitly. Do not modify guest
physics or mistake repeated display frames for interpolation. Measure unique
presentations and guest ticks independently before claiming actual 120 fps.
