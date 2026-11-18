// Inside troika_entry_runtime after mounted_fire. Existing GT/source/left owner
// only. Computes a clamped candidate and uses ORIGINAL SetRotation/FaceRotation.
// No raw Rotation/AimDir/atom writer; actual gun/C0/fire consumption is separate.
namespace mounted_aim_candidates {
namespace math=gears_weapon::troika_mechanics;namespace parts=troika_mounted_parts;
namespace meta=weaponhand::meta;namespace fire=gears_troika_fire;
struct Anchors {
    gears_rig_tracking::NativeKey native{};gears_weapon::TroikaIdentity identity{};hand::Lease lease{};
    hand::ObjectIdentity asset{};std::uint64_t right_press{},calibration{};
    double units{};unsigned handles[2]{};std::array<gears_vr::Vector,2> local{};bool active{};
} anchors;
std::uint64_t calibration_counter{};unsigned log_rows{};bool proposal_ready{};
struct Proposal {fire::Run run{};math::Geometry geometry{};gears_weapon::TroikaSerialCandidate candidate{};
    std::array<std::int32_t,3> requested{},clamped{};bool original_rotation_clamp_checked{};
} proposal;
namespace action=parts::original_aim;
namespace gun=gears_weapon::troika_gun_consumption;
parts::Observation prepared_configuration{};gun::Prediction prepared_prediction{};bool prepared_prediction_ready{};
action::AttemptGate write_gate;
std::atomic<bool> applying{};
struct WriteReceipt {
    fire::Run run{};action::ActorReceipt actor{};
    bool set_started{},face_started{},verified{};
} write_receipt;
std::uint64_t unknown_results{};
struct GunPending {fire::Run run{};action::ActorReceipt actor{};gun::Prediction predicted{};parts::Observation configuration{};bool active{};} gun_pending;
struct GunReceipt {fire::Run run{};hand::ObjectIdentity asset{};std::uint64_t written_publication{},written_scene{},calibration{};
    gun::Match match{};unsigned socket_bone{};std::array<std::int32_t,3> actor_direction{};bool ready{};} gun_receipt;
parts::Observation anchor_configuration{};
void interrupt() noexcept {anchors.active=false;proposal_ready=prepared_prediction_ready=false;write_receipt.verified=false;gun_pending.active=false;gun_receipt.ready=false;}
bool current(void* context) noexcept {
    if(!context||stop_requested.load(std::memory_order_acquire)||!configured())return false;
    auto const& run=*static_cast<fire::Run const*>(context);gears_troika_control::Decision actual{};
    troika_mounted_context::Context gameplay{};
    return run.control.both_owned&&mounted_fire::exact_mount(run,owner)&&troika_tracking::source_current(run.source.frame)&&
        troika_tracking::control_receipt(owner,run.source.frame,actual)&&!std::memcmp(&actual,&run.control,sizeof actual)&&
        troika_mounted_context::observe(owner,gameplay)&&mounted_geometry::input_allowed(gameplay);
}
math::Space space(parts::control_axes::Space source) noexcept {
    using S=parts::control_axes::Space;
    switch(source){case S::world:return math::Space::world;case S::actor:return math::Space::actor;
    case S::component:return math::Space::component;case S::parent_bone:return math::Space::parent_bone;
    case S::bone:return math::Space::bone;case S::other_bone:return math::Space::other_bone;default:return math::Space::unknown;}
}
gears_vr::Vector world(math::Matrix const& matrix,gears_vr::Vector local) noexcept {
    return {matrix[12]+matrix[0]*local.x+matrix[4]*local.y+matrix[8]*local.z,
        matrix[13]+matrix[1]*local.x+matrix[5]*local.y+matrix[9]*local.z,
        matrix[14]+matrix[2]*local.x+matrix[6]*local.y+matrix[10]*local.z};
}
bool component(fire::Run& run,hand::ObjectIdentity const& object,math::Matrix& output) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};troika_reflection::ScriptStruct structure{};math::Matrix value{},closing{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,object)||
       !weaponhand::field(r,object.address,L"LocalToWorld",L"StructProperty",64,field)||
       !parts::control_binding::field_owner(r,field,L"PrimitiveComponent")||
       !troika_reflection::read(r,field.property,L"Matrix",L"Object",L"Core",64,structure)||
       !current(&run)||!r.at(object.address,field.property.offset,value)||!math::rigid(value)||
       !r.at(object.address,field.property.offset,closing)||value!=closing||
       !troika_reflection::current(r,structure,L"Matrix",L"Object",L"Core")||!parts::socket_contract::retained_metadata(r)||!current(&run))return false;
    output=value;return true;
}
bool aim_direction(fire::Run& run,std::array<std::int32_t,3>& output) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};troika_reflection::ScriptStruct structure{};
    std::array<std::int32_t,3> value{},closing{};unsigned package{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::field(r,run.source.frame.turret.address,L"AimDir",L"StructProperty",12,field)||
       !r.named(field.owner,L"Turret")||!r.path(r.result.identities[field.owner].outer,package)||!r.named(package,L"WarfareGame")||
       r.result.identities[package].outer||!troika_reflection::numeric(r,field.property,L"Rotator",L"IntProperty",
         std::array<wchar_t const*,3>{L"Pitch",L"Yaw",L"Roll"},structure)||!current(&run)||
       !r.at(run.source.frame.turret.address,field.property.offset,value)||
       !r.at(run.source.frame.turret.address,field.property.offset,closing)||value!=closing||
       !troika_reflection::current(r,structure,L"Rotator",L"Object",L"Core")||!parts::socket_contract::retained_metadata(r)||!current(&run))return false;
    output=value;return true;
}
bool controller_direction(fire::Run& run,std::array<std::int32_t,3>& output) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};troika_reflection::ScriptStruct structure{};
    std::array<std::int32_t,3> value{},closing{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,run.source.frame.controller)||
       !weaponhand::field(r,run.source.frame.controller.address,L"Rotation",L"StructProperty",12,field)||
       !parts::control_binding::field_owner(r,field,L"Actor")||!troika_reflection::numeric(r,field.property,L"Rotator",L"IntProperty",
         std::array<wchar_t const*,3>{L"Pitch",L"Yaw",L"Roll"},structure)||!current(&run)||
       !r.at(run.source.frame.controller.address,field.property.offset,value)||
       !r.at(run.source.frame.controller.address,field.property.offset,closing)||value!=closing||
       !troika_reflection::current(r,structure,L"Rotator",L"Object",L"Core")||!parts::socket_contract::retained_metadata(r)||!current(&run))return false;
    output=value;return true;
}
#include "weapon_troika_gun_consumer.inl"
#include "weapon_troika_c0_consumer.inl"
bool before_set(void* context) noexcept {
    auto* run=static_cast<fire::Run*>(context);
    if(!run||write_receipt.set_started||!write_gate.active()||!current(run))return false;
    write_receipt.set_started=true;write_receipt.actor.setter.dispatch=action::Dispatch::uncertain;return true;
}
bool before_face(void* context) noexcept {
    auto* run=static_cast<fire::Run*>(context);
    if(!run||write_receipt.face_started||!write_gate.active()||!write_receipt.actor.setter.accepted_set()||
       !write_receipt.actor.controller_read||!action::rotation_matches(write_receipt.actor.controller,write_receipt.actor.requested)||!current(run))return false;
    write_receipt.face_started=true;write_receipt.actor.face.dispatch=action::Dispatch::uncertain;return true;
}
bool apply(Proposal& wanted) noexcept {
    write_receipt.verified=false;
    try{
        auto& run=wanted.run;auto const& s=run.source.frame;
        const std::array<std::uint64_t,7> scope{s.native.game_pid,s.native.game_thread,s.native.game_start,
            s.native.native_started_qpc,s.native.registration_cookie,run.source.entry_binding,run.source.entry_token};
        if(!wanted.original_rotation_clamp_checked||!current(&run)||!write_gate.begin(scope,s.publication_sequence))return false;
        applying.store(true,std::memory_order_release);
        struct Finish {~Finish(){write_gate.finish();applying.store(false,std::memory_order_release);}} finish;
        // Last diagnostic receipt and cumulative unknown count survive stop.
        // Rotation is one synchronous assignment, not an owned held input.
        // Stop withdraws future writes; it never restores a new actor's pose.
        write_receipt={};write_receipt.run=run;write_receipt.actor.requested=wanted.clamped;
        write_receipt.actor.setter=action::set_native(s.controller,wanted.clamped,&run,current,before_set);
        if(!write_receipt.actor.setter.accepted_set()){
            if(write_receipt.set_started&&write_receipt.actor.setter.dispatch==action::Dispatch::uncertain&&unknown_results!=UINT64_MAX)++unknown_results;
            return false;
        }
        write_receipt.actor.controller_read=controller_direction(run,write_receipt.actor.controller);
        if(!write_receipt.actor.controller_read||!action::rotation_matches(write_receipt.actor.controller,wanted.clamped))return false;
        write_receipt.actor.face=action::face_native(s.turret,write_receipt.actor.controller,&run,current,before_face);
        if(!write_receipt.actor.face.completed_face()){
            if(write_receipt.face_started&&write_receipt.actor.face.dispatch==action::Dispatch::uncertain&&unknown_results!=UINT64_MAX)++unknown_results;
            return false;
        }
        write_receipt.actor.aim_read=aim_direction(run,write_receipt.actor.aim);
        // These are original Actor/Controller acknowledgements ONLY. Original
        // UpdateTurret/animation/C0/muzzle still have to consume the direction.
        write_receipt.verified=write_receipt.actor.verified_actor()&&current(&run);
        return write_receipt.verified;
    }catch(...){return false;}
}
void observe(gears_troika_source::Frame const& source,gears_weapon::TroikaMountedMapping const& mapped,parts::Observation const& measurements) noexcept {
    if(applying.load(std::memory_order_acquire))return; // Original-call callbacks cannot replace the in-flight receipt/anchors.
    proposal_ready=false;
    try{
        Proposal next{};next.run.source={};next.run.source.frame=source;
        next.run.source.entry_binding=owner.receipt.binding;next.run.source.entry_token=owner.receipt.token;
        if(!measurements.controls_known||!measurements.control_enums_known||!measurements.control_binding_known||
           measurements.publication!=source.publication_sequence||measurements.input_sync_ns!=source.controllers.sync_ns||
           mapped.publication_sequence!=source.publication_sequence||mapped.source_ticket!=source.source_ticket||mapped.hands.tracked_mask!=3||
           !troika_tracking::control_receipt(owner,source,next.run.control)||!current(&next.run)){interrupt();return;}
        const bool boundary=!anchors.active||mapped.continuity_changed||!gears_rig_tracking::same(anchors.native,source.native)||
            !(anchors.identity==mapped.identity)||!hand::same(anchors.lease,next.run.control.lease)||
            anchors.right_press!=next.run.control.right_press_sequence||!hand::same(anchors.asset,measurements.asset)||
            anchors.units!=mapped.units_per_metre||anchors.handles[0]!=measurements.hand_handle[0]||anchors.handles[1]!=measurements.hand_handle[1]||
            !same_configuration(anchor_configuration,measurements);
        if(boundary){
            interrupt();if(calibration_counter==UINT64_MAX)return;Anchors capture{};
            for(unsigned side=0;side<2;++side){double scale{};gears_weapon::troika_handle_geometry::Contact contact{};
                if(!gears_weapon::troika_handle_geometry::contact(measurements.bones[5],mapped.hands.world[side].position,
                   measurements.hand_handle[side],mapped.units_per_metre,contact)||!contact.touched||
                   !gears_weapon::troika_handle_geometry::local_point(measurements.bones[5],mapped.hands.world[side].position,capture.local[side],scale))return;
                capture.handles[side]=measurements.hand_handle[side];}
            capture.native=source.native;capture.identity=mapped.identity;capture.lease=next.run.control.lease;
            capture.asset=measurements.asset;capture.right_press=next.run.control.right_press_sequence;capture.units=mapped.units_per_metre;
            capture.calibration=++calibration_counter;capture.active=true;anchors=capture;anchor_configuration=measurements;
        }
        observe_gun(next.run,mapped,measurements);
        observe_hands(next.run);
        math::Input input{};input.binding=next.run.source.entry_binding;input.calibration=anchors.calibration;
        input.units_per_metre=mapped.units_per_metre;input.maximum_error_metres=.025;
        input.actor_rotation=measurements.turret_rotation;input.control_bone=measurements.original_binding.bone_index;
        input.original_control_binding_and_reference_closed=true;input.component_world=math::identity();
        for(auto& bone:input.bone_world)bone=math::identity();constexpr unsigned indices[]={0,1,2,7,8,9};
        for(unsigned i=0;i<6;++i)input.bone_world[indices[i]]=measurements.bones[i];
        bool need_component{};
        for(unsigned role=0;role<2;++role){auto const& c=measurements.original_controls.controls[role];
            if(std::abs(c.strength-1)>1e-4||std::abs(c.alpha-1)>1e-4)return;
            input.rotation_mask[role]=c.flags&7u;input.space[role]=space(measurements.control_enums.rotation[role]);
            need_component=need_component||input.space[role]==math::Space::component;
            if(input.space[role]==math::Space::other_bone)for(unsigned bone=0;bone<13;++bone){meta::RawName name{};
                std::memcpy(&name,measurements.original_binding.reference_bytes[bone].data(),8);
                if(name.index==c.other_bone.index&&name.number==c.other_bone.number)input.other_bone[role]=bone;}
            input.anchors[role]=world(measurements.bones[5],anchors.local[role]);
        }
        if(need_component&&!component(next.run,measurements.component,input.component_world))return;
        if(!math::geometry(input,next.geometry)||!math::propose(next.geometry,source.source_ticket,
           {mapped.hands.world[0].position,mapped.hands.world[1].position},true,true,next.candidate))return;
        std::array<std::int32_t,3> aim{};if(!aim_direction(next.run,aim))return;
        constexpr double radians=6.28318530717958647692/65536;
        const auto basis=gears_vr::ue_matrix({double(aim[0]%65536)*radians,double(aim[1]%65536)*radians,double(aim[2]%65536)*radians});
        gears_vr::Vector direction{};
        if(!math::direction(next.geometry,next.candidate,{basis[0][0],basis[1][0],basis[2][0]},direction))return;
        next.requested={gears_vr::to_rotator_units(std::atan2(direction.z,std::hypot(direction.x,direction.y))),
            gears_vr::to_rotator_units(std::atan2(direction.y,direction.x)),aim[2]};
        if(!parts::original_limits::rotation_native(source.turret,next.requested,&next.run,current,next.clamped)||!current(&next.run))return;
        next.original_rotation_clamp_checked=true;proposal=next;proposal_ready=true;
        prepared_configuration=measurements;
        prepared_prediction_ready=action::rotation_matches(next.requested,next.clamped)&&gun::predict(next.geometry,next.candidate,measurements.bones,prepared_prediction);
    }catch(...){interrupt();}
}
// Fire qualifications inspect the actual current consumed aim BEFORE staging
// the next direction. Updating AimDir first would invalidate that same proof
// every moving frame. Original game Tick/animation still consumes this write.
void advance() noexcept {
    if(!proposal_ready)return;proposal_ready=false;
    try{
        auto const& next=proposal;auto const& source=next.run.source.frame;
        const bool actor_confirmed=apply(proposal);
        gun_pending.active=false;
        if(actor_confirmed&&prepared_prediction_ready){
            gun_pending.predicted=prepared_prediction;gun_pending.run=next.run;gun_pending.actor=write_receipt.actor;
            gun_pending.configuration=prepared_configuration;gun_pending.active=true;
        }
        if(log_file&&log_rows<8){++log_rows;fwprintf(log_file,L"troika_aim_candidate source=%llu calibration=%llu parent=%.9g child=%.9g child_follows=%u error_squared=%.9g requested=%d,%d,%d original_clamped=%d,%d,%d set_started=%u set_result=%u set_bool_known=%u set_bool=%u controller_read=%u face_started=%u face_result=%u aim_read=%u actor_confirmed=%u unknown=%llu previous_gun_observed=%u qualification_phase_closed=1\n",
            source.publication_sequence,anchors.calibration,next.candidate.fit.parent_angle,next.candidate.fit.child_angle,
            unsigned(next.geometry.serial.child_axis_follows_parent),next.candidate.fit.error_squared,
            next.requested[0],next.requested[1],next.requested[2],next.clamped[0],next.clamped[1],next.clamped[2],
            unsigned(write_receipt.set_started),unsigned(write_receipt.actor.setter.dispatch),unsigned(write_receipt.actor.setter.original_bool_known),
            unsigned(write_receipt.actor.setter.original_bool),unsigned(write_receipt.actor.controller_read),unsigned(write_receipt.face_started),
            unsigned(write_receipt.actor.face.dispatch),unsigned(write_receipt.actor.aim_read),unsigned(actor_confirmed),unknown_results,unsigned(gun_receipt.ready));fflush(log_file);}
    }catch(...){interrupt();}
}
} // namespace mounted_aim_candidates
