#pragma once
#include <cstdint>
#include <algorithm>
namespace gears_ngx_source {
// Constructor eligibility is separate from raw native Create identity. A
// cached feature can survive a real interval in which this renderer uses no
// SDK at all. Keep ordinary input geometry until a newly accepted pair proves
// SDK use again. These methods certify no SDK/GPU facts by themselves.
class Activity {
    bool gap_{};uint64_t gap_ticket_{},accepted_ticket_{};
public:
    uint64_t gap_frames{},recoveries{};
    bool eligible()const{return !gap_;}
    void hold(uint64_t ticket){if(ticket){gap_=true;gap_ticket_=std::max(gap_ticket_,ticket);}}
    void consumed(uint64_t ticket,bool sdk_pair_used){
        if(!ticket||sdk_pair_used||ticket<accepted_ticket_)return;
        ++gap_frames;hold(ticket);
    }
    void accepted_pair(uint64_t ticket){
        if(!ticket)return;accepted_ticket_=std::max(accepted_ticket_,ticket);
        if(gap_&&ticket>=gap_ticket_){gap_=false;++recoveries;}
    }
};
}
