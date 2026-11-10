#pragma once
#include "gx/render.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <unordered_map>
namespace ios_motion {
// Identify geometry/material, never the entire BP register snapshot: BP also
// contains changing copy destinations, tokens and texture upload bookkeeping.
inline uint64_t draw_key(const gx::Batch& b,const gx::Cmd& c){
    const auto& s=b.states[c.state];uint64_t h=1469598103934665603ull;
    auto add=[&](uint32_t x){h^=x;h*=1099511628211ull;};
    add(c.prim);add(c.vcount);add(c.count);add(s.num_texgens);add(s.num_colors);
    add(s.bp[0]);add(s.bp[0x40]);add(s.bp[0x41]);
    for(unsigned m=0;m<8;++m){
        if(!s.tex_id[m]){add(0);continue;}
        add(1);
        unsigned offset=(m&3)+(m>=4?0x20:0);
        add(s.bp[0x88+offset]);
        if(!s.tex_is_efb[m])add(s.bp[0x94+offset]?s.bp[0x94+offset]:s.tex_id[m]);
    }
    unsigned n=std::min(c.vcount,8u);
    for(unsigned j=0;j<n;++j)add(b.verts[c.vfirst+(uint64_t(j)*(c.vcount-1))/std::max(n-1,1u)].src_idx);
    return h;
}
struct Inverse {
    double a[9]{},t[3]{};bool valid=false;
    explicit Inverse(const float* m){
        if(!m)return;
        double d=double(m[0])*(double(m[5])*m[10]-double(m[6])*m[9])-double(m[1])*(double(m[4])*m[10]-double(m[6])*m[8])+double(m[2])*(double(m[4])*m[9]-double(m[5])*m[8]);
        if(!std::isfinite(d)||std::fabs(d)<1e-12)return;
        double v[]={double(m[5])*m[10]-double(m[6])*m[9],double(m[2])*m[9]-double(m[1])*m[10],double(m[1])*m[6]-double(m[2])*m[5],double(m[6])*m[8]-double(m[4])*m[10],double(m[0])*m[10]-double(m[2])*m[8],double(m[2])*m[4]-double(m[0])*m[6],double(m[4])*m[9]-double(m[5])*m[8],double(m[1])*m[8]-double(m[0])*m[9],double(m[0])*m[5]-double(m[1])*m[4]};
        for(int i=0;i<9;++i)a[i]=v[i]/d;
        for(int i=0;i<3;++i)t[i]=m[i*4+3];valid=true;
    }
    void local(const float* p,double* out)const{for(int r=0;r<3;++r){out[r]=0;for(int k=0;k<3;++k)out[r]+=a[r*3+k]*(p[k]-t[k]);}}
};
// Composite and translucent draws depend on coverage and framebuffer
// contents from the native tick. Keep their vertices at that tick.
inline bool can_interpolate(const gx::PixelState& s){
    const uint32_t blend=s.bp[0x41], depth=s.bp[0x40];
    if(s.proj[6]!=0 || s.view_space || (blend&0x803) || (depth&0x11)!=0x11)return false;
    for(unsigned unit=0;unit<8;++unit)
        if(s.tex_id[unit] && s.tex_is_efb[unit])return false;
    return true;
}
struct History {
    gx::Batch previous;
    std::unordered_map<uint64_t,std::vector<size_t>> byKey;
    History(){previous.pooled=true;}
    void clear(){byKey.clear();previous.cmds.clear();previous.states.clear();previous.verts.clear();previous.indices.clear();previous.mtxs.clear();}
    bool empty()const{return byKey.empty();}
    void remember(const gx::Batch& b){
        previous.cmds=b.cmds;previous.states=b.states;previous.verts=b.verts;previous.indices=b.indices;previous.mtxs=b.mtxs;
        byKey.clear();
        for(size_t i=0;i<previous.cmds.size();++i){const auto& c=previous.cmds[i];if(c.type==gx::CmdType::Draw && c.state<previous.states.size() && can_interpolate(previous.states[c.state]) && c.vcount && size_t(c.vfirst)+c.vcount<=previous.verts.size())byKey[draw_key(previous,c)].push_back(i);}
    }
    float distance(const gx::Batch& b,const gx::Cmd& c,const gx::Cmd& a)const{
        if(c.count!=a.count || size_t(c.first)+c.count>b.indices.size() || size_t(a.first)+a.count>previous.indices.size())return INFINITY;
        for(uint32_t j=0;j<c.count;++j)if(previous.indices[a.first+j]-a.vfirst!=b.indices[c.first+j]-c.vfirst)return INFINITY;
        const float* cm=size_t(c.mtx)*12+12<=b.mtxs.size()?&b.mtxs[size_t(c.mtx)*12]:nullptr;
        const float* am=size_t(a.mtx)*12+12<=previous.mtxs.size()?&previous.mtxs[size_t(a.mtx)*12]:nullptr;
        Inverse ci(cm),ai(am);
        if(ci.valid!=ai.valid)return INFINITY;
        if(ci.valid){
            double alignment=0;
            for(int r=0;r<3;++r){double dot=0,x=0,y=0;for(int k=0;k<3;++k){dot+=double(cm[r*4+k])*am[r*4+k];x+=double(cm[r*4+k])*cm[r*4+k];y+=double(am[r*4+k])*am[r*4+k];}if(x*y<1e-12)return INFINITY;alignment+=dot/std::sqrt(x*y);}
            if(alignment/3<.75)return INFINITY;
        }
        unsigned samples=std::min(c.vcount,8u);double center[3]{},oldCenter[3]{};
        for(uint32_t j=0;j<c.vcount;++j)if(b.verts[c.vfirst+j].src_idx!=previous.verts[a.vfirst+j].src_idx)return INFINITY;
        for(unsigned j=0;j<samples;++j){
            unsigned n=(uint64_t(j)*(c.vcount-1))/std::max(samples-1,1u);
            const auto& v=b.verts[c.vfirst+n];const auto& old=previous.verts[a.vfirst+n];
            for(int k=0;k<3;++k){if(!std::isfinite(v.pos[k])||!std::isfinite(old.pos[k]))return INFINITY;center[k]+=v.pos[k]/samples;oldCenter[k]+=old.pos[k]/samples;}
            if(ci.valid){double x[3],y[3];ci.local(v.pos,x);ai.local(old.pos,y);for(int k=0;k<3;++k)if(std::fabs(x[k]-y[k])>std::min(v.src_idx==UINT32_MAX?.02:INFINITY,.001*(1+std::max(std::fabs(x[k]),std::fabs(y[k])))))return INFINITY;}
            else if(std::memcmp(v.tex,old.tex,gx::draw_ntex(b.states[c.state])*12))return INFINITY;
        }
        double score=0,scale=std::max(1.,std::max(std::fabs(center[2]),std::fabs(oldCenter[2])));
        for(int k=0;k<3;++k)score+=std::pow((center[k]-oldCenter[k])/scale,2);
        return score<.36?float(score):INFINITY;
    }
    size_t midpoint(gx::Batch& b)const{
        size_t changed=0;std::vector<uint8_t> used(previous.cmds.size());
        for(auto& c:b.cmds){
            if(c.type!=gx::CmdType::Draw || !c.vcount || c.state>=b.states.size() || !can_interpolate(b.states[c.state]) || size_t(c.vfirst)+c.vcount>b.verts.size())continue;
            auto bucket=byKey.find(draw_key(b,c));if(bucket==byKey.end())continue;
            size_t best=SIZE_MAX;float first=INFINITY,second=INFINITY;
            for(size_t index:bucket->second){if(used[index])continue;float d=distance(b,c,previous.cmds[index]);if(d<first){second=first;first=d;best=index;}else second=std::min(second,d);}
            if(best==SIZE_MAX || (std::isfinite(second)&&second<first*1.5f+.00001f))continue;
            size_t stride=gx::packed_stride(gx::draw_ntex(b.states[c.state]));bool bounds=true,moved=false;
            for(uint32_t j=0;j<c.vcount;++j){int64_t at=int64_t(c.vfirst)+j+c.base_vertex;if(at<0||size_t(at)*stride+12>b.packed.size()){bounds=false;break;}}
            if(!bounds)continue;used[best]=1;const auto& a=previous.cmds[best];
            for(uint32_t j=0;j<c.vcount;++j){auto& v=b.verts[c.vfirst+j];const auto& old=previous.verts[a.vfirst+j];for(int k=0;k<3;++k){moved|=v.pos[k]!=old.pos[k];v.pos[k]=(old.pos[k]+v.pos[k])*.5f;}std::memcpy(b.packed.data()+(int64_t(c.vfirst)+j+c.base_vertex)*stride,v.pos,12);}
            changed+=moved;
        }
        return changed;
    }
};
}
