// Equipment action adapters run on normal PostPlayerTick, never Native
// prepare/commit, observe, rollback/NoAuth or input workers. The dedicated
// read_attached_frame helper is metadata only and grants no action permission.
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
namespace lancer_grasp {
void report_main_hold_trace(FILE*) noexcept;
bool boltok_hold_trace_for(gears_weapon_hand::Frame const&) noexcept;
bool hammerburst_source_scope_eligible(gears_weapon_hand::Frame const&) noexcept;
bool boltok_source_scope_eligible(gears_weapon_hand::Frame const&) noexcept;
}
#endif
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) && defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
namespace hod_input {bool prepare_equipment_stow(gears_weapon_hand::Frame const&,bool&) noexcept;}
#endif
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


// Diagnostic-only accounting for the existing equipment current/hold boundary.
// The wrapper returns the original current() result unchanged and grants nothing.
struct HoldBoundaryEntry {
    std::uint64_t frame{},sequence{};
    std::int64_t sync_ns{};
    bool baseline_ready{},input_bound{},actor_bound{},pending{},future_known{};
    bool held_known{},hold_published{},binding_known{},edges_known{},edge_changed{};
    std::uint64_t cursor{UINT64_MAX},model_hold_generation{};
    unsigned clears{};
    bool clear_binding_known{},clear_baseline_ready{},clear_input_bound{},clear_actor_bound{};
    bool clear_pending{},clear_future_known{},clear_held_known{},clear_hold_published{};
    std::uint64_t clear_cursor{UINT64_MAX},clear_input_sequence{};
    unsigned clear_input_status{};
    std::int64_t clear_age_ns{};
    unsigned current_calls{};
    bool current_result{};
    std::int64_t current_ns{},current_age_ns{-1};
    unsigned retain_calls{};
    unsigned inventory_calls{},inventory_kind{},attached_calls{};
    bool inventory_result{},attached_result{};
    std::int64_t inventory_age_ns{-1},attached_age_ns{-1};
    bool retain_last_result{};
    unsigned outcome{};
    bool exit_binding_known{};
};

std::array<HoldBoundaryEntry,8> hold_boundary_rows{};
unsigned hold_boundary_count{};
bool hold_boundary_active{};
HoldBoundaryEntry hold_boundary_entry{};
std::int64_t hold_boundary_started_ns{};

bool hold_boundary_edge_changed(runtime::State const& s,abi::Frame const& f) noexcept {
    for(unsigned hand=0;hand<2;++hand)for(unsigned control=0;control<gears_vr::control_count;++control)
        if(s.last_edges[hand][0][control]!=f.controllers.hands[hand].pressed[control]||
           s.last_edges[hand][1][control]!=f.controllers.hands[hand].released[control])return true;
    return false;
}

void hold_boundary_begin(abi::Frame const& f) noexcept {
    hold_boundary_active=false;
    hold_boundary_entry={};
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(hold_boundary_count>=hold_boundary_rows.size()||
       !lancer_grasp::boltok_hold_trace_for(f))return;
    auto const& s=state.state();
    hold_boundary_entry.frame=f.key.frame;
    hold_boundary_entry.sequence=f.controllers.sequence;
    hold_boundary_entry.sync_ns=f.controllers.sync_ns;
    hold_boundary_entry.baseline_ready=s.baseline_ready;
    hold_boundary_entry.input_bound=s.input_bound;
    hold_boundary_entry.actor_bound=s.actor_bound;
    hold_boundary_entry.pending=s.pending;
    hold_boundary_entry.future_known=s.future_known;
    hold_boundary_entry.held_known=s.held_known;
    hold_boundary_entry.hold_published=s.hold_published;
    hold_boundary_entry.binding_known=weaponhand::held_binding.known;
    hold_boundary_entry.cursor=s.cursor;
    hold_boundary_entry.model_hold_generation=vrequip::model.hold_generation();
    hold_boundary_entry.edges_known=s.edges_known;
    hold_boundary_entry.edge_changed=hold_boundary_edge_changed(s,f);
    hold_boundary_started_ns=clock_ns(nullptr);
    hold_boundary_active=true;
#endif
}

bool current_for_trace(void* context,abi::Frame const& frame) noexcept {
    if(!hold_boundary_active)return current(context,frame);
    const auto begin=clock_ns(nullptr);
    const bool result=current(context,frame);
    const auto end=clock_ns(nullptr);
    ++hold_boundary_entry.current_calls;
    hold_boundary_entry.current_result=result;
    hold_boundary_entry.current_ns=end-begin;
    hold_boundary_entry.current_age_ns=(frame.controllers.sync_ns&&end>=frame.controllers.sync_ns)?
        end-frame.controllers.sync_ns:-1;
    return result;
}

void hold_boundary_before_clear() noexcept {
    if(!hold_boundary_active)return;
    ++hold_boundary_entry.clears;
    auto const& s=state.state();
    hold_boundary_entry.clear_binding_known=weaponhand::held_binding.known;
    hold_boundary_entry.clear_baseline_ready=s.baseline_ready;
    hold_boundary_entry.clear_input_bound=s.input_bound;
    hold_boundary_entry.clear_actor_bound=s.actor_bound;
    hold_boundary_entry.clear_pending=s.pending;
    hold_boundary_entry.clear_future_known=s.future_known;
    hold_boundary_entry.clear_held_known=s.held_known;
    hold_boundary_entry.clear_hold_published=s.hold_published;
    hold_boundary_entry.clear_cursor=s.cursor;
    hold_boundary_entry.clear_age_ns=clock_ns(nullptr)-hold_boundary_started_ns;
    hold_boundary_entry.clear_input_status=unsigned(input_evidence.status);
    hold_boundary_entry.clear_input_sequence=input_evidence.sequence;
}

void hold_boundary_note_retain(bool retained) noexcept {
    if(!hold_boundary_active)return;
    ++hold_boundary_entry.retain_calls;
    hold_boundary_entry.retain_last_result=retained;
}

void hold_boundary_end(runtime::Outcome const& outcome) noexcept {
    if(!hold_boundary_active)return;
    hold_boundary_entry.outcome=unsigned(outcome.status);
    hold_boundary_entry.exit_binding_known=weaponhand::held_binding.known;
    const bool record=hold_boundary_entry.clears||
        outcome.status==runtime::Status::unavailable||
        outcome.status==runtime::Status::inventory_unavailable||
        outcome.status==runtime::Status::hold_failed;
    if(record&&hold_boundary_count<hold_boundary_rows.size())
        hold_boundary_rows[hold_boundary_count++]=hold_boundary_entry;
    hold_boundary_active=false;
}

void hold_boundary_report_and_clear() noexcept {
    if(log_file)for(unsigned i=0;i<hold_boundary_count;++i){
        auto const& r=hold_boundary_rows[i];
        fwprintf(log_file,L"boltok_equipment_hold_boundary frame=%llu sequence=%llu sync_ns=%lld baseline=%u input_bound=%u actor_bound=%u pending=%u future=%u held_known=%u hold_published=%u cursor=%llu model_hold_gen=%llu binding_known=%u edges_known=%u edge_changed=%u clears=%u clear_binding_known=%u clear_baseline=%u clear_input_bound=%u clear_actor_bound=%u clear_pending=%u clear_future=%u clear_held_known=%u clear_hold_published=%u clear_cursor=%llu clear_age_ns=%lld clear_input_status=%u clear_input_sequence=%llu current_calls=%u current_result=%u current_ns=%lld current_age_ns=%lld retain_calls=%u retain_last_result=%u outcome=%hs exit_binding_known=%u inventory_calls=%u inventory_kind=%u inventory_result=%u inventory_age_ns=%lld attached_calls=%u attached_result=%u attached_age_ns=%lld\n",
            r.frame,r.sequence,r.sync_ns,unsigned(r.baseline_ready),unsigned(r.input_bound),unsigned(r.actor_bound),
            unsigned(r.pending),unsigned(r.future_known),unsigned(r.held_known),unsigned(r.hold_published),
            r.cursor,r.model_hold_generation,unsigned(r.binding_known),unsigned(r.edges_known),unsigned(r.edge_changed),
            r.clears,unsigned(r.clear_binding_known),unsigned(r.clear_baseline_ready),unsigned(r.clear_input_bound),
            unsigned(r.clear_actor_bound),unsigned(r.clear_pending),unsigned(r.clear_future_known),
            unsigned(r.clear_held_known),unsigned(r.clear_hold_published),r.clear_cursor,r.clear_age_ns,
            r.clear_input_status,r.clear_input_sequence,r.current_calls,unsigned(r.current_result),
            r.current_ns,r.current_age_ns,r.retain_calls,unsigned(r.retain_last_result),
            runtime::status_name(static_cast<runtime::Status>(r.outcome)),unsigned(r.exit_binding_known),
            r.inventory_calls,r.inventory_kind,unsigned(r.inventory_result),r.inventory_age_ns,
            r.attached_calls,unsigned(r.attached_result),r.attached_age_ns);
    }
    if(log_file&&hold_boundary_count)fflush(log_file);
    hold_boundary_active=false;
    hold_boundary_count=0;
    hold_boundary_rows={};
    hold_boundary_entry={};
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
    static const bool perf_input=[](){wchar_t value[4]{};
        return GetEnvironmentVariableW(L"GEARSVR_TEST_PERF_UNHELD",value,4)==1&&value[0]==L'1';}();
    const auto input_begin_ns=clock_ns(nullptr);
    if(!current(nullptr,frame))return false;
    const auto validated_ns=clock_ns(nullptr);
    gears_equipment_input::Continuity continuity{};
    gears_current_tracking::Failure continuity_failure{};
    const bool requested_history=input.needs_recorded_right_press(frame,calibration,cursor);
    const bool matched=read_input_continuity(frame,continuity.tracking,continuity.publisher,
        requested_history?&continuity.right_press:nullptr,&continuity_failure);
    const auto continuity_ns=clock_ns(nullptr);
    if(matched)++input_continuity_matched;else ++input_continuity_unavailable;
    const bool index_acquire=stow_native::profile()&&!vrequip::model.held()&&
        !vrequip::model.switch_pending()&&!vrequip::model.animation_owns_hands();
    const bool available=input.next(frame,calibration,clock_ns(nullptr),clock_ms(nullptr),cursor,event,
        matched?&continuity:nullptr,index_acquire);
    static unsigned input_cost_logs{},input_cost_observed{};
    if(perf_input&&log_file&&input_cost_logs<16){
        const auto ordinal=input_cost_observed++;
        if(ordinal<4||ordinal%128==0){
            const auto end_ns=clock_ns(nullptr);
            fwprintf(log_file,L"native_equipment_input_cost observed=%u validation_ns=%lld continuity_read_ns=%lld input_next_ns=%lld total_ns=%lld requested_history=%u\n",
                ordinal,validated_ns-input_begin_ns,continuity_ns-validated_ns,end_ns-continuity_ns,
                end_ns-input_begin_ns,unsigned(requested_history));
            fflush(log_file);++input_cost_logs;
        }
    }
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
void clear_hold(void*) noexcept {
    hold_boundary_before_clear();
    weaponhand::held_binding={};
}
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
#include "weapon_equipment_idle_inventory.inl"
bool held_inventory(void*,abi::Frame const& frame,Snapshot& out) noexcept {
    try {
        inventory_stage=L"held_chain_observation";
        if(!current(nullptr,frame)||!idle_inventory::read(frame,out))return false;
        return out.world==frame.world.address&&out.pawn==frame.pawn.address&&
            out.current_weapon==frame.weapon.address&&current(nullptr,frame);
    }catch(...){out={};return false;}
}
bool quiet_unheld_inventory(void*,abi::Frame const& frame,Snapshot& out) noexcept {
    try {
        inventory_stage=L"quiet_unheld_chain_observation";
        static const bool measure=[](){wchar_t value[4]{};return
            GetEnvironmentVariableW(L"GEARSVR_TEST_PERF_UNHELD",value,4)==1&&value[0]==L'1';}();
        static unsigned quiet_observed{},logged{};
        const auto ordinal=measure?quiet_observed++:0;
        const bool sample=measure&&logged<16&&(ordinal<4||ordinal%128==0);
        const auto begin=sample?clock_ns(nullptr):0;
        if(!current(nullptr,frame))return false;
        const auto validated=sample?clock_ns(nullptr):0;
        bool can_switch{},animation_busy{};
        if(!vrequip::read_can_switch(frame.controller.address,can_switch)||
           !vrequip::read_bool(frame.pawn.address,L"bSwitchingWeapons",animation_busy))return false;
        const auto queried=sample?clock_ns(nullptr):0;
        // No slots or manager were consumed before the original switch query.
        // Observe its CURRENT result using one fully bracketed chain read.
        // A query-time slot change is observed, never used for a new action.
        if(!idle_inventory::read(frame,out)||!current(nullptr,frame))return false;
        out.can_switch=can_switch;out.animation_busy=animation_busy;
        if(sample&&log_file){const auto end=clock_ns(nullptr);
            fwprintf(log_file,L"native_equipment_quiet_inventory_cost observed=%u current_ns=%lld switch_ns=%lld chain_close_ns=%lld total_ns=%lld action_permission=0\n",
                ordinal,validated-begin,queried-validated,end-queried,end-begin);fflush(log_file);++logged;}
        inventory_stage=L"quiet_unheld_ok";return true;
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
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        if(gnasher_state::owns_stow(frame)&&!gnasher_state::stow_ready(frame))return false;
#endif
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
        if(!fragpreparation::stop()||!smokepreparation::stop()||!current(nullptr,frame))return false;
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
    try{
        const bool valid=current(nullptr,frame);
        const bool dispatched=valid&&vrequip::play_empty_slot_sound(game,slot);
        // Bounded observation of the existing grenade-slot feedback only.
        // Returning true means original sound dispatch, not audible output
        // (automatic validation is muted). No additional original call/read.
        static unsigned grenade_empty_reports{};
        if(valid&&slot==1&&log_file&&grenade_empty_reports++<24){
            const auto& right=frame.controllers.hands[1];
            fwprintf(log_file,L"native_grenade_empty_feedback frame=%llu sequence=%llu pressed=%u released=%u slot=%u selected=%llx current_weapon=%llx dispatched=%u\n",
                frame.key.frame,frame.controllers.sequence,right.pressed[gears_vr::control_squeeze],right.released[gears_vr::control_squeeze],
                slot,game.weapon_in_slot[slot],game.current_weapon,unsigned(dispatched));fflush(log_file);
        }
        return dispatched;
    }catch(...){return false;}
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
    weaponhand::HoldSourceObservation verified{};
    if(!weaponhand::observe_hold_source(nullptr,frame,clock_ns(nullptr),verified)||!verified.main_hand_held||
       !weaponhand::client.registration_alive(frame.key.registration_cookie,frame.key.native_started_qpc)||
       !abi::frame_active(frame,clock_ns(nullptr))){held={};return false;}
    if(playground::instance(frame.weapon.address,L"WEAP_HammerOfDawn")){
        const bool waiting=weaponhand::client.begin_hod_posthold_on_gt(frame,held.generation);
        static unsigned handoff_rows{};
        if(log_file&&handoff_rows++<4){
            fwprintf(log_file,L"hod_posthold_arm armed=%u frame=%llu weapon=%llx generation=%llu input_age_ns=%lld client_armed=%u disarm=%u\n",
                unsigned(waiting),frame.key.frame,frame.weapon.address,held.generation,
                clock_ns(nullptr)-frame.controllers.sync_ns,unsigned(weaponhand::client.armed()),weaponhand::client.last_disarm_reason());fflush(log_file);
        }
    }
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    lancer_grasp::committed_main_hold(frame);
#endif
    return true;
}
bool unholster_after_reload(void* context,abi::Frame const& frame,Snapshot const& game,bool& changed) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(vrequip::model.held()&&vrequip::model.held_weapon()==frame.weapon.address&&gnasher_state::owns_stow(frame)){
        if(!gnasher_state::prepare_stow(frame,changed))return false;
        if(changed)return true; // Existing ORIGINAL intent deadline; await a new publication.
    }
#endif
    return stow_native::unholster(context,frame,game,changed);
}
bool retain_idle_main_hold(void*,abi::Frame const& frame) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    const bool retained=lancer_grasp::retain_main_during_pose_gap(frame.controller.address);
    hold_boundary_note_retain(retained);
    return retained;
#else
    (void)frame;return false;
#endif
}
bool retain_boltok_existing_hold(void*,abi::Frame const& frame) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    // Eligibility is routing only; the original routine performs every fresh
    // game, lifetime, actor, generation and continuity check.
    if(!lancer_grasp::boltok_source_scope_eligible(frame))return false;
    const bool retained=lancer_grasp::retain_main_during_pose_gap(frame.controller.address);
    hold_boundary_note_retain(retained);
    return retained;
#else
    (void)frame;return false;
#endif
}

// Diagnostic-only wrappers around the existing inventory and attachment calls.
// Each calls its original function exactly once and returns that same result.
bool inventory_for_trace(void* context,abi::Frame const& frame,Snapshot& out) noexcept {
    if(!hold_boundary_active)return inventory(context,frame,out);
    const bool result=inventory(context,frame,out);
    const auto end=clock_ns(nullptr);
    ++hold_boundary_entry.inventory_calls;
    hold_boundary_entry.inventory_kind=1;
    hold_boundary_entry.inventory_result=result;
    hold_boundary_entry.inventory_age_ns=
        (frame.controllers.sync_ns&&end>=frame.controllers.sync_ns)?
        end-frame.controllers.sync_ns:-1;
    return result;
}

bool held_inventory_for_trace(void* context,abi::Frame const& frame,Snapshot& out) noexcept {
    if(!hold_boundary_active)return held_inventory(context,frame,out);
    const bool result=held_inventory(context,frame,out);
    const auto end=clock_ns(nullptr);
    ++hold_boundary_entry.inventory_calls;
    hold_boundary_entry.inventory_kind=2;
    hold_boundary_entry.inventory_result=result;
    hold_boundary_entry.inventory_age_ns=
        (frame.controllers.sync_ns&&end>=frame.controllers.sync_ns)?
        end-frame.controllers.sync_ns:-1;
    return result;
}

// Runtime alone brackets this metadata read with its mandatory current
// checks. Standalone attached()/attached_frame() callers keep their full
// validation. Routing is a negative work choice, never action permission.
bool attached_for_runtime(void* context,abi::Frame const& frame,Snapshot const& snapshot) noexcept {
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    if(lancer_grasp::boltok_source_scope_eligible(frame))
        return snapshot.world==frame.world.address&&snapshot.pawn==frame.pawn.address&&
            snapshot.current_weapon==frame.weapon.address&&read_attached_frame(frame,0);
#endif
    return attached(context,frame,snapshot);
}

bool attached_for_trace(void* context,abi::Frame const& frame,Snapshot const& snapshot) noexcept {
    if(!hold_boundary_active)return attached_for_runtime(context,frame,snapshot);
    const bool result=attached_for_runtime(context,frame,snapshot);
    const auto end=clock_ns(nullptr);
    ++hold_boundary_entry.attached_calls;
    hold_boundary_entry.attached_result=result;
    hold_boundary_entry.attached_age_ns=
        (frame.controllers.sync_ns&&end>=frame.controllers.sync_ns)?
        end-frame.controllers.sync_ns:-1;
    return result;
}

runtime::Backend backend{nullptr,clock_ms,clock_ns,current_for_trace,next,inventory_for_trace,attached_for_trace,identify,switch_slot,empty_sound,hold,clear_hold,reset_input,held_inventory_for_trace,
    stow_native::profile()?stow_native::stow:nullptr,stow_native::profile()?unholster_after_reload:nullptr,
    stow_native::profile()?stow_native::idle_unheld:nullptr,
    [](void*,abi::Frame const& f,bool& changed) noexcept {
        if(gnasher_state::owns_stow(f))return gnasher_state::prepare_stow(f,changed);
        return lancerreload_runtime::prepare_stow(f,changed);
    },begin_index_acquire,retain_idle_main_hold,quiet_unheld_inventory,retain_boltok_existing_hold,
    [](void*,abi::Frame const& f,bool& stop_issued) noexcept {
        stop_issued=false;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW) && defined(GEARS_PLAYGROUND_UNIFIED_MUZZLE_FIRE) && defined(GEARS_PLAYGROUND_NATIVE_LANCER_R2_TRIAL)
        if(playground::instance(f.weapon.address,L"WEAP_HammerOfDawn"))
            return current(nullptr,f)&&hod_input::prepare_equipment_stow(f,stop_issued);
#else
        (void)f;
#endif
        return true;
    }};
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
            fwprintf(log_file,L"native_equipment_support_withdrawal frame=%llu sequence=%llu token=%llu reason=%u game_difference=%u frame_difference=%u game_flags=%u age_ns=%lld candidate_flags=%u candidate_decision=%u candidate_lease=%u support_distance_m=%.9g\n",
                failure.frame,failure.sequence,failure.press_token,unsigned(failure.failure),failure.game_difference,
                failure.frame_difference,failure.game_flags,failure.input_age_ns,failure.candidate.flags,
                failure.candidate.decision,failure.candidate.lease,failure.candidate.support_distance_metres);fflush(log_file);
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
        smokeruntime::unavailable(pc);
        longshot_scope_owner::unavailable(pc);
        // Inventory remains disarmed. The existing original-saw owner alone
        // may retain its already committed hold through a missing pose Frame,
        // after fresh independent actor/lifetime/input-pin checks. All other
        // source loss still revokes the hold publication normally.
        if(lancer_grasp::retain_main_during_pose_gap(pc)||
           lancersaw_original::retain_hold_during_pose_gap(pc)||lancersaw_starter::retain_grasp_during_pose_gap(pc)||
           lancerreload_runtime::retain_magazine_during_pose_gap(pc)||
           lancerreload_runtime::retain_charging_during_pose_gap(pc)||fragruntime::retain_return_during_pose_gap(pc)||smokeruntime::retain_return_during_pose_gap(pc)||
           fragruntime::retain_preparation_during_pose_gap(pc)||smokeruntime::retain_preparation_during_pose_gap(pc)){
            state.suspend_input(backend);++saw_pose_gap_holds;}
        else
#endif
        // Keep only the input observer across an absent/expired render
        // publication. Current actions/held publication remain revoked. A
        // subsequent fresh Frame must prove the same actor/input cohort and
        // the existing <=100ms authentic press; startup, identity, Game or
        // transport failures still drop the entire input baseline.
        if(frame_read.failure==abi::FrameReadFailure::hold_handoff_pending)
            state.suspend_input(backend);
        else if(frame_read.failure==abi::FrameReadFailure::publication||
           frame_read.failure==abi::FrameReadFailure::stale_before)
            state.suspend_pose_gap(backend);
        else state.suspend(backend);
        // First real copy failure, before clear_gt erases its association.
        // POD diagnostics only: no new read, source, pose or input authority.
        static unsigned first_failures{},grip_failures{};
        if(frame_read.frame&&log_file&&((frame_read.left_grip_down&&grip_failures++<24)||
           (!frame_read.left_grip_down&&first_failures++<8))){
            fwprintf(log_file,L"native_equipment_frame_read failure=%u game_difference=%u frame=%llu sequence=%llu press=%llu left_grip_down=%u input_age_ns=%lld candidate_flags=%u candidate_decision=%u candidate_lease=%u support_distance_m=%.9g commit_check_input_age_ns=%lld since_commit_check_ns=%lld\n",
                unsigned(frame_read.failure),frame_read.game_difference,frame_read.frame,frame_read.sequence,
                frame_read.press_token,frame_read.left_grip_down,frame_read.input_age_ns,frame_read.candidate.flags,
                frame_read.candidate.decision,frame_read.candidate.lease,frame_read.candidate.support_distance_metres,
                frame_read.commit_check_input_age_ns,frame_read.since_commit_check_ns);fflush(log_file);
        }
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        weapon_part_assets::on_frame_unavailable();
#endif
        return false;
    }
    if(pc!=frame.controller.address){++other_controller;return false;}
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    // One Lancer, Hammerburst or currently unheld equipment callback, one
    // byte-revalidated boolean liveness proof. Full Frame/game/proposal readers
    // keep their path.
    // Every consumer still reads actual bytes; source mutation reruns the
    // original observer. Never cache game state, pose, input, clocks or grants.
    // Part-local scopes below borrow this scope rather than nesting disabled
    // proofs. Other held weapons keep their original scope/observation path.
    std::optional<weaponhand::source_validation::Scope> lancer_source_reads;
    if(!weaponhand::source_validation::active&&(lancer_grasp::source_scope_eligible(frame)||
       lancer_grasp::hammerburst_source_scope_eligible(frame)||
       lancer_grasp::boltok_source_scope_eligible(frame)||
       (!vrequip::model.held()&&copied_game.known&&!copied_game.main_hand_held)))
        lancer_source_reads.emplace(frame);
    if(fragruntime::pre_equipment(frame,copied_game)||smokeruntime::pre_equipment(frame,copied_game)){state.suspend_input(backend);return false;}
    fragruntime::tick(frame,copied_game);
    smokeruntime::tick(frame,copied_game);
    if(fragruntime::pre_equipment(frame,copied_game)||smokeruntime::pre_equipment(frame,copied_game)){state.suspend_input(backend);return false;}
    lancer_grasp::prepare_neutral_starter(frame,copied_game,copied_decision);
    (void)lancer_grasp::publish_magazine_fingers_early(frame,copied_game,copied_decision);
    const bool support_grasp=copied_decision.lease.kind==abi::LeaseKind::support;
    const bool passive_support_early=lancer_grasp::prefer_early_passive_support(frame,copied_game,copied_decision);
    // Publish a retained support pose at the same early admission point as a
    // new grant. Unrelated reload/starter work must not precede this request.
    // tick retains every current source, exact lease and geometry check.
    if(copied_decision.new_grant||support_grasp||passive_support_early)lancer_grasp::tick(frame,copied_game,copied_decision);
    weapon_part_assets::on_frame(frame);
    torquebow_arrow_visual::on_frame(frame);
    // Hidden resource preparation grants no firing or pickup permission.
    // On an idle, exact held Longshot DOWN, service it after this same tick's
    // fire path. Active reload and an already acquired cartridge keep order.
    const auto& longshot_right=frame.controllers.hands[1];
    const bool defer_longshot_round=longshot_runtime::selected(frame)&&!longshot_runtime::pending_work()&&
        !longshot_round::has_item()&&longshot_right.trigger_active==1&&std::isfinite(longshot_right.trigger)&&
        longshot_right.trigger>.35f&&longshot_right.trigger<=1.f;
    if(!defer_longshot_round)longshot_round::tick(frame);
    gnasher_round::tick(frame);
    longshot_fire::service(frame);
    gnasher_fire::service(frame);
    gnasher_inventory::service(frame);
    gnasher_state::service(frame,state.state().pending,copied_game.main_hand_held);
    gnasher_runtime::tick(frame,copied_game,copied_decision);
    if(!copied_decision.new_grant)lancer_magazine_appearance::prepare(frame);
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
            if(!support_grasp&&!passive_support_early)lancer_grasp::tick(frame,copied_game,copied_decision);
            if(lancersaw_original::ready_motion_allowed(frame))lancersaw_contact::tick(frame);
        }
        return false;
    }
    const auto timing_begin=clock_ns(nullptr);
    hold_boundary_begin(frame);
    const auto outcome=state.tick(frame,backend,vrequip::model);
    hold_boundary_end(outcome);
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
    const bool mechanism_first=!reload_ticked_early&&longshot_runtime::selected(frame)&&
        longshot_bolt::granted(frame,copied_decision)&&vrfire::longshot_mechanism_neutral(frame);
    const auto fire_current=[&]() noexcept {
        // The original starter query admits Weap_AssaultRifle only. The
        // existing exact held Longshot classification cannot enter that owner.
        if(!longshot_runtime::selected(frame))lancerreload_manual::prepare_starter(frame);
        if(frame.controllers.hands[1].trigger_active==1){
            lancerreload_manual::prepare(frame);
            vrfire::tick(pc);fire_ticked_early=true;
        }
        if(defer_longshot_round)longshot_round::tick(frame);
    };
    if(!mechanism_first)fire_current();
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
    if(!reload_ticked_early&&support_grasp&&lancerreload_runtime::idle_support_only(frame,copied_game,copied_decision))
        reload_ticked_early=true;
    if(!reload_ticked_early&&(frame.left_grip_down||lancerreload_runtime::needs_priority())){
        lancerreload_runtime::tick(pc);reload_ticked_early=true;
    }
    // Fresh follow/apply and the normal reload reauthentication remain first.
    // Fire still runs exactly once in this tick, with its own live guards.
    if(mechanism_first)fire_current();
    const auto timing_reload=clock_ns(nullptr);
    // A retained Lancer magazine/loading skin uses the SAME fresh part pose
    // just updated above. Do not spend its remaining100ms on unrelated C
    // contact/idle work first. C owns its own post-starter geometry ordering.
    const bool part_grasp_early=lancer_grasp::prefer_early_part(frame,copied_decision);
    if(part_grasp_early)lancer_grasp::tick(frame,copied_game,copied_decision);
    lancersaw_contact::tick(frame);
    const auto timing_contact=clock_ns(nullptr);
    lancersaw_starter::tick(frame,copied_decision);
    if(!support_grasp&&!passive_support_early&&!part_grasp_early)lancer_grasp::tick(frame,copied_game,copied_decision);
    const auto timing_starter=clock_ns(nullptr);
    longshot_grasp::tick(frame,copied_game,copied_decision);
    longshot_eyebox::observe(frame,copied_game,longshot_scope_owner::ready(frame));
    longshot_scope_owner::tick(frame);
    longshot_runtime::guide(frame);
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
    static const bool perf_unheld=[](){wchar_t value[4]{};
        return GetEnvironmentVariableW(L"GEARSVR_TEST_PERF_UNHELD",value,4)==1&&value[0]==L'1';}();
    if(log_file&&stage_logs<16&&(perf_unheld||vrequip::model.held_for(frame.world.address,frame.pawn.address,frame.weapon.address))){
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
            if(lancer_source_reads){const auto& proof_scope=*lancer_source_reads;
                fwprintf(log_file,L"native_equipment_source_validation_cost observed=%u validations=%u fallbacks=%u disabled=%u sealed=%u failure=%u ranges=%u merged=%u reads_per_pass=%u bytes=%u qualifications=%u capture_ns=%lld seal_ns=%lld validation_ns=%lld order_hits=%u order_misses=%u\n",
                    ordinal,proof_scope.validations,proof_scope.fallbacks,unsigned(proof_scope.disabled),unsigned(proof_scope.sealed),
                    proof_scope.proof?unsigned(proof_scope.proof->failure):0,
                    proof_scope.proof?proof_scope.proof->count:0,proof_scope.proof?proof_scope.proof->merged:0,
                    proof_scope.proof?proof_scope.proof->validation_reads_per_pass():0,proof_scope.proof?proof_scope.proof->used:0,
                    proof_scope.qualifications,proof_scope.capture_ns,proof_scope.seal_ns,proof_scope.validation_ns,
                    proof_scope.proof?proof_scope.proof->order_cache.hits:0,proof_scope.proof?proof_scope.proof->order_cache.misses:0);}
            fflush(log_file);++stage_logs;
        }
    }
#endif
    return fire_ticked_early;
}
void reset_after_drain() noexcept {
    stow_native::reset_after_drain();
    hold_boundary_report_and_clear();
    state.reset_after_drain(backend,vrequip::model);have_status=false;
    if(log_file)fwprintf(log_file,L"native_equipment_idle_inventory_summary hits=%llu qualifications=%llu rejected=%llu action_permission=0\n",
        idle_inventory::hits,idle_inventory::qualifications,idle_inventory::rejected);
    idle_inventory::certificate={};idle_inventory::hits=idle_inventory::qualifications=idle_inventory::rejected=0;
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
    lancersaw_starter::reset_after_drain();
    fragruntime::reset_after_drain();
    smokeruntime::reset_after_drain();
    longshot_scope_owner::reset_after_drain();
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
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_FOLLOW)
        lancer_grasp::report_main_hold_trace(log_file);
#endif
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
