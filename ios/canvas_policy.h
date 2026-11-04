#pragma once
#include "ui_policy.h"
#include <cmath>
#include <unordered_set>
#include <unordered_map>
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
template<class IsFull> inline bool feedback(const gx::PixelState& s,IsFull full){
    for(unsigned m=0,used=ios_scene::sampled_maps(s);m<8;++m)
        if((used&(1u<<m)) && s.tex_id[m] && s.tex_is_efb[m] && full(s.tex_id[m]))return true;
    return false;
}
inline bool solid_cover(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s){
    return full_quad(b,c,s) && !ios_scene::sampled_maps(s);
}
inline bool screen_effect(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s){
    return full_quad(b,c,s) && ((s.bp[0]>>16)&7);
}
inline bool screen_copy(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s,uint32_t id){
    if(!s.proj[6] || !ios_scene::full_view(s) || c.vcount<4 || size_t(c.vfirst)+c.vcount>b.verts.size())return false;
    float box[4];if(!ios_scene::draw_bounds(b,c,s,box))return false;
    if(box[0]>-.995f || box[1]>-.995f || box[2]<.995f || box[3]<.995f || std::fabs(box[0]+box[2])>.02f || std::fabs(box[1]+box[3])>.02f)return false;
    if((s.bp[0]>>16)&7)return true; // full-screen indirect feedback/warp
    for(unsigned j=0;j<((s.bp[0]>>10)&15)+1;++j){
        unsigned o=s.bp[0x28+j/2]>>((j&1)*12),m=o&7,at=(o>>3)&7;
        if(!(o&64) || s.tex_id[m]!=id || at>=s.num_texgens)continue;
        float u0=INFINITY,u1=-INFINITY,v0=INFINITY,v1=-INFINITY,q0=0;
        for(unsigned k=0;k<c.vcount;++k){const auto& uv=b.verts[c.vfirst+k].tex[at];if(std::fabs(uv[2])<1e-7f)return false;if(k && std::fabs(uv[2]-q0)>1e-5f)return false;q0=uv[2];float u=uv[0]/uv[2],v=uv[1]/uv[2];u0=std::min(u0,u);u1=std::max(u1,u);v0=std::min(v0,v);v1=std::max(v1,v);}
        if(u1-u0<1e-6f || v1-v0<1e-6f || std::fabs(u0+u1-1)>.01f)return false;
        bool xForward=true,xReverse=true,yForward=true,yReverse=true;
        for(unsigned k=0;k<c.vcount;++k){const auto& v=b.verts[c.vfirst+k];float u=(v.tex[at][0]/v.tex[at][2]-u0)/(u1-u0),t=(v.tex[at][1]/v.tex[at][2]-v0)/(v1-v0);
            float x=s.proj[0]*v.pos[0]+s.proj[1],y=s.proj[2]*v.pos[1]+s.proj[3];
            xForward &= std::fabs(x-(box[0]+u*(box[2]-box[0])))<.005f;
            xReverse &= std::fabs(x-(box[2]-u*(box[2]-box[0])))<.005f;
            yForward &= std::fabs(y-(box[1]+t*(box[3]-box[1])))<.005f;
            yReverse &= std::fabs(y-(box[3]-t*(box[3]-box[1])))<.005f;
        }
        // Centered axis-aligned zoom/flip commutes with the canvas transform;
        // rotated/sheared cards and independently placed previews do not.
        return (xForward||xReverse) && (yForward||yReverse);
    }return false;
}
inline void native_passes(const gx::Batch& b,std::vector<uint8_t>& skip,std::unordered_set<uint32_t>& native){
    for(auto id:b.dead_textures)native.erase(id);
    const auto f=frame(b);
    std::unordered_map<uint32_t,bool> writers,seen,screen;
    for(const auto& c:b.cmds)if(c.type==gx::CmdType::EfbCopy && !c.copy.to_xfb && c.copy.tex_id && c.copy.dst_w>=f.width && c.copy.dst_h>=f.height)writers[c.copy.tex_id]=c.copy.clear;
    for(const auto& c:b.cmds)if(c.type==gx::CmdType::Draw && c.count && c.state<b.states.size()){
        const auto& s=b.states[c.state];
        for(unsigned m=0,used=ios_scene::sampled_maps(s);m<8;++m)if((used&(1u<<m)) && s.tex_is_efb[m] && writers.count(s.tex_id[m])){
            auto id=s.tex_id[m];seen[id]=true;screen[id]=screen[id] || screen_copy(b,c,s,id);
        }
    }
    for(auto [id,clear]:writers){
        if(!clear || screen[id])native.erase(id);
        else if(seen[id])native.insert(id);
    }
    size_t start=0;
    for(size_t i=0;i<b.cmds.size();++i){const auto& c=b.cmds[i];if(c.type!=gx::CmdType::EfbCopy)continue;
        // A full-sized independent preview is still an offscreen pass. Its
        // target dimensions do not make it the final screen/postprocess.
        if(c.copy.clear && !c.copy.to_xfb && native.count(c.copy.tex_id))for(size_t j=start;j<=i;++j)skip[j]=1;
        if(c.copy.clear || c.copy.to_xfb)start=i+1;
    }
}
inline bool feedback_placement(const gx::Batch& b,const gx::Cmd& c,gx::PixelState& s,float ratio,const Frame& f){
    if(c.vcount!=4 || size_t(c.vfirst)+4>b.verts.size())return false;
    for(unsigned j=0;j<((s.bp[0]>>10)&15)+1;++j){unsigned o=s.bp[0x28+j/2]>>((j&1)*12),m=o&7,at=(o>>3)&7;
        if(!(o&64) || !s.tex_is_efb[m] || at>=s.num_texgens)continue;
        float u0=0,x0=0,du=0,dx=0;bool first=true,found=false;
        for(unsigned k=0;k<4;++k){const auto& v=b.verts[c.vfirst+k];float q=v.tex[at][2];if(std::fabs(q)<1e-7f)return false;
            float u=v.tex[at][0]/q,x=s.viewport[0]*(s.proj[0]*v.pos[0]+s.proj[1])+s.viewport[3]-int(s.bp[0x59]&1023)*2;
            if(first){u0=u;x0=x;first=false;}else if(std::fabs(u-u0)>1e-6f){du=u-u0;dx=x-x0;found=true;break;}
        }
        if(!found)return false;
        const float center=x0+(.5f-u0)*dx/du;
        s.viewport[3]+=(ratio-1)*(center-f.width*.5f);
        return true;
    }return false;
}
enum class Edge {None,Horizontal,Left,Right};
inline Edge edge(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s){
    if(!s.proj[6] || c.vcount!=4 || !ios_scene::full_view(s) || ios_scene::sampled_maps(s) || size_t(c.vfirst)+4>b.verts.size())return Edge::None;
    for(unsigned j=0;j<4;++j){const auto* col=b.verts[c.vfirst+j].col[0];if(col[0]||col[1]||col[2])return Edge::None;}
    float box[4];if(!ios_scene::draw_bounds(b,c,s,box))return Edge::None;
    // Solid edge mattes express coverage, not a picture's aspect. A native
    // canvas transform must not expose seams outside cinematic bars/masks.
    if(box[0]<=-.995f && box[2]>=.995f && box[3]-box[1]<.5f && (box[1]<=-.995f || box[3]>=.995f))return Edge::Horizontal;
    if(box[1]<=-.995f && box[3]>=.995f && box[2]-box[0]<.5f){
        if(box[0]<=-.995f)return Edge::Left;
        if(box[2]>=.995f)return Edge::Right;
    }
    return Edge::None;
}
inline void side(gx::PixelState& s,const float box[4],Edge edge,float ratio,const Frame& f){
    if(s.viewport[0]<=0 || box[2]-box[0]<1e-6f)return;
    const float offset=int(s.bp[0x59]&1023)*2;
    float left=s.viewport[0]*box[0]+s.viewport[3]-offset;
    float right=s.viewport[0]*box[2]+s.viewport[3]-offset;
    left=edge==Edge::Left?0:f.width*.5f+(left-f.width*.5f)*ratio;
    right=edge==Edge::Right?f.width:f.width*.5f+(right-f.width*.5f)*ratio;
    float x0=(left+offset-s.viewport[3])/s.viewport[0],x1=(right+offset-s.viewport[3])/s.viewport[0];
    float scale=(x1-x0)/(box[2]-box[0]),bias=x0-scale*box[0];
    s.proj[0]*=scale;s.proj[1]=s.proj[1]*scale+bias;
    unsigned l=unsigned(std::clamp(int(offset),0,2047)),r=unsigned(std::clamp(int(offset+f.width)-1,int(l),2047));
    s.bp[0x20]=(s.bp[0x20]&~(2047u<<12))|(l<<12);s.bp[0x21]=(s.bp[0x21]&~(2047u<<12))|(r<<12);
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
inline void compose_at(gx::PixelState& s,float ratio,const Frame& f,float center){
    (void)f;
    const float origin=center+int(s.bp[0x59]&1023)*2;
    s.viewport[0]*=ratio;
    s.viewport[3]=origin+(s.viewport[3]-origin)*ratio;
    clip(s,ratio,center);
}
inline void compose(gx::PixelState& s,float ratio,const Frame& f){compose_at(s,ratio,f,f.width*.5f);}
}
