#pragma once
#include "gx/render.h"
namespace gx {
// Only the iOS renderer exports these; simulation and desktop paths are unchanged.
void ios_widen(Batch& batch, int width, int height);
bool ios_render_motion(Batch& batch, bool smooth, void (*present)(void*), void* context);
}
