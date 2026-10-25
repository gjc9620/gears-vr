// Normal PostPlayerTick adapter. Never invoke from Native prepare/commit,
// observe, an ActionGate rollback/NoAuth callback, or an input worker.
namespace nativeequipment {
namespace runtime=::gears_equipment_runtime;
namespace abi=::gears_weapon_hand;
using Snapshot=::gears_weapon::EquipmentSnapshot;
gears_equipment_input::Input input;
gears_equipment_input::Evidence input_evidence;
gears_weapon::ArmspanCalibration calibration;
bool domain_ready() noexcept;
std::uint64_t domain_rejected{};
std::uint64_t ticks{},frame_unavailable{},frames{},other_controller{};
const wchar_t* inventory_stage=L"none";
std::array<std::uint64_t,static_cast<std::size_t>(runtime::Status::clock_invalid)+1> outcomes{};
runtime::Runtime state;
runtime::Status last_status{};
bool have_status{};
unsigned input_gap_logs{};

bool configure_native(HMODULE native) noexcept {
    const bool loaded=runtime::load_native_armspan(native,calibration);
    if(log_file){fwprintf(log_file,L"native_equipment_calibration loaded=%u armspan=%.9g hands_below_head=%.9g source=bound_native_capture_ini\n",
        unsigned(loaded),calibration.armspan_metres,calibration.hands_below_head_metres);fflush(log_file);}
    return loaded;
}

std::uint64_t clock_ms(void*) noexcept {return GetTickCount64();}
std::int64_t clock_ns(void*) noexcept {return weaponhand::clock_ns(nullptr);}
bool current(void*,abi::Frame const& frame) noexcept {
    return enabled.load(std::memory_order_acquire)&&weaponhand::current_gt(nullptr)&&domain_ready()&&
        weaponhand::client.source_current_on_gt(frame.key)&&abi::frame_active(frame,clock_ns(nullptr));
}
void reset_input(void*) noexcept {input.reset();}
bool next(void*,abi::Frame const& frame,std::uint64_t& cursor,gears_weapon::EquipmentIntentEvent& event) noexcept {
    if(!current(nullptr,frame))return false;
    const bool available=input.next(frame,calibration,clock_ns(nullptr),clock_ms(nullptr),cursor,event);
    // Runtime may drop its baseline immediately after next rejects. Retain
    // only diagnostics, never another frame or an action to retry.
    input_evidence=input.evidence();
    // Bounded timing evidence at the actual input rejection. This is not a new
    // poll, timestamp substitution, event retry or freshness allowance.
    if(log_file&&input_gap_logs<16&&input_evidence.status==gears_equipment_input::Status::source_gap){
        const auto& s=input_evidence;
        fwprintf(log_file,L"native_equipment_input_gap reason=%hs frame=%llu source_sequence=%llu observed_ns=%lld previous_observed_ns=%lld previous_sync_ns=%lld sync_ns=%lld received_ns=%lld frame_now_ns=%lld\n",
            gears_equipment_input::gap_name(s.gap),frame.key.frame,s.sequence,s.observed_ns,s.previous_observed_ns,
            s.previous_sync_ns,s.sync_ns,s.received_ns,frame.now_ns);fflush(log_file);++input_gap_logs;
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
bool attached_frame(abi::Frame const& frame,uptr expected_mesh=0) noexcept {
    try{
        if(!frame.weapon.address||!current(nullptr,frame))return false;
        const auto mesh=playground::pointer(frame.weapon.address,L"Mesh");
        if(expected_mesh&&mesh!=expected_mesh)return false;
        if(!playground::instance(mesh,L"SkeletalMeshComponent")||playground::deleted(mesh)||
           playground::pointer(frame.weapon.address,L"Instigator")!=frame.pawn.address)return false;
        weaponhand::meta::Snapshot schema{};schema.module_base=base;
        weaponhand::meta::Reader reader{schema,weaponhand::meta::Stage::final_check};weaponhand::BoolField hidden{};
        if(!weaponhand::meta::tables(base,schema.objects,schema.names)||
           !weaponhand::boolean(reader,mesh,L"HiddenGame",hidden)||(hidden.bits&hidden.mask))return false;
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
        if(matches!=1)return false;
        Name bone{};std::memcpy(&bone,selected.data()+8,8);if(!hand_bone(bone))return false;
        for(unsigned vector_offset:{16u,40u})for(unsigned axis=0;axis<3;++axis){float value{};
            std::memcpy(&value,selected.data()+vector_offset+axis*4,4);if(!std::isfinite(value))return false;}
        Array after{};std::array<unsigned char,52> again{};
        return copy(&after,frame.mesh.address+offset,sizeof after)&&
            !std::memcmp(&after,&entries,sizeof after)&&copy(again.data(),entry_address,again.size())&&again==selected&&
            playground::pointer(frame.weapon.address,L"Mesh")==mesh&&current(nullptr,frame);
    }catch(...){return false;}
}
bool attached(void*,abi::Frame const& frame,Snapshot const& game) noexcept {
    return game.world==frame.world.address&&game.pawn==frame.pawn.address&&game.current_weapon==frame.weapon.address&&
        attached_frame(frame);
}
bool switch_slot(void*,abi::Frame const& frame,Snapshot const& game,unsigned slot) noexcept {
    try{
        if(slot>=4||!game.can_switch||game.animation_busy||game.world!=frame.world.address||game.pawn!=frame.pawn.address||
           game.current_weapon!=frame.weapon.address)return false;
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
        // Normal original inventory API. Completion is observed later; there
        // is no silent model commit, skip-animation claim, or retry here.
        return call.run();
    }catch(...){return false;}
}
bool empty_sound(void*,abi::Frame const& frame,Snapshot const& game,unsigned slot) noexcept {
    try{return current(nullptr,frame)&&vrequip::play_empty_slot_sound(game,slot);}catch(...){return false;}
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
runtime::Backend backend{nullptr,clock_ms,clock_ns,current,next,inventory,attached,identify,switch_slot,empty_sound,hold,clear_hold,reset_input};
void tick(uptr pc) noexcept {
    ++ticks;
    // A recursively dispatched PlayerTick inside an original CF/Gate gap is
    // not the ordinary game-loop boundary. Do not enter inventory there.
    if(!domain_ready()){++domain_rejected;return;}
    abi::Frame frame{};
    if(!weaponhand::current_gt(nullptr)||!weaponhand::client.copy_frame_on_gt(frame)){
        ++frame_unavailable;state.suspend(backend);return;
    }
    if(pc!=frame.controller.address){++other_controller;return;}
    ++frames;
    const auto outcome=state.tick(frame,backend,vrequip::model);
    const auto status=static_cast<std::size_t>(outcome.status);
    if(status<outcomes.size())++outcomes[status];
    if(!have_status||outcome.status!=last_status||outcome.event_serial){
        have_status=true;last_status=outcome.status;
        const auto& source=input_evidence;
        if(log_file){fwprintf(log_file,L"native_equipment status=%hs slot=%u target=%llx event=%llu press=%llu frame=%llu source_sequence=%llu cookie=%llu inventory_stage=%ls input_status=%hs input_previous_sync_ns=%lld input_sync_ns=%lld input_received_ns=%lld\n",
            runtime::status_name(outcome.status),outcome.slot,outcome.target,outcome.event_serial,outcome.press_counter,
            frame.key.frame,frame.controllers.sequence,frame.key.registration_cookie,inventory_stage,
            gears_equipment_input::status_name(source.status),source.previous_sync_ns,source.sync_ns,source.received_ns);fflush(log_file);}
    }
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_ATTACHMENT_OBSERVER)
    nativeequipment_attachment::tick(frame);
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    nativeequipment_rig::tick(frame);
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_MAIN_MOUNT_TRIAL)
    nativeequipment_main_mount::tick(frame);
#endif
}
void reset_after_drain() noexcept {
    state.reset_after_drain(backend,vrequip::model);have_status=false;
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_ATTACHMENT_OBSERVER)
    nativeequipment_attachment::reset_after_drain();
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_RIG_READER)
    nativeequipment_rig::reset_after_drain();
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_EQUIPMENT_MAIN_MOUNT_TRIAL)
    nativeequipment_main_mount::reset_after_drain();
#endif
    calibration={};input_evidence={};input_gap_logs=0;
    if(log_file&&ticks){
        fwprintf(log_file,L"native_equipment_summary ticks=%llu frames=%llu frame_unavailable=%llu other_controller=%llu domain_rejected=%llu\n",
            ticks,frames,frame_unavailable,other_controller,domain_rejected);
        for(std::size_t i=0;i<outcomes.size();++i)if(outcomes[i])
            fwprintf(log_file,L"native_equipment_count status=%hs count=%llu\n",runtime::status_name(static_cast<runtime::Status>(i)),outcomes[i]);
        fflush(log_file);
    }
    ticks=frame_unavailable=frames=other_controller=domain_rejected=0;outcomes={};inventory_stage=L"none";
}
} // namespace nativeequipment
