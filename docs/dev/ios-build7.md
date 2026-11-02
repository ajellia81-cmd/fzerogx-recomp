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
