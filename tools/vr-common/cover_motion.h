#pragma once
#include "cover_interaction.h"

// Owned protocol data, not engine layouts or permission flags. The Native
// adapter must resolve the registrations to full current identities, source,
// real collision/occupancy evidence and the unique C0 consumer. No input
// ticket, broker nonce, pointer to a callback stack or old lease is retained.
namespace gears_cover_motion {
using Offset=gears_vr::CoverOffset;
struct Binding {
    gears_vr::CoverEpochs epochs{};
    std::uint64_t context{},basis{}; // actual owner's private registrations
};
struct Hand {
    gears_vr::CoverHandSample sample{};
    std::uint64_t grab{},edge{}; // association only, never a hand lease
    gears_vr::CoverEdgeKind axis{gears_vr::CoverEdgeKind::none};
};
struct Intent {
    Binding binding{};
    std::uint64_t id{},from_event{},from_version{};
    Offset from{},requested{};
    Hand hand{};
};
enum class Completion {baseline,accepted,clipped};
struct Commit {
    Binding binding{};
    std::uint64_t event{},previous_event{},version{},from_version{},intent{};
    // Opaque references issued by the actual body/occupancy owners. The
    // backend must verify their evidence; nonzero integers alone prove nothing.
    std::uint64_t body_publication{},occupancy{};
    Completion completion{Completion::baseline};
    Offset from{},requested{},position{};
    Hand hand{};
};
enum class Observation {unchanged,committed,uncertain,unavailable};
enum class Admission {current,unavailable,rejected};
class Backend {
public:
    virtual ~Backend()=default;
    // Read the unique body's completed publication AFTER this event, before
    // processing the next input. committed means authenticated actual motion,
    // not a queued query or a successful input transaction. An exception or
    // unknown partial write is uncertain; it is never proof of rollback.
    virtual Observation observe(std::uint64_t after,Commit& out) noexcept=0;
    // Authenticate current context/input, the input owner's exact read-only
    // candidate and actual logical hand ownership. Caller-supplied hand/offset
    // values by themselves cannot certify that candidate.
    // This grants no future Tick execution right. The body submitter must
    // independently revalidate this exact immutable intent, its live logical
    // lease, geometry/occupancy and from-version at actual execution.
    virtual Admission admit(Intent const&,std::int64_t now_ns) noexcept=0;
};
class UnavailableBackend final:public Backend {
public:
    Observation observe(std::uint64_t,Commit&) noexcept override{return Observation::unavailable;}
    Admission admit(Intent const&,std::int64_t) noexcept override{return Admission::unavailable;}
};
enum class Code {ready,no_motion,pending,waiting,cancelled,unavailable,rejected,stale,recovery_required,exhausted};
namespace detail {
constexpr std::uint64_t next_sequence(std::uint64_t value) noexcept{return value==UINT64_MAX?0:value+1;}
inline bool same(Offset a,Offset b) noexcept{return a.right==b.right&&a.up==b.up;}
inline bool finite(Offset a) noexcept{return std::isfinite(a.right)&&std::isfinite(a.up);}
inline bool same(Binding const& a,Binding const& b) noexcept {
    auto const& x=a.epochs;auto const& y=b.epochs;
    return a.context==b.context&&a.basis==b.basis&&x.world==y.world&&x.pawn==y.pawn&&
        x.cover==y.cover&&x.slot==y.slot&&x.session==y.session&&x.space==y.space&&x.reference==y.reference;
}
inline bool valid(Binding const& b) noexcept {
    auto const& e=b.epochs;
    return b.context&&b.basis&&e.world&&e.pawn&&e.cover&&e.slot&&e.session&&e.space&&e.reference;
}
inline bool same(Hand const& a,Hand const& b) noexcept {
    auto const& x=a.sample;auto const& y=b.sample;
    return a.grab==b.grab&&a.edge==b.edge&&a.axis==b.axis&&x.sequence==y.sequence&&x.observed_ns==y.observed_ns&&
        x.session_epoch==y.session_epoch&&x.space_epoch==y.space_epoch&&
        x.left_position_metres.x==y.left_position_metres.x&&x.left_position_metres.y==y.left_position_metres.y&&
        x.left_position_metres.z==y.left_position_metres.z&&x.input_active==y.input_active&&
        x.position_valid==y.position_valid&&x.position_tracked==y.position_tracked&&x.lgrip_down==y.lgrip_down;
}
inline bool same(Commit const& a,Commit const& b) noexcept {
    return same(a.binding,b.binding)&&a.event==b.event&&a.previous_event==b.previous_event&&a.version==b.version&&
        a.from_version==b.from_version&&a.intent==b.intent&&a.body_publication==b.body_publication&&
        a.occupancy==b.occupancy&&a.completion==b.completion&&same(a.from,b.from)&&
        same(a.requested,b.requested)&&same(a.position,b.position)&&same(a.hand,b.hand);
}
inline bool axis(gears_vr::CoverEdgeKind a) noexcept {
    return a==gears_vr::CoverEdgeKind::left||a==gears_vr::CoverEdgeKind::right||a==gears_vr::CoverEdgeKind::top;
}
inline bool between(double value,double from,double to) noexcept {
    return value>=(std::min)(from,to)&&value<=(std::max)(from,to);
}
}

// One serialized GT ledger per authenticated cover basis. Only observe() can
// advance committed position. Cancellation and query waiting never move it.
// A recovery debt has no public reset/occupancy-only escape; the actual Native
// restoration/fresh-retirement integration is still required before reuse.
class Ledger {
public:
    Ledger()=default;Ledger(Ledger const&)=delete;Ledger& operator=(Ledger const&)=delete;
    bool bound()const noexcept{return bound_;}
    bool recovery_required()const noexcept{return debt_;}
    bool pending()const noexcept{return pending_;}
    Commit committed()const noexcept{return committed_;}
    Intent requested()const noexcept{return intent_;}
    // This is cancellation/version state, not an execution or collision lease.
    bool current(Intent const& value)const noexcept {
        return !debt_&&pending_&&value.id==intent_.id&&value.from_event==intent_.from_event&&
            value.from_version==intent_.from_version&&detail::same(value.binding,intent_.binding)&&
            detail::same(value.from,intent_.from)&&detail::same(value.requested,intent_.requested)&&detail::same(value.hand,intent_.hand);
    }

    Code synchronize(Backend& backend) noexcept {
        if(debt_)return Code::recovery_required;
        if(in_backend_)return fail();
        if(bound_&&backend_!=&backend)return fail();
        Commit next{};
        in_backend_=true;const auto observed=backend.observe(bound_?committed_.event:0,next);in_backend_=false;
        if(debt_)return Code::recovery_required;
        switch(observed){
        case Observation::unchanged:return bound_?Code::ready:Code::unavailable;
        case Observation::unavailable:return Code::unavailable;
        case Observation::uncertain:return fail();
        case Observation::committed:break;
        default:return fail();
        }
        if(bound_&&next.event<committed_.event)return Code::stale; // old acknowledgement never rewinds
        if(bound_&&next.event==committed_.event)return detail::same(next,committed_)?Code::ready:fail();
        if(!next.event||!next.body_publication||!next.occupancy||!detail::valid(next.binding)||
           !detail::finite(next.from)||!detail::finite(next.requested)||!detail::finite(next.position))return fail();
        if(!bound_){
            if(next.completion!=Completion::baseline||next.previous_event||next.from_version||next.intent||
               next.version!=1||!detail::same(next.from,next.position)||!detail::same(next.requested,next.position))return fail();
        }else{
            if(!pending_||!detail::same(next.binding,committed_.binding)||!detail::same(next.binding,intent_.binding)||
               committed_.version==UINT64_MAX||next.version!=committed_.version+1||
               next.previous_event!=committed_.event||next.from_version!=committed_.version||
               next.intent!=intent_.id||!detail::same(next.from,committed_.position)||
               !detail::same(next.requested,intent_.requested)||!detail::same(next.hand,intent_.hand)||
               next.body_publication<=committed_.body_publication)return fail();
            if(next.completion==Completion::accepted){
                if(!detail::same(next.position,intent_.requested))return fail();
            }else if(next.completion==Completion::clipped){
                if(!detail::between(next.position.right,intent_.from.right,intent_.requested.right)||
                   !detail::between(next.position.up,intent_.from.up,intent_.requested.up))return fail();
            }else return fail();
        }
        committed_=next;backend_=&backend;bound_=true;pending_=false;return Code::ready;
    }
    Code propose(Backend& backend,Binding const& binding,Hand const& hand,Offset requested,
                 std::int64_t now_ns,Intent& out) noexcept {
        // A body completion precedes *all* newer hand input, including release
        // handled by cancel(). A rejected new sample cannot erase that fact.
        const auto sync=synchronize(backend);
        if(sync==Code::unavailable&&bound_)return fail();
        if(sync!=Code::ready)return sync;
        if(now_ns<last_now_ns_){pending_=false;return Code::rejected;}
        last_now_ns_=now_ns;
        auto const& s=hand.sample;
        if(have_hand_&&(s.sequence<last_hand_.sample.sequence||s.observed_ns<last_hand_.sample.observed_ns))return Code::stale;
        if(have_hand_&&s.sequence==last_hand_.sample.sequence&&!detail::same(hand,last_hand_)){
            pending_=false;return Code::rejected;
        }
        if(!detail::same(binding,committed_.binding)||!detail::finite(requested)||!hand.grab||!hand.edge||
           !detail::axis(hand.axis)||!s.sequence||!s.input_active||!s.position_valid||!s.position_tracked||!s.lgrip_down||
           !gears_vr::finite(s.left_position_metres)||s.session_epoch!=binding.epochs.session||s.space_epoch!=binding.epochs.space||
           now_ns<=0||s.observed_ns<=0||s.observed_ns>now_ns||now_ns-s.observed_ns>100000000){pending_=false;return Code::rejected;}
        // One locked axis; switching axes requires a new input grip, while
        // retaining the already committed orthogonal displacement.
        if(hand.axis==gears_vr::CoverEdgeKind::top?requested.right!=committed_.position.right:
                                                  requested.up!=committed_.position.up){pending_=false;return Code::rejected;}
        last_hand_=hand;have_hand_=true;
        if(detail::same(requested,committed_.position)){pending_=false;return Code::no_motion;}
        if(pending_&&detail::same(binding,intent_.binding)&&detail::same(hand,intent_.hand)&&detail::same(requested,intent_.requested)){
            out=intent_;return Code::pending;
        }
        pending_=false;
        const auto next=detail::next_sequence(serial_);if(!next)return Code::exhausted;
        serial_=next;
        Intent candidate{binding,serial_,committed_.event,committed_.version,committed_.position,requested,hand};
        in_backend_=true;const auto admitted=backend.admit(candidate,now_ns);in_backend_=false;
        if(debt_)return Code::recovery_required;
        switch(admitted){
        case Admission::current:break;
        case Admission::unavailable:return Code::unavailable;
        default:return Code::rejected;
        }
        intent_=candidate;pending_=true;out=candidate;return Code::pending;
    }
    Code wait(Backend& backend,std::uint64_t id) noexcept {
        const auto sync=synchronize(backend);if(sync==Code::unavailable&&bound_)return fail();if(sync!=Code::ready)return sync;
        return pending_&&id==intent_.id?Code::waiting:!pending_&&id&&id==committed_.intent?Code::ready:Code::stale;
    }
    Code cancel(Backend& backend) noexcept {
        const auto sync=synchronize(backend);
        pending_=false;
        // An unreadable completion channel cannot prove whether the body had
        // already moved. Release still cancels future intent; preserve debt.
        if(sync==Code::unavailable)return bound_?fail():Code::unavailable;
        return sync==Code::ready||sync==Code::stale?Code::cancelled:sync;
    }
private:
    Code fail() noexcept {debt_=true;pending_=false;return Code::recovery_required;}
    Commit committed_{};Intent intent_{};std::uint64_t serial_{};
    Backend* backend_{};
    std::int64_t last_now_ns_{};
    Hand last_hand_{};
    bool bound_{},pending_{},debt_{},in_backend_{},have_hand_{};
};
} // namespace gears_cover_motion
