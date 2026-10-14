#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include "projection_control.h"
#include "origin_trial.h"
#include "../vr-common/stereo_frame.h"

// Tracks caller-owned renderers, never frees them. A true close/retire return
// grants one state-release claim, which must be executed on a native thread.
namespace gears_submitted {
struct FrameTicket {bool enabled{};std::uint64_t id{};gears_vr::StereoSample sample{};gears_vr::StereoMatrices expected{};
    gears_vr::StereoMatrix source_view{},source_projection{};
    std::uint64_t reference_generation{};gears_vr::Pose reference{};gears_origin_trial::Metadata origin{};};
class Lifetime {
    std::mutex mutex;
    std::array<uintptr_t,64> roots{};
    std::array<gears_projection::Metadata,64> projections{};
    std::array<FrameTicket,64> frames{};
    bool closed{},claimed{},stop_requested{};
    unsigned count{};
    bool claim() {if(closed && !count && !claimed){claimed=true;return true;}return false;}
public:
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
    bool close() {std::lock_guard<std::mutex> lock(mutex);closed=true;return claim();}
    // Worker-safe admission barrier. Only native close/retire can grant release.
    void request_stop() {std::lock_guard<std::mutex> lock(mutex);stop_requested=true;}
    bool stopping() {std::lock_guard<std::mutex> lock(mutex);return stop_requested;}
    unsigned pending() {std::lock_guard<std::mutex> lock(mutex);return count;}
    bool is_closed() {std::lock_guard<std::mutex> lock(mutex);return closed;}
};
}
