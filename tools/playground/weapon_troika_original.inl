// Ordinary GT original entry adapter. No input, tracking, C0 or hook writer.
// The Troika runtime must supply its current one-shot mount request after raw
// handle-surface contact. This adapter never discovers contact from skin.
namespace troika_original {
namespace hand=gears_weapon_hand;
enum class EntryResult {rejected,original_rejected,mounted,unresolved};
struct EntryReceipt {
    EntryResult result{EntryResult::rejected};
    hand::FrameKey source_key{};
    gears_vr::StereoSource source{gears_vr::StereoSource::RuntimeTracked};
    hand::ObjectIdentity world{},controller{},driver{},turret{};
    std::uint64_t token{},binding{};bool dispatched{},returned{},original_accepted{};
};
struct EntryOwner {
    std::uint64_t binding{},last_token{};
    EntryReceipt receipt{};
    bool outstanding{},leave_dispatched{};
    EntryOwner()=default;
    EntryOwner(EntryOwner const&)=delete;
    EntryOwner& operator=(EntryOwner const&)=delete;
};
bool same_object(hand::ObjectIdentity const& expected) noexcept {
    hand::ObjectIdentity live{};
    return nativeequipment::identify(nullptr,expected.address,live)&&hand::same(live,expected);
}
template<std::size_t N>bool link(uptr object,wchar_t const (&label)[N],uptr expected,bool component=false) noexcept {
    namespace meta=weaponhand::meta;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    meta::PointerField field{};
    return meta::tables(base,schema.objects,schema.names)&&weaponhand::pointer(reader,object,label,expected,field,component)&&
        unified_weapon_fire::retained_metadata(reader);
}
// ClaimTurret/DriverEnter remain inside the original TryToDrive path. Calling
// them independently would duplicate the original vehicle ownership sequence.
EntryReceipt enter(EntryOwner& owner,hand::Frame const& f,hand::ObjectIdentity const& turret,
    gears_weapon::TroikaRequest const& request,std::atomic<bool> const& cancellation) noexcept {
    EntryReceipt out{};out.token=request.token;out.binding=request.stamp.binding;
    const auto& id=request.stamp.identity;
    const auto now=weaponhand::clock_ns(nullptr);
    const auto entry_reject=[&](unsigned reason){
#if defined(GEARS_TROIKA_ENTRY_DIAGNOSTIC)
        static unsigned entry_reject_rows{};
        if(log_file&&entry_reject_rows<12){++entry_reject_rows;const auto entry_observed_ns=weaponhand::clock_ns(nullptr);
            fwprintf(log_file,L"troika_entry_entry_reject reason=%u frame=%llu sequence=%llu token=%llu input_age_ns=%lld remaining_ns=%lld request_age_ns=%lld\n",
                reason,static_cast<unsigned long long>(f.key.frame),static_cast<unsigned long long>(f.controllers.sequence),
                static_cast<unsigned long long>(request.token),static_cast<long long>(entry_observed_ns-f.controllers.sync_ns),
                static_cast<long long>(request.valid_until_ns-entry_observed_ns),static_cast<long long>(entry_observed_ns-request.stamp.monotonic_ns));
            fflush(log_file);
        }
#else
        (void)reason;
#endif
        return out;
    };
    if(cancellation.load(std::memory_order_acquire)||owner.outstanding||
       (owner.binding==request.stamp.binding&&request.token<=owner.last_token))return entry_reject(1);
    if(request.kind!=gears_weapon::TroikaAction::mount_original||!request.token||!request.stamp.binding||
       request.frame_ticket!=f.key.frame)return entry_reject(2);
    if(id.world!=f.world.address||id.pawn!=f.pawn.address||id.controller!=f.controller.address||id.turret!=turret.address||
       id.world_generation!=f.key.world_generation||id.xr_session!=f.controllers.session||id.space_epoch!=f.controllers.space_epoch||
       id.input_epoch!=f.controllers.input_epoch||id.profile_generation!=f.controllers.profile_generation)return entry_reject(3);
    if(request.stamp.monotonic_ns<=0||request.stamp.monotonic_ns>now||request.valid_until_ns<now||
       request.valid_until_ns-request.stamp.monotonic_ns>100000000)return entry_reject(4);
    if(!nativeequipment::current(nullptr,f))return entry_reject(5);
    if(!same_object(turret))return entry_reject(6);
    if(!playground::instance(turret.address,L"Turret_TroikaCabal_base")||playground::deleted(turret.address))return entry_reject(7);
    if(!link(turret.address,L"WorldInfo",f.world.address))return entry_reject(8);
    if(!link(turret.address,L"Driver",0))return entry_reject(9);
    if(vrequip::model.held()||vrequip::model.switch_pending()||vrequip::model.animation_owns_hands())return entry_reject(10);
    // Full public hand arbitration is re-read; a pending/unknown participant
    // is not an empty hand, even if the currently visible skin is open.
    hand::Frame current{};hand::GripGameState game{};hand::Decision decision{};
    if(!weaponhand::client.copy_frame_on_gt(current,game,decision))return entry_reject(11);
    if(!hand::same_frame(current,f))return entry_reject(12);
    if(!game.known||game.main_hand_held||game.busy||game.animation_owns_hands)return entry_reject(13);
    if(decision.lease.kind!=hand::LeaseKind::none||game.object_incumbents[0].kind!=hand::LeaseKind::none||
       game.object_incumbents[1].kind!=hand::LeaseKind::none)return entry_reject(14);
    playground::Args can(turret.address,L"CanEnterVehicle"),drive(turret.address,L"TryToDrive");
    const auto valid_call=[&](playground::Args& call){
        return call.valid&&hammerburst_animation::input_field(call,call.prop(L"P"),L"ObjectProperty",8)&&
            vrequip::result_property(call,L"BoolProperty",4)&&call.set(L"P",f.pawn.address).valid;
    };
    if(!valid_call(can))return entry_reject(15);
    if(!valid_call(drive))return entry_reject(16);
    if(!nativeequipment::current(nullptr,f))return entry_reject(17);
    if(!same_object(turret))return entry_reject(18);
    if(!can.run())return entry_reject(19);
    if(!can.output<unsigned>())return entry_reject(20);
    if(!nativeequipment::current(nullptr,f))return entry_reject(21);
    if(!same_object(turret))return entry_reject(22);
    if(!link(turret.address,L"Driver",0))return entry_reject(23);
    if(vrequip::model.held()||vrequip::model.switch_pending()||vrequip::model.animation_owns_hands()||
       cancellation.load(std::memory_order_acquire))return entry_reject(24);
    // Dispatch once. The runtime retains this receipt if the invocation or
    // possession result is unknown; it must not retry the same contact/token.
    out.source_key=f.key;out.source=f.stereo.source;out.world=f.world;out.controller=f.controller;out.driver=f.pawn;out.turret=turret;
    out.token=request.token;out.binding=request.stamp.binding;out.dispatched=true;
    owner.binding=out.binding;owner.last_token=out.token;owner.leave_dispatched=false;owner.outstanding=true;
    out.returned=drive.run();
    out.original_accepted=out.returned&&drive.output<unsigned>()!=0;
    if(!same_object(f.world)||!same_object(f.controller)||!same_object(f.pawn)||!same_object(turret)){
        out.result=EntryResult::unresolved;owner.receipt=out;return out;
    }
    const bool mounted=link(f.controller.address,L"Pawn",turret.address)&&
        link(turret.address,L"Driver",f.pawn.address)&&link(f.pawn.address,L"DrivenVehicle",turret.address);
    const bool rejected=out.returned&&!out.original_accepted&&link(f.controller.address,L"Pawn",f.pawn.address)&&
        link(turret.address,L"Driver",0)&&link(f.pawn.address,L"DrivenVehicle",0);
    if(mounted)out.result=EntryResult::mounted;
    else if(rejected)out.result=EntryResult::original_rejected;
    else out.result=EntryResult::unresolved;
    owner.receipt=out;
    if(out.result==EntryResult::original_rejected)owner.outstanding=false;
    return out;
}
// Retire only this old mount ledger after an authenticated new context.
// No old-pawn/turret write is sent, including when another driver took over.
// A future owned-fire ledger must settle independently before retirement.
bool retire_for_context_change(EntryOwner& owner,hand::Frame const& current) noexcept {
    if(!owner.outstanding||owner.receipt.result!=EntryResult::mounted||
       !nativeequipment::current(nullptr,current))return false;
    const auto& r=owner.receipt;const auto& a=r.source_key;const auto& b=current.key;
    if(a.game_pid!=b.game_pid||a.game_start!=b.game_start||a.native_started_qpc!=b.native_started_qpc||
       !a.world_generation||b.world_generation<a.world_generation)return false;
    if(!hand::same(r.world,current.world)){
        if(b.world_generation<=a.world_generation)return false;
    }else{
        if(hand::same(r.driver,current.pawn)||b.lifecycle<=a.lifecycle||!same_object(r.turret))return false;
        namespace meta=weaponhand::meta;
        meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
        meta::PointerField driver{},world{};uptr closing_driver{},closing_world{};
        if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,r.turret)||
           !weaponhand::read_pointer(reader,r.turret.address,L"Driver",driver)||driver.value==r.driver.address||
           !weaponhand::pointer(reader,r.turret.address,L"WorldInfo",r.world.address,world))return false;
        for(auto const& field:{driver,world}){
            if(schema.field_count==schema.fields.size())return false;
            schema.fields[schema.field_count++]=field.property;
        }
        if(!reader.at(r.turret.address,driver.property.offset,closing_driver)||closing_driver!=driver.value||
           !reader.at(r.turret.address,world.property.offset,closing_world)||closing_world!=world.value||
           !unified_weapon_fire::retained_metadata(reader))return false;
    }
    if(!nativeequipment::current(nullptr,current))return false;
    owner.outstanding=false;return true;
}
// Resolve a delayed original possession transition without replaying TryToDrive.
bool observe_result(EntryOwner& owner) noexcept {
    if(!owner.outstanding||!weaponhand::current_gt(nullptr))return false;
    auto& r=owner.receipt;
    if(!same_object(r.world)||!same_object(r.controller)||!same_object(r.driver)||!same_object(r.turret))return false;
    if(link(r.controller.address,L"Pawn",r.turret.address)&&link(r.turret.address,L"Driver",r.driver.address)&&
       link(r.driver.address,L"DrivenVehicle",r.turret.address)){
        r.result=EntryResult::mounted;return true;
    }
    if(r.result==EntryResult::unresolved&&r.returned&&!r.original_accepted&&
       link(r.controller.address,L"Pawn",r.driver.address)&&link(r.turret.address,L"Driver",0)&&
       link(r.driver.address,L"DrivenVehicle",0)){
        r.result=EntryResult::original_rejected;owner.outstanding=false;return true;
    }
    return false;
}
// Shutdown-only original leave, once, for this exact mounted owner. Gameplay
// exit remains the original controller mapping. Future fire owner must settle
// its owned R2 before calling this function; no forced-unclaim shortcut exists.
bool leave(EntryOwner& owner) noexcept {
    if(!owner.outstanding||owner.leave_dispatched||!weaponhand::current_gt(nullptr)||
       !nativeequipment::restoration_quiet()||!observe_result(owner)||
       owner.receipt.result!=EntryResult::mounted)return false;
    const auto& r=owner.receipt;
    playground::Args call(r.turret.address,L"DriverLeave");
    if(!call.valid||!hammerburst_animation::input_field(call,call.prop(L"bForceLeave"),L"BoolProperty",4)||
       !vrequip::result_property(call,L"BoolProperty",4)||!call.flag(L"bForceLeave",true).valid||
       !same_object(r.turret)||!link(r.turret.address,L"Driver",r.driver.address)||
       !link(r.controller.address,L"Pawn",r.turret.address)||!link(r.driver.address,L"DrivenVehicle",r.turret.address))return false;
    owner.leave_dispatched=true;return call.run();
}
// Read-only release of THIS ledger after original exit. No DriverLeave retry,
// turret unclaim or cleanup write may be sent to a later occupant/new owner.
bool observe_departed(EntryOwner& owner,std::uint64_t binding,std::uint64_t token) noexcept {
    if(!owner.outstanding||owner.binding!=binding||owner.last_token!=token||
       !weaponhand::current_gt(nullptr))return false;
    const auto& r=owner.receipt;
    if(r.result!=EntryResult::mounted)return false;
    namespace meta=weaponhand::meta;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,r.world)||
       !weaponhand::object(reader,r.controller)||!weaponhand::object(reader,r.driver)||!weaponhand::object(reader,r.turret))return false;
    // Vehicle.DriverLeave restores possession before DriverLeft clears Driver.
    // A restored Pawn alone is not completion of our old turret association.
    // A later occupant is permitted here, but is never read through or changed.
    const std::array<uptr,6> objects{r.controller.address,r.driver.address,r.driver.address,
        r.turret.address,r.turret.address,r.turret.address};
    std::array<meta::PointerField,6> fields{};
    if(!weaponhand::pointer(reader,objects[0],L"Pawn",r.driver.address,fields[0])||
       !weaponhand::pointer(reader,objects[1],L"DrivenVehicle",0,fields[1])||
       !weaponhand::pointer(reader,objects[2],L"Controller",r.controller.address,fields[2])||
       !weaponhand::pointer(reader,objects[3],L"WorldInfo",r.world.address,fields[3])||
       !weaponhand::read_pointer(reader,objects[4],L"Driver",fields[4])||fields[4].value==r.driver.address||
       !weaponhand::read_pointer(reader,objects[5],L"Controller",fields[5])||fields[5].value==r.controller.address)return false;
    for(auto const& field:fields){
        if(schema.field_count==schema.fields.size())return false;
        schema.fields[schema.field_count++]=field.property;
    }
    for(std::size_t i=0;i<fields.size();++i){uptr closing{};
        if(!reader.at(objects[i],fields[i].property.offset,closing)||closing!=fields[i].value)return false;
    }
    if(!unified_weapon_fire::retained_metadata(reader))return false;
    owner.outstanding=false;return true;
}
} // namespace troika_original
