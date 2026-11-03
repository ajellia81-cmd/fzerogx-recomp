# iOS Build15: menus containing perspective previews

## Report and root cause

The owner reports stretched/out-of-panel Replay and Customize messages, text flicker, black rectangular regions in Profile and Shop, and menu tunnel geometry visible outside the fixed menu composition. These reports are from the device; the screenshots do not by themselves establish the temporal cause of flicker.

Build14 tagged Title/Selection/Replay/Story/Winning/Movie/Options/Interview only. Profile, Customize, Shop (`fze.pilotpoint.rel`) and paint (`fze.car_colchg.rel`) were not represented. Broad perspective geometry and retained race phases could therefore select World when those menu previews appeared. Replay also selected World by module, while the common framed-menu override was restricted to Title/Story. World then applied different canvases to text, panel and perspective/composite layers, exposing clears and masks that should remain in the original composition.

Register the four menu module families and encode them as ordered FIFO Menu context. Detect the paired menu frame independently of the module/race phase, using actually sampled textures only. Offscreen and single-strip HUD cases remain excluded. Encoded movies keep priority. Actual Replay playback without the menu frame keeps World behavior. Menu frames retain their complete command stream, copy targets, projection and scissor composition through the existing 16:9 presentation path.

Add the selected presentation mode to the motion-history boundary key. Crossing from a menu composition to World now clears prior geometry even if module and race-phase metadata have not changed. This addresses a demonstrated history-boundary gap; actual elimination of reported flicker still needs temporal/device verification.

## Controller scope

Remove the frontend button assignment page, capture state, assignment APIs and saved-map loading. Remove the legacy `recomp.controller.v1` preference at app startup. Use the upstream `default_gamepad_layout` mapping: physical A/B/X/Y -> guest A/X/B/Y, LB unassigned, RB -> Z, Start and D-pad unchanged; triggers remain L/R. Guest game settings stored on the memory card are not modified. Recomp retains Smooth motion, FPS and Return to game.

## Validation work

Cloud Linux ROM-free renderer/session fixtures pass, including menu context with perspective previews, Replay framed menus, exclusion of a single/offscreen strip, movie priority and unchanged menu command/state streams. Reclassification over all 68 existing private captures changes no decisions. Those captures do not contain the newly reported Profile/Shop/Replay-list cases.

Build15 adds four ordinary scripted-controller routes (Replay, Profile, Customize and an attempted Shop navigation) with encrypted-only decoded-frame captures before device compilation. Route names describe intended navigation; inspect actual module/frame metadata before claiming a screen was reached. No guest memory/scene is patched to force a route. Compilation, real-route rendering and device validation outcomes must be recorded separately once available.

No game inputs, generated game code or decoded frame captures are committed. Build12 warmup, Build13 resource fixes, Build14 World text correction and existing logo timing are retained.
