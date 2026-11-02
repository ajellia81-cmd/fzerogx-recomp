// Original synthetic renderer regression fixtures; no game inputs.
#include <cassert>
#include <cmath>
#include "render_ios.cpp"
void fatal(const char*,...){std::abort();}
int main(){
 auto*b=new gx::Batch;gx::PixelState s{};s.proj[6]=1;s.proj[0]=2.f/640;s.proj[1]=-1;s.proj[2]=2.f/480;s.proj[3]=-1;s.viewport[0]=320;s.viewport[1]=240;s.viewport[3]=1002;s.viewport[4]=922;s.bp[0x59]=342|(342<<10);s.bp[0x20]=682|(682<<12);s.bp[0x21]=1161|(1321<<12);s.tex_id[0]=1;
 gx::Cmd d{};d.type=gx::CmdType::Draw;d.vcount=d.count=4;b->cmds={d};b->states={s};b->verts.resize(4);
 for(int i=0;i<4;++i){auto&v=b->verts[i];v.pos[0]=320+((i&1)?37.5f:-37.5f);v.pos[1]=240+((i&2)?50.f:-50.f);}
 ios_guest::modules=ios_guest::Selection;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);
 auto&t=b->states[b->cmds[0].state];assert(t.proj[0]==s.proj[0]&&t.bp[0x20]==s.bp[0x20]&&t.bp[0x21]==s.bp[0x21]);float box[4];ios_scene::draw_bounds(*b,b->cmds[0],t,box);assert(fabs((box[2]-box[0])*16.f/9-(box[3]-box[1]))<1e-5);
 b->states={s};b->cmds={d};ios_guest::modules=ios_guest::Title;gx::ios_prepare_scene(*b,false);gx::ios_widen(*b,2868,1320);ios_scene::draw_bounds(*b,b->cmds[0],b->states[b->cmds[0].state],box);assert(fabs((box[2]-box[0])*2868/1320-(box[3]-box[1]))<1e-5);
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
 puts("16:9 game-compensated UI, unchanged menu scissor, fullscreen HUD and I8/IA8 movie policy passed");
}
