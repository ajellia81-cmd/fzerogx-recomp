// Original synthetic renderer regression fixtures; no game inputs.
#include <cassert>
#include <cmath>
#include "render_ios.cpp"
void fatal(const char*,...){std::abort();}
int main(){
 gx::ShaderKey cacheKey{};cacheKey.dst_alpha=2;gx::g_dual_source=false;gx::ios_shader_key(cacheKey);assert(cacheKey.dst_alpha==0);
 cacheKey.dst_alpha=2;gx::g_dual_source=true;gx::ios_shader_key(cacheKey);assert(cacheKey.dst_alpha==2);gx::g_dual_source=false;

 auto*b=new gx::Batch;gx::PixelState s{};s.proj[6]=1;s.proj[0]=2.f/640;s.proj[1]=-1;s.proj[2]=2.f/480;s.proj[3]=-1;s.viewport[0]=320;s.viewport[1]=240;s.viewport[3]=1002;s.viewport[4]=922;s.bp[0x59]=342|(342<<10);s.bp[0x20]=682|(682<<12);s.bp[0x21]=1161|(1321<<12);s.tex_id[0]=1;
 gx::Cmd d{};d.type=gx::CmdType::Draw;d.vcount=d.count=4;b->cmds={d};b->states={s};b->verts.resize(4);
 for(int i=0;i<4;++i){auto&v=b->verts[i];v.pos[0]=320+((i&1)?37.5f:-37.5f);v.pos[1]=240+((i&2)?50.f:-50.f);}
 ios_guest::modules=ios_guest::Selection;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);
 auto&t=b->states[b->cmds[0].state];assert(t.proj[0]==s.proj[0]&&t.bp[0x20]==s.bp[0x20]&&t.bp[0x21]==s.bp[0x21]);float box[4];ios_scene::draw_bounds(*b,b->cmds[0],t,box);assert(fabs((box[2]-box[0])*16.f/9-(box[3]-box[1]))<1e-5);
 b->states={s};b->cmds={d};ios_guest::modules=ios_guest::Replay;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);ios_scene::draw_bounds(*b,b->cmds[0],b->states[b->cmds[0].state],box);assert(fabs((box[2]-box[0])*2868/1320-(box[3]-box[1]))<1e-5);
 // Presentation context must belong to the queued batch, not newer guest state.
 for(unsigned module:{unsigned(ios_guest::Title),unsigned(ios_guest::Selection),unsigned(ios_guest::Movie)}){
  b->states={s};b->cmds={d};b->states[0].tag=ios_context::encode(module,41,false);
  auto context=ios_context::read(*b);assert(context.stamped && context.modules==module);
 }
 b->states={s};b->cmds={d};b->states[0].tag=ios_context::encode(ios_guest::Title,0,false);
 ios_guest::modules=ios_guest::Movie;gx::ios_prepare_scene(*b,false);
 assert(gx::g_ios_scene==ios_scene::Mode::Flat);
 // Known startup art wins even when legacy metadata points at a newer menu.
 b->states={s};b->cmds={d};
 assert(ios_ui::startup_art(*b,[](uint32_t){return uint64_t(0x4c8a45da63b18ea5ull);}));
 assert(!ios_ui::startup_art(*b,[](uint32_t){return uint64_t(123);}));
 b->states.push_back(s);b->states[1].proj[6]=0;b->cmds.push_back(d);b->cmds[1].state=1;b->cmds[1].count=0;
 assert(ios_ui::startup_art(*b,[](uint32_t){return uint64_t(0x4c8a45da63b18ea5ull);}));
 b->cmds[1].count=4;assert(!ios_ui::startup_art(*b,[](uint32_t){return uint64_t(0x4c8a45da63b18ea5ull);}));
 // Menu composition must retain perspective layers, copies and scissors too.
 b->states={s,s};b->states[0].proj[6]=0;b->states[0].proj[0]=1;b->states[0].proj[2]=4.f/3;b->cmds={d,d};b->cmds[1].state=1;
 gx::Cmd copy{};copy.type=gx::CmdType::EfbCopy;copy.copy.src_w=copy.copy.dst_w=640;copy.copy.src_h=copy.copy.dst_h=480;copy.copy.tex_id=42;copy.copy.clear=true;b->cmds.push_back(copy);
 auto states=b->states;auto commands=b->cmds;ios_guest::modules=ios_guest::Selection;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);
 assert(b->states.size()==states.size() && !memcmp(b->states.data(),states.data(),states.size()*sizeof(gx::PixelState)));
 assert(!memcmp(b->cmds.data(),commands.data(),commands.size()*sizeof(gx::Cmd)));
 gx::PixelState movie=s;movie.tex_id[1]=2;movie.bp[0x88]=(1<<20)|(639)|(335<<10);movie.bp[0x89]=(3<<20)|(319)|(167<<10);assert(ios_scene::movie_state(movie));b->states={movie};b->cmds={d};assert(ios_scene::classify(*b,false,ios_guest::Title,41)==ios_scene::Mode::Movie);
 movie.bp[0x89]=(1<<20)|(319)|(167<<10);assert(!ios_scene::movie_state(movie));movie.bp[0x89]=(3<<20)|(255)|(255<<10);assert(!ios_scene::movie_state(movie));
 // The same encoded movie keeps its aspect on either guest display canvas.
 movie=s;movie.tex_id[1]=2;movie.bp[0x88]=(1<<20)|639|(335<<10);movie.bp[0x89]=(3<<20)|319|(167<<10);
 b->states={movie};b->cmds={d};gx::Cmd present{};present.type=gx::CmdType::Present;present.copy.src_w=640;present.copy.src_h=480;b->cmds.push_back(present);
 for(int i=0;i<4;++i){b->verts[i].pos[0]=(i&1)?640:0;b->verts[i].pos[1]=(i&2)?408:72;}
 assert(fabs(ios_scene::movie_canvas_aspect(*b)-4.f/3)<1e-5);
 for(int i=0;i<4;++i)b->verts[i].pos[1]=(i&2)?464:16;
 assert(fabs(ios_scene::movie_canvas_aspect(*b)-16.f/9)<1e-5);
 // A paired menu frame takes precedence over stale Title/Story race metadata.
 // One HUD-like strip, or an offscreen texture, cannot trigger that override.
 b->states={s,s};b->cmds={d,d};b->cmds[1].state=1;
 b->states[0].bp[0x88]=(5<<20)|639|(43<<10);
 b->states[1].bp[0x88]=(5<<20)|639|(59<<10);
 for(int i=0;i<4;++i){b->verts[i].pos[0]=(i&1)?640:0;b->verts[i].pos[1]=(i&2)?60:0;}
 for(unsigned module:{unsigned(ios_guest::Title),unsigned(ios_guest::Story)}){
  assert(ios_scene::classify(*b,false,module,41)==ios_scene::Mode::Menu);
  assert(ios_scene::classify(*b,true,module,41)==ios_scene::Mode::Menu);
  assert(ios_scene::classify(*b,true,module|ios_guest::Movie,41)==ios_scene::Mode::Movie);
  b->states[1].tex_is_efb[0]=true;
  assert(ios_scene::classify(*b,false,module,41)!=ios_scene::Mode::Menu);
  b->states[1].tex_is_efb[0]=false;
 }
 // Standalone startup art is flat; a real Title camera retains fullscreen 3D.
 b->states={s};b->cmds={d};
 assert(ios_scene::classify(*b,false,ios_guest::Title,0)==ios_scene::Mode::Flat);
 assert(ios_scene::classify(*b,true,ios_guest::Title,41)==ios_scene::Mode::Flat);
 b->states[0].proj[6]=0;
 assert(ios_scene::classify(*b,false,ios_guest::Title,41)==ios_scene::Mode::World);
 // A modal's shadow, border, text and title stay together, while preceding HUD
 // and following unrelated text retain the compensated 16:9 canvas. The SAME
 // glyph PixelState appears on each side of that boundary (cache regression).
 gx::PixelState glyph=s,panel=s,hud=s,title=s;
 glyph.tex_id[0]=101;glyph.bp[0x88]=23|(23<<10);
 panel.tex_id[0]=102;panel.bp[0x88]=(5<<20)|51|(103<<10);
 hud.tex_id[0]=103;hud.bp[0x88]=(5<<20)|19|(23<<10);
 title.tex_id[0]=104;title.bp[0x88]=(2<<20)|215|(127<<10);
 b->states={glyph,panel,hud,title};b->cmds.assign(8,d);
 unsigned ids[]={0,2,0,1,0,3,0,0};for(int i=0;i<8;++i)b->cmds[i].state=ids[i];
 b->cmds[6]=copy;
 auto lookup=[](uint32_t id){return id==102?ios_ui::panel_hash:id==104?ios_ui::menu_title_hash:0;};
 auto modal=ios_ui::modal_draws(*b,{},lookup);
 for(int i=0;i<8;++i)assert(bool(modal[i])==(i>=2 && i<=5));
 auto rejected=ios_ui::modal_draws(*b,{},[](uint32_t){return uint64_t(0);});
 assert(std::none_of(rejected.begin(),rejected.end(),[](auto x){return x;}));
 std::vector<uint8_t> offscreen(8,1);rejected=ios_ui::modal_draws(*b,offscreen,lookup);
 assert(std::none_of(rejected.begin(),rejected.end(),[](auto x){return x;}));
 b->new_textures.clear();for(auto id:{102u,104u}){auto tex=std::make_shared<gx::TexData>();tex->id=id;tex->hash=lookup(id);b->new_textures.push_back(tex);}
 ios_guest::modules=ios_guest::Replay;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);
 for(int i:{0,1,2,3,4,5,7}){
  float canvas=i>=2 && i<=5?4.f/3:16.f/9;
  assert(fabs(b->states[b->cmds[i].state].proj[0]-s.proj[0]*canvas/(2868.f/1320))<1e-7);
 }
 puts("Menu, fullscreen HUD, flat logos, modal-group/cache boundaries and I8/IA8 movie policy passed");
}
