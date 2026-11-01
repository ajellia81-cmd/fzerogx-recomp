#include "controller.h"
#import <GameController/GameController.h>

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
