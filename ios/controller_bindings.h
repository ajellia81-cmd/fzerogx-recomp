#pragma once
#include <array>
#include "hw/pad.h"
namespace ios_bindings {
using Map=std::array<uint16_t,11>;
// Physical A, B, X, Y, LB, RB, Start and D-pad. Matches
// gcn-recomp/runtime/vr_config.cpp's default_gamepad_layout.
inline constexpr Map defaults={PAD_A,PAD_X,PAD_B,PAD_Y,0,PAD_Z,PAD_START,PAD_UP,PAD_DOWN,PAD_LEFT,PAD_RIGHT};
}
