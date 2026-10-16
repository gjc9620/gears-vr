#pragma once
#include <cstdint>

// WGC SystemRelativeTime and caller QPC time use 100 ns ticks. Arrival time
// cannot qualify an old buffered frame for a newly entered menu.
class MenuCaptureEpoch {
    std::int64_t epoch_{},captured_{};
public:
    static constexpr std::int64_t maximum_age=10000000; // One second.
    void begin(std::int64_t now){epoch_=now;captured_=0;}
    bool eligible(std::int64_t captured,std::int64_t now)const{
        return epoch_>0&&captured>epoch_&&now>=captured&&now-captured<=maximum_age;
    }
    bool accept(std::int64_t captured,std::int64_t now){
        if(!eligible(captured,now))return false;captured_=captured;return true;
    }
    std::int64_t started()const{return epoch_;}
    std::int64_t captured()const{return captured_;}
    bool fresh(std::int64_t now,std::int64_t max_age=maximum_age)const{
        return eligible(captured_,now)&&max_age>0&&now-captured_<=max_age;
    }
};
