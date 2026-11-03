#pragma once
#include <atomic>
#include <cstdint>
namespace ios_logo {
// Original timing policy, based on the owner-provided reference recording.
// Canonical return sites are metadata for the pinned executable, not game code.
inline constexpr uint32_t boot_fade_a=0x80005A74,boot_fade_b=0x80005A78;
inline constexpr uint32_t title_tick=0x81A2BA34;
inline constexpr uint64_t visible_hold_ns=2300000000ull;
inline std::atomic<uint64_t> submitted_ns{0},presented_ns{0};
inline void note_submission(uint64_t ns){uint64_t zero=0;submitted_ns.compare_exchange_strong(zero,ns+1);}
inline void note_presentation(uint64_t ns){uint64_t zero=0;presented_ns.compare_exchange_strong(zero,ns+1);}
inline uint64_t hold_deadline(){
    uint64_t t=presented_ns.load();if(!t)t=submitted_ns.load();
    return t?t-1+visible_hold_ns:0;
}
inline constexpr uint64_t delay_ticks(uint32_t caller){
    // Only the five boot fade frames need their own elapsed time. The hold is
    // anchored to presentation later, so loading time is included rather than
    // added to another estimated fixed wait.
    return caller==boot_fade_a || caller==boot_fade_b?40500000ull/20:0;
}
class AVTimer {
    unsigned half_=0;
public:
    uint16_t remaining(unsigned phase,uint16_t value){
        if(phase!=1){half_=0;return value;}
        // 108 steady frames + 12 fade frames. Retain the game's fade curve;
        // traverse its 30 counter units in twelve 60Hz updates.
        if(value>138)value=138;
        if(value>30){half_=0;return value;}
        if(!value){half_=0;return 0;}
        unsigned extra=1+(half_++&1);
        return value>extra?uint16_t(value-extra):uint16_t(1);
    }
};
}
