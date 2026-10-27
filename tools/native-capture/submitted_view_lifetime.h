#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include "crosshair_scene_evidence.h"
#include "projection_control.h"
#include "ngx_native_create_source.h"
#include "origin_trial.h"
#include "source_output_extent.h"
#include "../vr-common/stereo_frame.h"
#include "../vr-common/stereo_ticket_store.h"

// Tracks caller-owned renderers, never frees them. A true close/retire return
// grants one state-release claim, which must be executed on a native thread.
namespace gears_submitted {
struct FrameTicket {bool enabled{};std::uint64_t id{};gears_vr::StereoSample sample{};gears_vr::StereoMatrices expected{};
    // Frozen anchor mode for CPU diagnostics only; never part of a bridge ABI.
    bool unpossessed{};
    std::uint64_t world_generation{}; // Frozen internal world identity; no bridge ABI change.
    gears_source_output::Extent source_output{}; // Immutable original engine extent, independent of Present.
    gears_vr::StereoMatrix source_view{},source_projection{};
    std::uint64_t reference_generation{};gears_vr::Pose reference{};gears_origin_trial::Metadata origin{};
    std::optional<gears_crosshair_scene::Metadata> crosshair;
    gears_vr::StereoFiniteAim finite_aim{};
    bool aiming{};
    // Immutable completed Create parameters for reduced constructor inputs.
    // CPU identity only: owns no feature, resource, GPU work or producer proof.
    gears_ngx_create::NativeCreateSource ngx_constructor_source{};};
class Lifetime {
    std::mutex mutex;
    std::array<uintptr_t,64> roots{};
    std::array<gears_projection::Metadata,64> projections{};
    std::array<FrameTicket,64> frames{};
    bool closed{},claimed{},stop_requested{},permanent_closed{},quiescing{},destruction_confirmed{};
    std::uint64_t generation_{1};
    unsigned count{};
    bool claim() {if(closed && !count && !claimed){claimed=true;return true;}return false;}
public:
    struct QuiesceToken {Lifetime const* owner{};std::uint64_t generation{};};
    struct DestroyedToken {Lifetime const* owner{};std::uint64_t generation{};};
    struct QuiesceResult {bool accepted{},release_claim{};QuiesceToken token{};};
    struct Snapshot {std::uint64_t generation{};unsigned pending{};bool closed{},claimed{},stop_requested{},permanent_closed{},quiescing{},destruction_confirmed{};};
    Snapshot snapshot(){std::lock_guard<std::mutex> lock(mutex);
        return {generation_,count,closed,claimed,stop_requested,permanent_closed,quiescing,destruction_confirmed};}
    // Native caller executes a returned release claim, just as with retire().
    QuiesceResult quiesce_close(std::uint64_t expected_generation){
        std::lock_guard<std::mutex> lock(mutex);
        if(expected_generation!=generation_||stop_requested||permanent_closed||generation_==UINT64_MAX)return {};
        closed=true;quiescing=true;return {true,claim(),{this,generation_}};
    }
    // Explicit caller assertion: native state release AND destructor completed.
    // The store cannot observe that external lifecycle. Never call on request
    // or release-return alone. This method itself performs no native cleanup.
    DestroyedToken confirm_destroyed(QuiesceToken token){
        std::lock_guard<std::mutex> lock(mutex);
        if(token.owner!=this||token.generation!=generation_||!quiescing||!closed||count||!claimed||
           stop_requested||permanent_closed||destruction_confirmed)return {};
        destruction_confirmed=true;return {this,generation_};
    }
    bool reopen(DestroyedToken token){
        std::lock_guard<std::mutex> lock(mutex);
        if(token.owner!=this||token.generation!=generation_||!quiescing||!closed||count||!claimed||
           !destruction_confirmed||stop_requested||permanent_closed||generation_==UINT64_MAX)return false;
        ++generation_;closed=claimed=quiescing=destruction_confirmed=false;return true;
    }
    bool begin(uintptr_t root,const gears_projection::Metadata& projection={},const FrameTicket& frame={}) {
        std::lock_guard<std::mutex> lock(mutex);
        if(!root || closed || stop_requested || count==roots.size())return false;
        for(auto value:roots)if(value==root)return false;
        for(unsigned i=0;i<roots.size();++i)if(!roots[i]){roots[i]=root;projections[i]=projection;frames[i]=frame;++count;return true;}
        return false;
    }
    bool contains(uintptr_t root) {
        std::lock_guard<std::mutex> lock(mutex);
        if(!root)return false;
        for(auto value:roots)if(value==root)return true;
        return false;
    }
    bool retire(uintptr_t root) {
        std::lock_guard<std::mutex> lock(mutex);
        if(!root)return false;
        for(unsigned i=0;i<roots.size();++i)if(roots[i]==root){roots[i]=0;projections[i]={};frames[i]={};--count;return claim();}
        return false;
    }
    bool projection(uintptr_t root,gears_projection::Metadata& out) {
        std::lock_guard<std::mutex> lock(mutex);
        if(root)for(unsigned i=0;i<roots.size();++i)if(roots[i]==root){out=projections[i];return out.enabled;}
        out={};return false;
    }
    bool frame(uintptr_t root,FrameTicket& out) {
        std::lock_guard<std::mutex> lock(mutex);
        if(root)for(unsigned i=0;i<roots.size();++i)if(roots[i]==root){out=frames[i];return out.enabled;}
        out={};return false;
    }
    bool close() {std::lock_guard<std::mutex> lock(mutex);permanent_closed=true;closed=true;return claim();}
    // Worker-safe admission barrier. Only native close/retire can grant release.
    void request_stop() {std::lock_guard<std::mutex> lock(mutex);stop_requested=true;}
    bool stopping() {std::lock_guard<std::mutex> lock(mutex);return stop_requested;}
    unsigned pending() {std::lock_guard<std::mutex> lock(mutex);return count;}
    bool is_closed() {std::lock_guard<std::mutex> lock(mutex);return closed;}
};
}
