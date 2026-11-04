#pragma once
#include "gx/render.h"
#include "guest_state.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace ios_scene {
enum class Mode { Menu, World, Movie, Flat };
inline unsigned sampled_maps(const gx::PixelState& s){
    unsigned mask=0,stages=((s.bp[0]>>10)&15)+1;
    for(unsigned i=0;i<stages;++i){unsigned order=s.bp[0x28+i/2]>>((i&1)*12);if(order&0x40)mask|=1u<<(order&7);}
    return mask;
}
inline bool full_view(const gx::PixelState& s){return std::fabs(s.viewport[0])*2>=600 && std::fabs(s.viewport[1])*2>=400;}
inline bool movie_state(const gx::PixelState& s){
    // F-Zero GX's existing movie path uses I8 Y + half-size IA8 UV,
    // not three separate I8 planes (docs/dev/status.md). This only identifies
    // already-decoded texture metadata; the guest still decodes the movie.
    if(s.proj[6]==0)return false;
    for(unsigned y=0;y<8;++y)if(s.tex_id[y] && !s.tex_is_efb[y]){
        unsigned r=0x88+(y&3)+(y>=4?0x20:0),v=s.bp[r];
        unsigned w=(v&1023)+1,h=((v>>10)&1023)+1;
        if(((v>>20)&15)!=1 || w<320 || h<160)continue;
        for(unsigned uv=0;uv<8;++uv)if(uv!=y && s.tex_id[uv] && !s.tex_is_efb[uv]){
            unsigned q=0x88+(uv&3)+(uv>=4?0x20:0),t=s.bp[q];
            if(((t>>20)&15)==3 && ((t&1023)+1)*2==w && (((t>>10)&1023)+1)*2==h)return true;
        }
    }
    return false;
}
// Bounds in the guest's normalized screen, before any iOS correction.
inline bool draw_bounds(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& st,float box[4]){
    if(!c.vcount || size_t(c.vfirst)+c.vcount>b.verts.size())return false;
    box[0]=box[1]=INFINITY;box[2]=box[3]=-INFINITY;
    for(uint32_t j=0;j<c.vcount;++j){const auto& v=b.verts[c.vfirst+j];
        float x=st.proj[0]*v.pos[0]+st.proj[1],y=st.proj[2]*v.pos[1]+st.proj[3];
        if(!std::isfinite(x)||!std::isfinite(y))return false;
        box[0]=std::min(box[0],x);box[1]=std::min(box[1],y);box[2]=std::max(box[2],x);box[3]=std::max(box[3],y);
    }return true;
}
// Recover the display canvas from the guest's actual movie quad, so the
// encoded image keeps its texel aspect in either game display option.
inline float movie_canvas_aspect(const gx::Batch& b,float fallback=4.f/3){
    float sourceW=640,sourceH=480;
    for(const auto& c:b.cmds)if(c.type==gx::CmdType::Present && c.copy.src_w && c.copy.src_h){sourceW=c.copy.src_w;sourceH=c.copy.src_h;break;}
    for(const auto& c:b.cmds)if(c.type==gx::CmdType::Draw && c.state<b.states.size()){
        const auto& s=b.states[c.state];float box[4];
        if(!movie_state(s) || !draw_bounds(b,c,s,box))continue;
        float w=(box[2]-box[0])*std::fabs(s.viewport[0])*2/sourceW;
        float h=(box[3]-box[1])*std::fabs(s.viewport[1])*2/sourceH;
        if(w<.25f || h<.25f)continue;
        for(unsigned m=0;m<8;++m)if(s.tex_id[m]){
            unsigned r=0x88+(m&3)+(m>=4?0x20:0),t=s.bp[r];
            if(((t>>20)&15)!=1 || (t&1023)<319)continue;
            float aspect=float((t&1023)+1)/(((t>>10)&1023)+1)*h/w;
            if(std::isfinite(aspect) && aspect>=1.f && aspect<=2.5f)return aspect;
        }
    }
    return fallback;
}
inline bool black_side_mask(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& st,const float* box){
    if(box[3]-box[1]<1.5f || box[2]-box[0]>.5f || !(box[2]<-.5f || box[0]>.5f))return false;
    // Only opaque, solid black masks on the selection screen. Textured artwork,
    // working render targets, text panels and full-screen fades are preserved.
    for(auto id:st.tex_id)if(id)return false;
    for(uint32_t j=0;j<c.vcount;++j){const auto* col=b.verts[c.vfirst+j].col[0];if(col[0]||col[1]||col[2]||col[3]!=255)return false;}
    return true;
}
inline Mode classify(const gx::Batch& b,bool liveRace=false,unsigned modules=0,int phase=-1){
    bool world=false,movie=false,menu44=false,menu60=false,perspective=false;
    size_t passStart=0;
    std::vector<uint8_t> offscreen(b.cmds.size());
    for(size_t i=0;i<b.cmds.size();++i)if(b.cmds[i].type==gx::CmdType::EfbCopy){
        const auto& c=b.cmds[i].copy;
        bool smallPass=!c.to_xfb && c.clear && (c.src_w<600 || c.src_h<400);
        for(size_t j=passStart;smallPass && j<i;++j){
            const auto& d=b.cmds[j];
            if(d.type!=gx::CmdType::Draw || d.state>=b.states.size())continue;
            const auto& s=b.states[d.state];
            int xo=int(s.bp[0x59]&1023)*2,yo=int((s.bp[0x59]>>10)&1023)*2;
            float w=2*std::fabs(s.viewport[0]),h=2*std::fabs(s.viewport[1]);
            float x=s.viewport[3]-xo-w/2,y=s.viewport[4]-yo-h/2;
            float x0=std::max(x,float(int((s.bp[0x20]>>12)&2047)-xo));
            float y0=std::max(y,float(int(s.bp[0x20]&2047)-yo));
            float x1=std::min(x+w,float(int((s.bp[0x21]>>12)&2047)-xo+1));
            float y1=std::min(y+h,float(int(s.bp[0x21]&2047)-yo+1));
            if(x1>x0 && y1>y0 && (x0<float(c.src_x)-1 || y0<float(c.src_y)-1 || x1>float(c.src_x+c.src_w)+1 || y1>float(c.src_y+c.src_h)+1))smallPass=false;
        }
        if(smallPass)for(size_t j=passStart;j<=i;++j)offscreen[j]=1;
        passStart=i+1;
    }
    for(size_t i=0;i<b.cmds.size();++i){
        const auto& c=b.cmds[i];
        if(c.type!=gx::CmdType::Draw || !c.count || !c.vcount || offscreen[i] || c.state>=b.states.size())continue;
        const auto& s=b.states[c.state];
        perspective|=s.proj[6]==0;
        movie|=movie_state(s);
        // Name entry and chapter selection share the menu frame, even while
        // Title/Story is linked and the race phase still describes the last race.
        // Require both drawn full-width frame strips; an individual HUD sprite
        // or the presence of Story alone does not identify a menu.
        if(s.proj[6]!=0 && full_view(s)){
            for(unsigned m=0,used=sampled_maps(s);m<8;++m)if((used&(1u<<m)) && s.tex_id[m] && !s.tex_is_efb[m]){
                unsigned r=0x88+(m&3)+(m>=4?0x20:0),t=s.bp[r],h=(t>>10)&1023;
                if(((t>>20)&15)==5 && (t&1023)==639 && (h==43 || h==59)){
                    float box[4];
                    if(draw_bounds(b,c,s,box) && box[0]<=-.99f && box[2]>=.99f && box[3]<-.7f){
                        menu44|=h==43;menu60|=h==59;
                    }
                }
            }
        }
        if(s.proj[6]!=0 || s.view_space || !full_view(s) || c.vcount<16 || size_t(c.vfirst)+c.vcount>b.verts.size())continue;
        float x0=1,x1=-1,y0=1,y1=-1;unsigned samples=std::min(c.vcount,64u),front=0;
        for(unsigned j=0;j<samples;++j){
            const auto& v=b.verts[c.vfirst+(uint64_t(j)*(c.vcount-1))/(samples-1)];
            if(v.pos[2]>=-.01f)continue;
            float x=(s.proj[0]*v.pos[0]+s.proj[1]*v.pos[2])/-v.pos[2];
            float y=(s.proj[2]*v.pos[1]+s.proj[3]*v.pos[2])/-v.pos[2];
            if(!std::isfinite(x)||!std::isfinite(y))continue;
            x=std::clamp(x,-1.f,1.f);y=std::clamp(y,-1.f,1.f);
            x0=std::min(x0,x);x1=std::max(x1,x);y0=std::min(y0,y);y1=std::max(y1,y);++front;
        }
        // Full-view course/sky geometry, unlike an individual menu preview.
        if(front>=4 && x1-x0>=1.5f && y1-y0>=1.1f)world=true;
    }
    if(movie || (modules & ios_guest::Movie))return Mode::Movie;
    // The frame belongs to the screen being drawn, independently of the
    // linked module or a race phase retained after leaving gameplay. Replay
    // lists and data-management dialogs use the same frame as other menus.
    if(menu44 && menu60)return Mode::Menu;
    // Startup logos are orthographic-only Title frames. Linking Title does not
    // make those images a 3D world; the actual intro camera still has perspective.
    if((modules & ios_guest::Title) && !(modules & (ios_guest::Selection|ios_guest::Option)) && !perspective)return Mode::Flat;
    if(modules & ios_guest::Interview)return Mode::World;
    if(liveRace)return Mode::World;
    if(modules & ios_guest::MenuModules)return Mode::Menu;
    // Replay REL owns the card browser, including its transient dialogs.
    // Actual course playback keeps liveRace precedence above.
    if(modules & ios_guest::Replay)return Mode::Menu;
    if(modules & ios_guest::Title)return Mode::World;
    if(phase==37 || phase==38)return Mode::Menu;
    // A retained race phase is not evidence of a world in an orthographic
    // card/dialog frame (main keeps this value after leaving a race).
    if(phase>=39 && phase<=43)return perspective?Mode::World:Mode::Menu;
    if(world)return Mode::World;
    return perspective?Mode::Menu:Mode::Flat;
}
}
