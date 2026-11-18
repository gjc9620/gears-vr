#pragma once
#include <cstdint>
#include <limits>
#include <cmath>

// Pure immutable timing metadata for Boltok's measured insertion only.
// It carries no Frame, lease, tracking timestamp or action/reward authority.
namespace gears_boltok_insertion_window {

class AlignmentWindow {
public:
    bool begin(std::uint64_t serial,std::int64_t opened_time) noexcept {
        reset();
        if(!serial||opened_time<0)return false;
        serial_=serial;opened_time_=opened_time;started_=true;return true;
    }
    // The first authenticated alignment opens one nonrenewable window.
    bool aligned(std::uint64_t serial,std::int64_t aligned_time,
        std::int64_t native_window) noexcept {
        if(!serial||!started_||serial!=serial_||aligned_time<opened_time_||
           native_window<=0)return false;
        if(aligned_time>(std::numeric_limits<std::int64_t>::max)()-native_window)
            return false;
        if(!armed_){
            anchor_time_=aligned_time;
            window_end_time_=aligned_time+native_window;armed_=true;
        }
        return true;
    }
    // A broken geometric approach/alignment cannot start a later new window.
    bool lost() noexcept {
        if(!armed_)return false;
        armed_=false;return true;
    }
    bool duration(std::uint64_t serial,std::int64_t inserted_time,
        std::int64_t& out) const noexcept {
        if(!armed_||serial!=serial_||inserted_time<anchor_time_||
           inserted_time>window_end_time_)return false;
        out=window_end_time_-opened_time_;return true;
    }
    bool armed() const noexcept {return armed_;}
    std::int64_t window_end(std::uint64_t serial) const noexcept {
        return armed_&&serial==serial_?window_end_time_:0;
    }
    void reset() noexcept {*this={};}
private:
    std::uint64_t serial_{};std::int64_t opened_time_{},anchor_time_{},
    window_end_time_{};bool started_{},armed_{};
};

// An explicit commit receipt is distinct from alignment timing. Automatic close
// is allowed only for this exact transaction serial/item and accepted group ACK.
struct PendingClose {
    std::uint64_t item{},reload{},token{};
    bool armed{};
    bool begin(std::uint64_t i,std::uint64_t r,std::uint64_t t) noexcept {
        if(armed||!i||!r||!t)return false;
        item=i;reload=r;token=t;armed=true;return true;
    }
    bool consumes(std::uint64_t i,std::uint64_t r,std::uint64_t t) noexcept {
        if(!armed||i!=item||r!=reload||t!=token)return false;
        armed=false;item=reload=token=0;return true;
    }
    void reset() noexcept {armed=false;item=reload=token=0;}
};
}
