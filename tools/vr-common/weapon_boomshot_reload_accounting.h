#pragma once
#include "weapon_boomshot_types.h"

namespace gears_weapon {
// Sole inventory and reload request owner for the private mechanical candidate.
// Native authenticates frame/intent, carrier identity and original receipts.
class BoomshotReloadAccounting {
    using Ledger=ReloadTransaction<BoomshotIdentity>;
    Ledger ledger_{};
    std::uint64_t binding_{},serial_{},plan_{},ammo_{},insert_phase_{};
    std::int64_t perfect_end_{};
    static constexpr ReloadTxnInventory<BoomshotIdentity> convert(BoomshotInventory const& i){
        return {i.identity,i.revision,1,i.loaded,i.reserve,i.infinite_reserve};
    }
public:
    using Request=ReloadTxnRequest<BoomshotIdentity>;
    using Receipt=ReloadTxnReceipt<BoomshotIdentity>;
    constexpr bool bind_idle(BoomshotInventory const& i,std::uint64_t binding){
        if(!binding||binding<=binding_)return false;
        auto next=ledger_;
        if(next.stage()==ReloadTxnStage::unbound){if(!next.bind(convert(i)))return false;}
        else if(!next.rebind_closed(convert(i)))return false;
        ledger_=next;binding_=binding;return true;
    }
    constexpr bool sync_idle(BoomshotInventory const& i){return ledger_.sync_idle(convert(i));}
    constexpr bool begin(BoomshotReloadConfig const& c,BoomshotFrame const& f,
                         BoomshotReloadStart const& s,std::uint64_t intent,ReloadIntentSource source){
        const auto i=convert(f.observed);std::uint32_t units{};
        if(!c.valid()||!f.stamp.valid()||!(f.stamp.identity==c.identity)||
           !(i.identity==c.identity)||s.binding!=f.stamp.binding||
           !f.left_tracked||!f.right_tracked||!f.right_hand_equipped||
           !f.original_interaction_allowed||f.original_owns_hands||
           (source!=ReloadIntentSource::mechanism_open&&source!=ReloadIntentSource::auxiliary_button)||
           !i.valid()||i.loaded||(!i.reserve&&!i.infinite_reserve)||
           !s.original_reload_accepted||!s.automatic_completion_suppressed||
           !s.plan.valid(c.identity,c.config_id,BoomshotCondition::empty,units)||
           s.perfect.end_ticks<0)return false;
        if(s.perfect.current_instance_verified&&(!(s.perfect.identity==c.identity)||
           s.perfect.config_id!=c.config_id||s.perfect.original_reload_serial!=s.original_reload_serial))return false;
        auto next=ledger_;
        if(next.stage()==ReloadTxnStage::unbound){if(!next.bind(i))return false;}
        else if(!(next.inventory().identity==i.identity)){
            if(s.binding<=binding_||!next.rebind_closed(i))return false;
        }else if(s.binding!=binding_||!next.sync_idle(i))return false;
        if(!next.begin(i,intent,source,s.original_reload_serial,ReloadTxnMode::round,1,f.stamp.game_tick))return false;
        ledger_=next;binding_=s.binding;serial_=s.original_reload_serial;
        plan_=s.plan.plan_id;ammo_=c.ammunition_type;
        insert_phase_=s.plan.direct_exchange?s.plan.steps[0].phase_id:s.plan.steps[1].phase_id;
        perfect_end_=boomshot_perfect_duration_us;return true;
    }
    constexpr bool begin(BoomshotInstanceConfig const& c,BoomshotFrame const& f,
                         BoomshotReloadStart const& s,std::uint64_t intent,ReloadIntentSource source){
        return c.valid()&&begin(c.reload(),f,s,intent,source);
    }
    constexpr bool begin_inspection(BoomshotInspectionConfig const& c,BoomshotFrame const& f,
        BoomshotInspectionStart const& s,std::uint64_t intent){
        const auto i=convert(f.observed);std::uint32_t units{};
        if(!c.valid()||!f.stamp.valid()||!(f.stamp.identity==c.identity)||!(i.identity==c.identity)||
           !i.valid()||s.binding!=f.stamp.binding||s.binding!=binding_||
           !f.left_tracked||!f.right_tracked||!f.right_hand_equipped||!f.original_interaction_allowed||f.original_owns_hands||
           !s.original_gun_animation_owned||!s.native_reload_absent||!s.animation_serial||
           !s.plan.valid(c.identity,c.config_id,BoomshotCondition::inspection,units)||
           (i.loaded<i.capacity&&(i.infinite_reserve||i.reserve)))return false;
        auto next=ledger_;
        if(!next.sync_idle(i)||!next.begin(i,intent,ReloadIntentSource::mechanism_open,0,ReloadTxnMode::round,1,f.stamp.game_tick)||
           !next.request_close())return false;
        ledger_=next;serial_=0;plan_=s.plan.plan_id;ammo_=0;insert_phase_=0;perfect_end_=0;return true;
    }
    constexpr bool begin_inspection(BoomshotInstanceConfig const& c,BoomshotFrame const& f,
        BoomshotInspectionStart const& s,std::uint64_t intent){
        return c.valid()&&begin_inspection(c.inspection(),f,s,intent);
    }
    constexpr bool reserve(BoomshotAmmoReceipt const& r){
        return r.binding==binding_&&r.original_reload_serial==serial_&&r.plan_id==plan_&&
            r.phase_id==insert_phase_&&r.ammunition_type==ammo_&&r.units==1&&
            r.kind==BoomshotAmmoKind::verified_carrier&&r.source==BoomshotAmmoSource::left_shoulder&&
            r.left_grip_held&&ledger_.reserve_item(r.item_serial);
    }
    constexpr bool release(std::uint64_t item){return ledger_.release_item(item);}
    constexpr bool insert(std::uint64_t item){return ledger_.request_insertion(item);}
    constexpr bool take(Request& out){return ledger_.take_request(out);}
    constexpr bool dispatched(Request& out)const{return ledger_.dispatched_request(out);}
    constexpr bool acknowledge(Receipt const& r){return ledger_.acknowledge(r);}
    // Mechanical owner must first authenticate the actual closing stroke.
    constexpr bool close(std::int64_t tick){return ledger_.confirm_closed(tick,perfect_end_);}
    constexpr bool interrupt(){return ledger_.interrupt();}
    constexpr bool unknown_outcome(){return ledger_.require_reconciliation();}
    constexpr bool resume(BoomshotInventory const& i,std::uint64_t intent,std::uint64_t binding,std::uint64_t serial){
        return binding==binding_&&serial==serial_&&ledger_.resume(convert(i),intent,serial);
    }
    constexpr BoomshotInventory inventory()const{auto i=ledger_.inventory();return {i.identity,i.revision,i.loaded,i.reserve,i.infinite_reserve};}
    constexpr ReloadTxnStage stage()const{return ledger_.stage();}
    constexpr std::uint32_t reserved()const{return ledger_.reserved();}
    constexpr std::uint32_t available()const{return ledger_.available_reserve();}
    constexpr bool ready()const{return ledger_.mechanical_ready();}
};
}
