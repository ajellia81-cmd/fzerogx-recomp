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
    auto axis = [](float v) -> uint8_t {
        if (!std::isfinite(v)) return 128;
        return static_cast<uint8_t>(std::clamp(128 + static_cast<int>(v * 100.0f), 0, 255));
    };
    auto trigger = [](float v) -> uint8_t {
        if (!std::isfinite(v) || v <= 1000.0f / 32767.0f) return 0;
        return static_cast<uint8_t>(std::clamp(static_cast<int>(v * 255), 0, 255));
    };
    if (std::abs(s.leftX) > 4000.0f / 32767.0f || std::abs(s.leftY) > 4000.0f / 32767.0f) {
        p.stick_x = axis(s.leftX);
        p.stick_y = axis(s.leftY);
    }
    if (std::abs(s.rightX) > 1000.0f / 32767.0f || std::abs(s.rightY) > 1000.0f / 32767.0f) {
        p.cstick_x = axis(s.rightX);
        p.cstick_y = axis(s.rightY);
    }
    p.trig_l = trigger(s.leftTrigger);
    p.trig_r = trigger(s.rightTrigger);
    if (s.leftTrigger > 30000.0f / 32767.0f) p.buttons |= PAD_L;
    if (s.rightTrigger > 30000.0f / 32767.0f) p.buttons |= PAD_R;
    return p;
}
