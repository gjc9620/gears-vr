#pragma once
#include "controller_input_frame.h"
#include <array>
#include "bow_history_evidence.h"

// This is only the Bow's XR edge decoder. The current GT owner authenticates
// the actor/held mount, arms the Native R3 veto and executes the original calls.
// Cumulative counters survive skipped XR publications; this decoder never
// invents an edge from a held level or replays an ambiguous burst of edges.
namespace gears_torque_bow_controls {
namespace vr=gears_vr;
enum class Action:unsigned {press,release,cancel};
enum class Status:unsigned {invalid,baseline,observed,edges,cancel,ambiguous};
enum class Rejection:unsigned {none,input_shape,non_increasing,observation_gap,counter_regression,ambiguous_transition};
struct Result {
    Status status{Status::invalid};
    std::array<Action,2> actions{};unsigned count{};
    bool history_verified{};std::array<std::int64_t,2> original_action_ns{};
    bool release_observed{}; // An actually tracked active trigger at <=.35.
    // Passive return-site receipt, captured before lost()/save() changes history.
    Rejection rejection{};bool previous_known{};std::uint64_t previous_sequence{};std::int64_t previous_sync_ns{};
};
struct Continuity {
    gears_hand_continuity::TrackedInput current{};
    gears_hand_continuity::Publisher publisher{};
    gears_hand_continuity::MotionHistory history{};
};
class Decoder {
    vr::StereoSource source_{vr::StereoSource::RuntimeTracked};
    std::uint64_t session_{},space_{},profile_{},input_{},sequence_{};
    std::int64_t sync_{};
    std::uint32_t trigger_p_{},trigger_r_{},stick_p_{},stick_r_{};
    bool trigger_down_{},stick_down_{},known_{};
    vr::ControllerInputFrame saved_input_{};
    gears_hand_continuity::TrackedInput continuity_baseline_{};
    gears_hand_continuity::Publisher continuity_publisher_{};bool continuity_known_{};
    void save(vr::ControllerInputFrame const& f,bool trigger_down) noexcept {
        saved_input_=f;source_=f.source;
        session_=f.session;space_=f.space_epoch;profile_=f.profile_generation;input_=f.input_epoch;
        sequence_=f.sequence;sync_=f.sync_ns;
        const auto& h=f.hands[1];trigger_p_=h.pressed[vr::control_trigger];trigger_r_=h.released[vr::control_trigger];
        stick_p_=h.pressed[vr::control_stick];stick_r_=h.released[vr::control_stick];
        trigger_down_=trigger_down;stick_down_=(h.buttons&vr::controller_stick_click)!=0;known_=true;
    }
    static bool one_transition(std::uint32_t p,std::uint32_t r,bool before,bool after) noexcept {
        if(p>1||r>1)return false;
        if(!p&&!r)return before==after;
        if(p&&!r)return !before&&after;
        if(!p&&r)return before&&!after;
        return before==after; // One full tap/release-press, ordered by baseline.
    }
public:
    void lost() noexcept {known_=false;continuity_known_=false;}
private:
    Result advance_impl(vr::ControllerInputFrame const& f,vr::StereoSample const& stereo,
                   std::int64_t now,bool charge_owned,bool allow_sim,bool certified_gap) noexcept {
        Result out{};
        out.previous_known=known_;out.previous_sequence=sequence_;out.previous_sync_ns=sync_;
        const auto refused=[&](Rejection reason) noexcept {
            Result result{}; // Keep the original invalid result's release/actions cleared.
            result.rejection=reason;result.previous_known=out.previous_known;
            result.previous_sequence=out.previous_sequence;result.previous_sync_ns=out.previous_sync_ns;return result;
        };
        const auto& h=f.hands[1];
        if(!vr::controller_frame_matches(f,stereo,now,allow_sim)||now<f.sync_ns||now-f.sync_ns>100000000||
           !vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active)||
           h.trigger_active!=1||!(h.buttons_active&vr::controller_stick_click)){
            lost();return refused(Rejection::input_shape);
        }
        const bool cohort=known_&&source_==f.source&&session_==f.session&&space_==f.space_epoch&&
            profile_==f.profile_generation&&input_==f.input_epoch;
        // Schmitt state on a new cohort starts from the conservative >.35
        // held baseline. A recovered middle-valued trigger is never a press.
        const bool down=cohort?(trigger_down_?h.trigger>.35f:h.trigger>=.65f):h.trigger>.35f;
        out.release_observed=h.trigger<=.35f;
        if(!cohort){save(f,down);out.status=Status::baseline;return out;}
        if(f.sequence<=sequence_||f.sync_ns<=sync_)return refused(Rejection::non_increasing); // Exact replay cannot authorize release.
        if(f.sync_ns-sync_>100000000&&!certified_gap){lost();return refused(Rejection::observation_gap);} // Gap invokes the owner's non-firing interruption.
        const auto p=h.pressed[vr::control_trigger],r=h.released[vr::control_trigger];
        const auto sp=h.pressed[vr::control_stick],sr=h.released[vr::control_stick];
        if(p<trigger_p_||r<trigger_r_||sp<stick_p_||sr<stick_r_){lost();return refused(Rejection::counter_regression);}
        const auto dp=p-trigger_p_,dr=r-trigger_r_,dsp=sp-stick_p_,dsr=sr-stick_r_;
        const bool stick=(h.buttons&vr::controller_stick_click)!=0;
        const bool valid=one_transition(dp,dr,trigger_down_,down)&&one_transition(dsp,dsr,stick_down_,stick);
        const bool was_down=trigger_down_;
        save(f,down);
        if(!valid){out.status=Status::ambiguous;out.release_observed=false;out.rejection=Rejection::ambiguous_transition;return out;}
        // A fresh R3 press wins over a simultaneous R2 release while charging.
        // This includes a complete R3 tap skipped between two GT publications.
        // No R3 edge is ever emitted as a reload or an ordinary fire release.
        if(charge_owned&&dsp){out.status=Status::cancel;out.actions[0]=Action::cancel;out.count=1;return out;}
        if(dp&&dr){
            out.actions=was_down?std::array<Action,2>{Action::release,Action::press}:
                std::array<Action,2>{Action::press,Action::release};out.count=2;
        }else if(dp){out.actions[0]=Action::press;out.count=1;}
        else if(dr){out.actions[0]=Action::release;out.count=1;}
        out.status=out.count?Status::edges:Status::observed;return out;
    }
public:
    Result advance(vr::ControllerInputFrame const& f,vr::StereoSample const& stereo,
                   std::int64_t now,bool charge_owned,bool allow_sim=false,
                   Continuity const* companion=nullptr) noexcept {
        namespace hc=gears_hand_continuity;
        const bool matched=companion&&companion->publisher.pid&&companion->publisher.start&&
            hc::same_input(companion->current.input,f)&&
            hc::motion_history_valid(companion->history,companion->current,1);
        bow_history_evidence::Receipt receipt{};
        if(known_&&continuity_known_&&matched&&hc::same_input(continuity_baseline_.input,saved_input_))
            receipt=bow_history_evidence::inspect(continuity_baseline_,continuity_publisher_,
                companion->current,companion->publisher,companion->history,now,trigger_down_);
        const bool bridge=known_&&f.sync_ns>sync_&&f.sync_ns-sync_>100000000&&
            receipt.failure==bow_history_evidence::Failure::none&&receipt.all_changes_within_100ms;
        auto out=advance_impl(f,stereo,now,charge_owned,allow_sim,bridge);
        const bool valid=out.status==Status::baseline||out.status==Status::observed||
            out.status==Status::edges||out.status==Status::cancel;
        // A history certificate changes only the observation-gap decision.
        // Current stereo/shape, counters, ambiguity and R3 priority still use
        // the original decoder path. No synthetic stereo or reseeding call.
        if(bridge&&valid){
            out.history_verified=true;unsigned trigger_index=0;
            for(unsigned i=0;i<receipt.changes;++i){const auto& edge=receipt.original_changes[i];
                if(out.status==Status::cancel){
                    if(edge.control==vr::control_stick&&edge.pressed){out.original_action_ns[0]=edge.sync_ns;break;}
                }else if(edge.control==vr::control_trigger&&trigger_index<out.count)
                    out.original_action_ns[trigger_index++]=edge.sync_ns;
            }
        }
        continuity_known_=valid&&known_&&matched&&hc::same_input(saved_input_,f);
        if(continuity_known_){continuity_baseline_=companion->current;continuity_publisher_=companion->publisher;}
        return out;
    }
};
}
