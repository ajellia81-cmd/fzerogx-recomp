#pragma once
#include <cstdint>
#include "guest_state.h"
namespace ios_logo {
// Upstream documents ~400k back-edges, charged 800k TB ticks but about
// three times the hardware cost (docs/dev/boot.md). Restore that estimated
// duration only for the initial boot screen. Title's logo is frame-counted;
// restoring its per-frame busy work would reintroduce the old 20fps slowdown.
inline constexpr uint64_t delay_ticks(unsigned modules){return modules?0:800000/3;}
}
