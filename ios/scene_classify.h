#pragma once
#include "gx/render.h"
#include <algorithm>
#include <cmath>
#include <vector>
namespace ios_scene {
enum class Mode { Menu, World, Movie };
inline bool full_view(const gx::PixelState& s){return std::fabs(s.viewport[0])*2>=600 && std::fabs(s.viewport[1])*2>=400;}
inline bool movie_state(const gx::PixelState& s){
    // THP video is composed from three I8 luminance/chroma planes. Ordinary
    // menu/background textures must not be mistaken for a movie.
    if(s.proj[6]==0 || !full_view(s))return false;
    unsigned planes=0;
    for(unsigned m=0;m<8;++m)if(s.tex_id[m]){
        unsigned r=0x88+(m&3)+(m>=4?0x20:0);unsigned v=s.bp[r];
        if(((v>>20)&15)==1 && (v&1023)>=127 && ((v>>10)&1023)>=63)++planes;
    }
    return planes>=3;
}
inline Mode classify(const gx::Batch& b,bool liveRace=false){
    bool world=false,movie=false;
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
        if(c.type!=gx::CmdType::Draw || offscreen[i] || c.state>=b.states.size())continue;
        const auto& s=b.states[c.state];
        movie|=movie_state(s);
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
    return movie?Mode::Movie:(world||liveRace)?Mode::World:Mode::Menu;
}
}
