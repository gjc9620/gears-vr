#pragma once
#include "hand_animation_observation.h"
#include <cmath>
#include <initializer_list>
#include <limits>

namespace gears_hand_animation {

// A necessary normal-path hand-back gate, not a skeletal writer or a complete
// interception contract. The current observer misses VM calls and cannot
// certify cancellation/execution coverage. Do not copy normal_path_ready into
// FreeHandGameState::ownership_proved. All inputs below need an independently
// certified game-thread adapter; this header neither reads nor changes a game.
constexpr unsigned ownership_slots=32,ownership_children=8;
constexpr std::uint64_t ownership_fresh_ms=100,ownership_episode_ms=5000;
constexpr std::uint64_t ownership_settle_ms=20;

struct OwnershipName {
    unsigned index{},number{};
    bool operator==(OwnershipName const& other) const noexcept {
        return index==other.index&&number==other.number;
    }
    bool operator!=(OwnershipName const& other) const noexcept {return !(*this==other);}
};
enum class OwnershipAnimation : std::uint8_t {
    none,kickup,smack_a,smack_c,
    pickup_holster, // Exact AR_Holster_Rt / AR_Cov_Std_Holster_Rt / AR_Cov_Mid_Holster_Rt.
    unsupported
};
struct OwnershipChild {
    std::uintptr_t object{};
    unsigned known{}; // Same child known bits as hand_animation_probe.inl.
    float weight{},total{},target{};
    OwnershipName animation{};
    OwnershipAnimation kind{}; // Exact name classification, never an RVA/name-index guess.
    bool playing{};
};
struct OwnershipSlot {
    std::uintptr_t object{},component{};
    unsigned registry_index{},known{};
    OwnershipName name{};
    float total{},blend{},pending{};
    int custom_index{},target_index{};
    bool playing{};
    // An adapter must distinguish a successfully read null array element from
    // a failed pointer read. known=0/object=0 alone cannot make that assertion.
    bool presence_known{};
    unsigned children{};
    std::array<OwnershipChild,ownership_children> child{};
};
struct OwnershipControl {
    std::uintptr_t object{},component{},outer{},next{},cls{};
    unsigned registry_index{},known{},list_mask{};
    OwnershipName name{},object_name{};
    float strength{},target{},blend{};
};
struct OwnershipSnapshot {
    std::uint64_t epoch{},serial{},observed_ms{},tick{};
    std::uintptr_t world{},controller{},pawn{},mesh{},tree{},weapon{};
    unsigned known{}; // Probe state_known bits; all nine are required.
    std::uint8_t special{};
    bool melee{},wants_melee{},switching{},special_attack{},player_controlled{};
    int health{};
    bool control_lists_known{};
    unsigned slots{};
    std::array<OwnershipControl,8> controls{}; // Probe's right four, then left four.
    std::array<OwnershipSlot,ownership_slots> slot{};
};
enum class OwnershipBoundaryPhase : std::uint8_t {none,pre,post};
struct OwnershipBoundary {
    OwnershipBoundaryPhase phase{};
    Kind kind{Kind::count};
    Source source{};
    std::uint64_t call{};
    std::uintptr_t subject{},sequence_node{},pickup_weapon{};
    // exact_owner requires WarPawn/WarPC or the precise MeleeAttacking owner.
    // normal requires known non-forced/non-damage parameters. active_state
    // requires the exact PreviousStateName/NextStateName Active for melee.
    // Unknown, forced, damage, cancellation and execution paths stay closed.
    bool exact_owner{},normal{},active_state{};
};
enum class OwnershipPhase : std::uint8_t {
    unobserved,idle_unproved,original_animation,waiting_slot_exit,
    waiting_new_tick,normal_path_ready,rejected
};
enum class OwnershipReason : std::uint8_t {
    none,incomplete_snapshot,stale,time_regression,sequence_gap,identity_changed,
    controls_changed,slot_reused,started_active,missing_start,missing_end,
    missing_boundary,boundary_mismatch,unsupported_action,cancelled,
    interrupted,timeout
};
struct OwnershipDecision {
    OwnershipPhase phase{};
    OwnershipReason reason{};
    std::uint64_t episode{};
    bool normal_path_ready{};
    // No runtime adapter has proved complete action interception and a
    // reversible writer boundary. Even a complete normal path cannot grant it.
    static constexpr bool ownership_proved() noexcept {return false;}
};

class OwnershipGate {
    enum class Action : std::uint8_t {none,pickup,melee};
    struct TrackedChild {bool bound{};OwnershipName animation{};OwnershipAnimation kind{};};
    OwnershipSnapshot baseline_{};
    std::array<std::array<TrackedChild,ownership_children>,ownership_slots> tracked_{};
    OwnershipBoundary pending_{};
    OwnershipPhase phase_{OwnershipPhase::unobserved};
    OwnershipReason reason_{};
    Action action_{};
    std::uint64_t serial_{},last_ms_{},last_tick_{},episode_{},began_{},last_call_{};
    std::uint64_t settled_ms_{},settled_tick_{};
    std::uintptr_t weapon_{},pickup_weapon_{};
    std::uint8_t before_special_{};
    bool before_melee_{},ended_{},weapon_changed_{};
    unsigned tracked_count_{};
    int last_health_{};

    static bool unit(float value) noexcept {return std::isfinite(value)&&value>=0&&value<=1;}
    static bool duration(float value) noexcept {return std::isfinite(value)&&value>=0;}
    static bool active(OwnershipChild const& child) noexcept {
        return child.weight!=0||child.total!=0||child.target!=0||child.playing;
    }
    static bool state_idle(OwnershipSnapshot const& s) noexcept {
        return !s.special&&!s.melee&&!s.switching&&!s.special_attack&&!s.wants_melee;
    }
    static bool slots_idle(OwnershipSnapshot const& s) noexcept {
        for(unsigned i=0;i<s.slots;++i){
            auto const& slot=s.slot[i];
            if(!slot.object)continue; // Only complete() can admit a certified null element.
            if(slot.blend!=0||slot.playing||slot.target_index!=0)return false;
            for(unsigned j=1;j<slot.children;++j)if(active(slot.child[j]))return false;
        }
        // NodeTotalWeight includes the base child. PendingBlendOutTime is a
        // retained duration (0.2/0.25 in retail, or the -1 sentinel during
        // equipment playback), not a remaining blend clock.
        return true;
    }
    static bool tails_quiet(OwnershipSnapshot const& s) noexcept {
        for(const auto i:{2u,3u,6u,7u}){
            auto const& c=s.controls[i];
            if(c.strength!=0||c.target!=0||c.blend!=0)return false;
        }
        return true;
    }
    static bool complete(OwnershipSnapshot const& s) noexcept {
        if(!s.epoch||!s.serial||!s.observed_ms||!s.tick||!s.world||!s.controller||
           !s.pawn||!s.mesh||!s.tree||s.known!=511||!s.player_controlled||s.health<=0||
           !s.control_lists_known||!s.slots||s.slots>ownership_slots)return false;
        for(unsigned i=0;i<s.controls.size();++i){
            auto const& c=s.controls[i];
            if(c.known!=255||!c.object||!c.registry_index||!c.cls||!c.name.index||
               !c.object_name.index||c.component!=s.mesh||c.outer!=s.tree||!c.list_mask||
               !unit(c.strength)||!unit(c.target)||!duration(c.blend))return false;
            if(c.next!=(i%4==3?0:s.controls[i+1].object))return false;
            if(i%4&&c.list_mask!=s.controls[i-1].list_mask)return false;
            for(unsigned j=0;j<i;++j)if(c.object==s.controls[j].object)return false;
        }
        for(unsigned i=0;i<s.slots;++i){
            auto const& slot=s.slot[i];
            if(!slot.presence_known)return false;
            if(!slot.object){if(slot.known||slot.children)return false;continue;}
            if(slot.known!=2047||!slot.object||!slot.registry_index||!slot.name.index||
               slot.component!=s.mesh||!unit(slot.total)||!duration(slot.blend)||
               !(slot.pending==-1||duration(slot.pending))||slot.children<2||slot.children>ownership_children||
               slot.custom_index<0||unsigned(slot.custom_index)>=slot.children||
               slot.target_index<0||unsigned(slot.target_index)>=slot.children)return false;
            for(unsigned previous=0;previous<i;++previous)
                if(slot.object==s.slot[previous].object)return false;
            for(unsigned j=0;j<slot.children;++j){
                auto const& child=slot.child[j];
                if(!child.object||!unit(child.weight)||!unit(child.total)||!unit(child.target)||
                   (j?child.known!=31:(child.known&3)!=3))return false;
                if(j&&unsigned(child.kind)>unsigned(OwnershipAnimation::unsupported))return false;
                for(unsigned k=0;k<j;++k)if(child.object==slot.child[k].object)return false;
            }
        }
        return true;
    }
    void reject(OwnershipReason reason) noexcept {
        phase_=OwnershipPhase::rejected;reason_=reason;settled_ms_=settled_tick_=0;
    }
    bool identity(OwnershipSnapshot const& s) noexcept {
        if(s.epoch!=baseline_.epoch||s.world!=baseline_.world||s.controller!=baseline_.controller||
           s.pawn!=baseline_.pawn||s.mesh!=baseline_.mesh||s.tree!=baseline_.tree){
            reject(OwnershipReason::identity_changed);return false;
        }
        if(s.weapon!=weapon_){
            const bool pickup=action_==Action::pickup||
                (pending_.phase==OwnershipBoundaryPhase::pre&&pending_.kind==Kind::use_request);
            if(!pickup||weapon_changed_||!pickup_weapon_||s.weapon!=pickup_weapon_){
                reject(OwnershipReason::identity_changed);return false;
            }
            weapon_=s.weapon;weapon_changed_=true;
        }
        for(unsigned i=0;i<s.controls.size();++i){
            auto const& a=baseline_.controls[i];auto const& b=s.controls[i];
            if(a.object!=b.object||a.registry_index!=b.registry_index||a.cls!=b.cls||
               a.next!=b.next||a.list_mask!=b.list_mask||a.name!=b.name||a.object_name!=b.object_name){
                reject(OwnershipReason::controls_changed);return false;
            }
        }
        if(s.slots!=baseline_.slots){reject(OwnershipReason::slot_reused);return false;}
        for(unsigned i=0;i<s.slots;++i){
            auto const& a=baseline_.slot[i];auto const& b=s.slot[i];
            if(a.object!=b.object||a.registry_index!=b.registry_index||a.name!=b.name||a.children!=b.children){
                reject(OwnershipReason::slot_reused);return false;
            }
            for(unsigned j=0;j<a.children;++j)if(a.child[j].object!=b.child[j].object){
                reject(OwnershipReason::slot_reused);return false;
            }
        }
        return true;
    }
    bool track(OwnershipSnapshot const& s) noexcept {
        for(unsigned i=0;i<s.slots;++i)for(unsigned j=1;j<s.slot[i].children;++j){
            auto const& child=s.slot[i].child[j];auto& tracked=tracked_[i][j];
            if(tracked.bound&&(tracked.animation!=child.animation||tracked.kind!=child.kind)){
                reject(OwnershipReason::slot_reused);return false;
            }
            if(!active(child))continue;
            const bool expected=action_==Action::pickup?
                child.kind==OwnershipAnimation::kickup||child.kind==OwnershipAnimation::pickup_holster:
                child.kind==OwnershipAnimation::smack_a||child.kind==OwnershipAnimation::smack_c;
            if(!expected||!child.animation.index){reject(OwnershipReason::unsupported_action);return false;}
            if(!tracked.bound){
                if(ended_){reject(OwnershipReason::slot_reused);return false;}
                // A/C are separate episodes, not interchangeable animations.
                if(action_==Action::melee)for(auto const& row:tracked_)for(auto const& other:row)
                    if(other.bound&&(other.animation!=child.animation||other.kind!=child.kind)){
                        reject(OwnershipReason::slot_reused);return false;
                    }
                tracked={true,child.animation,child.kind};++tracked_count_;
            }
        }
        return true;
    }
    static bool same_boundary(OwnershipBoundary const& a,OwnershipBoundary const& b) noexcept {
        return a.kind==b.kind&&a.source==b.source&&a.call==b.call&&a.subject==b.subject&&
            a.sequence_node==b.sequence_node&&a.pickup_weapon==b.pickup_weapon&&
            a.exact_owner==b.exact_owner&&a.normal==b.normal&&a.active_state==b.active_state;
    }
    bool boundary_pre(OwnershipSnapshot const& s,OwnershipBoundary const& edge) noexcept {
        if(pending_.phase!=OwnershipBoundaryPhase::none){reject(OwnershipReason::missing_boundary);return false;}
        if(!edge.call||edge.call<=last_call_||!edge.exact_owner){
            reject(OwnershipReason::boundary_mismatch);return false;
        }
        if(!edge.normal){reject(OwnershipReason::cancelled);return false;}
        const bool start=edge.kind==Kind::use_request||edge.kind==Kind::melee_state_begin;
        if(start){
            if(action_!=Action::none&&!ended_){reject(OwnershipReason::unsupported_action);return false;}
            if(s.special||s.melee||s.switching||s.special_attack||!slots_idle(s)||!tails_quiet(s)){
                reject(OwnershipReason::missing_start);return false;
            }
            if(edge.kind==Kind::use_request){
                if(edge.subject!=s.controller||!edge.pickup_weapon){reject(OwnershipReason::boundary_mismatch);return false;}
                pickup_weapon_=edge.pickup_weapon;weapon_changed_=false;
            }else if(edge.source!=Source::hook||edge.subject!=s.weapon||!s.weapon||!edge.active_state){
                reject(OwnershipReason::boundary_mismatch);return false;
            }
            began_=s.observed_ms; // A later complete cycle gets its own bounded deadline.
        }else if(edge.kind==Kind::on_anim_end){
            if(action_!=Action::pickup||ended_||edge.source!=Source::hook||edge.subject!=s.pawn||
               !edge.sequence_node||s.special!=53||!tracked_count_){
                reject(OwnershipReason::boundary_mismatch);return false;
            }
            bool found=false;
            for(unsigned i=0;i<s.slots;++i)for(unsigned j=1;j<s.slot[i].children;++j)
                if(tracked_[i][j].bound&&tracked_[i][j].kind==OwnershipAnimation::kickup&&
                   s.slot[i].child[j].object==edge.sequence_node)found=true;
            if(!found){reject(OwnershipReason::boundary_mismatch);return false;}
        }else if(edge.kind==Kind::melee_state_end){
            if(action_!=Action::melee||ended_||edge.source!=Source::hook||edge.subject!=s.weapon||
               !edge.active_state||!s.melee||!tracked_count_){
                reject(OwnershipReason::boundary_mismatch);return false;
            }
        }else if(edge.kind==Kind::melee_stop_request){
            // A known ordinary stop request only removes intent; it is not EndState.
            if(action_!=Action::melee||ended_||edge.subject!=s.pawn){
                reject(OwnershipReason::boundary_mismatch);return false;
            }
        }else {reject(OwnershipReason::unsupported_action);return false;}
        pending_=edge;last_call_=edge.call;before_special_=s.special;before_melee_=s.melee;
        settled_ms_=settled_tick_=0;
        phase_=action_==Action::none?OwnershipPhase::idle_unproved:OwnershipPhase::original_animation;
        return true;
    }
    bool boundary_post(OwnershipSnapshot const& s,OwnershipBoundary const& edge) noexcept {
        if(pending_.phase!=OwnershipBoundaryPhase::pre||!same_boundary(pending_,edge)){
            reject(OwnershipReason::missing_boundary);return false;
        }
        pending_={};
        if(edge.kind==Kind::use_request||edge.kind==Kind::melee_state_begin){
            const bool pickup=edge.kind==Kind::use_request;
            if(before_special_||before_melee_||(pickup?(s.special!=53||s.melee):(s.special||!s.melee))){
                reject(OwnershipReason::missing_start);return false;
            }
            action_=pickup?Action::pickup:Action::melee;ended_=false;began_=s.observed_ms;
            tracked_={};tracked_count_=0;++episode_;phase_=OwnershipPhase::original_animation;
        }else if(edge.kind==Kind::on_anim_end||edge.kind==Kind::melee_state_end){
            if((edge.kind==Kind::on_anim_end?before_special_!=53:!before_melee_)||s.special||s.melee){
                reject(OwnershipReason::missing_end);return false;
            }
            ended_=true;phase_=OwnershipPhase::waiting_slot_exit;
        }
        return true;
    }
    OwnershipDecision result() const noexcept {return {phase_,reason_,episode_,phase_==OwnershipPhase::normal_path_ready};}
public:
    void reset() noexcept {*this=OwnershipGate{};}
    OwnershipDecision decision(std::uint64_t now) noexcept {
        if(phase_==OwnershipPhase::rejected)return result();
        if(last_ms_&&(now<last_ms_||now-last_ms_>ownership_fresh_ms))
            reject(now<last_ms_?OwnershipReason::time_regression:OwnershipReason::stale);
        else if(began_&&phase_!=OwnershipPhase::normal_path_ready&&now>=began_&&now-began_>ownership_episode_ms)
            reject(OwnershipReason::timeout);
        return result();
    }
    OwnershipDecision observe(OwnershipSnapshot const& s,std::uint64_t now,
                              OwnershipBoundary const& edge={}) noexcept {
        if(phase_==OwnershipPhase::rejected)return result();
        const bool was_ready=phase_==OwnershipPhase::normal_path_ready;
        if(!complete(s)){reject(OwnershipReason::incomplete_snapshot);return result();}
        if(now<s.observed_ms||now-s.observed_ms>ownership_fresh_ms){reject(OwnershipReason::stale);return result();}
        if(serial_){
            if(serial_==std::numeric_limits<std::uint64_t>::max()||s.serial!=serial_+1){
                reject(OwnershipReason::sequence_gap);return result();
            }
            if(s.observed_ms<last_ms_||s.tick<last_tick_){reject(OwnershipReason::time_regression);return result();}
            if(s.observed_ms-last_ms_>ownership_fresh_ms){reject(OwnershipReason::sequence_gap);return result();}
            if(!identity(s))return result();
            if(s.health<last_health_){reject(OwnershipReason::interrupted);return result();}
        }else {
            if(edge.phase!=OwnershipBoundaryPhase::none||!state_idle(s)||!slots_idle(s)||!tails_quiet(s)){
                reject(OwnershipReason::started_active);return result();
            }
            baseline_=s;weapon_=s.weapon;phase_=OwnershipPhase::idle_unproved;
        }
        serial_=s.serial;last_ms_=s.observed_ms;last_tick_=s.tick;last_health_=s.health;
        if(began_&&phase_!=OwnershipPhase::normal_path_ready&&s.observed_ms-began_>ownership_episode_ms){
            reject(OwnershipReason::timeout);return result();
        }
        if(unsigned(edge.source)>unsigned(Source::bridge_invoke)||
           unsigned(edge.phase)>unsigned(OwnershipBoundaryPhase::post)){
            reject(OwnershipReason::boundary_mismatch);return result();
        }
        if(edge.phase==OwnershipBoundaryPhase::pre){if(!boundary_pre(s,edge))return result();}
        else if(edge.phase==OwnershipBoundaryPhase::post){if(!boundary_post(s,edge))return result();}
        else if(pending_.phase!=OwnershipBoundaryPhase::none){reject(OwnershipReason::missing_boundary);return result();}
        if(was_ready&&edge.phase==OwnershipBoundaryPhase::none&&
           (!state_idle(s)||!slots_idle(s)||!tails_quiet(s))){
            reject(OwnershipReason::missing_start);return result();
        }
        if(s.special_attack||(s.special&&s.special!=53)||
           (action_==Action::melee&&(s.special||s.switching))||
           (action_==Action::pickup&&s.melee)||(ended_&&s.switching)){
            reject(OwnershipReason::unsupported_action);return result();
        }
        if(action_==Action::none){
            const bool start_pending=pending_.phase==OwnershipBoundaryPhase::pre&&
                (pending_.kind==Kind::use_request||pending_.kind==Kind::melee_state_begin);
            if((!state_idle(s)&&!start_pending)||!slots_idle(s)){
                reject(OwnershipReason::missing_start);return result();
            }
            return result(); // Idle has no positive ownership meaning.
        }
        if(!track(s))return result();
        const bool active_state=action_==Action::pickup?s.special==53:s.melee;
        if(!ended_&&!active_state){reject(OwnershipReason::missing_end);return result();}
        if(ended_&&active_state){reject(OwnershipReason::missing_start);return result();}
        if(!ended_||pending_.phase!=OwnershipBoundaryPhase::none)return result();
        if(!state_idle(s)||!slots_idle(s)||!tails_quiet(s)||
           (action_==Action::pickup&&s.weapon!=pickup_weapon_)){
            phase_=OwnershipPhase::waiting_slot_exit;settled_ms_=settled_tick_=0;return result();
        }
        if(!settled_ms_){
            settled_ms_=s.observed_ms;settled_tick_=s.tick;phase_=OwnershipPhase::waiting_new_tick;
        }else if(s.tick>settled_tick_&&s.observed_ms>=settled_ms_&&
                 s.observed_ms-settled_ms_>=ownership_settle_ms){
            phase_=OwnershipPhase::normal_path_ready;
        }
        return result();
    }
};
} // namespace gears_hand_animation
