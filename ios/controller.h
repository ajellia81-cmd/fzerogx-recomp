#pragma once
#include <cstdint>
#include "hw/pad.h"
#include "controller_bindings.h"

// Host button layout is translated to GameCube pad bits by the frontend.
struct IOSControllerState {
    float leftX{}, leftY{}, rightX{}, rightY{}, leftTrigger{}, rightTrigger{};
    uint32_t buttons{};
    bool connected{};
};
IOSControllerState ios_controller_snapshot();
PadState ios_controller_pad(const IOSControllerState& s);
inline constexpr uint32_t IOS_GUIDE_BUTTON=1u<<11;
ios_bindings::Map ios_controller_bindings();
void ios_controller_assign(unsigned physical,uint16_t action);
void ios_controller_reset_bindings();
