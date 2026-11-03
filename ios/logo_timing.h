#pragma once
#include <cstdint>
#include "guest_state.h"
namespace ios_logo {
// Build 9's divided estimate was visibly too short on the user's phone.
// Restore the documented virtual-loop budget ONLY before mode modules load.
// Never restore Title's per-frame work: it would reduce the frame-counted fade
// to 20fps. This is an explicit startup timing adjustment, not a measured claim
// about the original hardware duration.
inline constexpr uint64_t delay_ticks(unsigned modules){return modules?0:800000;}
}
