#pragma once
#include <atomic>
#include <string_view>
#include <array>
// Written by the game's existing guest-frame hook. UI/render threads never
// inspect mutable guest memory or SI output registers directly.
namespace ios_guest {
inline std::atomic<bool> motor{false};
inline std::atomic<int> phase{-1};
inline std::atomic<unsigned> modules{0};
enum : unsigned { Title=1, Selection=2, Replay=4, Story=8, Winning=16, Movie=32, Option=64, Interview=128, Profile=256, Customize=512, Shop=1024, Paint=2048 };
inline constexpr unsigned MenuModules=Selection|Option|Profile|Customize|Shop|Paint;
// Exhaustive inventory of the supported disc's 15 RELs. Main/sample are
// boot/runtime support, not independent display owners.
struct ModuleOwner {std::string_view name;unsigned mask;};
inline constexpr std::array<ModuleOwner,15> owners={{{"fze.main.rel",0},{"fze.sample.rel",0},{"fze.title.rel",Title},{"fze.sel.rel",Selection},{"fze.replay.rel",Replay},{"fze.story.rel",Story},{"fze.winning.rel",Winning},{"fze.interview.rel",Interview},{"fze.movie.rel",Movie},{"fze.movie_module.rel",Movie},{"fze.option.rel",Option},{"fze.profile.rel",Profile},{"fze.customize.rel",Customize},{"fze.pilotpoint.rel",Shop},{"fze.car_colchg.rel",Paint}}};
inline unsigned module_mask(std::string_view name){for(auto owner:owners)if(owner.name==name)return owner.mask;return 0;}
}
