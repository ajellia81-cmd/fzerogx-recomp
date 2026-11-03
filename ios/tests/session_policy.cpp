// ROM-free tests for suspend/resume and controller defaults.
#include "lifecycle.h"
#include "logo_timing.h"
#include "controller_bindings.h"
#include <cassert>
#include <atomic>
#include <future>
#include <thread>
using namespace std::chrono_literals;
int main(){
 assert(ios_logo::delay_ticks(ios_logo::boot_fade_a)==2025000);
 assert(ios_logo::delay_ticks(ios_logo::boot_fade_b)==2025000);
 assert(ios_logo::delay_ticks(0x80005A4C)==0 && ios_logo::delay_ticks(0x80005A94)==0);
 assert(ios_logo::delay_ticks(ios_logo::title_tick)==0);
 ios_logo::note_submission(100);assert(ios_logo::hold_deadline()==100+ios_logo::visible_hold_ns);
 ios_logo::note_presentation(200);ios_logo::note_presentation(999);
 assert(ios_logo::hold_deadline()==200+ios_logo::visible_hold_ns);
 ios_logo::AVTimer timer;unsigned frames=0,fade=0;uint16_t counter=280;
 while(counter){ios_logo::note_av_update(counter);auto next=timer.remaining(2,counter);if(next<=30)++fade;counter=next-1;++frames;assert(frames<=120);}
 assert(frames==120 && fade==12);
 for(int i=0;i<4;++i)ios_logo::note_av_update(0);
 assert(ios_logo::av_updates==120 && ios_logo::av_fades==12 && ios_logo::av_complete);
 for(unsigned phase:{0u,1u,3u})assert(timer.remaining(phase,280)==280);
 constexpr auto map=ios_bindings::defaults;
 static_assert(map[0]==PAD_A && map[1]==PAD_X && map[2]==PAD_B && map[3]==PAD_Y);
 static_assert(map[4]==0 && map[5]==PAD_Z && map[6]==PAD_START);

 ios_lifecycle::Gate gate;gate.reset_clock_baseline();gate.renderer_started();
 gate.pause();gate.pause();
 std::atomic<int> drained{0};
 auto renderer=std::async(std::launch::async,[&]{gate.render_checkpoint([&]{++drained;});});
 gate.wait_for_renderer();assert(drained==1 && gate.paused());
 auto guest=std::async(std::launch::async,[&]{gate.guest_checkpoint();});
 assert(guest.wait_for(1ms)==std::future_status::timeout);
 auto held=gate.clock_hold_ns();assert(held>0);
 assert(gate.resume());renderer.get();guest.get();
 auto frozen=gate.clock_hold_ns();std::this_thread::sleep_for(2ms);assert(gate.clock_hold_ns()==frozen);
 // Repeated pause/resume cycles must always drain before acknowledgement.
 for(int i=0;i<30;++i){gate.pause();auto r=std::async(std::launch::async,[&]{gate.render_checkpoint([&]{++drained;});});gate.wait_for_renderer();assert(gate.paused());gate.resume();r.get();}
 gate.renderer_stopped();gate.pause();gate.wait_for_renderer();gate.resume();
 assert(!gate.resume());
}
