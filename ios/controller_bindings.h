#pragma once
#include <array>
#include "hw/pad.h"
namespace ios_bindings {
using Map=std::array<uint16_t,11>;
inline constexpr Map defaults={PAD_A,PAD_B,PAD_Y,0,PAD_X,PAD_Z,PAD_START,PAD_UP,PAD_DOWN,PAD_LEFT,PAD_RIGHT};
inline bool valid(uint16_t value){
    constexpr uint16_t allowed=PAD_A|PAD_B|PAD_X|PAD_Y|PAD_Z|PAD_START|PAD_L|PAD_R|PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT;
    return !value || (!(value&(value-1)) && !(value&~allowed));
}
inline void assign(Map& map,unsigned physical,uint16_t action){
    if(physical>=map.size() || !valid(action))return;
    for(auto& value:map)if(action && value==action)value=0;
    map[physical]=action;
}
}
