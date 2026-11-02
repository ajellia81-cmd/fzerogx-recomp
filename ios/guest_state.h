#pragma once
#include <atomic>
// Written by the game's existing guest-frame hook. UI/render threads never
// inspect mutable guest memory or SI output registers directly.
namespace ios_guest {
inline std::atomic<bool> motor{false};
inline std::atomic<int> phase{-1};
inline std::atomic<unsigned> modules{0};
enum : unsigned { Title=1, Selection=2, Replay=4, Story=8, Winning=16, Movie=32, Option=64 };
}
