#pragma once
#include <cstdint>

// Host button layout is translated to GameCube pad bits by the frontend.
struct IOSControllerState {
    float leftX{}, leftY{}, rightX{}, rightY{}, leftTrigger{}, rightTrigger{};
    uint32_t buttons{};
    bool connected{};
};
IOSControllerState ios_controller_snapshot();
