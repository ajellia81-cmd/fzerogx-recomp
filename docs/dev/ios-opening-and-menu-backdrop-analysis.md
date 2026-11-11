# Opening demo and finite menu backdrop investigation — Build 31 baseline

## Confirmed causes

### Title / attract-demo bitmap captions

Private macOS Actions run 43028380247 captured a fresh boot without controller input through the real attract race. Twelve supplied snapshots (guest frames 900 through 6600) belong to Title, race phase 41, and contain no ordinary 24x24 I4 glyph votes.

The prompt uses a 324-pixel atlas row, submitted as a 243-wide, 24-high quad. Copyright samples an effective 250 by 34 texel region, submitted as a 187.5 by 34 quad. Both already contain the guest's 0.75 horizontal compensation. Their orthographic coordinate space is 640 by 480; their authored presentation canvas is therefore 16:9.

The port's unmarked canvas starts at 4:3 and changes only when ordinary glyphs vote. These image-only frames retain 4:3. Applying source/target with 4:3 instead of 16:9 makes both captions 25% too narrow. This is additional compensation applied by the port, not a defect in the original bitmap. A previous menu can change the retained fallback, making screen history relevant.

Do not infer arbitrary artwork aspect from texture size: intentional scaling, crops and nine-slice compositions exist. The correction needs authoritative canvas ownership / compensation information for bitmap compositions as well as ordinary fonts, preserved through sorted submissions. Existing independent card/window canvas ownership must remain intact.

### Shared finite menu background

Existing private captures identify the same background plane in Selection, Options, Profile, Replay, Story and Customize. Its source perspective camera is 4:3; other menu camera groups can already be 16:9.

Its four projected horizontal coordinates are approximately +/-1.1104 and +/-1.1930. It covers the original horizontal frustum. The current Menu target is 16:9, and the camera branch multiplies X projection by (4/3)/(16/9) = 0.75, bringing the plane edges to +/-0.8328 and +/-0.8947. The exposed edge positions match the slanted blue boundaries in the supplied screenshot.

The same camera branch exists in Build 18 source 0ccf3a8c49a90b32b0a106a8265139f083ba3118 and delivered Build 31 source f82d422555d7845126ac38a7b44661980c11c409. Thus the responsible code predates the Save Replay fix; this is not a claim that identical old device screenshots were compared.

The correction must distinguish authored finite menu backdrop camera groups from actual world-camera frustum expansion. Do not globally disable race camera widening or resize each foreground picture independently. Retain background coverage and coherent decoration placement, and preserve independent offscreen previews.

## Whole-game source-path inspection

The private generated corpus contains 7,377 functions, 27,356 direct call edges and 602 functions with indirect dispatch. Direct common image submission has 249 callers; alternate queue submission has 11; their union contains 253 callers. The common ordinary-font entry has 91 callers. Of the 253 image submitters, 208 do not directly call that font entry. These are not 208 proven bad screens: they expose why ordinary-font evidence alone cannot establish bitmap canvas ownership.

Inspection includes DOL, runtime main and every declared REL owner: title/attract, selection and Records, options/card, replay, story, winning/results, interview, profiles, customization, pilot-point shop, paint, movie and movie support, and sample/runtime support. Shared immediate and sorted sprite paths, direct sprite render wrappers, projection setup, native intermediate copies, final screen feedback, clipping, startup logos and video paths were traced as separate presentation paths.

Systemic risk cases:
- Image-only frames without a positive canvas vote can retain an inappropriate earlier canvas.
- Mixed native and already-compensated compositions cannot safely use one frame-wide inferred source.
- Finite backdrop cameras must not automatically inherit world-FOV expansion.
- Queue ownership must describe the composition when submitted, not the caller that eventually flushes it.

Existing Build 31 captures confirm the backdrop defect in six menu owners. Twenty-two existing race snapshots did not contain an isolated full-view finite plane that covers the original X frustum but becomes uncovered under the device camera ratio. This limited negative result is not full-stage visual verification.

## Verification and status

- Baseline private diagnostic run: https://github.com/ajellia81-cmd/fzerogx-recomp/actions/runs/43028380247
- Changes in diagnostic source 7255af94299117aba55f5ed4e614fa60f2fb6863 add only an unattended attract route and select diagnostic routes; application renderer is unchanged.
- The strict existing audit passes 75 supplied snapshots, including the new attract captures. It verifies ordinary native glyph aspect and protected data, but has no expected-aspect assertion for these bitmap captions or coverage assertion for finite backgrounds. Passing this audit did not establish that either reported issue was absent.
- Save Replay is user-confirmed resolved and remains closed.
- No new application fix or IPA is represented by this investigation commit.
