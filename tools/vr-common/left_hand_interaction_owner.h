#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace gears_vr {

enum class LeftHandInteraction : std::uint8_t { none,support,ammo,magazine,cover,other };
struct LeftHandInteractionLease {
    LeftHandInteraction kind=LeftHandInteraction::none;
    std::uint64_t lifecycle=0,id=0;
};
struct LeftHandInteractionRequests {
    // Nonzero only for a current, uniquely eligible CoverInteraction request.
    std::uint64_t cover_request_id=0;
    bool support=false,ammo=false,magazine=false;
    // Authenticated unavailable part observation; this supplies no candidate/lease.
    bool magazine_observation_pending=false;
};
struct LeftHandInteractionFrame {
    // One authenticated game-thread owner's generations, NOT XR time or raw
    // session identities. Frame increases within each monotonic lifecycle.
    std::uint64_t lifecycle=0,frame=0;
    // Already decoded logical LGrip. A nonzero token is allocated once per
    // press, increases within a lifecycle, and stays unchanged through hold
    // and release. Input/space/profile discontinuities must invalidate context
    // or tracking before resuming. Never pass an epoch-reset raw press count.
    std::uint64_t press_token=0;
    bool context_valid=false,tracked=false,grip_down=false;
    // Certified still-valid leases, not proposed acquisitions. At most one may
    // exist; report any two when multiple holders conflict, never drop one to
    // manufacture a free hand. Empty entries have all-zero fields. A released
    // cover's held BODY OFFSET and a reload TIMER are not left-hand leases.
    std::array<LeftHandInteractionLease,2> incumbents{};
    LeftHandInteractionRequests requests;
    std::int64_t now_ns=0; // Actual owner clock, used only by the bounded wait.
};
enum class LeftHandInteractionStatus {
    released,retained,granted,needs_release,no_candidate,incumbent_conflict,
    candidate_conflict,inactive,lifecycle_changed,invalid,replayed,ambiguous,lease_lost
};
struct LeftHandInteractionDecision {
    // The ONLY authorized lease. None authorizes no acquisition/writer. On a
    // rejection the caller restores only resources it actually owns; this is
    // not permission to cancel another incumbent or choose between conflicts.
    LeftHandInteractionLease lease;
    LeftHandInteractionStatus status=LeftHandInteractionStatus::needs_release;
    std::uint64_t press_token=0;
    bool new_grant=false,release_required=true;
};

// Shared pure decision interface for the existing native/Playground owner.
// It is not cross-DLL transport or proof of native geometry/object validity.
// No analog input, poses, memory access, clocks or environment are consumed.
//
// Observe a valid release before new acquisition. A pre-existing certified
// holder may be retained on initial attachment, but no lease is resumed across
// an interruption until release. A grant must be reported as an incumbent on
// the NEXT frame if committed; a missing/changed lease cannot redirect the
// held press to another action. Valid cover requests outrank new weapon grabs.
//
// Advance once per frozen owner frame. Exact repeats return the same decision
// including new_grant; consumers commit once by (lifecycle,press_token,lease).
// For cover's two-stage handshake reuse that decision and re-call ONLY the
// CoverInteraction kernel with its request_id grant. Adding the newly acquired
// incumbent or changing requests under the same frame is a rejected mutation.
class LeftHandInteractionOwner {
public:
    // Observation only for the existing bounded owner receipt. Preserve the
    // first withdrawal cause while subsequent held frames say needs_release;
    // it cannot arm input, create a candidate or change an arbitration result.
    LeftHandInteractionStatus blocking_status() const noexcept {return blocking_status_;}
    // A failed external commit is a terminal interruption, not a second
    // arbitration of the same frame. Keep frame/token history and poison the
    // cached grant so retries cannot resurrect it. No incumbent is freed by
    // this logical decision: its participant still owns cleanup responsibility.
    LeftHandInteractionDecision invalidate() noexcept {
        return reject(LeftHandInteractionStatus::inactive,true);
    }

    LeftHandInteractionDecision update(LeftHandInteractionFrame const& input){
        if(!input.lifecycle||!input.frame)return reject(LeftHandInteractionStatus::invalid,true);
        const bool initial=!last_;
        if(last_){
            if(input.lifecycle<last_->lifecycle||
               (input.lifecycle==last_->lifecycle&&input.frame<last_->frame))
                return reject(LeftHandInteractionStatus::replayed,true);
            if(input.lifecycle==last_->lifecycle&&input.frame==last_->frame){
                if(!poisoned_&&same(input,*last_))return cached_;
                return reject(LeftHandInteractionStatus::replayed,true);
            }
        }
        const bool lifecycle_changed=last_&&input.lifecycle!=last_->lifecycle;
        last_=input;poisoned_=false;
        if(lifecycle_changed){
            last_press_=input.press_token;
            return reject(LeftHandInteractionStatus::lifecycle_changed,false);
        }
        if(!valid(input))return reject(LeftHandInteractionStatus::invalid,false);
        if(!input.context_valid||!input.tracked){
            if(input.press_token>last_press_)last_press_=input.press_token;
            return reject(LeftHandInteractionStatus::inactive,false);
        }
        if(input.press_token<last_press_)return reject(LeftHandInteractionStatus::replayed,false);
        if(!input.grip_down){
            pending_since_=pending_last_=0;
            last_press_=input.press_token;selected_={};previous_down_=false;armed_=true;blocked_=false;
            blocking_status_=LeftHandInteractionStatus::released;
            return save(LeftHandInteractionStatus::released,false);
        }
        const bool rising=!previous_down_;
        if(initial)last_press_=input.press_token;
        else if(rising){
            if(input.press_token<=last_press_)return reject(LeftHandInteractionStatus::replayed,false);
            last_press_=input.press_token;
        }else if(input.press_token!=last_press_){
            last_press_=input.press_token;
            return reject(LeftHandInteractionStatus::ambiguous,false);
        }
        previous_down_=true;
        const bool deferred=pending_since_>0;
        if(deferred&&(input.now_ns<=pending_last_||input.now_ns<pending_since_||
           input.now_ns-pending_since_>500000000))return reject(LeftHandInteractionStatus::needs_release,false);
        if(deferred)pending_last_=input.now_ns;
        unsigned count=0;LeftHandInteractionLease incumbent{};
        for(auto const& lease:input.incumbents)if(lease.kind!=LeftHandInteraction::none){++count;incumbent=lease;}
        if(count>1)return reject(LeftHandInteractionStatus::incumbent_conflict,false);
        if(deferred&&count)return reject(LeftHandInteractionStatus::incumbent_conflict,false);
        if(blocked_)return save(LeftHandInteractionStatus::needs_release,false);
        if(selected_.kind!=LeftHandInteraction::none){
            if(count==1&&same(incumbent,selected_))return save(LeftHandInteractionStatus::retained,false);
            return reject(count?LeftHandInteractionStatus::incumbent_conflict:LeftHandInteractionStatus::lease_lost,false);
        }
        if(count==1){
            selected_=incumbent;
            return save(LeftHandInteractionStatus::retained,false);
        }
        if(!armed_||(!rising&&!deferred))return reject(LeftHandInteractionStatus::needs_release,false);
        LeftHandInteraction chosen=LeftHandInteraction::none;
        auto const& request=input.requests;
        if(request.cover_request_id)chosen=LeftHandInteraction::cover;
        else{
            const unsigned candidates=unsigned(request.support)+unsigned(request.ammo)+unsigned(request.magazine);
            if(candidates>1)return reject(LeftHandInteractionStatus::candidate_conflict,false);
            if(request.support)chosen=LeftHandInteraction::support;
            if(request.ammo)chosen=LeftHandInteraction::ammo;
            if(request.magazine)chosen=LeftHandInteraction::magazine;
        }
        //46860: the unresolved magazine pair initially hid an otherwise
        // valid fore-end proposal. Once that observation resolves, allow the
        // SAME undispatched press to acquire its uniquely certified support
        // contact within a bounded500ms press-intent window. This stores no
        // pose or authority: the caller must still certify fresh actual contact
        // under its unchanged source-age limits on the granting frame.
        // Unknown/ambiguous contact,
        // another owner, ammo/cover, timeout or any real interruption still
        // cannot redirect the press. No incumbent has existed on this path.
        const bool resolved_support=chosen==LeftHandInteraction::support&&!request.magazine_observation_pending;
        if(deferred&&chosen!=LeftHandInteraction::none&&chosen!=LeftHandInteraction::magazine&&!resolved_support)
            return reject(LeftHandInteractionStatus::needs_release,false);
        if(chosen==LeftHandInteraction::none){
            if(input.requests.magazine_observation_pending&&input.now_ns>0){
                if(!deferred)pending_since_=pending_last_=input.now_ns;
                // Preserve only this press. A later frozen frame must supply
                // current actual contact; known no-contact consumes the press.
                return save(LeftHandInteractionStatus::no_candidate,false);
            }
            return reject(LeftHandInteractionStatus::no_candidate,false);
        }
        pending_since_=pending_last_=0;
        selected_={chosen,input.lifecycle,chosen==LeftHandInteraction::cover?request.cover_request_id:input.press_token};
        armed_=false;
        return save(LeftHandInteractionStatus::granted,true);
    }

private:
    static bool same(LeftHandInteractionLease a,LeftHandInteractionLease b){
        return a.kind==b.kind&&a.lifecycle==b.lifecycle&&a.id==b.id;
    }
    static bool same(LeftHandInteractionFrame const& a,LeftHandInteractionFrame const& b){
        return a.lifecycle==b.lifecycle&&a.frame==b.frame&&a.press_token==b.press_token&&
            a.context_valid==b.context_valid&&a.tracked==b.tracked&&a.grip_down==b.grip_down&&
            same(a.incumbents[0],b.incumbents[0])&&same(a.incumbents[1],b.incumbents[1])&&
            a.requests.cover_request_id==b.requests.cover_request_id&&a.requests.support==b.requests.support&&
            a.requests.ammo==b.requests.ammo&&a.requests.magazine==b.requests.magazine&&
            a.requests.magazine_observation_pending==b.requests.magazine_observation_pending&&a.now_ns==b.now_ns;
    }
    static bool valid(LeftHandInteractionFrame const& input){
        if(input.grip_down&&!input.press_token)return false;
        for(auto const& lease:input.incumbents){
            if(unsigned(lease.kind)>unsigned(LeftHandInteraction::other))return false;
            if(lease.kind==LeftHandInteraction::none){if(lease.lifecycle||lease.id)return false;}
            else if(lease.lifecycle!=input.lifecycle||!lease.id)return false;
        }
        return true;
    }
    LeftHandInteractionDecision save(LeftHandInteractionStatus status,bool new_grant){
        cached_={selected_,status,last_press_,new_grant,blocked_||(!armed_&&selected_.kind==LeftHandInteraction::none)};
        return cached_;
    }
    LeftHandInteractionDecision reject(LeftHandInteractionStatus status,bool poison){
        if(!blocked_)blocking_status_=status;
        selected_={};armed_=false;blocked_=true;previous_down_=true;
        pending_since_=pending_last_=0;
        poisoned_=poison;
        return save(status,false);
    }
    std::optional<LeftHandInteractionFrame> last_;
    LeftHandInteractionDecision cached_;
    LeftHandInteractionLease selected_;
    std::uint64_t last_press_=0;
    std::int64_t pending_since_=0,pending_last_=0;
    bool previous_down_=true,armed_=false,blocked_=false,poisoned_=false;
    LeftHandInteractionStatus blocking_status_=LeftHandInteractionStatus::released;
};
} // namespace gears_vr
