#pragma once
#include "stereo_frame.h"
#include <mutex>

namespace gears_vr {
enum class TicketRetainResult {retained,duplicate,invalid,stale,conflict,overflow};

// Original game camera plus the actual eye matrices frozen for this ticket.
// These are independent values: never reconstruct them from a newer pose.
// Optional owning world point. The caller supplies its authority; this value
// and the pure projection layer do not validate game aim or camera equivalence.
struct StereoFiniteAim {
    bool valid{};
    std::array<float,3> point{};
    std::uint64_t world{},tick{};
};
struct StereoTicketGeometry {
    StereoMatrix source_view{},source_projection{};
    StereoMatrices eyes{};
    StereoFiniteAim finite_aim{};
    bool aiming{}; // Frozen certified game targeting state; not an XR input.
};

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
    struct Entry {std::uint64_t id{};StereoSample sample{};StereoTicketGeometry geometry{};bool has_geometry{};};
    std::array<Entry,128> entries_{};
    mutable std::mutex mutex_;
    std::uint64_t maximum_id_{},retired_id_{};
    std::size_t count_{};
    static bool valid(StereoSample const& s,bool allow_sim) {
        return s.session&&s.sequence&&s.space_epoch&&s.received_ns>0&&stereo_render_active(s)&&
            stereo_tracking_valid(s,allow_sim)&&stereo_geometry_valid(s);
    }
    static bool valid(StereoTicketGeometry const& g) {
        if(!stereo_rigid(g.source_view)||!stereo_projection_supported(g.source_projection))return false;
        if(g.finite_aim.valid){
            if(!g.finite_aim.world||!g.finite_aim.tick)return false;
            for(float value:g.finite_aim.point)if(!std::isfinite(value))return false;
        }
        for(unsigned eye=0;eye<2;++eye)
            if(!stereo_rigid(g.eyes.view[eye])||!stereo_projection_supported(g.eyes.projection[eye]))return false;
        return true;
    }
    static bool same(StereoTicketGeometry const& a,StereoTicketGeometry const& b) {
        if(a.source_view!=b.source_view||a.source_projection!=b.source_projection||a.aiming!=b.aiming)return false;
        if(a.finite_aim.valid!=b.finite_aim.valid||a.finite_aim.point!=b.finite_aim.point||
           a.finite_aim.world!=b.finite_aim.world||a.finite_aim.tick!=b.finite_aim.tick)return false;
        for(unsigned eye=0;eye<2;++eye)
            if(a.eyes.view[eye]!=b.eyes.view[eye]||a.eyes.projection[eye]!=b.eyes.projection[eye])return false;
        return true;
    }
public:
    static constexpr std::size_t capacity=128;
    StereoTicketStore()=default;
    StereoTicketStore(StereoTicketStore const&)=delete;
    StereoTicketStore& operator=(StereoTicketStore const&)=delete;
    static bool accepted(TicketRetainResult result) {
        return result==TicketRetainResult::retained||result==TicketRetainResult::duplicate;
    }
    TicketRetainResult retain(std::uint64_t id,StereoSample const& sample,bool allow_sim=false,
                              StereoTicketGeometry const* geometry=nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!id||!valid(sample,allow_sim)||(geometry&&!valid(*geometry)))return TicketRetainResult::invalid;
        if(id<=retired_id_)return TicketRetainResult::stale;
        Entry* empty{};
        for(auto& entry:entries_) {
            if(entry.id==id)return stereo_same(entry.sample,sample)&&entry.has_geometry==(geometry!=nullptr)&&
                (!geometry||same(entry.geometry,*geometry))?TicketRetainResult::duplicate:TicketRetainResult::conflict;
            if(!entry.id&&!empty)empty=&entry;
        }
        // Existing retained duplicates may be older than maximum_id_. A new
        // insertion must be newer, even if that old id was never stored.
        if(id<=maximum_id_)return TicketRetainResult::stale;
        if(!empty)return TicketRetainResult::overflow;
        empty->sample=sample;empty->has_geometry=geometry!=nullptr;
        if(geometry)empty->geometry=*geometry;
        empty->id=id;maximum_id_=id;++count_;
        return TicketRetainResult::retained;
    }
    bool lookup(std::uint64_t id,StereoSample& sample,bool allow_sim=false) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!id)return false;
        for(auto const& entry:entries_)if(entry.id==id){if(!valid(entry.sample,allow_sim))return false;sample=entry.sample;return true;}
        return false; // Failed lookup leaves the caller's copy unchanged.
    }
    bool lookup_geometry(std::uint64_t id,StereoTicketGeometry& geometry,bool allow_sim=false) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if(!id)return false;
        for(auto const& entry:entries_)if(entry.id==id) {
            if(!entry.has_geometry||!valid(entry.sample,allow_sim)||!valid(entry.geometry))return false;
            geometry=entry.geometry;return true;
        }
        return false; // Missing/retired geometry leaves the caller's copy unchanged.
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
