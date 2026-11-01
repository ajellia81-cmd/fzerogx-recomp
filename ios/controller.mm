#include "controller.h"
#import <GameController/GameController.h>
#include <algorithm>
#include <cmath>

// Read on the main thread, then pass the value to the guest thread. Never share
// mutable GCController objects with the simulation. Backbone uses this API.
IOSControllerState ios_controller_snapshot() {
    IOSControllerState s;
    GCController* controller = GCController.current;
    if (!controller) controller = GCController.controllers.firstObject;
    GCExtendedGamepad* p = controller.extendedGamepad;
    if (!p) return s;
    s.connected = true;
    s.leftX = p.leftThumbstick.xAxis.value;
    s.leftY = p.leftThumbstick.yAxis.value;
    s.rightX = p.rightThumbstick.xAxis.value;
    s.rightY = p.rightThumbstick.yAxis.value;
    s.leftTrigger = p.leftTrigger.value;
    s.rightTrigger = p.rightTrigger.value;
    GCControllerButtonInput* buttons[] = {
        p.buttonA, p.buttonB, p.buttonX, p.buttonY,
        p.leftShoulder, p.rightShoulder, p.buttonMenu,
        p.dpad.up, p.dpad.down, p.dpad.left, p.dpad.right
    };
    for (unsigned i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i)
        if (buttons[i].pressed) s.buttons |= 1u << i;
    return s;
}

PadState ios_controller_pad(const IOSControllerState& s) {
    PadState p;
    p.connected = s.connected;
    if (!s.connected) return p;
    // Match the upstream desktop face-button layout: A/X/B/Y -> GC A/B/X/Y.
    const uint16_t mapping[] = {PAD_A, PAD_X, PAD_B, PAD_Y, 0, PAD_Z,
                               PAD_START, PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT};
    for (unsigned i = 0; i < sizeof(mapping) / sizeof(mapping[0]); ++i)
        if (s.buttons & (1u << i)) p.buttons |= mapping[i];
    auto axis = [](float v, float deadzone) -> uint8_t {
        if (!std::isfinite(v) || std::abs(v) < deadzone) return 128;
        return static_cast<uint8_t>(std::clamp(std::lround(128 + v * 127), 0L, 255L));
    };
    auto trigger = [](float v) -> uint8_t {
        if (!std::isfinite(v)) return 0;
        return static_cast<uint8_t>(std::clamp(std::lround(v * 255), 0L, 255L));
    };
    p.stick_x = axis(s.leftX, 0.12f);
    p.stick_y = axis(s.leftY, 0.12f);
    p.cstick_x = axis(s.rightX, 0.03f);
    p.cstick_y = axis(s.rightY, 0.03f);
    p.trig_l = trigger(s.leftTrigger);
    p.trig_r = trigger(s.rightTrigger);
    if (s.leftTrigger > 30000.0f / 32767.0f) p.buttons |= PAD_L;
    if (s.rightTrigger > 30000.0f / 32767.0f) p.buttons |= PAD_R;
    return p;
}
