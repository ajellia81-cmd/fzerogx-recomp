#pragma once
#include "gx/render.h"
#include <algorithm>
#include <cmath>
#include <utility>
namespace ios_gpu_work {
// Render at the actual presented content extent. Letterbox pixels do not
// increase the image's resolution and must not inflate the offscreen target.
inline std::pair<int,int> content_extent(int w,int h,bool wide,float aspect){
    if(wide || w<=0 || h<=0 || !std::isfinite(aspect) || aspect<=0)return {w,h};
    int vh=int(w/aspect);
    if(vh>h)return {std::min(w,int(h*aspect)),h};
    return {w,vh};
}
struct FrameStart {
    gx::EfbCopyCmd clear{};
    bool valid=false;
    void reset(){valid=false;}
    bool reusable(const gx::Batch& b)const{
        if(!valid)return false;
        unsigned presents=0;
        for(const auto& c:b.cmds){
            // Independent copies can observe pixels outside the display rect.
            if(c.type==gx::CmdType::EfbCopy && !c.copy.to_xfb)return false;
            if(c.type!=gx::CmdType::Present)continue;
            ++presents;const auto& p=c.copy;
            if(!p.src_w || !p.src_h || p.src_x<clear.src_x || p.src_y<clear.src_y ||
               uint64_t(p.src_x)+p.src_w>uint64_t(clear.src_x)+clear.src_w ||
               uint64_t(p.src_y)+p.src_h>uint64_t(clear.src_y)+clear.src_h)return false;
        }
        return presents==1;
    }
    void remember(const gx::Batch& b){
        valid=false;
        for(auto i=b.cmds.rbegin();i!=b.cmds.rend();++i){
            if(i->type==gx::CmdType::Draw && i->count)return;
            if(i->type!=gx::CmdType::EfbCopy)continue;
            clear=i->copy;
            valid=clear.to_xfb && clear.clear && clear.clear_color && clear.clear_alpha && clear.clear_z && clear.src_w && clear.src_h;
            return;
        }
    }
};
}
