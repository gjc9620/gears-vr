// Equipment action adapters run on normal PostPlayerTick, never Native
// prepare/commit, observe, rollback/NoAuth or input workers. The dedicated
// read_attached_frame helper is metadata only and grants no action permission.
namespace nativeequipment {
namespace runtime=::gears_equipment_runtime;
namespace abi=::gears_weapon_hand;
using Snapshot=::gears_weapon::EquipmentSnapshot;
gears_equipment_input::Input input;
gears_equipment_input::Evidence input_evidence;
gears_current_tracking::Reader input_continuity_reader;
std::uint64_t input_continuity_matched{},input_continuity_unavailable{},input_continuity_gaps{};
std::uint64_t input_continuity_native_selected{};
gears_weapon::ArmspanCalibration calibration;
bool calibration_measured{}; // guarded by the same configuration fence
bool domain_ready() noexcept;
bool restoration_quiet() noexcept;
namespace stow_native {bool profile() noexcept;}
std::uint64_t domain_rejected{};
std::uint64_t ticks{},frame_unavailable{},frames{},other_controller{},observer_budget_skips{},saw_pose_gap_holds{};
const wchar_t* inventory_stage=L"none";
std::array<std::uint64_t,static_cast<std::size_t>(runtime::Status::clock_invalid)+1> outcomes{};
runtime::Runtime state;
runtime::Status last_status{};
bool have_status{};
unsigned input_gap_logs{};

bool configure_native(HMODULE native) noexcept {
    gears_weapon::ArmspanCalibration candidate{};
    const bool loaded=runtime::load_native_armspan(native,candidate);
    calibration_measured=loaded;
    // Called only with ConfigurationWrite held. A reconnect must not borrow
    // a previous session's measurement if this Native has no valid one yet.
    const auto next=loaded?candidate:(unified_weapon_fire::manual_profile()?
        gears_weapon::default_body_slot_layout():gears_weapon::ArmspanCalibration{});
    const bool changed=calibration.armspan_metres!=next.armspan_metres||
        calibration.hands_below_head_metres!=next.hands_below_head_metres;
    calibration=next;
    if(log_file&&changed){fwprintf(log_file,L"native_equipment_calibration loaded=%u armspan=%.9g hands_below_head=%.9g source=%hs\n",
        unsigned(loaded),calibration.armspan_metres,calibration.hands_below_head_metres,
        loaded?"bound_native_capture_ini":"authored_default_slot_layout_not_measurement");fflush(log_file);}
    return loaded;
}

std::uint64_t clock_ms(void*) noexcept {return GetTickCount64();}
std::int64_t clock_ns(void*) noexcept {return weaponhand::clock_ns(nullptr);}
bool current(void*,abi::Frame const& frame) noexcept {
    return enabled.load(std::memory_order_acquire)&&weaponhand::current_gt(nullptr)&&domain_ready()&&
        weaponhand::client.source_current_on_gt(frame.key)&&abi::frame_active(frame,clock_ns(nullptr));
}
void reset_input(void*) noexcept {input.reset();}
bool read_input_continuity(abi::Frame const& frame,gears_hand_continuity::TrackedInput& tracking,
    gears_hand_continuity::Publisher& publisher,gears_hand_continuity::TrackedInput* right_press=nullptr,
    gears_current_tracking::Failure* failure=nullptr,gears_hand_continuity::MotionHistory* history=nullptr) noexcept {
    tracking={};publisher={};if(right_press)*right_press={};if(history)*history={};
    auto result=gears_current_tracking::Failure::wrong_owner;
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    // Reuse Native's exact Scene selection, including its actual producer
    // witness. No late second ring read is needed for an already matched
    // companion. Historical press/motion requests still use the ring below.
    if(!right_press&&!history&&current(nullptr,frame)){
        gears_rig_tracking::Frame selected{};
        const unsigned source=unsigned(frame.stereo.source);
        const auto read=handrig::read(selected,0,source);
        if(read.reply==gears_rig_tracking::Reply::accepted&&current(nullptr,frame)&&
           gears_equipment_rig::continuity(frame,selected,clock_ns(nullptr),source,tracking,publisher)){
            ++input_continuity_native_selected;
            if(failure)*failure=gears_current_tracking::Failure::none;return true;
        }
    }
#endif
    if(weaponhand::current_gt(nullptr)&&domain_ready())
        result=input_continuity_reader.read_matching(GetCurrentProcessId(),[](){return clock_ns(nullptr);},
            (frame.flags&abi::allow_sim)!=0,frame.stereo,frame.controllers,tracking,publisher,right_press,history);
    if(failure)*failure=result;
    return result==gears_current_tracking::Failure::none;
}
bool next(void*,abi::Frame const& frame,std::uint64_t& cursor,gears_weapon::EquipmentIntentEvent& event) noexcept {
    const auto input_begin_ns=clock_ns(nullptr);
    if(!current(nullptr,frame))return false;
    const auto validated_ns=clock_ns(nullptr);
    gears_equipment_input::Continuity continuity{};
    gears_current_tracking::Failure continuity_failure{};
    const bool matched=read_input_continuity(frame,continuity.tracking,continuity.publisher,&continuity.right_press,&continuity_failure);
    const auto continuity_ns=clock_ns(nullptr);
    if(matched)++input_continuity_matched;else ++input_continuity_unavailable;
    const bool index_acquire=stow_native::profile()&&!vrequip::model.held()&&
        !vrequip::model.switch_pending()&&!vrequip::model.animation_owns_hands();
    const bool available=input.next(frame,calibration,clock_ns(nullptr),clock_ms(nullptr),cursor,event,
        matched?&continuity:nullptr,index_acquire);
    // Runtime may drop its baseline immediately after next rejects. Retain
    // only diagnostics, never another frame or an action to retry.
    input_evidence=input.evidence();
    if(input_evidence.continuous_source_gap)++input_continuity_gaps;
    // Bounded timing evidence at the actual input rejection. This is not a new
    // poll, timestamp substitution, event retry or freshness allowance.
    if(log_file&&input_gap_logs<16&&input_evidence.status==gears_equipment_input::Status::source_gap){
        const auto& s=input_evidence;
        fwprintf(log_file,L"native_equipment_input_gap reason=%hs frame=%llu source_sequence=%llu observed_ns=%lld previous_observed_ns=%lld previous_sync_ns=%lld sync_ns=%lld received_ns=%lld frame_now_ns=%lld continuity_failure=%u continuity_matched=%u right_press_sync_ns=%lld right_press_sequence=%llu right_pressed=%u right_released=%u validation_ns=%lld continuity_read_ns=%lld\n",
            gears_equipment_input::gap_name(s.gap),frame.key.frame,s.sequence,s.observed_ns,s.previous_observed_ns,
            s.previous_sync_ns,s.sync_ns,s.received_ns,frame.now_ns,unsigned(continuity_failure),unsigned(matched),
            continuity.right_press.input.sync_ns,continuity.right_press.input.sequence,
            frame.controllers.hands[1].pressed[gears_vr::control_squeeze],frame.controllers.hands[1].released[gears_vr::control_squeeze],
            validated_ns-input_begin_ns,continuity_ns-validated_ns);fflush(log_file);++input_gap_logs;
    }
    return available;
}
void clear_hold(void*) noexcept {weaponhand::held_binding={};}
bool identify(void*,std::uint64_t address,abi::ObjectIdentity& out) noexcept {
    out={};
    namespace meta=weaponhand::meta;
    meta::Table objects{},names{},after_objects{},after_names{};
    meta::Identity id{},again{};meta::NameText text{},again_text{};
    if(!address||!meta::tables(base,objects,names)||!meta::raw_identity(objects,names,address,id,text)||
       !meta::raw_identity(objects,names,address,again,again_text)||
       !meta::tables(base,after_objects,after_names)||!meta::same_table(objects,after_objects)||!meta::same_table(names,after_names))return false;
    const abi::ObjectIdentity a{id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};
    const abi::ObjectIdentity b{again.address,again.outer,again.klass,again.index,again.name.index,again.name.number,1};
    if(!abi::identity_valid(a)||!abi::same(a,b))return false;out=a;return true;
}
bool inventory(void*,abi::Frame const& frame,Snapshot& out) noexcept {
    try{
        inventory_stage=L"current_before";
        if(!current(nullptr,frame))return false;
        if(!vrequip::snapshot(frame.controller.address,out,inventory_stage))return false;
        inventory_stage=L"frame_binding";
        if(out.world!=frame.world.address||out.pawn!=frame.pawn.address||out.current_weapon!=frame.weapon.address)return false;
        inventory_stage=L"current_after";
        if(!current(nullptr,frame))return false;
        inventory_stage=L"ok";return true;
    }catch(...){inventory_stage=L"cpp_exception";out={};return false;}
}
bool held_inventory(void*,abi::Frame const& frame,Snapshot& out) noexcept {
    try {
        if(!current(nullptr,frame)||!vrequip::snapshot(frame.controller.address,out,inventory_stage,false))return false;
        return out.world==frame.world.address&&out.pawn==frame.pawn.address&&
            out.current_weapon==frame.weapon.address&&current(nullptr,frame);
    }catch(...){out={};return false;}
}
bool hand_bone(Name value) noexcept {
    if(value.number)return false;
    const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000||value.index>=unsigned(names.count))return false;
    const auto entry=get<uptr>(names.data+size_t(value.index)*8);wchar_t text[20]{};
    constexpr wchar_t expected[]=L"b_MF_Weapon_R_End";
    return entry&&get<unsigned>(entry)==value.index&&copy(text,entry+20,sizeof expected)&&!std::memcmp(text,expected,sizeof expected);
}
// Exact existing retail Attachments layout, already used by the stow trial.
// This is a read, not a cached "mesh is somewhere on Pawn" assertion: require
// a unique attachment to the actual right-hand weapon bone and a visible mesh.
// Metadata only, no source/ownership permission. The retained main-mount
// caller encloses this read in its actual before/after Game observations;
// standalone callers must continue through attached_frame below.
bool read_attached_frame(abi::Frame const& frame,uptr expected_mesh,bool allow_detached=false,
    std::array<unsigned char,52>* attachment=nullptr,bool* detached=nullptr,bool require_visible=true) noexcept {
    try{
        if(!frame.weapon.address)return false;
        const auto mesh=playground::pointer(frame.weapon.address,L"Mesh");
        if(expected_mesh&&mesh!=expected_mesh)return false;
        if(!playground::instance(mesh,L"SkeletalMeshComponent")||playground::deleted(mesh)||
           playground::pointer(frame.weapon.address,L"Instigator")!=frame.pawn.address)return false;
        weaponhand::meta::Snapshot schema{};schema.module_base=base;
        weaponhand::meta::Reader reader{schema,weaponhand::meta::Stage::final_check};weaponhand::BoolField hidden{};
        if(!weaponhand::meta::tables(base,schema.objects,schema.names)||
           !weaponhand::boolean(reader,mesh,L"HiddenGame",hidden)||(require_visible&&(hidden.bits&hidden.mask)))return false;
        const auto property=playground::property(frame.mesh.address,L"Attachments");
        if(!property||!playground::named(get<uptr>(property+0x50),L"ArrayProperty")||
           get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=sizeof(Array))return false;
        const auto offset=get<unsigned>(property+0x94),extent=get<unsigned>(get<uptr>(frame.mesh.address+0x50)+0x88);
        if(!extent||offset>extent||sizeof(Array)>extent-offset)return false;
        Array entries{};
        if(!copy(&entries,frame.mesh.address+offset,sizeof entries)||
           entries.count<0||entries.count>entries.capacity||entries.capacity>128||(entries.count&&!entries.data))return false;
        const auto inner=get<uptr>(property+0xd0),structure=get<uptr>(inner+0xd0);
        if(!registered(inner)||!registered(structure)||
           !playground::named(get<uptr>(inner+0x50),L"StructProperty")||get<unsigned>(inner+0x74)!=52||get<unsigned>(structure+0x88)!=52)return false;
        const wchar_t* labels[]={L"Component",L"BoneName",L"RelativeLocation",L"RelativeRotation",L"RelativeScale"};
        const wchar_t* kinds[]={L"ComponentProperty",L"NameProperty",L"StructProperty",L"StructProperty",L"StructProperty"};
        const unsigned offsets[]={0,8,16,28,40},sizes[]={8,8,12,12,12};
        for(unsigned i=0;i<5;++i){const auto field=playground::field(structure,labels[i],kinds[i]);
            if(!registered(field)||get<unsigned>(field+0x70)!=1||get<unsigned>(field+0x74)!=sizes[i]||get<unsigned>(field+0x94)!=offsets[i])return false;}
        unsigned matches{};uptr entry_address{};std::array<unsigned char,52> selected{};
        for(int i=0;i<entries.count;++i){std::array<unsigned char,52> bytes{};uptr component{};
            const auto address=entries.data+size_t(i)*52;
            if(!copy(bytes.data(),address,bytes.size()))return false;std::memcpy(&component,bytes.data(),8);
            if(component==mesh){selected=bytes;entry_address=address;++matches;}}
        if(matches>1||(!matches&&!allow_detached))return false;
        if(matches){
            Name bone{};std::memcpy(&bone,selected.data()+8,8);if(!hand_bone(bone))return false;
            for(unsigned vector_offset:{16u,40u})for(unsigned axis=0;axis<3;++axis){float value{};
                std::memcpy(&value,selected.data()+vector_offset+axis*4,4);if(!std::isfinite(value))return false;}
        }
        Array after{};std::array<unsigned char,52> again{};
        if(!copy(&after,frame.mesh.address+offset,sizeof after)||std::memcmp(&after,&entries,sizeof after)||
           (matches&&(!copy(again.data(),entry_address,again.size())||again!=selected))||
           playground::pointer(frame.weapon.address,L"Mesh")!=mesh)return false;
        if(attachment)*attachment=selected;if(detached)*detached=!matches;
        return true;
    }catch(...){return false;}
}
bool attached_frame(abi::Frame const& frame,uptr expected_mesh=0) noexcept {
    return frame.weapon.address&&current(nullptr,frame)&&read_attached_frame(frame,expected_mesh)&&current(nullptr,frame);
}
#include "weapon_equipment_stow_native.inl"
bool attached(void*,abi::Frame const& frame,Snapshot const& game) noexcept {
    return game.world==frame.world.address&&game.pawn==frame.pawn.address&&game.current_weapon==frame.weapon.address&&
        attached_frame(frame);
}
bool switch_slot(void*,abi::Frame const& frame,Snapshot const& game,unsigned slot) noexcept {
    try{
        if(slot>=4||!game.can_switch||game.animation_busy||game.world!=frame.world.address||game.pawn!=frame.pawn.address||
           game.current_weapon!=frame.weapon.address)return false;
        // End our old handoff before the original inventory mutation. Never
        // restore a detached old weapon after switching to a different one.
        if(stow_native::lease.dirty()&&(!stow_native::restore()||!current(nullptr,frame)))return false;
        const auto inv=playground::pointer(game.pawn,L"InvManager");
        if(!playground::instance(inv,L"WarInventoryManager")||playground::deleted(inv))return false;
        playground::Args call(inv,L"SetWeaponFromSlot");const auto parameter=call.prop(L"Slot");
        if(!call.valid||!parameter||get<unsigned>(parameter+0x70)!=1||get<unsigned>(parameter+0x74)!=1||
           !playground::named(get<uptr>(parameter+0x50),L"ByteProperty")||(get<unsigned long long>(parameter+0x78)&0x580)!=0x80||
           !call.set(L"Slot",static_cast<std::uint8_t>(slot)).valid||!current(nullptr,frame)||
           playground::pointer(game.pawn,L"InvManager")!=inv)return false;
        uptr item{};bool can_switch{};
        if(!vrequip::read_slot(inv,static_cast<std::uint8_t>(slot),item)||item!=game.weapon_in_slot[slot]||
           !vrequip::read_can_switch(frame.controller.address,can_switch)||!can_switch||!current(nullptr,frame))return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        if(!fragpreparation::stop()||!current(nullptr,frame))return false;
#endif
        if(!vrfire::yield_for_native_equipment(frame)||!current(nullptr,frame))return false;
#endif
        // Normal original inventory API. Completion is observed later; there
        // is no silent model commit, skip-animation claim, or retry here.
        const bool dispatched=call.run();
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        if(dispatched)lancerreload_runtime::switch_dispatched(frame);
#endif
        return dispatched;
    }catch(...){return false;}
}
bool empty_sound(void*,abi::Frame const& frame,Snapshot const& game,unsigned slot) noexcept {
    try{return current(nullptr,frame)&&vrequip::play_empty_slot_sound(game,slot);}catch(...){return false;}
}
bool begin_index_acquire(void*,abi::Frame const& frame,Snapshot const& game,unsigned slot) noexcept {
    if(!stow_native::profile()||slot<2||slot>=4||vrequip::model.held()||
       vrequip::model.switch_pending()||vrequip::model.animation_owns_hands()||!current(nullptr,frame))return false;
    const auto address=game.weapon_in_slot[slot];
    if(!address)return true; // Same original empty-slot sound; no hold/fire change.
    namespace meta=weaponhand::meta;
    abi::ObjectIdentity target{},after{};
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!identify(nullptr,address,target)||!meta::tables(base,schema.objects,schema.names)||
       !weaponhand::object(r,target)||
       lancermain_asset::held_asset_name(r,address,lancermain_asset::AssetScope::lancer_only)!=lancermain_asset::HeldAsset::lancer||
       r.result.failure.reason!=meta::Reason::none||!identify(nullptr,address,after)||!abi::same(target,after)||
       !current(nullptr,frame))return false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
    // Reuse the SOLE original R2 ledger: seal this acquisition edge until a
    // real release. This also covers unholstering the already selected gun,
    // which does not necessarily pass through switch_slot's existing yield.
    if(!vrfire::yield_for_native_equipment(frame)||!current(nullptr,frame))return false;
#endif
    return true;
}
bool hold(void*,abi::Frame const& frame,Snapshot const& game) noexcept {
    // Caller has already confirmed current publication + actual attachment,
    // then committed its proposed software model. Re-reading that publication
    // would intentionally fail same_game until the next Scene sees the latch.
    if(!weaponhand::current_gt(nullptr)||!weaponhand::client.registration_alive(frame.key.registration_cookie,frame.key.native_started_qpc)||
       !abi::frame_active(frame,clock_ns(nullptr)))return false;
    weaponhand::equipment_hold_committed(frame.controller.address,game);
    auto& held=weaponhand::held_binding;
    if(!held.known||!abi::same(held.world,frame.world)||!abi::same(held.controller,frame.controller)||
       !abi::same(held.pawn,frame.pawn)||!abi::same(held.weapon,frame.weapon))return false;
    held.native_cookie=frame.key.registration_cookie;held.native_started=frame.key.native_started_qpc;held.world_generation=frame.key.world_generation;
    abi::GripGameState verified{};
    if(!weaponhand::observe(nullptr,frame,clock_ns(nullptr),verified)||!verified.main_hand_held||
       !weaponhand::client.registration_alive(frame.key.registration_cookie,frame.key.native_started_qpc)||
       !abi::frame_active(frame,clock_ns(nullptr))){held={};return false;}
    return true;
}
runtime::Backend backend{nullptr,clock_ms,clock_ns,current,next,inventory,attached,identify,switch_slot,empty_sound,hold,clear_hold,reset_input,held_inventory,
    stow_native::profile()?stow_native::stow:nullptr,stow_native::profile()?stow_native::unholster:nullptr,
    stow_native::profile()?stow_native::idle_unheld:nullptr,
    [](void*,abi::Frame const& f,bool& changed) noexcept {return lancerreload_runtime::prepare_stow(f,changed);},begin_index_acquire};
bool tick(uptr pc,bool& reload_ticked_early,bool reload_already_ticked) noexcept {
    reload_ticked_early=reload_already_ticked;
    ++ticks;
    stow_native::service();
    // A recursively dispatched PlayerTick inside an original CF/Gate gap is
    // not the ordinary game-loop boundary. Do not enter inventory there.
    if(!domain_ready()){++domain_rejected;return false;}
    abi::Frame frame{};
    abi::GripGameState copied_game{};abi::Decision copied_decision{};
    abi::FrameReadObservation frame_read{};
    const auto report_support_withdrawal=[]() noexcept {
        static unsigned rows{};static std::uint64_t last_frame{};
        const auto failure=weaponhand::client.withdrawal_on_gt();
        if(log_file&&rows<8&&failure.frame&&failure.frame!=last_frame&&failure.left_grip_down){
            ++rows;last_frame=failure.frame;
            fwprintf(log_file,L"native_equipment_support_withdrawal frame=%llu sequence=%llu token=%llu reason=%u game_difference=%u frame_difference=%u game_flags=%u age_ns=%lld\n",
                failure.frame,failure.sequence,failure.press_token,unsigned(failure.failure),failure.game_difference,
                failure.frame_difference,failure.game_flags,failure.input_age_ns);fflush(log_file);
            weaponaction_diagnostic::report(log_file);
        }
    };
    report_support_withdrawal();
#if defined(GEARS_PLAYGROUND_LANCER_COMPONENT_PROBE)
    const auto copy_begin_ns=clock_ns(nullptr);
#endif
    if(!weaponhand::current_gt(nullptr)||!weaponhand::client.copy_frame_on_gt(frame,copied_game,copied_decision,&frame_read)){
        ++frame_unavailable;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        fragruntime::unavailable(pc);
        // Inventory remains disarmed. The existing original-saw owner alone
        // may retain its already committed hold through a missing pose Frame,
        // after fresh independent actor/lifetime/input-pin checks. All other
        // source loss still revokes the hold publication normally.
        if(lancersaw_original::retain_hold_during_pose_gap(pc)||lancersaw_starter::retain_grasp_during_pose_gap(pc)||
           lancerreload_runtime::retain_magazine_during_pose_gap(pc)||
           lancerreload_runtime::retain_charging_during_pose_gap(pc)||fragruntime::retain_return_during_pose_gap(pc)||
           fragruntime::retain_preparation_during_pose_gap(pc)){
            state.suspend_input(backend);++saw_pose_gap_holds;}
        else
#endif
        // Keep only the input observer across an absent/expired render
        // publication. Current actions/held publication remain revoked. A
        // subsequent fresh Frame must prove the same actor/input cohort and
        // the existing <=100ms authentic press; startup, identity, Game or
        // transport failures still drop the entire input baseline.
        if(frame_read.failure==abi::FrameReadFailure::publication||
           frame_read.failure==abi::FrameReadFailure::stale_before)
            state.suspend_pose_gap(backend);
        else state.suspend(backend);
        // First real copy failure, before clear_gt erases its association.
        // POD diagnostics only: no new read, source, pose or input authority.
        static unsigned first_failures{},grip_failures{};
        if(frame_read.frame&&log_file&&((frame_read.left_grip_down&&grip_failures++<24)||
           (!frame_read.left_grip_down&&first_failures++<8))){
            fwprintf(log_file,L"native_equipment_frame_read failure=%u game_difference=%u frame=%llu sequence=%llu press=%llu left_grip_down=%u input_age_ns=%lld\n",
                unsigned(frame_read.failure),frame_read.game_difference,frame_read.frame,frame_read.sequence,
                frame_read.press_token,frame_read.left_grip_down,frame_read.input_age_ns);fflush(log_file);
        }
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        weapon_part_assets::on_frame_unavailable();
#endif
        return false;
    }
    if(pc!=frame.controller.address){++other_controller;return false;}
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(fragruntime::pre_equipment(frame,copied_game)){state.suspend_input(backend);return false;}
    fragruntime::tick(frame,copied_game);
    if(fragruntime::pre_equipment(frame,copied_game)){state.suspend_input(backend);return false;}
    if(copied_decision.new_grant)lancer_grasp::tick(frame,copied_decision);
    weapon_part_assets::on_frame(frame);
#endif
    ++frames;
    // Original saw ownership blocks inventory mutation, not observation. A
    // deliberate shoulder edge is retained before the B/ready cancellation
    // can span a game tick; ordinary Runtime consumes it once after cleanup.
    if(lancersaw_original::active())state.observe_input_while_blocked(frame,backend,[pc]() noexcept {
        // The read-only input observer may consume this Frame's remaining
        // lifetime. Preserve only the already authenticated original C hold;
        // no inventory action, pose, grip edge or permit is manufactured.
        return lancersaw_original::retain_hold_during_pose_gap(pc)||
            lancersaw_starter::retain_grasp_during_pose_gap(pc);
    });
    if(lancersaw_original::pre_equipment(frame)){
        //65768 reached the opponent with original motor/SM33, but the old
        // inventory veto returned before BOTH gun follow and blade contact.
        // Keep all equipment/reload/firing mutations blocked; let only the
        // existing sole follow writer and original contact adapter consume
        // this same fresh ready33 association. Never write through paired34.
        if(lancersaw_original::ready_motion_allowed(frame)){
            nativelancerfollow::tick(frame);
            nativelancerfollow::apply();
            // Ready33 still owns inventory/fire, but the acquired C rail must
            // keep consuming this SAME actual hand/part frame. Otherwise the
            // old early return freezes the lever at ON and never sees OFF.
            // Paired34 remains original-owned and cannot enter this branch.
            lancersaw_starter::tick(frame,copied_decision);
            lancer_grasp::tick(frame,copied_decision);
            if(lancersaw_original::ready_motion_allowed(frame))lancersaw_contact::tick(frame);
        }
        return false;
    }
    const auto timing_begin=clock_ns(nullptr);
    const auto outcome=state.tick(frame,backend,vrequip::model);
    const auto timing_equipment=clock_ns(nullptr);
    const auto status=static_cast<std::size_t>(outcome.status);
    if(status<outcomes.size())++outcomes[status];
    if(!have_status||outcome.status!=last_status||outcome.event_serial){
        have_status=true;last_status=outcome.status;
        const auto& source=input_evidence;
        if(log_file){fwprintf(log_file,L"native_equipment status=%hs slot=%u target=%llx event=%llu press=%llu frame=%llu source_sequence=%llu cookie=%llu inventory_stage=%ls input_status=%hs input_previous_sync_ns=%lld input_sync_ns=%lld input_received_ns=%lld input_recorded_press_sync_ns=%lld input_control=%u\n",
            runtime::status_name(outcome.status),outcome.slot,outcome.target,outcome.event_serial,outcome.press_counter,
            frame.key.frame,frame.controllers.sequence,frame.key.registration_cookie,inventory_stage,
            gears_equipment_input::status_name(source.status),source.previous_sync_ns,source.sync_ns,source.received_ns,source.recorded_press_sync_ns,outcome.input_control);fflush(log_file);}
    }
    bool fire_ticked_early=false;
    // This bounds optional observation work only, never an action/restore or
    // an identity/freshness check. Follow/fire/reload keep their original path.
    const auto observer_budget=[&]()noexcept {const auto now=clock_ns(nullptr);
        return now>=timing_begin&&now-timing_begin<12000000;};
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    // Firing consumes this tick's authenticated controller plan before the
    // visual lease. A missing reach-resolved render endpoint must not erase
    // the independent shot plan or swallow a fresh trigger edge.
    lancerreload_manual::prepare_starter(frame);
    if(frame.controllers.hands[1].trigger_active==1){
        lancerreload_manual::prepare(frame);
        vrfire::tick(pc);fire_ticked_early=true;
    }
    nativelancerfollow::tick(frame);
    const auto timing_follow=clock_ns(nullptr);
    // Display follows this fresh hand publication before reload/reflection
    // work can consume its lifetime. Keep the frame-bound plan for muzzle
    // consumers, but the component lease is acquired at most once per tick.
    nativelancerfollow::apply();
    const auto timing_draw=clock_ns(nullptr);
    // Consume an already arbitrated magazine/receiver edge before idle
    // starter display work spends the actual hand pair's100ms lifetime.
    // tick still reauthenticates its own source, exact lease and geometry.
    if(!reload_ticked_early&&(frame.left_grip_down||lancerreload_runtime::needs_priority())){
        lancerreload_runtime::tick(pc);reload_ticked_early=true;
    }
    const auto timing_reload=clock_ns(nullptr);
    lancersaw_contact::tick(frame);
    const auto timing_contact=clock_ns(nullptr);
    lancersaw_starter::tick(frame,copied_decision);
    lancer_grasp::tick(frame,copied_decision);
    const auto timing_starter=clock_ns(nullptr);
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_ATTACHMENT_OBSERVER)
    if(observer_budget())nativeequipment_attachment::tick(frame);else ++observer_budget_skips;
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    if(observer_budget())nativeequipment_rig::tick(frame);else ++observer_budget_skips;
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_MAIN_MOUNT_TRIAL)
    if(observer_budget())nativeequipment_main_mount::tick(frame);else ++observer_budget_skips;
#endif
#if defined(GEARS_PLAYGROUND_LANCER_COMPONENT_PROBE)
    if(observer_budget())nativeequipment_component::tick(frame,copied_game,copy_begin_ns);else ++observer_budget_skips;
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    static unsigned stage_logs{},stage_observed{};
    if(log_file&&stage_logs<16&&vrequip::model.held_for(frame.world.address,frame.pawn.address,frame.weapon.address)){
        const auto ordinal=stage_observed++;
        if(ordinal<4||ordinal%128==0){
            const auto end=clock_ns(nullptr);
            // Refine the existing bounded receipt after outer PlayerTick
            // evidence identified equipment work. The phase clocks follow
            // magazine admission's earlier position; authority is unchanged.
            const auto& starter=lancersaw_visual::last_cost;
            fwprintf(log_file,L"native_equipment_stage_cost equipment_ns=%lld follow_ns=%lld remainder_ns=%lld frame=%llu observed=%u draw_ns=%lld saw_contact_ns=%lld starter_ns=%lld reload_ns=%lld observers_ns=%lld starter_prepare_ns=%lld starter_gun_ns=%lld starter_stage_ns=%lld starter_flush_ns=%lld starter_ack_ns=%lld starter_asset_source_calls=%llu starter_asset_source_ns=%lld\n",
                timing_equipment-timing_begin,timing_follow-timing_equipment,end-timing_follow,frame.key.frame,ordinal,
                timing_draw-timing_follow,timing_contact-timing_reload,timing_starter-timing_contact,
                timing_reload-timing_draw,end-timing_starter,starter.prepare_ns,starter.gun_ns,starter.stage_ns,
                starter.flush_ns,starter.ack_ns,starter.asset_source_calls,starter.asset_source_ns);
            fflush(log_file);++stage_logs;
        }
    }
#endif
    return fire_ticked_early;
}
void reset_after_drain() noexcept {
    stow_native::reset_after_drain();
    state.reset_after_drain(backend,vrequip::model);have_status=false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    lancersaw_starter::reset_after_drain();
    fragruntime::reset_after_drain();
#endif
    input_continuity_reader.close_after_drain();
    if(log_file)fwprintf(log_file,L"native_equipment_input_continuity matched=%llu unavailable=%llu consumer_jitter=%llu native_selected=%llu reader_closed=1\n",
        input_continuity_matched,input_continuity_unavailable,input_continuity_gaps,input_continuity_native_selected);
#if defined(GEARS_PLAYGROUND_LANCER_COMPONENT_PROBE)
    nativeequipment_component::reset_after_drain();
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_ATTACHMENT_OBSERVER)
    nativeequipment_attachment::reset_after_drain();
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    nativeequipment_rig::reset_after_drain();
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_MAIN_MOUNT_TRIAL)
    nativeequipment_main_mount::reset_after_drain();
#endif
    calibration={};calibration_measured=false;input_evidence={};input_gap_logs=0;
    if(log_file&&ticks){
        fwprintf(log_file,L"native_equipment_summary ticks=%llu frames=%llu frame_unavailable=%llu other_controller=%llu domain_rejected=%llu saw_pose_gap_holds=%llu\n",
            ticks,frames,frame_unavailable,other_controller,domain_rejected,saw_pose_gap_holds);
        for(std::size_t i=0;i<outcomes.size();++i)if(outcomes[i])
            fwprintf(log_file,L"native_equipment_count status=%hs count=%llu\n",runtime::status_name(static_cast<runtime::Status>(i)),outcomes[i]);
        fflush(log_file);
    }
    if(log_file&&observer_budget_skips){fwprintf(log_file,L"native_equipment_observer_budget skips=%llu budget_ns=12000000 action_checks_unchanged=1\n",observer_budget_skips);fflush(log_file);}
    ticks=frame_unavailable=frames=other_controller=domain_rejected=observer_budget_skips=saw_pose_gap_holds=0;outcomes={};inventory_stage=L"none";
}
} // namespace nativeequipment
