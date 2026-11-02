// ROM-free tests for suspend/resume and controller defaults.
#include "lifecycle.h"
#include "controller_bindings.h"
#include <cassert>
#include <atomic>
#include <future>
#include <thread>
using namespace std::chrono_literals;
int main(){
 auto map=ios_bindings::defaults;
 assert(map[0]==PAD_A && map[1]==PAD_B && map[2]==PAD_Y && map[4]==PAD_X);
 ios_bindings::assign(map,3,PAD_X);assert(map[3]==PAD_X && !map[4]);
 auto saved=map;ios_bindings::assign(map,99,PAD_A);assert(map==saved);
 ios_bindings::assign(map,4,PAD_A|PAD_B);assert(map==saved);
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
