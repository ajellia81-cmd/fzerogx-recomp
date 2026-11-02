#pragma once
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace ios_lifecycle {
// One cooperative gate for the guest, renderer and host-clock accounting.
// Pausing never destroys the guest or its pending writebacks. A guest waiting
// for the renderer can remain there until foreground rendering resumes.
class Gate {
    using Clock=std::chrono::steady_clock;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool paused_=false,renderer_=false,drained_=false;
    Clock::time_point start_{};
    uint64_t held_=0,clockBase_=0;
    uint64_t held_locked() const {
        return held_+(paused_?std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-start_).count():0);
    }
public:
    bool paused() const {std::lock_guard lock(mutex_);return paused_;}
    void pause(){
        std::lock_guard lock(mutex_);
        if(paused_)return;
        paused_=true;start_=Clock::now();
    }
    bool resume(){
        std::lock_guard lock(mutex_);
        if(!paused_)return false;
        held_=held_locked();paused_=false;cv_.notify_all();return true;
    }
    void renderer_started(){std::lock_guard lock(mutex_);renderer_=true;drained_=false;}
    void renderer_stopped(){std::lock_guard lock(mutex_);renderer_=false;cv_.notify_all();}
    void wait_for_renderer(){std::unique_lock lock(mutex_);cv_.wait(lock,[&]{return !renderer_||drained_;});}
    template<class Drain> void render_checkpoint(Drain drain){
        std::unique_lock lock(mutex_);
        if(!paused_){drained_=false;return;}
        lock.unlock();drain();lock.lock();
        drained_=true;cv_.notify_all();
        cv_.wait(lock,[&]{return !paused_;});
        drained_=false;
    }
    void guest_checkpoint(){std::unique_lock lock(mutex_);cv_.wait(lock,[&]{return !paused_;});}
    void reset_clock_baseline(){std::lock_guard lock(mutex_);clockBase_=held_locked();}
    uint64_t clock_hold_ns() const {std::lock_guard lock(mutex_);return held_locked()-clockBase_;}
};
inline Gate& gate(){static auto* value=new Gate;return *value;}
inline void guest_checkpoint(){gate().guest_checkpoint();}
inline uint64_t clock_hold_ns(){return gate().clock_hold_ns();}
}
