#pragma once
#include "gx/render.h"
namespace gx {
// Only the iOS renderer exports these; simulation and desktop paths are unchanged.
bool ios_warm_pipeline_step(int* done, int* total);
void ios_configure_native(const Batch& batch, int width, int height);
bool ios_prepare_scene(const Batch& batch, bool liveRace);
const char* ios_scene_name();
const char* ios_motion_status();
void ios_frame_totals(uint64_t* native, uint64_t* intermediate);
void ios_widen(Batch& batch, int width, int height);
bool ios_render_motion(Batch& batch, bool smooth, void (*present)(void*), void* context);
}
