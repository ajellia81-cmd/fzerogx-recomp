#pragma once
// Headless validation of the iOS scene/motion inputs. No pixels, game data,
// vertex coordinates or generated code are written to diagnostics.
#include "scene_classify.h"
#include "motion_history.h"
#include "runtime.h"
#include "guest_state.h"
#include "frame_capture.h"
#include <cstdio>
namespace ios_probe {
inline ios_motion::History history;
inline unsigned long long frames=0,world=0,movie=0,menu=0,movingFrames=0,eligibleMoving=0,movingDraws=0,partial=0,multi=0,menuMasks=0;
inline void observe(gx::Batch& b){
    ++frames;
    static int lastPhase=-999;static unsigned lastModules=~0u;
    int phase=ios_guest::phase.load();unsigned modules=ios_guest::modules.load();
    if(phase!=lastPhase || modules!=lastModules){
        fprintf(stderr,"iOS scene transition: batch=%llu phase=%d modules=%u\n",frames,phase,modules);
        lastPhase=phase;lastModules=modules;
    }
    unsigned nativeDraws=0;
    for(const auto& draw:b.cmds)if(draw.type==gx::CmdType::Draw && draw.state<b.states.size() && (b.states[draw.state].tag&0x40))++nativeDraws;
    if(nativeDraws)fprintf(stderr,"Native card canvas rendered: frame=%llu draws=%u\n",frames,nativeDraws);
    ios_capture::observe(b,unsigned(frames),modules,phase);
    auto mode=ios_scene::classify(b,false,modules,phase);
    if(mode==ios_scene::Mode::Menu){
        for(const auto& c:b.cmds)if(c.type==gx::CmdType::Draw && c.state<b.states.size()){
            const auto& st=b.states[c.state];float box[4];
            if(st.proj[6]!=0 && ios_scene::draw_bounds(b,c,st,box) && ios_scene::black_side_mask(b,c,st,box))++menuMasks;
        }
    }
    if(mode==ios_scene::Mode::Movie)++movie;
    if(mode==ios_scene::Mode::Menu || mode==ios_scene::Mode::Flat)++menu;
    if(mode==ios_scene::Mode::World)++world;
    if(mode!=ios_scene::Mode::Movie){
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
        fprintf(stderr,"iOS input probe: batches=%llu world=%llu menu=%llu movie=%llu moving_frames=%llu moving_draws=%llu partial=%llu multi=%llu eligible_moving=%llu menu_masks=%llu\n",
                frames,world,menu,movie,movingFrames,movingDraws,partial,multi,eligibleMoving,menuMasks);
        fflush(stderr);
    }
}
inline int finish(){
    fprintf(stderr,"iOS input probe FINAL: batches=%llu world=%llu menu=%llu movie=%llu moving_frames=%llu moving_draws=%llu partial=%llu multi=%llu eligible_moving=%llu menu_masks=%llu\n",
            frames,world,menu,movie,movingFrames,movingDraws,partial,multi,eligibleMoving,menuMasks);
    fprintf(stderr,"Headless input validation only; no iOS execution, pixels, Metal or display FPS measured.\n");
    if(getenv("FZGX_MENU_PROBE"))return frames>=4500 && menu?0:2;
    return world && eligibleMoving?0:2;
}
}
