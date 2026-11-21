#pragma once
#include "core.h"
namespace workbench {
class Recorder {
    static constexpr std::size_t capacity=21600; // Five minutes at72 Hz; bounded on device.
    std::deque<Frame> recent_;std::vector<Frame> frames_;bool recording_{};std::uint64_t epoch_{};
public:
    bool recording() const{return recording_;}auto const& frames()const{return frames_;}
    void start(){frames_.clear();recording_=true;epoch_=0;}
    void stop(){recording_=false;}
    void retain_recent(){frames_.assign(recent_.begin(),recent_.end());recording_=false;}
    bool append(Frame const& f){
        if(f.time_ns<=0||!valid(f.head)||!valid(f.object))return false;
        if(!recent_.empty()&&(f.epoch!=recent_.back().epoch||f.time_ns<=recent_.back().time_ns))recent_.clear();
        recent_.push_back(f);while(!recent_.empty()&&f.time_ns-recent_.front().time_ns>5'000'000'000ll)recent_.pop_front();
        if(!recording_)return true;
        if(!frames_.empty()&&(f.epoch!=epoch_||f.time_ns<=frames_.back().time_ns)){recording_=false;return false;}
        epoch_=f.epoch;if(frames_.size()>=capacity){recording_=false;return false;}frames_.push_back(f);return true;
    }
    Frame const* at(std::size_t index) const{return index<frames_.size()?&frames_[index]:nullptr;}
    std::size_t index_at(std::int64_t time) const{if(frames_.empty())return 0;auto i=std::lower_bound(frames_.begin(),frames_.end(),time,[](auto const& f,auto t){return f.time_ns<t;});return i==frames_.end()?frames_.size()-1:std::size_t(i-frames_.begin());}
    void save(std::string const& path)const;
    void load(std::string const& path);
};
}
