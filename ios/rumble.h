#pragma once
// Main-thread entry points. Engine/pattern preparation runs off the UI thread.
void ios_rumble_update(bool motor);
void ios_rumble_pause_menu();
void ios_rumble_suspend();
// Prepare directly from the activation callback, even while a menu stays muted.
void ios_rumble_prepare_foreground();
void ios_rumble_resume();
void ios_rumble_test();
const char* ios_rumble_route();
