#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace gears_xr {
// Value-only helper. XrTime values must share one runtime time domain.
// No reference-space handle or pose is changed by enqueue().
class ReferenceSpaceEpoch {
    std::array<std::int64_t,16> changes_{};
    std::size_t count_{};
    std::uint64_t epoch_{1};
    std::int64_t last_change_{},last_predicted_{};
    bool advance_rejected_{};
public:
    ReferenceSpaceEpoch()=default;
    // Allows a restored counter and makes overflow behavior directly testable.
    explicit ReferenceSpaceEpoch(std::uint64_t initial_epoch):epoch_(initial_epoch?initial_epoch:1){}
    bool enqueue(std::int64_t change_time) {
        if(change_time<=0)return false;
        if(change_time<=last_change_)return true;
        std::size_t position=0;
        while(position<count_&&changes_[position]<change_time)++position;
        if(position<count_&&changes_[position]==change_time)return true;
        if(count_==changes_.size())return false;
        for(std::size_t i=count_;i>position;--i)changes_[i]=changes_[i-1];
        changes_[position]=change_time;++count_;return true;
    }
    // False means unchanged; advance_rejected() distinguishes invalid input.
    // Overflow rejects the whole advance without consuming any pending event.
    bool advance(std::int64_t predicted_time) {
        advance_rejected_=false;
        if(predicted_time<=0||predicted_time<last_predicted_){advance_rejected_=true;return false;}
        std::size_t due=0;
        while(due<count_&&changes_[due]<=predicted_time)++due;
        if(epoch_>(std::numeric_limits<std::uint64_t>::max)()-due){advance_rejected_=true;return false;}
        last_predicted_=predicted_time;
        if(!due)return false;
        epoch_+=due;last_change_=changes_[due-1];
        for(std::size_t i=due;i<count_;++i)changes_[i-due]=changes_[i];
        count_-=due;return true;
    }
    std::uint64_t epoch()const{return epoch_;}
    std::int64_t last_change_time()const{return last_change_;}
    std::int64_t last_predicted_time()const{return last_predicted_;}
    std::size_t pending()const{return count_;}
    bool advance_rejected()const{return advance_rejected_;}
};
}
