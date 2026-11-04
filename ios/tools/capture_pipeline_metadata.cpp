// Only renderer configuration metadata is emitted. Decoded assets stay private.
#include "gx/shadergen.h"
#include "scene_classify.h"
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_set>
#include <vector>
#include <cassert>
// Standalone diagnostic batches own their vectors; no runtime pool is linked.
gx::Batch::~Batch()=default;
using namespace gx;
template<class T> std::vector<T> readVector(std::ifstream& in){uint64_t n=0;in.read((char*)&n,8);assert(in && n<10000000);std::vector<T> v(n);in.read((char*)v.data(),sizeof(T)*n);assert(in);return v;}
template<class T> void skipVector(std::ifstream& in){uint64_t n=0;in.read((char*)&n,8);assert(in && n<100000000);in.seekg(sizeof(T)*n,std::ios::cur);assert(in);}
int main(int argc,char**argv){
 assert(argc>=5);std::ifstream input(argv[1],std::ios::binary);uint32_t header[2];input.read((char*)header,8);assert(input && header[0]==kShaderKeysMagic && header[1]==kShaderKeyFormat);
 std::vector<ShaderKey> keys;std::unordered_set<ShaderKey,ShaderKeyHash> have;ShaderKey key;
 while(input.read((char*)&key,sizeof key)){if(have.insert(key).second)keys.push_back(key);}
 auto initial=keys.size();std::set<std::pair<uint64_t,uint32_t>> recipes;unsigned frames=0;
 for(int i=4;i<argc;++i)for(auto& file:std::filesystem::recursive_directory_iterator(argv[i]))if(file.path().extension()==".capture"){
  std::ifstream in(file.path(),std::ios::binary);uint32_t h[8];in.read((char*)h,sizeof h);
  assert(in && h[0]==0x36585A46 && h[1]==1 && h[2]==sizeof(Cmd) && h[3]==sizeof(GpuVertex) && h[4]==sizeof(PixelState));
  auto cmds=readVector<Cmd>(in);auto vertices=readVector<GpuVertex>(in);skipVector<uint8_t>(in);skipVector<uint32_t>(in);auto states=readVector<PixelState>(in);
  auto batch=std::make_unique<Batch>();batch->cmds=cmds;batch->states=states;batch->verts=std::move(vertices);
  auto mode=ios_scene::classify(*batch,false,h[6],int32_t(h[7]));
  fprintf(stderr,"Private capture policy: %s modules=%u phase=%d mode=%u\n",file.path().filename().string().c_str(),h[6],int32_t(h[7]),unsigned(mode));
  if(h[6]&(ios_guest::MenuModules|ios_guest::Replay))assert(mode==ios_scene::Mode::Menu || mode==ios_scene::Mode::Movie);
  for(const auto& cmd:cmds)if(cmd.type==CmdType::Draw && cmd.count && cmd.vcount && cmd.state<states.size()){
   const auto& state=states[cmd.state];auto k=make_shader_key(state);k.fg_depth=0;if(k.dst_alpha==2)k.dst_alpha=0;canonicalize_shader_key(k);
   if(have.insert(k).second)keys.push_back(k);
   recipes.emplace(ShaderKeyHash{}(k),state.bp[0x41]);
  }++frames;
 }
 std::ofstream out(argv[2],std::ios::binary);out.write((char*)header,8);for(auto& k:keys)out.write((char*)&k,sizeof k);assert(out);
 std::ofstream inc(argv[3]);inc<<"// Renderer state metadata collected privately; no textures or geometry.\n";for(auto [hash,blend]:recipes)inc<<"{0x"<<std::hex<<hash<<"ull,0x"<<blend<<"u},\n";assert(inc);
 fprintf(stderr,"Pipeline metadata: %u captures, %zu base keys, %zu added keys, %zu observed states\n",frames,initial,keys.size()-initial,recipes.size());
}
