#pragma once
// Main-thread entry points. Engine/pattern preparation runs off the UI thread.
void ios_rumble_update(bool motor);
void ios_rumble_test();
const char* ios_rumble_route();
