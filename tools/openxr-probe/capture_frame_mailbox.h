#pragma once
#include <mutex>
#include <utility>

// Each WGC binding owns a separate mailbox. Retired callbacks may still arrive,
// but cannot publish into a replacement binding or keep their frame alive.
template<class Frame> class CaptureFrameMailbox {
    mutable std::mutex mutex_;
    Frame latest_{nullptr};
    bool active_=true,closed_=false;
    static void close(Frame frame)noexcept{if(frame)try{frame.Close();}catch(...){} }
    void clear_locked(){Frame previous=std::move(latest_);latest_=nullptr;close(std::move(previous));}
public:
    void publish(Frame frame){
        std::lock_guard<std::mutex> lock(mutex_);
        if(!active_||closed_){close(std::move(frame));return;}
        clear_locked();latest_=std::move(frame);
    }
    Frame take(){
        std::lock_guard<std::mutex> lock(mutex_);
        Frame result=std::move(latest_);latest_=nullptr;return result;
    }
    void discard(){std::lock_guard<std::mutex> lock(mutex_);clear_locked();}
    void retire(){std::lock_guard<std::mutex> lock(mutex_);active_=false;clear_locked();}
    void mark_closed(){std::lock_guard<std::mutex> lock(mutex_);closed_=true;clear_locked();}
    bool closed()const{std::lock_guard<std::mutex> lock(mutex_);return closed_;}
};
