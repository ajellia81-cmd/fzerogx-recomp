#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
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
inline constexpr uint32_t IOS_GUIDE_BUTTON=1u<<13;

void ios_controller_load_bindings();
uint16_t ios_controller_binding(unsigned source);
void ios_controller_assign(unsigned source,uint16_t target);
void ios_controller_clear(uint16_t target);
void ios_controller_reset_bindings();
inline PadState ios_controller_translate(const IOSControllerState& s, const ios_bindings::Map& mapping) {
    PadState p;
    p.connected = s.connected;
    if (!s.connected) return p;
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
    p.buttons=0;
    for(unsigned i=0;i<mapping.size();++i){
        float value=i==11?s.leftTrigger:i==12?s.rightTrigger:((s.buttons&(1u<<i))?1.f:0.f);
        auto t=mapping[i];
        if(t==PAD_L)p.trig_l=std::max(p.trig_l,trigger(value));
        if(t==PAD_R)p.trig_r=std::max(p.trig_r,trigger(value));
        if(value>30000.f/32767.f)p.buttons|=t;
    }
    return p;
}
