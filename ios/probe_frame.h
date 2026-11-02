#pragma once
// Headless validation of the iOS scene/motion inputs. No pixels, game data,
// vertex coordinates or generated code are written to diagnostics.
#include "scene_classify.h"
#include "motion_history.h"
#include "runtime.h"
#include <cstdio>
namespace ios_probe {
inline ios_motion::History history;
inline unsigned long long frames=0,world=0,movie=0,menu=0,movingFrames=0,eligibleMoving=0,movingDraws=0,partial=0,multi=0;
inline void observe(gx::Batch& b){
    ++frames;
    auto mode=ios_scene::classify(b);
    if(mode==ios_scene::Mode::Movie)++movie;
    if(mode==ios_scene::Mode::Menu)++menu;
    if(mode==ios_scene::Mode::World){
        ++world;
        size_t presents=0;bool strip=false;
        for(const auto& c:b.cmds)if(c.type==gx::CmdType::Present){++presents;strip|=vi_display_height() && c.copy.dst_h<vi_display_height();}
        partial+=strip;multi+=presents!=1;
        auto original=b.verts;auto packed=b.packed;
        size_t moving=history.midpoint(b);
        if(moving){++movingFrames;eligibleMoving+=!strip && presents==1;movingDraws+=moving;}
        b.verts.swap(original);b.packed.swap(packed);
        history.remember(b);
    }else history.clear();
    if(frames%600==0){
        fprintf(stderr,"iOS input probe: batches=%llu world=%llu menu=%llu movie=%llu moving_frames=%llu moving_draws=%llu partial=%llu multi=%llu eligible_moving=%llu\n",
                frames,world,menu,movie,movingFrames,movingDraws,partial,multi,eligibleMoving);
        fflush(stderr);
    }
}
inline int finish(){
    fprintf(stderr,"iOS input probe FINAL: batches=%llu world=%llu menu=%llu movie=%llu moving_frames=%llu moving_draws=%llu partial=%llu multi=%llu eligible_moving=%llu\n",
            frames,world,menu,movie,movingFrames,movingDraws,partial,multi,eligibleMoving);
    fprintf(stderr,"Headless input validation only; no iOS execution, pixels, Metal or display FPS measured.\n");
    return world && eligibleMoving?0:2;
}
}
