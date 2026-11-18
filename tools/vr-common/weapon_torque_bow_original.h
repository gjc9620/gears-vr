#pragma once
#include "weapon_torque_bow_interaction.h"

// Bow-only original cancellation recipe. The one GT adapter supplies the
// certified ABI; this layer never discovers memory, writes bones or emits an
// EndFire/StopFire. Every partial/unknown call retains its exact old owner.
namespace gears_weapon {
enum class TorqueBowCallResult : std::uint8_t {rejected,accepted,unknown};
enum class TorqueBowCancelRoute : std::uint8_t {active,put_down};
enum class TorqueBowCancelPhase : std::uint8_t {
    idle,stop_injection,clear_pending,clear_end,charge_ended,leave_charge,verify,complete,uncertain,already_fired
};
struct TorqueBowCancelSnapshot {
    TorqueBowIdentity identity{};
    TorqueBowInventory inventory{};
    bool charge{},fire{},pending_fire{},pending_end_fire{},injection_stopped{},
         end_state_cleanup_verified{};
    bool original_input_owned{}; // Existing journal, including a refused/no-Charge press.
    bool original_projectile_accounted{}; // Exact journal's real callback + ammo delta, never Fire state alone.
};
class TorqueBowOriginalCancel {
    TorqueBowIdentity owner_{};TorqueBowInventory before_{};
    std::uint64_t serial_{};TorqueBowCancelRoute route_{};
    TorqueBowCancelPhase phase_{};
    TorqueBowCancelPhase uncertain_step_{};
    bool began_in_charge_{},post_shot_{};
    TorqueBowCancelPhase following(TorqueBowCancelPhase p) const noexcept {
        switch(p){
        case TorqueBowCancelPhase::stop_injection:return TorqueBowCancelPhase::clear_pending;
        case TorqueBowCancelPhase::clear_pending:return TorqueBowCancelPhase::clear_end;
        case TorqueBowCancelPhase::clear_end:return began_in_charge_?TorqueBowCancelPhase::charge_ended:TorqueBowCancelPhase::verify;
        case TorqueBowCancelPhase::charge_ended:return TorqueBowCancelPhase::leave_charge;
        case TorqueBowCancelPhase::leave_charge:return TorqueBowCancelPhase::verify;
        default:return TorqueBowCancelPhase::uncertain;
        }
    }
    void advance(TorqueBowCallResult result,TorqueBowCancelPhase next) noexcept {
        if(result==TorqueBowCallResult::accepted)phase_=next;
        else if(result==TorqueBowCallResult::unknown){uncertain_step_=phase_;phase_=TorqueBowCancelPhase::uncertain;}
        // Rejected means the adapter certified NO call/no write. It may be
        // attempted later; unknown results are never blindly re-executed.
    }
public:
    bool begin(std::uint64_t serial,TorqueBowCancelSnapshot const& s,TorqueBowCancelRoute route) noexcept {
        if(phase_!=TorqueBowCancelPhase::idle||!serial||!s.identity.valid()||!s.inventory.revision||
           (!s.charge&&!s.original_input_owned)||
           (s.fire&&(!s.original_input_owned||!s.original_projectile_accounted||s.charge)))return false;
        owner_=s.identity;before_=s.inventory;serial_=serial;route_=route;
        began_in_charge_=s.charge;post_shot_=s.fire&&s.original_projectile_accounted;
        phase_=TorqueBowCancelPhase::stop_injection;return true;
    }
    bool pending() const noexcept {return phase_!=TorqueBowCancelPhase::idle&&phase_!=TorqueBowCancelPhase::complete;}
    TorqueBowCancelPhase phase() const noexcept {return phase_;}
    std::uint64_t serial() const noexcept {return serial_;}
    TorqueBowIdentity owner() const noexcept {return owner_;}
    bool began_in_charge() const noexcept {return began_in_charge_;}
    // A late receipt/readback may prove the unknown step actually completed.
    // Ops.reconcile_applied is READ-ONLY: it certifies that exact old-owner
    // step's full effect, including original callback receipts where needed.
    // Neither false nor unknown authorizes a replay or discards the journal.
    template<class Ops> bool reconcile_on_gt(Ops& ops) noexcept {
        if(phase_!=TorqueBowCancelPhase::uncertain||uncertain_step_==TorqueBowCancelPhase::idle||
           !ops.current(owner_,serial_))return false;
        TorqueBowCancelSnapshot s{};
        if(!ops.read(owner_,s)||!(s.identity==owner_))return false;
        if(s.fire&&!post_shot_){phase_=TorqueBowCancelPhase::already_fired;return false;}
        if(post_shot_&&(!s.original_input_owned||!s.original_projectile_accounted||s.charge))return false;
        if(s.inventory.loaded!=before_.loaded||s.inventory.reserve!=before_.reserve)return false;
        if(ops.reconcile_applied(owner_,serial_,uncertain_step_)!=TorqueBowCallResult::accepted)return false;
        phase_=following(uncertain_step_);uncertain_step_=TorqueBowCancelPhase::idle;
        return drain_on_gt(ops);
    }
    // Ops.current authenticates THIS original owner, not the current selected
    // weapon. Cleanup may survive pose expiry; it cannot adopt a new actor.
    template<class Ops> bool service(Ops& ops) noexcept {
        if(!pending()||phase_==TorqueBowCancelPhase::uncertain||phase_==TorqueBowCancelPhase::already_fired||
           !ops.current(owner_,serial_))return false;
        TorqueBowCancelSnapshot s{};
        if(!ops.read(owner_,s)||!(s.identity==owner_))return false;
        if(s.fire&&!post_shot_){
            phase_=TorqueBowCancelPhase::already_fired;return false; // account real event, no refund
        }
        if(post_shot_&&(!s.original_input_owned||!s.original_projectile_accounted||s.charge))return false;
        if(s.inventory.loaded!=before_.loaded||s.inventory.reserve!=before_.reserve){
            phase_=TorqueBowCancelPhase::uncertain;return false;
        }
        switch(phase_){
        case TorqueBowCancelPhase::stop_injection:
            advance(ops.stop_injection(owner_,serial_),TorqueBowCancelPhase::clear_pending);break;
        case TorqueBowCancelPhase::clear_pending:
            advance(ops.clear_owned_pending_fire(owner_,serial_),TorqueBowCancelPhase::clear_end);break;
        case TorqueBowCancelPhase::clear_end:
            advance(ops.clear_owned_pending_end(owner_,serial_),
                began_in_charge_?TorqueBowCancelPhase::charge_ended:TorqueBowCancelPhase::verify);break;
        case TorqueBowCancelPhase::charge_ended:
            advance(ops.charge_ended(owner_,serial_),TorqueBowCancelPhase::leave_charge);break;
        case TorqueBowCancelPhase::leave_charge:
            // Normal transition calls Charge.EndState exactly once. No manual
            // EndState call and no resetting the original fatigue clock.
            if(s.pending_fire||s.pending_end_fire||!s.injection_stopped)return false;
            advance(ops.leave_charge(owner_,serial_,route_),TorqueBowCancelPhase::verify);break;
        case TorqueBowCancelPhase::verify:
            if(!s.charge&&!s.fire&&!s.pending_fire&&!s.pending_end_fire&&s.injection_stopped&&
               s.end_state_cleanup_verified&&s.inventory.revision>=before_.revision)
                phase_=TorqueBowCancelPhase::complete;
            break;
        default:break;
        }
        return phase_==TorqueBowCancelPhase::complete;
    }
    // Run the whole sequence in the SAME owning GT callback, without yielding
    // to another Charge.Tick/fatigue update between cleanup steps. A rejected
    // step returns responsibility to the host; it must keep injection fenced.
    template<class Ops> bool drain_on_gt(Ops& ops) noexcept {
        for(unsigned i=0;i<7&&pending();++i){
            const auto before=phase_;
            if(service(ops))return true;
            if(phase_==before)break;
        }
        return phase_==TorqueBowCancelPhase::complete;
    }
};
}
