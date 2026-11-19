#pragma once
#include <cstdint>
namespace ios_audio {
class Recovery {
    uint64_t generation_=0;
    bool pending_=false;
    unsigned failures_=0;
public:
    uint64_t generation()const{return generation_;}
    bool pending()const{return pending_;}
    bool current(uint64_t ticket)const{return pending_ && ticket==generation_;}
    unsigned failures()const{return failures_;}
    uint64_t start(){if(!pending_){++generation_;pending_=true;failures_=0;}return generation_;}
    void cancel(){++generation_;pending_=false;failures_=0;}
    void complete(uint64_t ticket){if(current(ticket))cancel();}
    unsigned next_delay_ms(uint64_t ticket){
        if(!current(ticket))return 0;
        constexpr unsigned fast[]={50,100,200,400};
        unsigned n=failures_++;
        return n<4?fast[n]:1000;
    }
};
}
