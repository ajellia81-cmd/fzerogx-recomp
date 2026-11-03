# iOS Build15: menus containing perspective previews

## Report and root cause

The owner reports stretched/out-of-panel Replay and Customize messages, text flicker, black rectangular regions in Profile and Shop, and menu tunnel geometry visible outside the fixed menu composition. These reports are from the device; the screenshots do not by themselves establish the temporal cause of flicker.

Build14 tagged Title/Selection/Replay/Story/Winning/Movie/Options/Interview only. Profile, Customize, the pilot-point module (`fze.pilotpoint.rel`) and paint (`fze.car_colchg.rel`) were not represented. The inspected Shop category screen is drawn while Customize remains linked; the pilot-point family was not reached in these captures. Broad perspective geometry and retained race phases could therefore select World when those menu previews appeared. Replay also selected World by module, while the common framed-menu override was restricted to Title/Story. World then applied different canvases to text, panel and perspective/composite layers, exposing clears and masks that should remain in the original composition.

Register the four menu module families and encode them as ordered FIFO Menu context. Detect the paired menu frame independently of the module/race phase, using actually sampled textures only. Offscreen and single-strip HUD cases remain excluded. Encoded movies keep priority. Actual Replay playback without the menu frame keeps World behavior. Menu frames retain their complete command stream, copy targets, projection and scissor composition through the existing 16:9 presentation path.

Add the selected presentation mode to the motion-history boundary key. Crossing from a menu composition to World now clears prior geometry even if module and race-phase metadata have not changed. This addresses a demonstrated history-boundary gap; actual elimination of reported flicker still needs temporal/device verification.

## Controller scope

Remove the frontend button assignment page, capture state, assignment APIs and saved-map loading. Remove the legacy `recomp.controller.v1` preference at app startup. Use the upstream `default_gamepad_layout` mapping: physical A/B/X/Y -> guest A/X/B/Y, LB unassigned, RB -> Z, Start and D-pad unchanged; triggers remain L/R. Guest game settings stored on the memory card are not modified. Recomp retains Smooth motion, FPS and Return to game.

## Validation work

Cloud Linux ROM-free renderer/session fixtures pass, including menu context with perspective previews, Replay framed menus, exclusion of a single/offscreen strip, movie priority and unchanged menu command/state streams. Reclassification over all 68 existing private captures changes no decisions. Those captures do not contain the newly reported Profile/Shop/Replay-list cases.

Build15 adds four ordinary scripted-controller routes (Replay, Profile, Customize and an attempted Shop navigation) with encrypted-only decoded-frame captures before device compilation. Route names describe intended navigation; inspect actual module/frame metadata before claiming a screen was reached. No guest memory/scene is patched to force a route. Compilation, real-route rendering and device validation outcomes must be recorded separately once available.

No game inputs, generated game code or decoded frame captures are committed. Build12 warmup, Build13 resource fixes, Build14 World text correction and existing logo timing are retained.

## Completed build and private rendering review

Functional source: `45f0205e612d69d7734e11db8d41991d23d13ac7`. Full unsigned device run 41723646853 succeeded. ARM64 runtime/policy/resource checks 41723646831 and 41726393108 succeeded. Host logo verification reports `hold=1 AV_updates=120 AV_fade_updates=12 completed=1 result=PASS`.

The initial review artifact 13275590248 contains 32 decoded frames. It reached Replay's file list, Profile's pilot list, Customize's parent menu and Garage, rather than the intended Shop. Thirty frames that the prior policy classified as World now classify as Menu.

Test-only commit `3253930de6d551cf77b94bcd2398ad1d1e699b4a` changes only navigation/diagnostic scripts, not compiled app behavior. Additional host run 41726393083 succeeded; artifact 13275896302 contains 44 frames. It reaches the Profile detail with Captain Falcon and his machine, the Garage creation question, Replay's file list and the Shop category menu. Forty-one frames change World -> Menu; the remaining three were already Menu. The actual Shop merchandise grid and Replay's specific no-data message were not captured, so do not claim those exact views were visually verified. Their enclosing module/frame menu policy is covered by the source change.

Before/after host GLES rendering of these real captured inputs reproduces the old background spill, black/cut composition and distorted text, then shows the corrected fixed 16:9 composition. All reviewed host draws report GL error zero. The comparison uses Build14 renderer policy, reconstructing Build14 presentation tags from module/phase metadata; it is not an iPhone screenshot. Perspective character detail remains inside its menu composition.

Four consecutive captures (4000-4003) of the Garage question keep 188 central font draws. Host replay of the static bright-text region differs by at most eight threshold-mask pixels (approximately 9,524 bright pixels); no whole glyph disappears in this small sample. Replay/Profile/Shop-category central glyph geometry/UV/texture signatures are identical across their four samples. This is limited host evidence, not proof that Metal/device flicker is eliminated over arbitrary durations.

`FZeroGX-iOS-build15-unsigned.ipa`: 14,930,188 bytes; SHA-256 `96c949b81f1b8968b71c5743bc775eb81890e3d97eced3ae33b10747e02383a0`. Encrypted delivery artifact 13274907812 has ZIP digest `bf9d5936bc75743d014b01db35b7e0239b968c8b447a6e8c8c5fc9657535b909`. Independent decrypted ZIP/plist/Mach-O audit passes: bundle version 15, seven files, no game inputs/save/source/capture data, no embedded provisioning or signature load command. Both binaries are unsigned thin iPhoneOS ARM64. Effective minimum OS remains 26.2 because of the preserved ANGLE dependency.

Compilation/link/package: verified. Host execution into the listed menus: verified. iPhone app launch, device gameplay, the merchandise grid, physical 120fps and complete elimination of device text flicker: not verified for this build.
