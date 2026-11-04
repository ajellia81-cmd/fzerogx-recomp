#pragma once
#include <filesystem>
#include <fstream>
#include <map>
namespace ios_audit {
template<class T> std::vector<T> read(std::ifstream& in){uint64_t n=0;in.read((char*)&n,8);assert(in && n<100000000);std::vector<T> v(n);in.read((char*)v.data(),sizeof(T)*n);assert(in);return v;}
inline int captures(int argc,char**argv){
 unsigned frames=0;unsigned long long draws=0,ortho=0,changed=0,protectedDraws=0;std::map<unsigned,unsigned> owners,formats;
 for(int arg=1;arg<argc;++arg)for(const auto& file:std::filesystem::recursive_directory_iterator(argv[arg]))if(file.path().extension()==".capture"){
  std::ifstream in(file.path(),std::ios::binary);uint32_t h[8];in.read((char*)h,sizeof h);assert(in && h[0]==0x36585A46 && h[1]==1 && h[2]==sizeof(gx::Cmd) && h[3]==sizeof(gx::GpuVertex) && h[4]==sizeof(gx::PixelState));
  gx::Batch b;b.cmds=read<gx::Cmd>(in);b.verts=read<gx::GpuVertex>(in);b.packed=read<uint8_t>(in);b.indices=read<uint32_t>(in);b.states=read<gx::PixelState>(in);b.mtxs=read<float>(in);
  uint64_t n=0;in.read((char*)&n,8);assert(in && n<100000);
  for(uint64_t i=0;i<n;++i){uint32_t meta[4];uint64_t hash;in.read((char*)meta,sizeof meta);in.read((char*)&hash,8);assert(in);auto t=std::make_shared<gx::TexData>();t->id=meta[0];t->width=meta[1];t->height=meta[2];t->hash=hash;b.new_textures.push_back(t);for(unsigned l=0;l<meta[3];++l){uint64_t pixels=0;in.read((char*)&pixels,8);assert(in && pixels<100000000);in.seekg(pixels*4,std::ios::cur);assert(in);}}
  for(auto& s:b.states)s.tag=ios_context::encode(h[6],int32_t(h[7]),false);
  const auto commands=b.cmds;const auto states=b.states;const auto verts=b.verts;const auto packed=b.packed;const auto matrices=b.mtxs;
  gx::g_fullscreen_tex.clear();gx::g_ios_canvas_source=4.f/3;
  auto mode=ios_scene::classify(b,false,h[6],int32_t(h[7]));
  if(ios_ui::startup_art(b,[&](uint32_t id){for(auto& t:b.new_textures)if(t->id==id)return t->hash;return uint64_t(0);}))mode=ios_scene::Mode::Flat;
  gx::g_ios_scene=mode;std::vector<uint8_t> skip;gx::mark_offscreen_passes(b,skip);
  gx::ios_widen(b,2868,1320);
  assert(b.cmds.size()==commands.size());assert(b.verts.size()==verts.size());assert(!memcmp(b.verts.data(),verts.data(),verts.size()*sizeof(gx::GpuVertex)));assert(b.packed==packed && b.mtxs==matrices);
  for(size_t i=0;i<b.cmds.size();++i){const auto& c=b.cmds[i];const auto& old=commands[i];
   if(c.type!=gx::CmdType::Draw){assert(!memcmp(&c,&old,sizeof c));continue;}
   assert(c.count==old.count && c.vfirst==old.vfirst && c.vcount==old.vcount && c.state<b.states.size());if(!c.count || old.state>=states.size())continue;
   ++draws;const auto& before=states[old.state];const auto& after=b.states[c.state];
   bool protectedPath=skip[i] || mode==ios_scene::Mode::Flat || mode==ios_scene::Mode::Movie || ios_scene::movie_state(before) || (before.proj[6] && ios_canvas::feedback(before,[](uint32_t id){return gx::is_fullscreen_tex(id);})) || ios_canvas::solid_cover(b,old,before) || ios_canvas::screen_effect(b,old,before) || ios_canvas::edge(b,old,before)==ios_canvas::Edge::Horizontal;
   if(protectedPath){assert(!memcmp(&before,&after,sizeof before));++protectedDraws;}
   if(before.proj[6]){
    ++ortho;changed+=c.state!=old.state;
    for(unsigned m=0,used=ios_scene::sampled_maps(before);m<8;++m)if((used&(1u<<m)) && before.tex_id[m]){unsigned t=before.bp[0x88+(m&3)+(m>=4?32:0)];++formats[(t>>20)&15];}
    if(!protectedPath && ios_canvas::edge(b,old,before)==ios_canvas::Edge::None){
     // A native UI element may move only through the shared viewport map:
     // never through individually edited geometry/UVs or per-word anchors.
     assert(!memcmp(before.proj,after.proj,sizeof before.proj));
     assert(before.viewport[1]==after.viewport[1] && before.viewport[4]==after.viewport[4]);
     assert(((after.bp[0x20]>>12)&2047)<=((after.bp[0x21]>>12)&2047));
    }
   }
   for(float f:after.proj)assert(std::isfinite(f));for(float f:after.viewport)assert(std::isfinite(f));
  }
  ++frames;++owners[h[6]];
 }
 fprintf(stderr,"Presentation audit: frames=%u draws=%llu orthographic=%llu corrected_ortho=%llu protected=%llu\n",frames,draws,ortho,changed,protectedDraws);
 for(auto [m,n]:owners)fprintf(stderr,"Presentation owner mask=%u frames=%u\n",m,n);
 for(auto [f,n]:formats)fprintf(stderr,"Presentation sampled format=%u draws=%u\n",f,n);
 return 0;
}
}
