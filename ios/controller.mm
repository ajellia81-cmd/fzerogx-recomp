#include "controller.h"
#import <GameController/GameController.h>
#import <Foundation/Foundation.h>
#include <algorithm>
#include <cmath>

// Read on the main thread, then pass the value to the guest thread. Never share
// mutable GCController objects with the simulation. Backbone uses this API.
IOSControllerState ios_controller_snapshot() {
    IOSControllerState s;
    GCController* controller = GCController.current;
    if (!controller.extendedGamepad) {
        for (GCController* candidate in GCController.controllers) {
            if (candidate.extendedGamepad) { controller = candidate; break; }
        }
    }
    GCExtendedGamepad* p = controller.extendedGamepad;
    if (!p) return s;
    if(p.buttonHome.preferredSystemGestureState!=GCSystemGestureStateDisabled) p.buttonHome.preferredSystemGestureState=GCSystemGestureStateDisabled;
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
        p.dpad.up, p.dpad.down, p.dpad.left, p.dpad.right, p.leftTrigger, p.rightTrigger
    };
    for (unsigned i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i)
        if (buttons[i].pressed) s.buttons |= 1u << i;
    if(p.buttonHome.pressed)s.buttons|=IOS_GUIDE_BUTTON;
    return s;
}


static ios_bindings::Map bindings=ios_bindings::defaults;
static NSString* const bindingsKey=@"recomp.controller.v2";
static void saveBindings(){NSMutableArray* a=[NSMutableArray array];for(auto v:bindings)[a addObject:@(v)];[NSUserDefaults.standardUserDefaults setObject:a forKey:bindingsKey];}
void ios_controller_load_bindings(){
    bindings=ios_bindings::defaults;
    id a=[NSUserDefaults.standardUserDefaults objectForKey:bindingsKey];
    if(![a isKindOfClass:NSArray.class] || [a count]!=bindings.size())return;
    ios_bindings::Map candidate{};
    for(unsigned i=0;i<candidate.size();++i){id n=a[i];if(![n isKindOfClass:NSNumber.class] || [n unsignedIntValue]>65535 || !ios_bindings::valid([n unsignedIntValue]))return;candidate[i]=[n unsignedIntValue];}
    bindings=candidate;
}
uint16_t ios_controller_binding(unsigned source){return source<bindings.size()?bindings[source]:0;}
void ios_controller_assign(unsigned source,uint16_t target){ios_bindings::assign(bindings,source,target);saveBindings();}
void ios_controller_clear(uint16_t target){ios_bindings::clear(bindings,target);saveBindings();}
void ios_controller_reset_bindings(){bindings=ios_bindings::defaults;saveBindings();}
PadState ios_controller_pad(const IOSControllerState& s){return ios_controller_translate(s,bindings);}
