// Bow-only typed state readback. It is not action permission and does not
// advance/reset ChargeTimeCount. Cleanup can read an exact retained lifetime
// after pose expiry, while every original object and selected Pawn link holds.
#include "weapon_torque_bow_pending.inl"
#include "weapon_torque_bow_tick_probe.inl"
#include "weapon_torque_bow_controller_fire.inl"
namespace torquebow_state {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
#include "weapon_torque_bow_observe_fields.inl"
unsigned read_gate{};wchar_t const* failed_field=L"";int pending_count{},pending_capacity{};
struct Observed {
    Name state{};std::uint64_t fingerprint{};
    lancerreload_inventory::Counts inventory{};
    unsigned fire_mode{};
    bool active{},charge{},fire{},input_down{},fire_requested{},pending_fire{},pending_end{},pawn_charging{},
         charge_particle{},aim_audio{},trail{},failed_trail{},endpoint{},full_endpoint{},failed_endpoint{},
         simulation_visible{},simulation_collision{},simulation_ticking{},tension_timer{},max_charge_timer{};
    bool stopped() const noexcept {
        return !charge&&!fire&&!input_down&&!fire_requested&&!pending_fire&&!pending_end&&!pawn_charging&&
            !charge_particle&&!aim_audio&&!trail&&!failed_trail&&!endpoint&&!full_endpoint&&
            !failed_endpoint&&!simulation_visible&&!simulation_collision&&!simulation_ticking&&
            !tension_timer&&!max_charge_timer&&fire_mode==0;
    }
};
bool actors(hand::Frame const& f) noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField pc{},pawn{},world{},instigator{};
    r.reuse_initial_identities=true; // Discovery only, confined to this one actor query.
    const bool discovered=meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,f.world)&&
        weaponhand::object(r,f.controller)&&weaponhand::object(r,f.pawn)&&weaponhand::object(r,f.weapon)&&
        weaponhand::derives(r,f.weapon.address,L"Weap_Bow_base",L"WarfareGame")&&
        weaponhand::pointer(r,f.controller.address,L"Pawn",f.pawn.address,pc)&&
        weaponhand::pointer(r,f.pawn.address,L"WorldInfo",f.world.address,world)&&
        weaponhand::pointer(r,f.pawn.address,L"Weapon",f.weapon.address,pawn)&&
        weaponhand::pointer(r,f.weapon.address,L"Instigator",f.pawn.address,instigator);
    r.reuse_initial_identities=false; // Full identity/metadata closure is always fresh.
    if(!discovered)return false;
    // Retain the four exact selected layouts for a fresh closing check, too.
    for(auto const* field:{&pc,&world,&pawn,&instigator}){
        if(schema.field_count==schema.fields.size())return false;
        schema.fields[schema.field_count++]=field->property;
    }
    if(!unified_weapon_fire::retained_metadata(r))return false;
    uptr current_pc{},current_world{},current_weapon{},current_instigator{};
    return r.at(f.controller.address,pc.property.offset,current_pc)&&current_pc==f.pawn.address&&
        r.at(f.pawn.address,world.property.offset,current_world)&&current_world==f.world.address&&
        r.at(f.pawn.address,pawn.property.offset,current_weapon)&&current_weapon==f.weapon.address&&
        r.at(f.weapon.address,instigator.property.offset,current_instigator)&&current_instigator==f.pawn.address;
}
#include "weapon_torque_bow_state_fields.inl"
bool timer(uptr weapon,wchar_t const* label,bool& active) noexcept {
    playground::Args fn(weapon,label);
    if(!torquebow_original::original_function(fn,L"Weap_Bow_base",L"WarfareGame"))return false;
    const auto name=get<Name>(fn.r.function+0x48);
    return !name.number&&lancerreload_state::timer_active(weapon,name,active);
}
bool tick_enabled(uptr actor,bool& out) noexcept {
    return torquebow_tick_probe::read(actor,out);
}
bool read_once(hand::Frame const& source,Observed& out) noexcept {
    out={};read_gate=2;failed_field=L"";pending_count=pending_capacity=-1;if(!actors(source))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    read_gate=3;if(!meta::tables(base,schema.objects,schema.names))return false;
    Fields fields{r};Observed value{};const auto weapon=source.weapon.address;
    playground::Args state(weapon,L"GetStateName");meta::NameText state_label{};
    read_gate=4;if(!torquebow_original::original_function(state,L"Object",L"Core")||state.r.size!=8||
       !vrequip::result_property(state,L"NameProperty",8)||!state.run())return false;
    value.state=state.output<Name>();
    read_gate=5;if(value.state.number||!meta::fresh_name(schema.names,value.state.index,state_label))return false;
    value.active=meta::exact_text(state_label,L"Active");
    value.charge=meta::exact_text(state_label,L"Charge");value.fire=meta::exact_text(state_label,L"Fire");
    fields.hash.word(value.state.index);fields.hash.word(value.state.number);
    // Discovery only, local to this read_once. Values remain fresh. The
    // complete selector/metadata/actor closure below uses fresh identities;
    // neither another read_once nor an original call inherits this switch.
    r.reuse_initial_identities=true;
    if(!fields.weapon.begin(r,weapon))return false;
    uptr inv_manager{},charge_fx{},audio{},trail_fx{},failed_fx{},endpoint_actor{},full_actor{},failed_actor{},simulation{};
    read_gate=6;if(!fields.flag(source.controller.address,L"bHoldingButton_R2",value.input_down)||
       !fields.flag(weapon,L"bPendingEndFire",value.pending_end,BowObserveFields::pending_end)||
       !fields.flag(source.pawn.address,L"bChargingBow",value.pawn_charging)||
       !fields.pointer(weapon,L"InvManager",inv_manager,false,BowObserveFields::inv_manager)||!inv_manager||
       !weaponhand::derives(r,inv_manager,L"InventoryManager",L"Engine"))return false;
    torquebow_controller_fire::Value fire_request{};
    if(!torquebow_controller_fire::read(r,source.controller,fire_request)){failed_field=L"bFire";return false;}
    value.fire_requested=fire_request.byte!=0;fields.hash.word(fire_request.byte);
    weaponhand::BoolField mode{};torquebow_pending::Observed bow_pending_readback{};
    read_gate=7;
    const auto pending_known=torquebow_pending::read(r,inv_manager,bow_pending_readback);
    const auto& pending_values=bow_pending_readback.values;
    pending_count=pending_values.count;pending_capacity=pending_values.capacity;
    if(!pending_known){read_gate=70+bow_pending_readback.gate;return false;}
    read_gate=8;if(!fields.retain(bow_pending_readback.property)||!fields.retain(bow_pending_readback.element))return false;
    const auto primary=bow_pending_readback.primary;unsigned char fire_mode{};
    read_gate=9;if(
       !fields.weapon.binding(r,BowObserveFields::fire_mode,mode)||!fields.retain(mode.property)||
       !r.at(weapon,mode.property.offset,fire_mode))return false;
    value.pending_fire=primary!=0;value.fire_mode=fire_mode;
    fields.hash.word(pending_values.data);fields.hash.word(unsigned(pending_values.count));fields.hash.word(unsigned(pending_values.capacity));
    fields.hash.word(primary);fields.hash.word(fire_mode);
    read_gate=10;if(!fields.pointer(weapon,L"PSC_TorqueBowChargeUp",charge_fx,true,BowObserveFields::charge_fx)||!fields.particle(charge_fx,value.charge_particle)||
       !fields.pointer(weapon,L"AimingSound",audio,true,BowObserveFields::audio)||
       (audio&&(!weaponhand::derives(r,audio,L"AudioComponent",L"Engine")||!fields.flag(audio,L"bWasPlaying",value.aim_audio)))||
       !fields.pointer(weapon,L"PSC_Trail",trail_fx,true,BowObserveFields::trail_fx)||!fields.particle(trail_fx,value.trail)||
       !fields.pointer(weapon,L"PSC_FailedTrail",failed_fx,true,BowObserveFields::failed_fx)||!fields.particle(failed_fx,value.failed_trail)||
       !fields.pointer(weapon,L"EM_EndPoint",endpoint_actor,false,BowObserveFields::endpoint)||!fields.emitter(endpoint_actor,value.endpoint)||
       !fields.pointer(weapon,L"EM_EndPointFullCharge",full_actor,false,BowObserveFields::full_endpoint)||!fields.emitter(full_actor,value.full_endpoint)||
       !fields.pointer(weapon,L"EM_EndPointFail",failed_actor,false,BowObserveFields::failed_endpoint)||!fields.emitter(failed_actor,value.failed_endpoint)||
       !fields.pointer(weapon,L"SimulatedGrenade",simulation,false,BowObserveFields::simulation))return false;
    read_gate=11;if(simulation){bool hidden{},collide{},block{};
        if(!weaponhand::derives(r,simulation,L"Actor",L"Engine")||!fields.flag(simulation,L"bHidden",hidden)||
           !fields.flag(simulation,L"bCollideActors",collide)||!fields.flag(simulation,L"bBlockActors",block)||
           !tick_enabled(simulation,value.simulation_ticking))return false;
        value.simulation_visible=!hidden;value.simulation_collision=collide||block;
        fields.hash.word(value.simulation_ticking);}
    r.reuse_initial_identities=false;
    read_gate=12;if(!timer(weapon,L"TensionIntroEnded",value.tension_timer)||!timer(weapon,L"PlayMaxCharge",value.max_charge_timer)||
       !lancerreload_inventory::query(weapon,value.inventory)||value.inventory.capacity!=1||
       !fields.weapon.finish(r)||!unified_weapon_fire::retained_metadata(r)||!actors(source))return false;
    fields.hash.word(value.tension_timer);fields.hash.word(value.max_charge_timer);fields.hash.word(value.inventory.used);
    fields.hash.word(value.inventory.reserve);unified_weapon_fire::schema_fingerprint(schema,fields.hash);
    value.fingerprint=fields.hash.value;out=value;read_gate=0;return true;
}
bool read(hand::Frame const& source,Observed& out,hand::ExistingActionLifetime const* cleanup=nullptr) noexcept {
    out={};const auto allowed=[&]() noexcept {return cleanup?nativeequipment::restoration_quiet()&&
        weaponhand::client.existing_action_lifetime_on_gt(*cleanup):nativeequipment::current(nullptr,source);};
    read_gate=1;if(!allowed())return false;Observed first{},second{};
    if(!read_once(source,first)||!read_once(source,second))return false;
    read_gate=13;if(!first.fingerprint||first.fingerprint!=second.fingerprint)return false;
    read_gate=14;if(!allowed())return false;
    out=second;read_gate=0;return true;
}
bool read_owned(hand::Frame const& source,Observed& out,torquebow_cleanup::Anchor const& cleanup,
    std::uint64_t serial,torquebow_cleanup::Owns owns) noexcept {
    out={};if(!cleanup.current(source,serial,owns))return false;
    Observed first{},second{};
    if(!read_once(source,first)||!read_once(source,second)||!first.fingerprint||first.fingerprint!=second.fingerprint||
       !cleanup.current(source,serial,owns))return false;
    out=second;return true;
}
}
