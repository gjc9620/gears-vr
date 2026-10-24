#pragma once
#include "pose_math.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>

namespace gears_vr {

// This kernel owns only the virtual grip offset, in metres in cover-local
// right/up coordinates. It never accepts, filters, resets or bounds HMD motion.
// The game owner must certify the cover, geometry and action permissions; these
// types are not native object layouts or a substitute for collision/exposure.
struct CoverOffset { double right=0,up=0; };
struct CoverRange { double min_right=0,max_right=0,min_up=0,max_up=0; };
enum class CoverEdgeKind { none,left,right,top };

// Nonzero monotonic generations, maintained by adapters, not native pointers.
// world/pawn/cover changes invalidate the persistent offset. A cover generation
// identifies one common neutral/basis for its complete legal range. Slot and
// reference changes interrupt a grip but may retain the re-certified offset in
// that same basis; changing the neutral/basis requires a new cover generation.
struct CoverEpochs {
    std::uint64_t world=0,pawn=0,cover=0,slot=0,session=0,space=0,reference=0;
};
struct CoverEdgeObservation {
    // Nonzero only for one uniquely nominated edge. The owner must reject
    // ambiguous edges, wrong-side approaches, high-wall tops and blocked reach.
    // While requested/grabbed it must certify this same edge, even after the
    // real hand leaves acquisition reach. A different edge ends the old grip.
    std::uint64_t id=0;
    CoverEdgeKind kind=CoverEdgeKind::none;
    bool allowed=false;
    bool within_acquisition_reach=false;
};
struct CoverObservation {
    std::uint64_t sequence=0;
    std::int64_t observed_ns=0;
    CoverEpochs epochs;
    bool cover_valid=false;
    // False during pause, loss of XR/head availability, or input suspension.
    // Actual cover/world/pawn loss or special-action takeover uses cover_valid
    // false (or a new world/pawn/cover epoch), which also clears the offset.
    bool interaction_enabled=false;
    // The entire rectangle must be legal, including combinations of both axes.
    // Bounds use one cover neutral, never a fresh neutral for each regrab.
    CoverRange range;
    // Unit, orthogonal axes expressed in the SAME raw tracking space as the
    // hand. OpenXR uses +X right/+Y up/-Z forward; these axes can be rotated for
    // the actual cover/body reference. Neither may follow the live head angle.
    Vector right_axis_tracking{1,0,0},up_axis_tracking{0,1,0};
    CoverEdgeObservation edge;
};
struct CoverHandSample {
    std::uint64_t sequence=0;
    std::int64_t observed_ns=0;
    std::uint64_t session_epoch=0,space_epoch=0;
    Vector left_position_metres;
    bool input_active=false,position_valid=false,position_tracked=false;
    bool lgrip_down=false;
};
enum class CoverLeftHandOwner { free,cover,other };
struct CoverOwnership {
    CoverLeftHandOwner owner=CoverLeftHandOwner::free;
    // Only an actual grant of this kernel's current request can create a grip.
    std::uint64_t request_id=0;
};
enum class CoverPhase { outside,unavailable,attached,held,requesting,grabbing };
enum class CoverStatus {
    okay,no_cover,inactive,stale,replayed,invalid,epoch_changed,
    needs_release,edge_unavailable,ownership_conflict
};
struct CoverResult {
    CoverOffset offset;
    CoverPhase phase=CoverPhase::outside;
    CoverStatus status=CoverStatus::no_cover;
    // False means that this update did not certify an applicable cover offset.
    // Keep transition continuity in the game owner; do not apply an uncertified
    // value as a new camera/body state or infer that clearing it is a teleport.
    bool offset_certified=false;
    bool request_left_hand=false,owns_left_hand=false;
    std::uint64_t request_id=0,edge_id=0;
};

class CoverInteraction {
public:
    explicit CoverInteraction(double gain=1,std::int64_t maximum_age_ns=100000000)
        : gain_(gain),maximum_age_ns_(maximum_age_ns) {
        if(!std::isfinite(gain_)||gain_<=0||maximum_age_ns_<=0)
            throw std::invalid_argument("Invalid cover interaction configuration");
    }

    // Abort publication without replaying or changing an already consumed
    // hand sample. Retain only the last committed offset; a later observation
    // must certify it again, and a valid release must precede another grip.
    CoverResult suspend() noexcept {
        offset_certified_=false;interrupt();return result(CoverStatus::inactive);
    }

    // Both producers and now_ns must use the same monotonic clock. Cover
    // sequences are monotonic for this owner's lifetime; hand sequences may
    // restart only on a new session/space epoch. Exact newest-sample repeats
    // are allowed while fresh. Mutating their payload, rolling sequence/time
    // backwards, future stamps and regressing epochs all interrupt the grip.
    //
    // Start with an observed valid release, then a press requests ownership.
    // The caller arbitrates with weapon/reload/etc. and may call again with
    // the EXACT SAME samples plus a matching grant. A delayed grant anchors at
    // the then-current hand/offset, so arbitration cannot introduce a jump.
    // On every result that relinquishes ownership, the caller releases its
    // grant; a request alone never owns the hand. No external state is mutated.
    CoverResult update(CoverObservation const& cover,CoverHandSample const& hand,
                       CoverOwnership ownership,std::int64_t now_ns) {
        offset_certified_=false;
        if(now_ns<=0||now_ns<last_now_ns_)return stop(CoverStatus::stale);
        last_now_ns_=now_ns;
        auto status=check_stream(cover,last_cover_,now_ns);
        if(status!=CoverStatus::okay)return stop(status);
        last_cover_=cover;
        if(!cover.cover_valid){
            have_cover_=false;offset_={};interrupt();
            return result(CoverStatus::no_cover);
        }
        if(!valid(cover.epochs)||!valid(cover.range)||
           !valid_axes(cover.right_axis_tracking,cover.up_axis_tracking))
            return stop(CoverStatus::invalid);
        if(epochs_&&regressed(cover.epochs,*epochs_))return stop(CoverStatus::replayed);

        const bool changed=epochs_&&!same(cover.epochs,*epochs_);
        const bool source_changed=epochs_&&(cover.epochs.session!=epochs_->session||
                                                          cover.epochs.space!=epochs_->space);
        if(!have_cover_||!epochs_||cover.epochs.world!=epochs_->world||
           cover.epochs.pawn!=epochs_->pawn||cover.epochs.cover!=epochs_->cover){
            offset_={};interrupt();
        }else if(changed)interrupt();
        if(source_changed)last_hand_.reset();
        epochs_=cover.epochs;have_cover_=true;
        offset_=clamped(offset_,cover.range);
        offset_certified_=true;
        if(changed)return result(CoverStatus::epoch_changed);
        if(!cover.interaction_enabled)return stop(CoverStatus::inactive);
        if(hand.session_epoch!=cover.epochs.session||hand.space_epoch!=cover.epochs.space)
            return stop(CoverStatus::epoch_changed);
        status=check_stream(hand,last_hand_,now_ns);
        if(status!=CoverStatus::okay)return stop(status);
        last_hand_=hand;
        if(!hand.input_active||!hand.position_valid||!hand.position_tracked)
            return stop(CoverStatus::inactive);
        if(!finite(hand.left_position_metres))return stop(CoverStatus::invalid);

        if(!hand.lgrip_down){
            interrupt();release_required_=false;previous_down_=false;
            return result(CoverStatus::okay);
        }
        const bool rising=!previous_down_;
        previous_down_=true;
        if(release_required_)return result(CoverStatus::needs_release);

        if(grabbing_||requesting_){
            if(!available(cover.edge)||cover.edge.id!=edge_id_||cover.edge.kind!=edge_kind_)
                return stop(CoverStatus::edge_unavailable);
            if(grabbing_){
                if(ownership.owner!=CoverLeftHandOwner::cover||ownership.request_id!=request_id_)
                    return stop(CoverStatus::ownership_conflict);
                CoverOffset candidate=grab_offset_;
                const Vector delta{hand.left_position_metres.x-grab_hand_.x,
                    hand.left_position_metres.y-grab_hand_.y,hand.left_position_metres.z-grab_hand_.z};
                const double motion=gain_*dot(delta,grab_axis_);
                if(!finite(delta)||!std::isfinite(motion))return stop(CoverStatus::invalid);
                if(edge_kind_==CoverEdgeKind::top)candidate.up-=motion;
                else candidate.right-=motion;
                if(!std::isfinite(candidate.right)||!std::isfinite(candidate.up))return stop(CoverStatus::invalid);
                offset_=clamped(candidate,cover.range);
                return result(CoverStatus::okay);
            }
            if(!cover.edge.within_acquisition_reach)return stop(CoverStatus::edge_unavailable);
            if(ownership.owner==CoverLeftHandOwner::free)return result(CoverStatus::okay);
            if(ownership.owner!=CoverLeftHandOwner::cover||ownership.request_id!=request_id_)
                return stop(CoverStatus::ownership_conflict);
            requesting_=false;grabbing_=true;
            grab_hand_=hand.left_position_metres;grab_offset_=offset_;
            grab_axis_=edge_kind_==CoverEdgeKind::top?cover.up_axis_tracking:cover.right_axis_tracking;
            return result(CoverStatus::okay);
        }
        if(!rising)return result(CoverStatus::needs_release);
        if(ownership.owner!=CoverLeftHandOwner::free)return stop(CoverStatus::ownership_conflict);
        if(!available(cover.edge)||!cover.edge.within_acquisition_reach)
            return stop(CoverStatus::edge_unavailable);
        if(next_request_id_==(std::numeric_limits<std::uint64_t>::max)())return stop(CoverStatus::invalid);
        request_id_=++next_request_id_;edge_id_=cover.edge.id;edge_kind_=cover.edge.kind;
        requesting_=true;
        return result(CoverStatus::okay);
    }

private:
    static double dot(Vector a,Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;}
    static bool available(CoverEdgeObservation const& edge){
        return edge.id&&edge.allowed&&(edge.kind==CoverEdgeKind::left||
            edge.kind==CoverEdgeKind::right||edge.kind==CoverEdgeKind::top);
    }
    static bool valid_axes(Vector right,Vector up){
        constexpr double tolerance=1e-5; // Numeric rigid-basis tolerance, not geometry tuning.
        return finite(right)&&finite(up)&&std::abs(dot(right,right)-1)<=tolerance&&
            std::abs(dot(up,up)-1)<=tolerance&&std::abs(dot(right,up))<=tolerance;
    }
    static bool valid(CoverRange const& r){
        return std::isfinite(r.min_right)&&std::isfinite(r.max_right)&&
            std::isfinite(r.min_up)&&std::isfinite(r.max_up)&&
            r.min_right<=0&&r.max_right>=0&&r.min_up<=0&&r.max_up>=0;
    }
    static bool valid(CoverEpochs const& e){
        return e.world&&e.pawn&&e.cover&&e.slot&&e.session&&e.space&&e.reference;
    }
    static bool regressed(CoverEpochs const& a,CoverEpochs const& b){
        return a.world<b.world||a.pawn<b.pawn||a.cover<b.cover||a.slot<b.slot||
            a.session<b.session||a.space<b.space||a.reference<b.reference;
    }
    static bool same(CoverEpochs const& a,CoverEpochs const& b){
        return a.world==b.world&&a.pawn==b.pawn&&a.cover==b.cover&&a.slot==b.slot&&
            a.session==b.session&&a.space==b.space&&a.reference==b.reference;
    }
    static bool same_number(double a,double b){return std::memcmp(&a,&b,sizeof(a))==0;}
    static bool same(Vector a,Vector b){return same_number(a.x,b.x)&&same_number(a.y,b.y)&&same_number(a.z,b.z);}
    static bool same(CoverObservation const& a,CoverObservation const& b){
        return a.sequence==b.sequence&&a.observed_ns==b.observed_ns&&same(a.epochs,b.epochs)&&
            a.cover_valid==b.cover_valid&&a.interaction_enabled==b.interaction_enabled&&
            same_number(a.range.min_right,b.range.min_right)&&same_number(a.range.max_right,b.range.max_right)&&
            same_number(a.range.min_up,b.range.min_up)&&same_number(a.range.max_up,b.range.max_up)&&
            same(a.right_axis_tracking,b.right_axis_tracking)&&same(a.up_axis_tracking,b.up_axis_tracking)&&
            a.edge.id==b.edge.id&&a.edge.kind==b.edge.kind&&a.edge.allowed==b.edge.allowed&&
            a.edge.within_acquisition_reach==b.edge.within_acquisition_reach;
    }
    static bool same(CoverHandSample const& a,CoverHandSample const& b){
        return a.sequence==b.sequence&&a.observed_ns==b.observed_ns&&a.session_epoch==b.session_epoch&&
            a.space_epoch==b.space_epoch&&same(a.left_position_metres,b.left_position_metres)&&
            a.input_active==b.input_active&&a.position_valid==b.position_valid&&
            a.position_tracked==b.position_tracked&&a.lgrip_down==b.lgrip_down;
    }
    template<class Sample> CoverStatus check_stream(Sample const& sample,
                       std::optional<Sample> const& last,std::int64_t now_ns) const {
        if(sample.observed_ns<=0||sample.observed_ns>now_ns||now_ns-sample.observed_ns>maximum_age_ns_)
            return CoverStatus::stale;
        if(!sample.sequence)return CoverStatus::replayed;
        if(last&&(sample.sequence<last->sequence||
           (sample.sequence==last->sequence&&!same(sample,*last))||
           (sample.sequence>last->sequence&&sample.observed_ns<last->observed_ns)))
            return CoverStatus::replayed;
        return CoverStatus::okay;
    }
    static CoverOffset clamped(CoverOffset d,CoverRange const& r){
        return {std::clamp(d.right,r.min_right,r.max_right),std::clamp(d.up,r.min_up,r.max_up)};
    }
    void interrupt(){
        grabbing_=false;requesting_=false;release_required_=true;previous_down_=true;
        request_id_=0;edge_id_=0;edge_kind_=CoverEdgeKind::none;
    }
    CoverResult stop(CoverStatus status){interrupt();return result(status);}
    CoverResult result(CoverStatus status) const {
        CoverPhase phase=CoverPhase::outside;
        if(have_cover_){
            if(!offset_certified_)phase=CoverPhase::unavailable;
            else if(grabbing_)phase=CoverPhase::grabbing;
            else if(requesting_)phase=CoverPhase::requesting;
            // Classification tolerance only; do not round away any body motion.
            else phase=std::abs(offset_.right)<=1e-12&&std::abs(offset_.up)<=1e-12?CoverPhase::attached:CoverPhase::held;
        }
        return {offset_,phase,status,offset_certified_,requesting_,grabbing_,request_id_,edge_id_};
    }

    double gain_;
    std::int64_t maximum_age_ns_,last_now_ns_=0;
    std::uint64_t next_request_id_=0,request_id_=0,edge_id_=0;
    bool have_cover_=false,offset_certified_=false,release_required_=true,previous_down_=true;
    bool requesting_=false,grabbing_=false;
    CoverEdgeKind edge_kind_=CoverEdgeKind::none;
    CoverOffset offset_,grab_offset_;
    Vector grab_hand_,grab_axis_;
    std::optional<CoverEpochs> epochs_;
    std::optional<CoverObservation> last_cover_;
    std::optional<CoverHandSample> last_hand_;
};
} // namespace gears_vr
