#pragma once
#include "scene_classify.h"
#include <array>

namespace ios_ui {
inline unsigned sampled_maps(const gx::PixelState& s){return ios_scene::sampled_maps(s);}
// Recognize the shared 24x24 I4 glyph path by its actual texel mapping.
// Already-compensated geometry yields 16:9 and is left alone. Whole bitmaps
// cannot use this inference: intentional image scaling and nine-slice widgets
// otherwise separate their faces, borders, numeric fields or anchors.
inline bool fixed_glyph(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s){
    if(s.proj[6]==0 || ((s.bp[0]>>16)&7) || c.vcount!=4 || size_t(c.vfirst)+4>b.verts.size())return false;
    unsigned used=sampled_maps(s);
    if(!used || (used&(used-1)))return false;
    unsigned map=0;while(!(used&(1u<<map)))++map;
    if(!s.tex_id[map] || s.tex_is_efb[map])return false;
    unsigned coords=0,stages=((s.bp[0]>>10)&15)+1;
    for(unsigned i=0;i<stages;++i){unsigned order=s.bp[0x28+i/2]>>((i&1)*12);if(order&0x40)coords|=1u<<((order>>3)&7);}
    if(!coords || (coords>>std::min<unsigned>(s.num_texgens,8u)))return false;
    unsigned coord=0;while(!(coords&(1u<<coord)))++coord;
    unsigned t=s.bp[0x88+(map&3)+(map>=4?0x20:0)];
    unsigned w=(t&1023)+1,h=((t>>10)&1023)+1,format=(t>>20)&15;
    if(format!=0 || w!=24 || h!=24)return false;
    float box[4];if(!ios_scene::draw_bounds(b,c,s,box))return false;
    float dx=box[2]-box[0],dy=box[3]-box[1];
    if(dx<1e-6f || dy<1e-6f)return false;
    // Full-frame regular artwork may be a background/fade, not a UI sprite.
    if(box[0]<=-.99f && box[1]<=-.99f && box[2]>=.99f && box[3]>=.99f)return false;
    std::array<float,8> ulo,vlo,uhi,vhi;ulo.fill(INFINITY);vlo.fill(INFINITY);uhi.fill(-INFINITY);vhi.fill(-INFINITY);
    for(unsigned j=0;j<4;++j){const auto& v=b.verts[c.vfirst+j];
        float x=s.proj[0]*v.pos[0]+s.proj[1],y=s.proj[2]*v.pos[1]+s.proj[3];
        if(std::min(fabs(x-box[0]),fabs(x-box[2]))>dx*.001f || std::min(fabs(y-box[1]),fabs(y-box[3]))>dy*.001f)return false;
        for(unsigned at=0;at<8;++at)if(coords&(1u<<at)){
            float q=v.tex[at][2];if(!std::isfinite(q)||fabs(q)<1e-7f)return false;
            float u=v.tex[at][0]/q,vv=v.tex[at][1]/q;
            if(!std::isfinite(u)||!std::isfinite(vv))return false;
            ulo[at]=std::min(ulo[at],u);uhi[at]=std::max(uhi[at],u);vlo[at]=std::min(vlo[at],vv);vhi[at]=std::max(vhi[at],vv);
        }
    }
    float u0=ulo[coord],u1=uhi[coord],v0=vlo[coord],v1=vhi[coord];
    if(u1-u0<1e-6f || v1-v0<1e-6f)return false;
    // Outline text samples the same atlas using translated coordinates. The
    // spans must agree; unrelated multi-coordinate effects remain excluded.
    for(unsigned at=0;at<8;++at)if(coords&(1u<<at)){
        if(fabs((uhi[at]-ulo[at])-(u1-u0))>(u1-u0)*.001f || fabs((vhi[at]-vlo[at])-(v1-v0))>(v1-v0)*.001f)return false;
    }

    const float texelAspect=(u1-u0)*w/((v1-v0)*h);
    const float naturalCanvas=texelAspect*dy/dx;
    return std::isfinite(naturalCanvas) && fabs(naturalCanvas-4.f/3)<.02f;
}
struct GlyphLine {bool fixed=false;float anchor=0;};
inline std::vector<GlyphLine> glyph_lines(const gx::Batch& b,const std::vector<uint8_t>& skip,const std::vector<uint8_t>& modal){
    std::vector<GlyphLine> result(b.cmds.size());
    auto valid=[&](size_t i){const auto& c=b.cmds[i];return c.type==gx::CmdType::Draw && c.state<b.states.size() && !(i<skip.size()&&skip[i]) && !(i<modal.size()&&modal[i]) && fixed_glyph(b,c,b.states[c.state]);};
    for(size_t begin=0;begin<b.cmds.size();){
        if(!valid(begin)){++begin;continue;}
        const auto& first=b.states[b.cmds[begin].state];float bounds[4];ios_scene::draw_bounds(b,b.cmds[begin],first,bounds);
        const float height=bounds[3]-bounds[1],cy=(bounds[3]+bounds[1])*.5f;
        float left=bounds[0],right=bounds[2];size_t end=begin+1;
        for(;end<b.cmds.size()&&valid(end);++end){
            const auto& state=b.states[b.cmds[end].state];float box[4];ios_scene::draw_bounds(b,b.cmds[end],state,box);
            if(memcmp(first.proj,state.proj,sizeof(first.proj)) || memcmp(first.viewport,state.viewport,sizeof(first.viewport)) || first.bp[0x20]!=state.bp[0x20] || first.bp[0x21]!=state.bp[0x21])break;
            if(fabs((box[3]-box[1])-height)>height*.1f || fabs((box[3]+box[1])*.5f-cy)>height*.25f)break;
            if(box[0]>right+height*4 || box[2]<left-height*4)break;
            left=std::min(left,box[0]);right=std::max(right,box[2]);
        }
        for(size_t i=begin;i<end;++i)result[i]={true,(left+right)*.5f};
        begin=end;
    }
    return result;
}
inline bool logo_hash(uint64_t h){return h==0xffef214b102f891cull || h==0x9bf11ed63fad3dc8ull || h==0x4c8a45da63b18ea5ull || h==0xc98c5bd376ab7632ull;}
// Logos can be submitted again after the display option changes. Derive their
// canvas from the sampled texels and actual quad instead of assuming the boot
// geometry survives a return from the menu. General UI atlases are excluded.
template<class HashLookup> float startup_canvas_aspect(const gx::Batch& b,HashLookup hash,float fallback=4.f/3){
    float best=0,result=fallback;
    for(const auto& c:b.cmds){
        if(c.type!=gx::CmdType::Draw || !c.count || c.vcount!=4 || c.state>=b.states.size() || size_t(c.vfirst)+4>b.verts.size())continue;
        const auto& s=b.states[c.state];float box[4];
        if(s.proj[6]==0 || !ios_scene::full_view(s) || !ios_scene::draw_bounds(b,c,s,box))continue;
        float dx=box[2]-box[0],dy=box[3]-box[1];if(dx<1e-6f || dy<1e-6f)continue;
        unsigned used=sampled_maps(s),coords=0;
        for(unsigned j=0;j<((s.bp[0]>>10)&15)+1;++j){unsigned order=s.bp[0x28+j/2]>>((j&1)*12);if(order&0x40)coords|=1u<<((order>>3)&7);}
        if(!coords || (coords&(coords-1)) || (coords>>std::min<unsigned>(s.num_texgens,8u)))continue;
        unsigned at=0;while(!(coords&(1u<<at)))++at;
        float u0=INFINITY,u1=-INFINITY,v0=INFINITY,v1=-INFINITY;bool valid=true;
        for(unsigned j=0;j<4;++j){const auto& uv=b.verts[c.vfirst+j].tex[at];float q=uv[2];if(!std::isfinite(q)||fabs(q)<1e-7f){valid=false;break;}float u=uv[0]/q,v=uv[1]/q;valid&=std::isfinite(u)&&std::isfinite(v);u0=std::min(u0,u);u1=std::max(u1,u);v0=std::min(v0,v);v1=std::max(v1,v);}
        if(!valid || u1-u0<1e-6f || v1-v0<1e-6f)continue;
        for(unsigned m=0;m<8;++m)if((used&(1u<<m)) && s.tex_id[m] && !s.tex_is_efb[m] && logo_hash(hash(s.tex_id[m]))){
            unsigned t=s.bp[0x88+(m&3)+(m>=4?0x20:0)];
            float candidate=(u1-u0)*((t&1023)+1)/((v1-v0)*(((t>>10)&1023)+1))*dy/dx;
            float canvas=fabs(candidate-4.f/3)<.025f?4.f/3:fabs(candidate-16.f/9)<.025f?16.f/9:0;
            if(canvas && dx*dy>best){best=dx*dy;result=canvas;}
        }
    }
    return result;
}
template<class HashLookup> bool startup_art(const gx::Batch& b,HashLookup hash,bool segaOnly=false){
    // Identify the actual logo draw, not every retained binding or unrelated
    // pass in its batch. Transitions can submit both logos and fade geometry.
    for(const auto& c:b.cmds){
        if(c.type!=gx::CmdType::Draw || !c.count || !c.vcount || c.state>=b.states.size())continue;
        const auto& s=b.states[c.state];
        if(s.proj[6]==0 || !ios_scene::full_view(s))continue;
        unsigned used=sampled_maps(s);
        for(unsigned m=0;m<8;++m)if((used&(1u<<m)) && s.tex_id[m] && !s.tex_is_efb[m] && logo_hash(hash(s.tex_id[m])) && (!segaOnly || hash(s.tex_id[m])!=0xc98c5bd376ab7632ull))return true;
    }
    return false;
}
inline bool only_transition_draws(const gx::Batch& b){
    for(const auto& c:b.cmds){
        if(c.type!=gx::CmdType::Draw || !c.count || !c.vcount)continue;
        if(c.state>=b.states.size())return false;
        const auto& s=b.states[c.state];
        if(s.proj[6]==0 || !ios_scene::full_view(s))return false;
        unsigned used=sampled_maps(s);
        for(unsigned m=0;m<8;++m)if((used&(1u<<m)) && s.tex_id[m] && !s.tex_is_efb[m])return false;
    }
    return true;
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
        const unsigned used=sampled_maps(s);
        for(unsigned m=0;m<8;++m)if((used&(1u<<m)) && s.tex_id[m]){
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

