#pragma once
// Private diagnostic format for already-decoded renderer inputs. Never commit
// captures, distribute them unencrypted, or use this to decode disc contents.
#include "gx/render.h"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <type_traits>
namespace ios_capture {
template<class T> inline void vector(FILE* f,const std::vector<T>& v){
    static_assert(std::is_trivially_copyable_v<T>);
    uint64_t n=v.size();fwrite(&n,8,1,f);fwrite(v.data(),sizeof(T),v.size(),f);
}
inline void observe(const gx::Batch& b,unsigned frame,unsigned modules,int phase){
    if(!getenv("FZGX_PRIVATE_CAPTURE"))return;
    static std::map<uint32_t,std::shared_ptr<gx::TexData>> live;
    for(auto id:b.dead_textures)live.erase(id);
    for(auto& t:b.new_textures)live[t->id]=t;
    const unsigned selected[]={30,60,120,180,240,360,480,600,1000,1900,2300,2700,3100,3500,4000,4500,5400,5600,5900,6100,6300,6500,6800,7200,7800,8400};
    bool capture=false;
    if(const char* list=getenv("FZGX_CAPTURE_FRAMES")){
        for(const char* at=list;*at;){char* end=nullptr;unsigned long value=strtoul(at,&end,10);if(end==at)break;capture|=frame==value;at=*end==','?end+1:end;}
    }else for(auto f:selected)capture|=frame==f;
    if(!capture)return;
    char path[128];snprintf(path,sizeof path,"frame-%u.capture",frame);
    FILE* f=fopen(path,"wb");if(!f)std::abort();
    uint32_t header[]={0x36585A46,2,sizeof(gx::Cmd),sizeof(gx::GpuVertex),sizeof(gx::PixelState),frame,modules,uint32_t(phase)};
    fwrite(header,sizeof header,1,f);
    vector(f,b.cmds);vector(f,b.verts);vector(f,b.packed);vector(f,b.indices);vector(f,b.states);vector(f,b.mtxs);
    uint64_t n=live.size();fwrite(&n,8,1,f);
    const bool pixels=!getenv("FZGX_CAPTURE_METADATA") || frame==4500 || frame==5400;
    for(auto& [id,t]:live){uint32_t h[]={id,t->width,t->height,pixels?uint32_t(t->levels.size()):0u};fwrite(h,sizeof h,1,f);fwrite(&t->hash,8,1,f);if(pixels)for(auto& l:t->levels)vector(f,l);}
    fclose(f);fprintf(stderr,"Private renderer capture: frame=%u modules=%u phase=%d textures=%zu\n",frame,modules,phase,live.size());
}
}


