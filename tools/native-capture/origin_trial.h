#pragma once
#include "../vr-common/view_origin.h"
#include "anchor_observer.h"
#include <cstdint>

// Pure CPU policy; the native caller serializes select/begin/commit. No clock,
// lock, lifetime operation or GPU ABI is owned by this helper.
namespace gears_origin_trial {
enum class Phase : unsigned {a1,b,a2};
struct Selection {
    Phase phase{Phase::a1};
    std::int64_t phase_qpc{},epoch_qpc{},frequency{};
};
inline bool valid(Selection const& s) {
    if(s.frequency<=0||s.epoch_qpc<=0||s.phase_qpc<s.epoch_qpc)return false;
    // Both counters are positive and ordered, so subtraction cannot overflow.
    // Division avoids overflowing 10*frequency or 20*frequency.
    auto seconds=(s.phase_qpc-s.epoch_qpc)/s.frequency;
    auto expected=seconds<10?Phase::a1:seconds<20?Phase::b:Phase::a2;
    return s.phase==expected;
}
class Trial {
    std::int64_t epoch_qpc_{},frequency_{},last_committed_qpc_{};
public:
    std::int64_t epoch_qpc() const {return epoch_qpc_;}
    // Select exactly once before constructing the corresponding candidate.
    // Until a candidate begins successfully, every candidate has its own A1
    // epoch. Failure leaves out and all trial state unchanged.
    bool select(std::int64_t qpc,std::int64_t frequency,Selection& out) const {
        if(qpc<=0||frequency<=0||qpc<last_committed_qpc_||
            (epoch_qpc_&&frequency!=frequency_))return false;
        Selection candidate{Phase::a1,qpc,epoch_qpc_?epoch_qpc_:qpc,frequency};
        auto seconds=(candidate.phase_qpc-candidate.epoch_qpc)/frequency;
        candidate.phase=seconds<10?Phase::a1:seconds<20?Phase::b:Phase::a2;
        out=candidate;return true;
    }
    // Call only with the result of this candidate's Lifetime::begin. The
    // selected phase is retained on the ticket through late consumption.
    // First successful admission commits its selection epoch, never now().
    bool commit(Selection const& selected,bool lifetime_begin_succeeded) {
        if(!lifetime_begin_succeeded||!valid(selected))return false;
        Selection expected{};
        if(!select(selected.phase_qpc,selected.frequency,expected)||
            selected.epoch_qpc!=expected.epoch_qpc||selected.phase!=expected.phase)return false;
        if(!epoch_qpc_){epoch_qpc_=selected.epoch_qpc;frequency_=selected.frequency;}
        last_committed_qpc_=selected.phase_qpc;
        return true;
    }
};
// View only: projection and later per-eye stereo construction remain caller
// concerns. A phases use this ticket's original view, not a saved A1 matrix.
// B changes only translation in a local copy via the established error bound.
inline bool base_view(Selection const& selected,gears_vr::StereoMatrix const& original,
    gears_vr::Vector candidate_eye,gears_vr::StereoMatrix& out) {
    if(!valid(selected)||!gears_vr::stereo_rigid(original))return false;
    gears_vr::StereoMatrix candidate=original;
    if(selected.phase==Phase::b&&!gears_vr::view_with_origin(original,candidate_eye,candidate))return false;
    out=candidate;return true;
}
// Native-only metadata: copied with the admitted renderer, never through the
// GPU shared-memory ABI. The receipt belongs to this exact renderer invocation.
struct Metadata {
    bool enabled{};
    Selection selection{};
    gears_anchor::Receipt receipt{};
    gears_vr::StereoMatrix base{};
};
inline bool prepare(Trial const& trial,gears_anchor::Receipt const* receipt,
    std::uintptr_t family,std::uintptr_t source,std::uintptr_t local,std::uint32_t thread,
    gears_vr::StereoMatrix const& view,gears_vr::StereoMatrix const& projection,
    std::int64_t qpc,std::int64_t frequency,Metadata& out) {
    if(!receipt||!receipt->invocation||!thread||receipt->thread!=thread||!family||!source||!local||
        receipt->key.family!=family||receipt->key.owner!=family||receipt->key.source!=source||receipt->key.local!=local||
        receipt->scene_qpc<=0||receipt->family_qpc<receipt->scene_qpc||receipt->renderer_qpc<receipt->family_qpc||qpc<receipt->renderer_qpc||
        std::memcmp(view.data(),receipt->renderer_matrices.data(),64)||
        std::memcmp(projection.data(),receipt->renderer_matrices.data()+64,64))return false;
    Metadata candidate{};
    if(!trial.select(qpc,frequency,candidate.selection))return false;
    const auto& a=receipt->anchor;
    for(float v:a.location)if(!std::isfinite(v))return false;
    if(!std::isfinite(a.eye_height))return false;
    // Match the observed candidate's float addition; this is not an eye bone.
    gears_vr::Vector eye{a.location[0],a.location[1],a.location[2]+a.eye_height};
    if(!std::isfinite(eye.z)||!base_view(candidate.selection,view,eye,candidate.base))return false;
    candidate.enabled=true;candidate.receipt=*receipt;out=candidate;return true;
}
}
