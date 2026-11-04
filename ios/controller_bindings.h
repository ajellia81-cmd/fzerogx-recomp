#pragma once
#include <array>
#include "hw/pad.h"
namespace ios_bindings {
using Map=std::array<uint16_t,13>;
// Physical A/B/X/Y/LB/RB/Start/D-pad/LT/RT. Upstream defaults.
inline constexpr Map defaults={PAD_A,PAD_X,PAD_B,PAD_Y,0,PAD_Z,PAD_START,PAD_UP,PAD_DOWN,PAD_LEFT,PAD_RIGHT,PAD_L,PAD_R};
inline constexpr std::array<uint16_t,12> targets={PAD_A,PAD_B,PAD_X,PAD_Y,PAD_Z,PAD_L,PAD_R,PAD_START,PAD_UP,PAD_DOWN,PAD_LEFT,PAD_RIGHT};
inline constexpr bool valid(uint16_t v){if(!v)return true;for(auto t:targets)if(v==t)return true;return false;}
inline void clear(Map& m,uint16_t target){for(auto& v:m)if(v==target)v=0;}
inline void assign(Map& m,unsigned source,uint16_t target){if(source>=m.size() || !valid(target))return;clear(m,target);m[source]=target;}
}
