#pragma once
#include "scene_classify.h"
#include <array>

namespace ios_ui {
template<class HashLookup> bool startup_art(const gx::Batch& b,HashLookup hash){
    bool logo=false;
    for(const auto& c:b.cmds){
        if(c.type!=gx::CmdType::Draw || !c.count || !c.vcount)continue;
        if(c.state>=b.states.size())return false;
        const auto& s=b.states[c.state];
        if(s.proj[6]==0 || !ios_scene::full_view(s))return false;
        bool textured=false;
        for(unsigned m=0;m<8;++m)if(s.tex_id[m]){
            textured=true;if(s.tex_is_efb[m])continue;
            const auto h=hash(s.tex_id[m]);
            if(h!=0xffef214b102f891cull && h!=0x9bf11ed63fad3dc8ull && h!=0x4c8a45da63b18ea5ull && h!=0xc98c5bd376ab7632ull)return false;
            logo=true;
        }
        if(!textured){float box[4];if(!ios_scene::draw_bounds(b,c,s,box) || box[0]>-.99f || box[1]>-.99f || box[2]<.99f || box[3]<.99f)return false;}
    }
    return logo;
}
// Stable material metadata from private decoded-frame inspection, not asset
// bytes. A complete modal run includes its shadow text before the panel, the
// panel pieces, foreground glyphs and the menu-title atlas. HUD draws terminate
// the run, even when they share a PixelState or occupy the same screen region.
inline constexpr uint64_t panel_hash=0xd5cd6a67911a0327ull;
inline constexpr uint64_t menu_title_hash=0x1ac2f0673821c091ull;
template<class HashLookup>
std::vector<uint8_t> modal_draws(const gx::Batch& b,const std::vector<uint8_t>& skip,HashLookup hash){
    std::vector<uint8_t> member(b.cmds.size()),panel(b.cmds.size()),result(b.cmds.size());
    for(size_t i=0;i<b.cmds.size();++i){
        const auto& c=b.cmds[i];
        if(c.type!=gx::CmdType::Draw || (i<skip.size() && skip[i]) || c.state>=b.states.size())continue;
        const auto& s=b.states[c.state];
        if(s.proj[6]==0 || !ios_scene::full_view(s))continue;
        bool any=false,valid=true;
        for(unsigned m=0;m<8;++m)if(s.tex_id[m]){
            any=true;
            unsigned r=0x88+(m&3)+(m>=4?0x20:0),t=s.bp[r];
            unsigned w=(t&1023)+1,h=((t>>10)&1023)+1,f=(t>>20)&15;
            bool p=!s.tex_is_efb[m] && f==5 && w==52 && h==104 && hash(s.tex_id[m])==panel_hash;
            bool title=!s.tex_is_efb[m] && f==2 && w==216 && h==128 && hash(s.tex_id[m])==menu_title_hash;
            bool glyph=!s.tex_is_efb[m] && f==0 && w==24 && h==24;
            valid&=p||title||glyph;panel[i]|=p;
        }
        member[i]=any && valid;
    }
    for(size_t start=0;start<member.size();){
        if(!member[start]){++start;continue;}
        size_t end=start;bool confirmed=false;
        while(end<member.size() && member[end])confirmed|=panel[end++];
        if(confirmed)std::fill(result.begin()+start,result.begin()+end,1);
        start=end;
    }
    return result;
}
}
