#pragma once
#include <cstdint>
#include "hw/pad.h"

// Host button layout is translated to GameCube pad bits by the frontend.
struct IOSControllerState {
    float leftX{}, leftY{}, rightX{}, rightY{}, leftTrigger{}, rightTrigger{};
    uint32_t buttons{};
    bool connected{};
};
IOSControllerState ios_controller_snapshot();
PadState ios_controller_pad(const IOSControllerState& s);
