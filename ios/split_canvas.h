#pragma once
#include "canvas_policy.h"
#include <numeric>
namespace ios_split {
struct Rect {float l,t,r,b;};
inline float area(Rect a){return std::max(0.f,a.r-a.l)*std::max(0.f,a.b-a.t);}
inline Rect intersect(Rect a,Rect b){return {std::max(a.l,b.l),std::max(a.t,b.t),std::min(a.r,b.r),std::min(a.b,b.b)};}
inline bool contains(Rect a,Rect b,float e=1.5f){return b.l>=a.l-e && b.t>=a.t-e && b.r<=a.r+e && b.b<=a.b+e;}
inline bool same(Rect a,Rect b){return std::fabs(a.l-b.l)<2 && std::fabs(a.t-b.t)<2 && std::fabs(a.r-b.r)<2 && std::fabs(a.b-b.b)<2;}
inline Rect clip(const gx::PixelState& s){
    int x=int(s.bp[0x59]&1023)*2,y=int((s.bp[0x59]>>10)&1023)*2;
    return {float(int(s.bp[0x20]>>12&2047)-x),float(int(s.bp[0x20]&2047)-y),float(int(s.bp[0x21]>>12&2047)-x+1),float(int(s.bp[0x21]&2047)-y+1)};
}
inline Rect view(const gx::PixelState& s){
    float w=std::fabs(s.viewport[0]),h=std::fabs(s.viewport[1]);
    float x=s.viewport[3]-int(s.bp[0x59]&1023)*2,y=s.viewport[4]-int((s.bp[0x59]>>10)&1023)*2;
    return intersect({x-w,y-h,x+w,y+h},clip(s));
}
inline bool bounds(const gx::Batch& b,const gx::Cmd& c,const gx::PixelState& s,Rect& r){
    float a[4];if(!s.proj[6] || !ios_scene::draw_bounds(b,c,s,a))return false;
    float ox=int(s.bp[0x59]&1023)*2,oy=int((s.bp[0x59]>>10)&1023)*2;
    float x0=s.viewport[0]*a[0]+s.viewport[3]-ox,x1=s.viewport[0]*a[2]+s.viewport[3]-ox;
    float y0=s.viewport[1]*a[1]+s.viewport[4]-oy,y1=s.viewport[1]*a[3]+s.viewport[4]-oy;
    r=intersect({std::min(x0,x1),std::min(y0,y1),std::max(x0,x1),std::max(y0,y1)},clip(s));
    return area(r)>0;
}
struct Layout {
    std::vector<Rect> cells;
    std::vector<int> owner;
    bool active()const{return cells.size()>1;}
    float anchor(size_t i,float fallback)const{return i<owner.size() && owner[i]>=0?(cells[owner[i]].l+cells[owner[i]].r)*.5f:fallback;}
};
inline Layout layout(const gx::Batch& b,const std::vector<uint8_t>& skip){
    Layout result;result.owner.assign(b.cmds.size(),-1);
    const auto f=ios_canvas::frame(b);Rect full{0,0,f.width,f.height};
    bool wholeCamera=false;
    auto aligned=[](float x,float extent){return std::fabs(x/extent*2-std::round(x/extent*2))<.015f;};
    for(size_t i=0;i<b.cmds.size();++i){const auto& c=b.cmds[i];
        if(c.type!=gx::CmdType::Draw || !c.count || c.vcount<16 || c.state>=b.states.size() || (i<skip.size()&&skip[i]))continue;
        const auto& s=b.states[c.state];if(s.proj[6] || s.view_space)continue;
        auto r=intersect(view(s),full);float a=area(r)/area(full);
        if(a>.98f){wholeCamera=true;break;}
        if(a<.20f || a>.76f || !aligned(r.l,f.width) || !aligned(r.r,f.width) || !aligned(r.t,f.height) || !aligned(r.b,f.height))continue;
        if(std::none_of(result.cells.begin(),result.cells.end(),[&](Rect x){return same(r,x);}))result.cells.push_back(r);
    }
    if(wholeCamera || result.cells.size()<2 || result.cells.size()>4){result.cells.clear();return result;}
    float covered=0;for(size_t i=0;i<result.cells.size();++i){covered+=area(result.cells[i]);for(size_t j=0;j<i;++j)if(area(intersect(result.cells[i],result.cells[j]))>4){result.cells.clear();return result;}}
    if(covered<area(full)*.49f){result.cells.clear();return result;}
    // 3D HUD/countdown cameras keep the same local canvas as their player.
    for(size_t i=0;i<b.cmds.size();++i){const auto& c=b.cmds[i];if(c.type!=gx::CmdType::Draw || c.state>=b.states.size())continue;
        const auto& s=b.states[c.state];if(s.proj[6])continue;auto r=intersect(view(s),full);
        for(size_t j=0;j<result.cells.size();++j)if(same(r,result.cells[j]))result.owner[i]=int(j);
    }
    struct Item {size_t draw;Rect box;unsigned pass;};
    std::vector<Item> items;unsigned pass=0;
    for(size_t i=0;i<b.cmds.size();++i){const auto& c=b.cmds[i];
        if(c.type==gx::CmdType::EfbCopy || c.type==gx::CmdType::Present){++pass;continue;}
        if(c.type!=gx::CmdType::Draw || !c.count || c.state>=b.states.size() || (i<skip.size()&&skip[i]))continue;
        const auto& s=b.states[c.state];Rect r;if(!bounds(b,c,s,r) || area(r)>area(full)*.8f)continue;
        items.push_back({i,r,pass});
    }
    std::vector<size_t> parent(items.size());std::iota(parent.begin(),parent.end(),0);
    auto root=[&](size_t a){while(parent[a]!=a){parent[a]=parent[parent[a]];a=parent[a];}return a;};
    auto join=[&](size_t a,size_t z){a=root(a);z=root(z);if(a!=z)parent[z]=a;};
    auto glyphs=ios_ui::glyph_lines(b,skip,{});
    for(size_t i=0;i<items.size();++i)for(size_t j=0;j<i;++j){
        const auto& a=items[i];const auto& z=items[j];if(a.pass!=z.pass)continue;
        bool touching=a.box.l<=z.box.r+1.5f && a.box.r>=z.box.l-1.5f && a.box.t<=z.box.b+1.5f && a.box.b>=z.box.t-1.5f;
        const auto& x=b.states[b.cmds[a.draw].state];const auto& y=b.states[b.cmds[z.draw].state];
        bool word=glyphs[a.draw].fixed && glyphs[z.draw].fixed && std::fabs(glyphs[a.draw].anchor-glyphs[z.draw].anchor)<1e-6f &&
            std::fabs(a.box.t-z.box.t)<2 && std::fabs(a.box.b-z.box.b)<2 &&
            !memcmp(x.proj,y.proj,sizeof(x.proj)) && !memcmp(x.viewport,y.viewport,sizeof(x.viewport)) && x.bp[0x20]==y.bp[0x20] && x.bp[0x21]==y.bp[0x21];
        if(touching || word)join(i,j);
    }
    std::vector<Rect> boxes(items.size(),{INFINITY,INFINITY,-INFINITY,-INFINITY});
    for(size_t i=0;i<items.size();++i){auto& r=boxes[root(i)];const auto& a=items[i].box;r={std::min(r.l,a.l),std::min(r.t,a.t),std::max(r.r,a.r),std::max(r.b,a.b)};}
    for(size_t i=0;i<items.size();++i){auto r=boxes[root(i)];for(size_t j=0;j<result.cells.size();++j)if(contains(result.cells[j],r)){result.owner[items[i].draw]=int(j);break;}}
    return result;
}
}
