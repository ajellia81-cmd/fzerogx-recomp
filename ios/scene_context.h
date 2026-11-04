#pragma once
#include "guest_state.h"
#include "gx/render.h"
namespace ios_context {
// This project does not otherwise use draw tags. Send a compact presentation
// context through the existing ordered FIFO tag path; never sample future guest
// atomics while displaying an older queued batch.
enum Kind : unsigned { Unknown,Title,Menu,Replay,Story,Winning,Movie,Settings37,Settings38,Course39,Grid40,Race41,Race42,End43,Interview,Option,Profile,Customize,Shop,Paint };
struct Snapshot {unsigned modules=0;int phase=-1;bool live=false,stamped=false;};
inline uint8_t encode(unsigned modules,int phase,bool live){
    unsigned k=modules&ios_guest::Movie?Movie:
        modules&ios_guest::Interview?Interview:
        modules&ios_guest::Paint?Paint:
        modules&ios_guest::Shop?Shop:
        modules&ios_guest::Customize?Customize:
        modules&ios_guest::Profile?Profile:
        modules&ios_guest::Option?Option:
        modules&ios_guest::Selection?Menu:
        modules&ios_guest::Replay?Replay:modules&ios_guest::Title?Title:
        modules&ios_guest::Story?Story:modules&ios_guest::Winning?Winning:
        phase>=37 && phase<=43?Settings37+unsigned(phase-37):Unknown;
    return uint8_t(0x80|k|(live?0x20:0));
}
inline Snapshot decode(uint8_t tag){
    Snapshot s;if(!(tag&0x80))return s;
    s.stamped=true;s.live=(tag&0x20)!=0;
    switch(tag&31){
        case Title:s.modules=ios_guest::Title;break;
        case Menu:s.modules=ios_guest::Selection;break;
        case Option:s.modules=ios_guest::Option;break;
        case Profile:s.modules=ios_guest::Profile;break;
        case Customize:s.modules=ios_guest::Customize;break;
        case Shop:s.modules=ios_guest::Shop;break;
        case Paint:s.modules=ios_guest::Paint;break;
        case Replay:s.modules=ios_guest::Replay;break;
        case Story:s.modules=ios_guest::Story;s.phase=41;break;
        case Winning:s.modules=ios_guest::Winning;s.phase=43;break;
        case Movie:s.modules=ios_guest::Movie;break;
        case Interview:s.modules=ios_guest::Interview;break;
        default:if((tag&31)>=Settings37 && (tag&31)<=End43)s.phase=37+int((tag&31)-Settings37);break;
    }
    return s;
}
inline Snapshot read(const gx::Batch& b){
    Snapshot s;
    for(const auto& c:b.cmds)if(c.type==gx::CmdType::Draw && c.count && c.vcount && c.state<b.states.size()){
        auto value=decode(b.states[c.state].tag);if(value.stamped)s=value;
    }
    return s;
}
}

