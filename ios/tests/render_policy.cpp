// Original synthetic renderer regression fixtures; no game inputs.
#include <cassert>
#include <cmath>
#include "render_ios.cpp"
void fatal(const char*,...){std::abort();}
int main(){
 gx::ShaderKey cacheKey{};cacheKey.dst_alpha=2;gx::g_dual_source=false;gx::ios_shader_key(cacheKey);assert(cacheKey.dst_alpha==0);
 cacheKey.dst_alpha=2;gx::g_dual_source=true;gx::ios_shader_key(cacheKey);assert(cacheKey.dst_alpha==2);gx::g_dual_source=false;

 auto*b=new gx::Batch;gx::PixelState s{};s.proj[6]=1;s.proj[0]=2.f/640;s.proj[1]=-1;s.proj[2]=2.f/480;s.proj[3]=-1;s.viewport[0]=320;s.viewport[1]=240;s.viewport[3]=1002;s.viewport[4]=922;s.bp[0x59]=342|(342<<10);s.bp[0x20]=682|(682<<12);s.bp[0x21]=1161|(1321<<12);s.tex_id[0]=1;s.bp[0x28]=0x40;
 gx::Cmd d{};d.type=gx::CmdType::Draw;d.vcount=d.count=4;b->cmds={d};b->states={s};b->verts.resize(4);
 for(int i=0;i<4;++i){auto&v=b->verts[i];v.pos[0]=320+((i&1)?37.5f:-37.5f);v.pos[1]=240+((i&2)?50.f:-50.f);}
 ios_guest::modules=ios_guest::Selection;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);
 auto&t=b->states[b->cmds[0].state];assert(t.proj[0]==s.proj[0]&&t.bp[0x20]==s.bp[0x20]&&t.bp[0x21]==s.bp[0x21]);float box[4];ios_scene::draw_bounds(*b,b->cmds[0],t,box);assert(fabs((box[2]-box[0])*16.f/9-(box[3]-box[1]))<1e-5);
 b->states={s};b->cmds={d};ios_guest::modules=ios_guest::Interview;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);ios_scene::draw_bounds(*b,b->cmds[0],b->states[b->cmds[0].state],box);assert(fabs((box[2]-box[0])*2868/1320-(box[3]-box[1]))<1e-5);
 // Every menu owner must remain a menu without headers, including transient
 // card views and perspective preview geometry, despite retained race phases.
 for(unsigned module:{ios_guest::Selection,ios_guest::Option,ios_guest::Profile,ios_guest::Customize,ios_guest::Shop,ios_guest::Paint,ios_guest::Replay}){
  for(int phase:{-1,0,37,38,39,40,41,42,43})for(bool preview:{false,true}){
   b->states={s};b->cmds={d};b->states[0].proj[6]=preview?0:1;
   unsigned encoded=module==ios_guest::Replay?module|ios_guest::Title:module;
   b->states[0].tag=ios_context::encode(encoded,phase,false);
   auto context=ios_context::read(*b);assert(ios_scene::classify(*b,context.live,context.modules,context.phase)==ios_scene::Mode::Menu);
  }
 }
 b->states={s};b->cmds={d};
 // An old course/race/finish phase cannot turn a wholly 2D view into World.
 for(int phase:{39,40,41,42,43})for(unsigned module:{0u,unsigned(ios_guest::Story),unsigned(ios_guest::Winning)}){
  b->states={s};b->cmds={d};assert(ios_scene::classify(*b,false,module,phase)==ios_scene::Mode::Menu);
 }
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
 b->cmds[1].count=4;assert(ios_ui::startup_art(*b,[](uint32_t){return uint64_t(0x4c8a45da63b18ea5ull);}));
 // An unrelated transition draw or unused stale binding cannot change the
 // logo canvas; a stale logo binding that TEV does not sample cannot select it.
 b->states={s};b->cmds={d};b->states[0].tex_id[1]=2;
 assert(ios_ui::startup_art(*b,[](uint32_t id){return id==1?0x4c8a45da63b18ea5ull:123ull;}));
 b->states[0].bp[0x28]=0x41;
 assert(!ios_ui::startup_art(*b,[](uint32_t id){return id==1?0x4c8a45da63b18ea5ull:123ull;}));
 b->states[0].bp[0x28]=0;
 assert(ios_ui::only_transition_draws(*b));
 b->states[0].bp[0x28]=0x40;assert(!ios_ui::only_transition_draws(*b));
 // End-to-end queued logo -> fade-only -> unrelated 2D -> world sequence.
 b->states={s};b->cmds={d};b->states[0].tag=ios_context::encode(ios_guest::Selection,41,false);
 auto logoTex=std::make_shared<gx::TexData>();logoTex->id=1;logoTex->hash=0x4c8a45da63b18ea5ull;b->new_textures={logoTex};
 gx::ios_prepare_scene(*b,false);assert(gx::g_ios_scene==ios_scene::Mode::Flat);
 b->new_textures.clear();b->states[0].bp[0x28]=0;
 gx::ios_prepare_scene(*b,false);assert(gx::g_ios_scene==ios_scene::Mode::Flat);
 b->states[0].tex_id[0]=99;b->states[0].bp[0x28]=0x40;
 gx::ios_prepare_scene(*b,false);assert(gx::g_ios_scene==ios_scene::Mode::Menu);
 b->states[0].tag=ios_context::encode(ios_guest::Title,41,false);b->states[0].proj[6]=0;
 gx::ios_prepare_scene(*b,false);assert(gx::g_ios_scene==ios_scene::Mode::World);
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
 for(unsigned module:{0u,unsigned(ios_guest::Title),unsigned(ios_guest::Story),unsigned(ios_guest::Replay),unsigned(ios_guest::Profile),unsigned(ios_guest::Customize),unsigned(ios_guest::Shop),unsigned(ios_guest::Paint)}){
  assert(ios_scene::classify(*b,false,module,41)==ios_scene::Mode::Menu);
  assert(ios_scene::classify(*b,true,module,41)==ios_scene::Mode::Menu);
  assert(ios_scene::classify(*b,true,module|ios_guest::Movie,41)==ios_scene::Mode::Movie);
  b->states[1].tex_is_efb[0]=true;
  if(!(module&(ios_guest::MenuModules|ios_guest::Replay)))assert(ios_scene::classify(*b,false,module,-1)!=ios_scene::Mode::Menu);
  b->states[1].tex_is_efb[0]=false;
 }
 // Menu-specific modules retain their entire composition even when a large
 // perspective preview appears. The encoded context must carry that decision.
 for(unsigned module:{unsigned(ios_guest::Profile),unsigned(ios_guest::Customize),unsigned(ios_guest::Shop),unsigned(ios_guest::Paint)}){
  b->states={s,s};b->states[1].proj[6]=0;b->cmds={d,d};b->cmds[1].state=1;
  auto tag=ios_context::decode(ios_context::encode(module,41,false));assert(tag.stamped && (tag.modules&ios_guest::MenuModules));
  for(auto& st:b->states)st.tag=ios_context::encode(module,41,false);
  b->new_textures.clear();gx::ios_prepare_scene(*b,false);assert(gx::g_ios_scene==ios_scene::Mode::Menu);
  auto before=b->states;gx::ios_widen(*b,2868,1320);assert(b->states.size()==before.size()&&!memcmp(b->states.data(),before.data(),before.size()*sizeof(gx::PixelState)));
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
 ios_guest::modules=ios_guest::Interview;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);
 for(int i:{0,1,2,3,4,5,7}){
  float canvas=i>=2 && i<=5?4.f/3:16.f/9;
  assert(fabs(b->states[b->cmds[i].state].proj[0]-s.proj[0]*canvas/(2868.f/1320))<1e-7);
 }
 // Ignore retained, unsampled maps when identifying a modal run; a map
 // actually sampled by another stage must still terminate that run.
 b->states={glyph,panel,glyph};b->cmds.assign(3,d);
 for(unsigned i=0;i<3;++i){b->cmds[i].state=i;b->states[i].tex_id[7]=777;b->states[i].bp[0xAB]=777;}
 auto sampled=ios_ui::modal_draws(*b,{},lookup);assert(sampled[0]&&sampled[1]&&sampled[2]);
 b->states[1].bp[0]=1u<<10;b->states[1].bp[0x28]|=(0x40u|7u)<<12;
 sampled=ios_ui::modal_draws(*b,{},lookup);assert(!sampled[0]&&!sampled[1]&&!sampled[2]);
 // The interview context survives FIFO encode/decode even if old selection
 // and winning modules remain linked during a transition. Movies win priority.
 auto code=ios_context::encode(ios_guest::Interview|ios_guest::Selection|ios_guest::Winning,43,false);
 auto interview=ios_context::decode(code);assert(interview.stamped&&interview.modules==ios_guest::Interview);
 assert(ios_context::decode(ios_context::encode(ios_guest::Movie|ios_guest::Interview,43,false)).modules==ios_guest::Movie);
 assert(ios_scene::classify(*b,false,ios_guest::Interview,0)==ios_scene::Mode::World);
 // Per-primitive glyph detection must work in ANY fullscreen 3D
 // scene, while preserving already-compensated HUD art and panel atlas pieces.
 gx::PixelState sprite=s;sprite.num_texgens=1;sprite.tex_id[0]=101;sprite.bp[0x88]=23|(23<<10);
 b->states={sprite};b->cmds={d};b->verts.resize(4);
 auto quad=[&](float width,float height,float umax=1.f){
  memset(b->verts.data(),0,4*sizeof(gx::GpuVertex));
  for(int i=0;i<4;++i){auto& v=b->verts[i];v.pos[0]=100+((i&1)?width:0);v.pos[1]=100+((i&2)?height:0);v.tex[0][0]=(i&1)?umax:0;v.tex[0][1]=(i&2)?1:0;v.tex[0][2]=1;}
 };
 quad(24,24);assert(ios_ui::fixed_glyph(*b,d,sprite));
 sprite.bp[0]=1u<<16;assert(!ios_ui::fixed_glyph(*b,d,sprite));sprite.bp[0]=0;
 sprite.num_texgens=2;sprite.bp[0]=1u<<10;sprite.bp[0x28]=0x40u|(0x48u<<12);
 for(auto& v:b->verts){v.tex[1][0]=v.tex[0][0]+.1f;v.tex[1][1]=v.tex[0][1]-.1f;v.tex[1][2]=1;}
 assert(ios_ui::fixed_glyph(*b,d,sprite));
 b->verts[1].tex[1][0]+=.5f;assert(!ios_ui::fixed_glyph(*b,d,sprite));
 sprite.num_texgens=1;sprite.bp[0]=0;sprite.bp[0x28]=0x40;

 quad(18,24);assert(!ios_ui::fixed_glyph(*b,d,sprite));
 quad(48,24);assert(!ios_ui::fixed_glyph(*b,d,sprite)); // deliberately wide
 // A generic bitmap must not be inferred from aspect alone (HUD faces and
 // borders intentionally use different primitive dimensions).
 sprite.bp[0x88]=(5<<20)|99|(49<<10);quad(100,50);assert(!ios_ui::fixed_glyph(*b,d,sprite));
 sprite.bp[0x88]=23|(23<<10);quad(24,24);sprite.tex_is_efb[0]=true;assert(!ios_ui::fixed_glyph(*b,d,sprite));sprite.tex_is_efb[0]=false;
 b->verts[0].tex[0][2]=0;assert(!ios_ui::fixed_glyph(*b,d,sprite));
 // Independent fixed and compensated primitives share the SAME PixelState.
 b->states={sprite};b->cmds.assign(3,d);b->cmds[1].vfirst=4;b->cmds[2].state=1;b->verts.resize(8);
 for(int i=0;i<8;++i){auto& v=b->verts[i];memset(&v,0,sizeof(v));v.pos[0]=100+((i&1)?(i<4?24:18):0);v.pos[1]=100+((i&2)?24:0);v.tex[0][0]=(i&1)?1:0;v.tex[0][1]=(i&2)?1:0;v.tex[0][2]=1;}
 for(unsigned module:{0u,unsigned(ios_guest::Title),unsigned(ios_guest::Story),unsigned(ios_guest::Winning),unsigned(ios_guest::Interview)}){
  gx::PixelState worldSprite=sprite;worldSprite.proj[6]=0;worldSprite.proj[0]=1;worldSprite.proj[2]=4.f/3;
  b->states={sprite,worldSprite};b->cmds[0].state=b->cmds[1].state=0;b->cmds[2].state=1;
  for(auto& st:b->states)st.tag=ios_context::encode(module,41,false);
  auto lines=ios_ui::glyph_lines(*b,{},{});assert(lines[0].fixed&&!lines[1].fixed);
  float center=lines[0].anchor;
  b->new_textures.clear();gx::ios_prepare_scene(*b,true);gx::ios_widen(*b,2868,1320);
  const auto& correctedGlyph=b->states[b->cmds[0].state];float actualCenter=correctedGlyph.proj[0]*112+correctedGlyph.proj[1];
  assert(fabs(actualCenter-center*(16.f/9)/(2868.f/1320))<1e-6);
  for(int i=0;i<2;++i){float canvas=i==0?4.f/3:16.f/9;assert(fabs(b->states[b->cmds[i].state].proj[0]-s.proj[0]*canvas/(2868.f/1320))<1e-7);}
 }
 // Correct spacing together with glyph widths; the run must keep its center.
 b->states={sprite};b->cmds.assign(2,d);b->cmds[1].vfirst=4;b->verts.resize(8);
 for(int i=0;i<8;++i){auto& v=b->verts[i];memset(&v,0,sizeof(v));v.pos[0]=100+(i/4)*30+((i&1)?24:0);v.pos[1]=100+((i&2)?24:0);v.tex[0][0]=(i&1)?1:0;v.tex[0][1]=(i&2)?1:0;v.tex[0][2]=1;}
 auto lines=ios_ui::glyph_lines(*b,{},{});assert(lines[0].fixed&&lines[1].fixed&&lines[0].anchor==lines[1].anchor);
 auto excluded=ios_ui::glyph_lines(*b,{1,1},{});assert(!excluded[0].fixed&&!excluded[1].fixed);
 for(float target:{4.f/3,16.f/9,2868.f/1320,21.f/9}){
  b->states={sprite};b->cmds[0].state=b->cmds[1].state=0;gx::g_ios_scene=ios_scene::Mode::World;gx::ios_widen(*b,int(target*1320+.5f),1320);
  const auto& first=b->states[b->cmds[0].state];const auto& next=b->states[b->cmds[1].state];
  float gap=(next.proj[0]*130+next.proj[1])-(first.proj[0]*124+first.proj[1]);
  float glyphWidth=first.proj[0]*24;assert(fabs(gap/glyphWidth-.25f)<1e-5);
 }
 // The same glyphs accompanying a prerendered movie must not receive the
 // new World-font correction, irrespective of their dimensions.
 b->states={sprite};b->cmds[0].state=b->cmds[1].state=0;gx::g_ios_scene=ios_scene::Mode::Movie;gx::g_ios_fixed_aspect=16.f/9;gx::ios_widen(*b,2868,1320);
 assert(b->states[b->cmds[0].state].proj[0]==sprite.proj[0]);
 puts("Menu, fullscreen HUD, flat logos, modal-group/cache boundaries and I8/IA8 movie policy passed");
}
