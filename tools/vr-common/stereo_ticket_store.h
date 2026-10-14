#pragma once
#include "stereo_frame.h"
#include <mutex>

namespace gears_vr {
enum class TicketRetainResult {retained,duplicate,invalid,stale,conflict,overflow};

// Single-swapchain bounded CPU provenance store. Contains values, not renderer
// pointers or bridge references. Store operations allocate no heap memory.
//
// Integration contract: retain ONLY when a ticket is observed in Draw/consume,
// under the same external hook-order lock used for the Present boundary. After
// consuming that window, retire_through(maximum actually observed ticket id).
// Tickets above that boundary belong to a future window and survive. A bare
// clear/reset API is intentionally absent: it could erase concurrent future
// work or allow old ids to be reused. A new trial constructs a new store.
//
// The internal mutex protects values, not native API ordering. Freshness was
// checked when the sample was frozen by StereoGate; lookup must also support
// retaining a completed image for reprojection and never re-reads the bridge.
class StereoTicketStore {
    struct Entry {std::uint64_t id{};StereoSample sample{};};
    std::array<Entry,128> entries_{};
    mutable std::mutex mutex_;
    std::uint64_t maximum_id_{},retired_id_{};
    std::size_t count_{};
    static bool valid(StereoSample const& s,bool allow_sim) {
        return s.session&&s.sequence&&s.received_ns>0&&s.focused==1&&s.should_render==1&&
            stereo_tracking_valid(s,allow_sim)&&stereo_geometry_valid(s);
    }
public:
    static constexpr std::size_t capacity=128;
    StereoTicketStore()=default;
    StereoTicketStore(StereoTicketStore const&)=delete;
    StereoTicketStore& operator=(StereoTicketStore const&)=delete;
    static bool accepted(TicketRetainResult result) {
        return result==TicketRetainResult::retained||result==TicketRetainResult::duplicate;
    }
    TicketRetainResult retain(std::uint64_t id,StereoSample const& sample,bool allow_sim=false) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!id||!valid(sample,allow_sim))return TicketRetainResult::invalid;
        if(id<=retired_id_)return TicketRetainResult::stale;
        Entry* empty{};
        for(auto& entry:entries_) {
            if(entry.id==id)return stereo_same(entry.sample,sample)?TicketRetainResult::duplicate:TicketRetainResult::conflict;
            if(!entry.id&&!empty)empty=&entry;
        }
        // Existing retained duplicates may be older than maximum_id_. A new
        // insertion must be newer, even if that old id was never stored.
        if(id<=maximum_id_)return TicketRetainResult::stale;
        if(!empty)return TicketRetainResult::overflow;
        empty->sample=sample;empty->id=id;maximum_id_=id;++count_;
        return TicketRetainResult::retained;
    }
    bool lookup(std::uint64_t id,StereoSample& sample,bool allow_sim=false) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!id)return false;
        for(auto const& entry:entries_)if(entry.id==id){if(!valid(entry.sample,allow_sim))return false;sample=entry.sample;return true;}
        return false; // Failed lookup leaves the caller's copy unchanged.
    }
    bool retire_through(std::uint64_t last_observed_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!last_observed_id)return true; // Empty observed window.
        if(last_observed_id==retired_id_)return true;
        if(last_observed_id<retired_id_||last_observed_id>maximum_id_)return false;
        bool observed=false;
        for(auto const& entry:entries_)if(entry.id==last_observed_id)observed=true;
        if(!observed)return false; // Reject an invented/unretained boundary.
        for(auto& entry:entries_)if(entry.id&&entry.id<=last_observed_id){entry={};--count_;}
        retired_id_=last_observed_id;return true;
    }
    std::size_t size() const {std::lock_guard<std::mutex> lock(mutex_);return count_;}
    std::uint64_t highest_id() const {std::lock_guard<std::mutex> lock(mutex_);return maximum_id_;}
    std::uint64_t retired_id() const {std::lock_guard<std::mutex> lock(mutex_);return retired_id_;}
};
}
