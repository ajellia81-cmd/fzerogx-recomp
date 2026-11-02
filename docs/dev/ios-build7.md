# iOS follow-up after build 6 device feedback

The user reports that build 6 resolves the earlier selection-menu, HUD and movie
issues. Remaining device examples show horizontally stretched startup logos and
in-race/after-race popup menus and confirmation dialogs over a correct fullscreen
3D world. Treating every orthographic overlay as an already-compensated 16:9 HUD
is therefore not valid. The 3D scene, corrected HUD, complete selection-menu
composition and encoded video must retain their existing policies while those
other 2D layers are audited.

Additional private capture sampling for early startup and a scripted
pause/retire/quit route is prepared locally but has not been run. No capture or decoded game texture may be committed or uploaded in
plaintext. The user requires review and approval of affected screen categories before
any aspect-code correction and the next build. No renderer change or new
Actions run has been made for this follow-up.

## Requested iOS controller mapping

Physical GameController face-button order is A, B, X, Y. The requested actions
map to guest GameCube A, B, Y, X respectively: A accelerates, B brakes, X boosts,
Y performs the side attack. This is an iOS controller-adapter change; touch input
continues through its separate direct GameCube-pad path. Existing game menu hints
still refer to guest GameCube labels. The requested actions assume the game's
default control configuration.

## Static audit for user review (not an implementation)

`ios_prepare_scene` forces linked title scenes into World mode even on a pure
2D startup-logo frame. `ios_widen` then treats every full-viewport orthographic
overlay in a World frame as a pre-compensated 16:9 HUD. The user's AV-logo,
post-race menu and GP-quit-dialog screenshots disprove that universal assumption.
These are confirmed affected categories; they do not establish a measured ratio
for every other popup.

Review scope: standalone startup logos; pause/restart/retire/quit dialogs;
after-race and replay menus; common save/load/overwrite and memory-card or
controller notices; story-mode notices, where present. Shared projection,
scissor, view-space and fullscreen-copy composition paths all need checking.
Already-correct selection menus, racing HUD, 3D camera coverage, encoded movie
presentation and 60-Hz simulation/120-Hz render-only interpolation are regression
boundaries. Proposed correction is per overlay/group, with text and panel borders
kept together and their alignment preserved, rather than assigning a new ratio
to every 2D draw. Implementation awaits the user's approval of this scope.

## Inspection results before renderer changes

The uploaded AV-logo image is 2868x1320. A central white-logo bounding region is
774x580 pixels (aspect 1.334), consistent with the 4:3-to-16:9 expansion factor
1.333. The user's three screenshots confirm startup-logo, after-race menu and
GP-quit-dialog distortion; the panel's exact original dimensions have not been
inferred from its screenshot alone.

Four additional existing captures were replayed with the unchanged build-6
renderer executable, without compiling or dispatching a new workflow:

| Capture | Real metadata | Build-6 classification | Inspection |
|---|---|---|---|
| title batch 1000, name entry | Title module, phase 0 | World | Menu background leaks into side margins |
| story batch 2700, chapter selection | Story module, stale phase 41 | World | Same side-background/composition error |
| selection batch 3100, cup/course UI | Selection module | Menu | Correct menu composition retained |
| batch 3500, machine settings | phase 38 | Menu | Correct menu composition retained |

A classification-only experiment replayed private *copies* of the first two
captures as Menu by overriding only their diagnostic header's mode metadata.
The game, renderer source, vertices and textures were unchanged. Both background
errors disappeared and GL error was zero. This is an ablation experiment, not an
implemented guest-state change and not iPhone validation. It supports adding
name entry and story chapter selection to the proposed correction list.

The renderer audit also checked the final output aspect, orthographic and
perspective projection branches, scissor remapping, offscreen-copy exclusions,
fullscreen-copy composites, solid fades and encoded-movie exclusions. The
problem is not evidence that every orthographic draw should switch to 4:3:
main/selection menus and the user's racing HUD are regression boundaries.
Reused save/load/overwrite, result/ranking/winning, replay and controller-notice
paths are risk categories, but their individual runtime distortion is not
confirmed by the available captures. Do not change them preemptively or claim
that every game screen has been device-tested.

Proposed confirmed correction categories for approval: startup logo handling,
after-race popup menu, GP-quit/common confirmation composition, name-entry scene
classification and story chapter-selection classification. No renderer source
has been edited for these changes; no new Actions run has been dispatched.

## Approved implementation in progress

The user approved the five confirmed categories and the controller remap. Private
diagnostic run 41479820392 adds early-startup samples and a pause/quit route; it
does not build an IPA. The initial universal 16:9 overlay assumption is rejected
for logos and common modal panels, while the working HUD policy remains a
regression boundary.

Name entry and chapter selection now require both actually drawn, full-width
menu frame strips before overriding Title/Story and stale race-phase metadata.
The Story module alone does not force Menu, and movies retain first priority.
Private software replay of both original captures matches the previous
classification-only Menu experiment pixel for pixel, with GL error zero.
Five other captured scenes (machine selection, course selection, machine
settings, live race/HUD and movie) remain pixel-identical to build 6. Original
ROM-free fixtures check both the paired-frame positive case and the negative
single/offscreen-strip case. This is not device or physical-FPS validation.
