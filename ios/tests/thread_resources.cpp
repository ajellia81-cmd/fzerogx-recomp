// Exercise the actual platform thread API; no game code or data.
#include "platform.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <sys/resource.h>
static std::atomic<unsigned> completed{0};
static void* finish(void*){++completed;plat_thread_exit();}
int main(){
#if defined(__linux__)
 // Joinable stacks exhaust this budget after only a few finished threads.
 struct rlimit limit{512u<<20,512u<<20};if(setrlimit(RLIMIT_AS,&limit))return 2;
#endif
 constexpr unsigned count=1500;
 for(unsigned i=0;i<count;++i){
  if(!plat_thread_start(finish,nullptr,64u<<20)){fprintf(stderr,"Resource test failed after %u creations\n",i);return 1;}
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(completed.load()!=i+1){if(std::chrono::steady_clock::now()>deadline)return 3;std::this_thread::yield();}
  // The completion flag precedes pthread_exit. Allow OS teardown to finish.
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 }
 printf("Host-thread resource test: %u creations/exits at 64 MB stack passed\n",count);
}
