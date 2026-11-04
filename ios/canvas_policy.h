#pragma once
#include "ui_policy.h"
#include <cmath>
namespace ios_canvas {
// All pieces of the guest's composition use ONE coordinate transform. Never
// infer a separate anchor from texture dimensions, draw adjacency or a quad's
// bounding box: borders, gauges, font shadows and EFB previews must stay joined.
inline float source_aspect(const gx::Batch& b,const std::vector<uint8_t>& skip,float fallback=4.f/3){
    unsigned normal=0,wide=0;
    for(size_t i=0;i<b.cmds.size();++i){const auto& c=b.cmds[i];
        if(c.type!=gx::CmdType::Draw || !c.count || c.state>=b.states.size() || (i<skip.size()&&skip[i]))continue;
        const auto& s=b.states[c.state];if(!ios_scene::full_view(s))continue;
        float a=ios_ui::glyph_canvas(b,c,s);
        normal+=a==4.f/3;wide+=a==16.f/9;
    }
    // Only positive geometric evidence may select an already-compensated
    // canvas. A frame without ordinary text retains the console's 4:3 canvas.
    return normal?4.f/3:wide?16.f/9:fallback;
}
struct Frame {float width=640,height=480;};
inline Frame frame(const gx::Batch& b){
    Frame f;
    for(const auto& c:b.cmds)if(c.type==gx::CmdType::Present && c.copy.src_w && c.copy.src_h){f.width=c.copy.src_w;f.height=c.copy.src_h;break;}
    return f;
}
inline bool full_quad(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s){
    if(!s.proj[6] || c.vcount!=4 || !ios_scene::full_view(s))return false;
    float box[4];return ios_scene::draw_bounds(b,c,s,box) && box[0]<=-.995f && box[1]<=-.995f && box[2]>=.995f && box[3]>=.995f;
}
inline bool solid_cover(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s){
    return full_quad(b,c,s) && !ios_scene::sampled_maps(s);
}
inline bool background(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s){
    if(!full_quad(b,c,s) || ((s.bp[0]>>16)&7))return false;
    unsigned used=ios_scene::sampled_maps(s);if(!used || (used&(used-1)))return false;
    unsigned m=0;while(!(used&(1u<<m)))++m;
    if(!s.tex_id[m] || s.tex_is_efb[m])return false;
    unsigned t=s.bp[0x88+(m&3)+(m>=4?32:0)];
    // Full-size colour backdrops behind a live composition. Glyph/number
    // atlases, panels, video planes and the startup artwork are excluded.
    return ((t>>20)&15)>=4 && (t&1023)>=599 && ((t>>10)&1023)>=399;
}
inline void clip(gx::PixelState& s,float ratio,float center){
    int offset=int(s.bp[0x59]&1023)*2;
    float origin=center+offset;
    int left=int((s.bp[0x20]>>12)&2047),right=int((s.bp[0x21]>>12)&2047)+1;
    int l=std::clamp(int(std::floor(origin+(left-origin)*ratio)),0,2047);
    int r=std::clamp(int(std::ceil(origin+(right-origin)*ratio))-1,l,2047);
    s.bp[0x20]=(s.bp[0x20]&~(2047u<<12))|(unsigned(l)<<12);
    s.bp[0x21]=(s.bp[0x21]&~(2047u<<12))|(unsigned(r)<<12);
}
inline void compose(gx::PixelState& s,float ratio,const Frame& f){
    const float origin=f.width*.5f+int(s.bp[0x59]&1023)*2;
    s.viewport[0]*=ratio;
    s.viewport[3]=origin+(s.viewport[3]-origin)*ratio;
    clip(s,ratio,f.width*.5f);
}
}
