#pragma once
#include <atomic>
// Written by the game's existing guest-frame hook. UI/render threads never
// inspect mutable guest memory or SI output registers directly.
namespace ios_guest {
inline std::atomic<bool> motor{false};
}
