#pragma once
#include <cstdint>
#include <utility>

// Render-thread ownership only. Arrival bounds resource retention, never the
// image timestamp, epoch eligibility or display freshness.
template<class Frame> class PendingCaptureFrame {
    Frame frame_{nullptr};
    std::int64_t captured_{},held_since_{},epoch_{};
    std::uint64_t generation_{};
    static void close(Frame frame)noexcept{if(frame)try{frame.Close();}catch(...){} }
public:
    PendingCaptureFrame()=default;
    PendingCaptureFrame(PendingCaptureFrame const&)=delete;
    PendingCaptureFrame& operator=(PendingCaptureFrame const&)=delete;
    static constexpr std::int64_t maximum_hold=2500000; // 250 ms, 100 ns ticks.
    std::uint64_t reused{},dropped{};
    ~PendingCaptureFrame(){discard();}
    bool has_frame()const{return bool(frame_);}
    void discard()noexcept{
        if(frame_){++dropped;Frame old=std::move(frame_);frame_=nullptr;close(std::move(old));}
    }
    bool defer(Frame frame,std::int64_t captured,std::int64_t now,std::int64_t epoch,std::uint64_t generation){
        if(frame_||!frame||now<=0||epoch<=0||captured<=epoch||captured<=now||captured-now>maximum_hold){
            if(frame)++dropped;close(std::move(frame));return false;
        }
        frame_=std::move(frame);captured_=captured;held_since_=now;epoch_=epoch;generation_=generation;return true;
    }
    Frame take(std::int64_t now,std::int64_t epoch,std::uint64_t generation){
        if(!frame_)return nullptr;
        if(epoch!=epoch_||generation!=generation_||now<held_since_||now-held_since_>maximum_hold||
           (now>=captured_&&now-captured_>maximum_hold)){discard();return nullptr;}
        if(now<captured_)return nullptr;
        Frame result=std::move(frame_);frame_=nullptr;++reused;return result;
    }
};
