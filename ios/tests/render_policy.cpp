// Original, asset-free fixtures for the game's shared presentation paths.
#include <cassert>
#include <cmath>
#include <cstring>
#include "render_ios.cpp"
#include "frame_capture.h"
gx::Batch::~Batch()=default;
void fatal(const char*,...){std::abort();}
namespace {
bool near(float a,float b,float e=1e-4f){return std::fabs(a-b)<e;}
gx::PixelState state(){
 gx::PixelState s{};s.proj[6]=1;s.proj[0]=2.f/640;s.proj[1]=-1;s.proj[2]=-2.f/480;s.proj[3]=1;
 s.viewport[0]=320;s.viewport[1]=-240;s.viewport[3]=1004;s.viewport[4]=924;
 s.bp[0x59]=342|(342<<10);s.bp[0x20]=684|(684<<12);s.bp[0x21]=1163|(1323<<12);
 s.tex_id[0]=1;s.bp[0x28]=0x40;s.num_texgens=1;s.bp[0x88]=(5u<<20)|91|(115<<10);return s;
}
gx::Cmd quad(gx::Batch& b,gx::PixelState s,float x,float y,float w,float h){
 gx::Cmd c{};c.type=gx::CmdType::Draw;c.vcount=c.count=4;c.vfirst=b.verts.size();c.state=b.states.size();b.states.push_back(s);
 for(unsigned i=0;i<4;++i){gx::GpuVertex v;memset(&v,0,sizeof v);v.pos[0]=x+((i&1)?w:0);v.pos[1]=y+((i&2)?h:0);v.tex[0][0]=(i&1)?1:0;v.tex[0][1]=(i&2)?1:0;v.tex[0][2]=1;v.col[0][3]=255;b.verts.push_back(v);}
 b.cmds.push_back(c);return c;
}
unsigned world_cell(gx::Batch& b,float x,float y,float w,float h){
 auto c=state();c.proj[6]=0;c.proj[0]=1;c.proj[1]=0;c.proj[2]=(4.f/3)*(w/640)/(h/480);c.proj[3]=0;
 c.viewport[0]=w*.5f;c.viewport[1]=-h*.5f;c.viewport[3]=684+x+w*.5f;c.viewport[4]=684+y+h*.5f;
 c.bp[0x20]=unsigned(684+y)|(unsigned(684+x)<<12);c.bp[0x21]=unsigned(684+y+h-1)|(unsigned(684+x+w-1)<<12);
 unsigned draw=b.cmds.size();auto cmd=quad(b,c,-1,-1,2,2);for(unsigned i=0;i<12;++i)b.verts.push_back(b.verts[cmd.vfirst+i%4]);
 for(unsigned i=0;i<16;++i)b.verts[cmd.vfirst+i].pos[2]=-5;
 b.cmds[draw].vcount=16;b.cmds[draw].count=24;return draw;
}

float screen(const gx::PixelState& s,float x){return s.viewport[0]*(s.proj[0]*x+s.proj[1])+s.viewport[3]-int(s.bp[0x59]&1023)*2;}
float sx(const gx::Batch& b,unsigned i,float x){return screen(b.states[b.cmds[i].state],x);}
void widen(gx::Batch& b,ios_scene::Mode mode,float target=16.f/9){gx::g_ios_canvas_source=4.f/3;gx::g_ios_ui_canvas_source={4.f/3,4.f/3};gx::g_ios_world_wide_canvas=false;gx::g_ios_scene=mode;gx::ios_widen(b,int(target*1440+.5f),1440);}
}
#include "tools/capture_presentation_audit.h"
int main(int argc,char**argv){
 gx::ShaderKey k{};k.dst_alpha=2;gx::g_dual_source=false;gx::ios_shader_key(k);assert(k.dst_alpha==0);
 k.dst_alpha=2;gx::g_dual_source=true;gx::ios_shader_key(k);assert(k.dst_alpha==2);gx::g_dual_source=false;
 const auto s=state();
 // UI ownership/family bits cannot alter any of the 40 scene/live codes.
 for(unsigned kind=0;kind<20;++kind)for(bool live:{false,true}){
  auto base=ios_context::decode(uint8_t(1+kind+(live?20:0)));
  assert(base.stamped && base.live==live);
  for(unsigned flags:{0u,0x40u,0x80u,0xc0u}){
   auto tagged=ios_context::decode(uint8_t(1+kind+(live?20:0))|flags);
   assert(tagged.stamped && tagged.live==base.live && tagged.modules==base.modules && tagged.phase==base.phase);
  }
 }
 assert(!ios_context::decode(0).stamped && !ios_context::decode(63).stamped);
 // Records can outlive Selection and contain only orthographic draws.
 // Neither an absent owner nor a stale non-race phase may switch to 4:3 output.
 for(int phase:{-1,0,1,36,37,38,39,40,41,42,43}){
  gx::Batch records;quad(records,s,100,100,92,116);
  assert(ios_scene::classify(records,false,0,phase)==ios_scene::Mode::Menu);
 }

 // Exhaustive inventory of display-owning modules; stale race state must not
 // turn menu artwork into World or films into a menu.
 for(unsigned m:{ios_guest::Selection,ios_guest::Option,ios_guest::Profile,ios_guest::Customize,ios_guest::Shop,ios_guest::Paint,ios_guest::Replay})for(int phase:{-1,0,37,38,39,40,41,42,43})for(bool camera:{false,true}){
  gx::Batch b;auto st=s;st.proj[6]=camera?0:1;quad(b,st,100,100,92,116);
  assert(ios_scene::classify(b,false,m|ios_guest::Title,phase)==ios_scene::Mode::Menu);
  for(auto& p:b.states)p.tag=ios_context::encode(m|ios_guest::Title,phase,false);
  auto context=ios_context::read(b);assert(context.stamped && context.modules==m);assert(ios_scene::classify(b,context.live,context.modules,context.phase)==ios_scene::Mode::Menu);
  auto old=b.verts;auto matrix=b.states[0];widen(b,ios_scene::Mode::Menu);
  assert(!memcmp(old.data(),b.verts.data(),old.size()*sizeof(gx::GpuVertex)));
  // Menu preview and UI land on the same logical canvas.
  if(!camera)assert(near(sx(b,0,146),320+(146-320)*.75f));
  assert(ios_scene::classify(b,false,m|ios_guest::Movie,phase)==ios_scene::Mode::Movie);
 }
 for(unsigned m:{0u,unsigned(ios_guest::Story),unsigned(ios_guest::Winning)})for(int phase:{39,40,41,42,43}){
  gx::Batch b;quad(b,s,100,100,92,116);assert(ios_scene::classify(b,false,m,phase)==ios_scene::Mode::Menu);
 }
 // One native square, portrait and emblem must keep their actual aspect on
 // 4:3, 16:9, the iPhone drawable and 21:9, in menus AND in a live HUD.
 for(auto mode:{ios_scene::Mode::Menu,ios_scene::Mode::World})for(float target:{4.f/3,16.f/9,2868.f/1320,21.f/9})for(auto wh:{std::pair{92.f,116.f},std::pair{112.f,112.f},std::pair{320.f,400.f}}){
  gx::Batch b;quad(b,s,50,60,wh.first,wh.second);widen(b,mode,target);
  const auto& st=b.states[b.cmds[0].state];float effective=mode==ios_scene::Mode::Menu?16.f/9:float(int(target*1440+.5f))/1440;
  float dx=std::fabs(st.viewport[0]*st.proj[0]*wh.first)/640;
  float dy=std::fabs(st.viewport[1]*st.proj[2]*wh.second)/480;
  assert(near(dx/dy*effective,wh.first/wh.second));
 }
 // Ordered regular-canvas fallback covers bitmap-only frames with neither
 // ordinary text nor a camera. Native UI families remain independent, and
 // an option change cannot inherit the preceding screen's aspect.
 for(unsigned kind=0;kind<40;++kind)for(bool wide:{false,true}){
  gx::Batch noEvidence;auto tagged=s;tagged.tag=uint8_t(1+kind)|(wide?0x80:0);
  quad(noEvidence,tagged,100,100,120,24);
  assert(near(ios_canvas::source_aspect(noEvidence,{},wide?4.f/3:16.f/9),wide?16.f/9:4.f/3));
  assert(ios_context::authored_canvas(tagged.tag|0x40)==0);
 }
 // Bitmap-only compositions inherit the guest camera's display canvas,
 // independently of an earlier menu or the device output aspect. No font
 // vote and no texture-size production heuristic is involved.
 for(float source:{4.f/3,16.f/9})for(float target:{16.f/9,2868.f/1320,21.f/9}){
  gx::Batch title;auto cam=s;cam.proj[6]=0;cam.proj[0]=1;cam.proj[1]=0;cam.proj[2]=source;cam.proj[3]=0;
  auto world=quad(title,cam,-2,-2,4,4);for(unsigned j=0;j<4;++j)title.verts[world.vfirst+j].pos[2]=-5;
  auto image=s;image.bp[0x88]=(1u<<20)|323|(95<<10);
  const float width=324*(4.f/3)/source;quad(title,image,320-width/2,362,width,24);
  assert(near(ios_canvas::source_aspect(title,{},source==4.f/3?16.f/9:4.f/3),source));
  widen(title,ios_scene::Mode::World,target);
  const auto& after=title.states[title.cmds[1].state];
  const float actualTarget=float(int(target*1440+.5f))/1440;
  assert(near(after.viewport[0]/image.viewport[0],source/actualTarget));
  assert(near((after.viewport[0]*after.proj[0]*width/640)/(std::fabs(after.viewport[1]*after.proj[2]*24)/480)*actualTarget,324.f/24));
 }
 // The finite 4:3 menu backdrop covers the authored frame and stays covered.
 // A second 16:9 decoration camera must not change that framing. Real world
 // cameras still expand to the drawable, and native UI families do not borrow
 // a camera vote from an excluded owner.
 {
  gx::Batch menu;auto cam=s;cam.proj[6]=0;cam.proj[0]=1.4407365f;cam.proj[1]=0;cam.proj[2]=1.9209821f;cam.proj[3]=0;
  auto bg=quad(menu,cam,-1.193f/cam.proj[0],-1/cam.proj[2],2*1.193f/cam.proj[0],2/cam.proj[2]);
  for(unsigned j=0;j<4;++j)menu.verts[bg.vfirst+j].pos[2]=-1;
  auto decoration=cam;decoration.proj[2]=decoration.proj[0]*16.f/9;quad(menu,decoration,-1,-1,2,2);
  assert(near(ios_canvas::source_aspect(menu,{}),16.f/9));
  assert(near(ios_canvas::source_aspect(menu,{0,1},16.f/9),4.f/3));
  assert(near(ios_canvas::source_aspect(menu,{1,1},4.f/3),4.f/3));
  widen(menu,ios_scene::Mode::Menu);
  assert(!memcmp(cam.proj,menu.states[menu.cmds[0].state].proj,sizeof cam.proj));
  float box[4];assert(ios_scene::draw_bounds(menu,menu.cmds[0],menu.states[menu.cmds[0].state],box));
  assert(box[0]<-1 && box[2]>1);
 }
 // Composite continuity, across different materials and texture-sorted order:
 // metal border meets panel, gauge marker meets gauge, letters keep baseline
 // and run spacing. No per-letter or per-texture translation is permitted.
 gx::Batch group;auto font=s;font.bp[0x88]=23|(23<<10);auto panel=s;panel.tex_id[0]=2;
 quad(group,panel,100,100,40,24);quad(group,font,148,100,24,24);quad(group,panel,140,100,8,24);quad(group,font,172,100,24,24);
 auto original=group.cmds;std::swap(group.cmds[0],group.cmds[3]);std::swap(group.cmds[1],group.cmds[2]);
 widen(group,ios_scene::Mode::Menu);
 auto byVertex=[&](unsigned first){for(unsigned i=0;i<group.cmds.size();++i)if(group.cmds[i].vfirst==first)return i;std::abort();};
 assert(near(sx(group,byVertex(0),140),sx(group,byVertex(8),140)));
 assert(near(sx(group,byVertex(8),148),sx(group,byVertex(4),148)));
 assert(near(sx(group,byVertex(4),172),sx(group,byVertex(12),172)));
 // State caches must not alias fullscreen effects and small sprites.
 gx::Batch mixed;auto flat=s;flat.tex_id[0]=0;flat.bp[0x28]=0;
 quad(mixed,flat,0,0,640,480);quad(mixed,flat,100,100,40,40);mixed.cmds[1].state=0;
 widen(mixed,ios_scene::Mode::Menu);assert(mixed.cmds[0].state==0);assert(mixed.cmds[1].state!=0);
 assert(near(mixed.states[mixed.cmds[1].state].viewport[0],240));
 // Scissor and sub-viewports follow the SAME global transform. Full glyphs
 // remain inside their transformed clip; no old black strip cuts a portrait.
 gx::Batch clipped;auto local=s;local.viewport[0]=80;local.viewport[3]=684+480;local.bp[0x20]=684|((684+400)<<12);local.bp[0x21]=1163|((684+560-1)<<12);
 quad(clipped,local,0,0,640,480);widen(clipped,ios_scene::Mode::Menu);const auto& clip=clipped.states[clipped.cmds[0].state];
 assert(near(clip.viewport[0],60));assert(near(clip.viewport[3]-684,440));
 assert(((clip.bp[0x20]>>12)&2047)==1064);assert(((clip.bp[0x21]>>12)&2047)==1183);
 // Native offscreen render targets, copy rectangles and writeback inputs stay
 // unchanged. Only their subsequent on-screen picture joins the UI canvas.
 gx::Batch off;auto tiny=s;tiny.viewport[0]=64;tiny.viewport[1]=-64;tiny.viewport[3]=684+64;tiny.viewport[4]=684+64;tiny.bp[0x21]=(684+127)|((684+127)<<12);
 quad(off,tiny,0,0,640,480);gx::Cmd copy{};copy.type=gx::CmdType::EfbCopy;copy.copy.src_w=copy.copy.dst_w=128;copy.copy.src_h=copy.copy.dst_h=128;copy.copy.tex_id=42;copy.copy.clear=true;off.cmds.push_back(copy);
 auto preview=s;preview.tex_id[0]=42;preview.tex_is_efb[0]=true;quad(off,preview,100,100,128,128);
 auto before=off.states[0];widen(off,ios_scene::Mode::Menu);assert(off.cmds[0].state==0 && !memcmp(&before,&off.states[0],sizeof before));assert(!memcmp(&copy,&off.cmds[1],sizeof copy));assert(off.cmds[2].state!=1);
 // Full-frame copies already contain corrected pixels; never squeeze them a
 // second time. Real world cameras expand their frustum instead of stretching.
 gx::Batch feedback;auto fb=s;fb.tex_id[0]=42;fb.tex_is_efb[0]=true;gx::Cmd fullCopy=copy;fullCopy.copy.src_w=fullCopy.copy.dst_w=640;fullCopy.copy.src_h=fullCopy.copy.dst_h=480;feedback.cmds.push_back(fullCopy);quad(feedback,fb,0,0,640,480);
 gx::Cmd xfb=fullCopy;xfb.copy.to_xfb=true;feedback.cmds.push_back(xfb);gx::Cmd present{};present.type=gx::CmdType::Present;present.copy.src_w=640;present.copy.src_h=480;feedback.cmds.push_back(present);
 widen(feedback,ios_scene::Mode::World);assert(feedback.cmds[1].state==0);
 gx::Batch world;auto camera=s;camera.proj[6]=0;camera.proj[0]=1;camera.proj[2]=4.f/3;quad(world,camera,1,1,2,2);widen(world,ios_scene::Mode::World,21.f/9);auto& wc=world.states[world.cmds[0].state];assert(near(wc.proj[0],4.f/7));assert(wc.viewport[0]==320);
 // Positive evidence for an already-compensated guest display prevents double
 // correction and persists on text-free frames. Deliberately scaled glyphs do
 // not vote for either display canvas.
 gx::Batch wide;quad(wide,font,100,100,18,24);assert(ios_canvas::source_aspect(wide,{})==16.f/9);widen(wide,ios_scene::Mode::Menu);assert(wide.states[wide.cmds[0].state].viewport[0]==320);
 // Opening a native 4:3 pause panel cannot change an already compensated HUD.
 gx::Batch paused;quad(paused,font,100,100,18,24);
 auto pausePanel=panel;pausePanel.tag=0x40;pausePanel.bp[0x88]=(5u<<20)|51|(103<<10);
 auto modalTexture=std::make_shared<gx::TexData>();
 modalTexture->id=2;modalTexture->hash=ios_ui::panel_hash;
 paused.new_textures.push_back(modalTexture);
 quad(paused,s,80,120,92,116);
 quad(paused,pausePanel,200,140,240,200);
 auto pauseFont=font;pauseFont.tag=0x40;quad(paused,pauseFont,310,220,24,24);
 widen(paused,ios_scene::Mode::World);
 assert(gx::g_ios_canvas_source==16.f/9);
 assert(near(paused.states[paused.cmds[1].state].viewport[0],320));
 assert(near(paused.states[paused.cmds[2].state].viewport[0],240));
 assert(near(paused.states[paused.cmds[3].state].viewport[0],240));
 // An unrecognised 4:3 overlay cannot replace the established race canvas.
 gx::Batch unrecognised;quad(unrecognised,s,80,120,92,116);
 quad(unrecognised,font,310,220,24,24);
 gx::ios_widen(unrecognised,2560,1440);
 assert(gx::g_ios_canvas_source==16.f/9);
 assert(near(unrecognised.states[unrecognised.cmds[0].state].viewport[0],320));
 // Reproduce a compensated card browser over a 4:3 gameplay dialog:
 // text, borders and unrelated icon formats must share THEIR family's map.
 gx::Batch mixedWindows;
 auto narrowFont=font;narrowFont.tag=0x40;
 auto wideFont=font;wideFont.tag=0xc0;
 auto narrowPanel=panel;narrowPanel.tag=0x40;
 auto widePanel=panel;widePanel.tag=0xc0;
 quad(mixedWindows,narrowFont,160,180,24,24);
 quad(mixedWindows,narrowPanel,100,100,240,200);
 quad(mixedWindows,wideFont,350,180,18,24);
 quad(mixedWindows,widePanel,300,100,240,200);
 widen(mixedWindows,ios_scene::Mode::World);
 assert(near(mixedWindows.states[mixedWindows.cmds[0].state].viewport[0],240));
 assert(near(mixedWindows.states[mixedWindows.cmds[1].state].viewport[0],240));
 assert(near(mixedWindows.states[mixedWindows.cmds[2].state].viewport[0],320));
 assert(near(mixedWindows.states[mixedWindows.cmds[3].state].viewport[0],320));

 // Either UI family may be native 4:3 or guest-compensated 16:9, including
 // the postrace card family over a full-width world presentation.
 for(auto mode:{ios_scene::Mode::Menu,ios_scene::Mode::World})
 for(unsigned family:{0u,1u})for(float source:{4.f/3,16.f/9}){
  gx::Batch dialog;
  auto letters=font;letters.tag=uint8_t(0x40|(family?0x80:0));
  auto border=panel;border.tag=letters.tag;
  quad(dialog,letters,160,180,source>1.5f?18:24,24);
  quad(dialog,border,100,100,240,200);
  float target=mode==ios_scene::Mode::World?21.f/9:16.f/9;
  widen(dialog,mode,target);
  assert(near(gx::g_ios_ui_canvas_source[family],source));
  for(const auto& draw:dialog.cmds)
   assert(near(dialog.states[draw.state].viewport[0],320*source/target));
 }


 // A native card scope includes unrelated icon formats between text draws.
 for(auto mode:{ios_scene::Mode::World,ios_scene::Mode::Menu}){
  gx::Batch card;quad(card,font,80,40,18,24);
  auto native=font;native.tag=0x40;quad(card,native,160,180,24,24);
  auto icon=s;icon.tag=0x40;icon.bp[0x88]=(5u<<20)|63|(63<<10);
  quad(card,icon,300,300,40,40);
  quad(card,native,350,300,24,24);
  widen(card,mode);
  assert(near(card.states[card.cmds[0].state].viewport[0],320));
  for(unsigned i=1;i<4;++i)assert(near(card.states[card.cmds[i].state].viewport[0],240));
 }
 gx::Batch noText;quad(noText,s,100,100,92,116);assert(ios_canvas::source_aspect(noText,{},16.f/9)==16.f/9);
 gx::Batch stretched;quad(stretched,font,100,100,48,24);assert(ios_ui::glyph_canvas(stretched,stretched.cmds[0],stretched.states[0])==0);
 // Startup and movies keep their ORIGINAL presentation, including re-entry,
 // two guest display canvases, fades, unused bindings and Y/UV validation.
 for(auto mode:{ios_scene::Mode::Movie,ios_scene::Mode::Flat}){gx::Batch b;quad(b,s,0,0,640,480);auto old=b.states;widen(b,mode,21.f/9);assert(b.states.size()==old.size() && !memcmp(b.states.data(),old.data(),old.size()*sizeof(s)));}
 for(float compensation:{1.f,.75f}){
  gx::Batch b;auto art=s;art.bp[0x88]=(5u<<20)|99|(49<<10);quad(b,art,320-50*compensation,215,100*compensation,50);
  auto hash=[](uint32_t){return 0x4c8a45da63b18ea5ull;};assert(near(ios_ui::startup_canvas_aspect(b,hash),4.f/3/compensation));assert(ios_ui::startup_art(b,hash));assert(!ios_ui::startup_art(b,[](uint32_t){return 123ull;}));
  b.states[0].tex_id[1]=2;b.states[0].bp[0x28]=0x41;assert(!ios_ui::startup_art(b,[](uint32_t id){return id==1?0x4c8a45da63b18ea5ull:123ull;}));
 }
 gx::PixelState movie=s;movie.bp[0]=1u<<10;movie.bp[0x28]=0x40u|(0x41u<<12);movie.tex_id[1]=2;movie.bp[0x88]=(1<<20)|639|(335<<10);movie.bp[0x89]=(3<<20)|319|(167<<10);assert(ios_scene::movie_state(movie));movie.bp[0x89]=(1<<20)|319|(167<<10);assert(!ios_scene::movie_state(movie));
 // Retained but unsampled movie/copy bindings cannot misclassify a menu or
 // bypass its compensation. Only TEV inputs used by this draw count.
 movie.bp[0x89]=(3<<20)|319|(167<<10);movie.bp[0]=0;movie.bp[0x28]=0x40;assert(!ios_scene::movie_state(movie));
 gx::Batch stale;auto staleState=s;staleState.tex_id[7]=42;staleState.tex_is_efb[7]=true;quad(stale,staleState,100,100,92,116);widen(stale,ios_scene::Mode::Menu);assert(stale.cmds[0].state!=0);
 // Cinematic edge masks cover the drawable rather than becoming short bars
 // inside the native UI canvas; their inner boundary stays joined to that UI.
 gx::Batch matte;quad(matte,flat,0,0,640,40);quad(matte,flat,0,0,40,480);quad(matte,flat,600,0,40,480);
 widen(matte,ios_scene::Mode::Menu);assert(matte.cmds[0].state==0);
 assert(near(sx(matte,1,0),0) && near(sx(matte,1,40),110));
 assert(near(sx(matte,2,600),530) && near(sx(matte,2,640),640));
 // A full-sized EFB target can be an independent preview. Keep its producer
 // native, then transform the consumer AND its clip with the surrounding UI.
 gx::Batch previewFull;quad(previewFull,camera,1,1,2,2);auto pc=fullCopy;pc.copy.tex_id=77;previewFull.cmds.push_back(pc);
 auto pr=s;pr.tex_id[0]=77;pr.tex_is_efb[0]=true;quad(previewFull,pr,120,0,640,480);previewFull.cmds.push_back(xfb);previewFull.cmds.push_back(present);
 widen(previewFull,ios_scene::Mode::Menu);assert(previewFull.cmds[0].state==0);assert(previewFull.cmds[2].state!=1);
 assert(near(previewFull.states[previewFull.cmds[2].state].viewport[0],240));assert(near(sx(previewFull,2,440),410));
 assert(!memcmp(&previewFull.cmds[1],&pc,sizeof pc));
 // If the same target is also the final world screen, its producer is corrected
 // once. Translated consumers conjugate placement, preserving picture scale.
 gx::Batch post;quad(post,camera,1,1,2,2);post.cmds.push_back(pc);quad(post,pr,0,0,640,480);quad(post,pr,120,0,640,480);post.cmds.push_back(xfb);post.cmds.push_back(present);
 widen(post,ios_scene::Mode::World);assert(post.cmds[0].state!=0);assert(post.cmds[2].state==1);
 assert(near(post.states[post.cmds[3].state].viewport[0],320));assert(near(sx(post,3,440),410));
 // Real VS input uses two narrow world viewports but a shared full-screen
 // HUD state. Expand each camera; center each connected HUD group in its own
 // player field. A cross-field panel/word must remain one global composition.
 gx::Batch two;auto cam0=world_cell(two,0,0,320,480);auto cam1=world_cell(two,320,0,320,480);
 unsigned left=two.cmds.size();quad(two,font,148,100,24,24);unsigned shared=two.cmds[left].state;
 unsigned right=two.cmds.size();quad(two,font,468,100,24,24);two.cmds[right].state=shared;
 unsigned panelDraw=two.cmds.size();quad(two,flat,240,200,160,60);
 unsigned text0=two.cmds.size();quad(two,font,294,210,24,24);two.cmds[text0].state=shared;
 unsigned text1=two.cmds.size();quad(two,font,318,210,24,24);two.cmds[text1].state=shared;
 auto fields=ios_split::layout(two,{});assert(fields.active() && fields.cells.size()==2);
 assert(fields.owner[left]!=fields.owner[right] && fields.owner[panelDraw]==-1 && fields.owner[text0]==-1 && fields.owner[text1]==-1);
 widen(two,ios_scene::Mode::World,21.f/9);
 assert(near(two.states[two.cmds[cam0].state].viewport[0],160) && near(two.states[two.cmds[cam1].state].viewport[0],160));
 assert(near(two.states[two.cmds[cam0].state].proj[0],4.f/7));
 assert(near(sx(two,left,160),160) && near(sx(two,right,480),480));
 assert(two.cmds[left].state!=two.cmds[right].state && two.cmds[left].state!=two.cmds[text0].state);
 assert(near(sx(two,text0,306),320+(306-320)*4.f/7));
 // Three/four-player quarter layouts use the same rule, without moving a
 // player's native HUD toward the center of the entire display.
 for(unsigned players:{3u,4u}){
  gx::Batch quarters;std::vector<unsigned> hud;
  for(unsigned i=0;i<players;++i){float x=(i&1)*320,y=(i>>1)*240;world_cell(quarters,x,y,320,240);}
  for(unsigned i=0;i<players;++i){hud.push_back(quarters.cmds.size());quad(quarters,font,(i&1)*320+148,(i>>1)*240+100,24,24);}
  auto q=ios_split::layout(quarters,{});assert(q.cells.size()==players);widen(quarters,ios_scene::Mode::World,21.f/9);
  for(unsigned i=0;i<players;++i){assert(near(sx(quarters,hud[i],(i&1)*320+160),(i&1)*320+160));}
 }
 // A real full-screen world camera prevents two incidental small previews
 // from being misidentified as a multiplayer partition.
 gx::Batch ordinary;world_cell(ordinary,0,0,640,480);world_cell(ordinary,0,0,320,240);world_cell(ordinary,320,0,320,240);
 assert(!ios_split::layout(ordinary,{}).active());
 // Full-colour artwork is never classified as an expendable background from
 // its size. Native artwork and neighbouring labels share one canvas, without
 // cropping baked text. The game's own oversized scenery still covers 16:9.
 gx::Batch bg;auto colour=s;colour.bp[0x88]=(5u<<20)|639|(479<<10);quad(bg,colour,0,0,640,480);quad(bg,colour,100,100,92,116);bg.cmds[1].state=0;widen(bg,ios_scene::Mode::Menu);assert(bg.cmds[0].state==bg.cmds[1].state);assert(near(bg.states[bg.cmds[0].state].proj[2],s.proj[2]));assert(near(bg.states[bg.cmds[1].state].viewport[0],240));
 // Perspective water/refraction that samples a full-frame copy remains a
 // world surface: only orthographic feedback bypasses canvas correction.
 gx::Batch water;auto wet=camera;wet.tex_id[0]=42;wet.tex_is_efb[0]=true;quad(water,wet,1,1,2,2);widen(water,ios_scene::Mode::World,21.f/9);assert(near(water.states[water.cmds[0].state].proj[0],4.f/7));
 if(int result=ios_audit::captures(argc,argv))return result;
 puts("Presentation fixtures and supplied captures passed; visual screen coverage is reported separately");
}

