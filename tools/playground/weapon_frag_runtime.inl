#include "weapon_frag_authored_main.inl"
// One ordinary PostPlayerTick dispatcher. The existing equipment owner grants
// the held Frag; raw producer history supplies motion. Only original methods
// prepare/spawn/consume/notify/re-equip. No second skeleton or input writer.
namespace fragruntime {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace model=gears_weapon;
namespace meta=gripdamage_metadata;
using Counts=lancerreload_inventory::Counts;
std::optional<model::BoloGrenadeInteraction> owner;
model::BoloTriggerObserver trigger;
model::BoloInstanceConfig config{};
hand::Frame parent{};
Counts observed_counts{};std::uint64_t revision{},sequence{},event_serial{},last_input{},held_generation{};
bool counts_known{},faulted{},finishing{};
std::uint64_t disarmed_hold{};std::uint32_t disarmed_press{};
double binding_units{};
unsigned sample_gate{};
unsigned rig_mismatch{};
model::BoloFrame observation{}; // Caller-owned motion workspace, not another publication.
std::uint64_t reports{};unsigned previous_gate{~0u};
unsigned launch_reports{};
unsigned mapping_reports{};
unsigned pitch_reports{};
unsigned launch_reject_reports{};
unsigned trigger_refusal_reports{};
unsigned adoption_refusal_reports{};
model::BoloTriggerDiagnostic trigger_diagnostic{};
std::int64_t trigger_sample_started_ns{};
struct SampleCostMarks {
    std::int64_t begin_ns{},current_held_ns{},rig_ns{},history_ns{},velocity_inventory_ns{},stamp_final_current_ns{};
    std::int64_t frame_now_ns{},controller_sync_ns{};
} sample_cost{};
struct ReturnTransition {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};
    gears_hand_continuity::TrackedInput input{};gears_hand_continuity::Publisher publisher{};
    std::uint64_t hold{};std::int64_t deadline{},verified_ns{};
    std::uint32_t release_pressed{},release_released{};bool active{};
};
ReturnTransition returning{};
gears_hand_continuity::TrackedInput sampled_input{};
gears_hand_continuity::Publisher sampled_publisher{};
void report(unsigned gate,model::BoloEvent event=model::BoloEvent::observed) noexcept {
    if(!log_file||(gate==previous_gate&&event==model::BoloEvent::observed)||reports>=96)return;
    fwprintf(log_file,L"frag_original_runtime gate=%u event=%u stage=%u binding=%llu input=%llu count_revision=%llu fault=%u preparation_owed=%u rig_mismatch=%u grip_release=1 virtual_mass_kg=2 preview_suppressed=%llu\n",
        gate,unsigned(event),owner?unsigned(owner->stage()):0,owner?owner->binding_token():0,
        last_input,revision,unsigned(faulted),unsigned(fragpreparation::owed()),rig_mismatch,fragpreview::suppressed);
    fflush(log_file);previous_gate=gate;++reports;
}
model::BoloIdentity identity(hand::Frame const& f) noexcept {
    return {f.world.address,f.pawn.address,f.controller.address,f.weapon.address,f.key.world_generation,
        f.stereo.session,f.stereo.space_epoch,f.controllers.input_epoch,f.controllers.profile_generation};
}
bool same_parent(hand::Frame const& a,hand::Frame const& b) noexcept {
    auto key=b.key;key.frame=a.key.frame;
    return a.key.frame>=b.key.frame&&hand::same(a.key,key)&&
        hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&hand::same(a.pawn,b.pawn)&&
        hand::same(a.mesh,b.mesh)&&hand::same(a.weapon,b.weapon)&&a.reference_generation==b.reference_generation&&
        vr::stereo_same_pose(a.reference,b.reference)&&a.stereo.source==b.stereo.source;
}
template<class T,std::size_t N,std::size_t K>
bool field(hand::ObjectIdentity const& object,wchar_t const(&label)[N],wchar_t const(&kind)[K],T& out) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField descriptor{};T a{},b{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,object)||
       !weaponhand::field(r,object.address,label,kind,sizeof(T),descriptor)||
       !r.at(object.address,descriptor.property.offset,a)||!r.at(object.address,descriptor.property.offset,b)||
       std::memcmp(&a,&b,sizeof(T))||!unified_weapon_fire::retained_metadata(r))return false;
    if constexpr(std::is_same_v<T,playground::Vector>){
        if(!unified_weapon_fire::core_struct(r,descriptor.property,L"Vector")||
           !std::isfinite(a.x)||!std::isfinite(a.y)||!std::isfinite(a.z))return false;
    }
    out=a;return true;
}
bool inventory(hand::Frame const& f,model::BoloInventory& out,Counts* raw=nullptr) noexcept {
    Counts c{};if(!fragpreparation::inventory(f,c))return false;
    const auto total=std::int64_t(c.capacity)-c.used+c.reserve;
    if(total<0||total>10000)return false;
    if(!counts_known||!(c==observed_counts)){
        if(revision==UINT64_MAX)return false;
        ++revision;observed_counts=c;counts_known=true;
    }
    out={revision,std::uint32_t(total),true};if(raw)*raw=c;return true;
}
bool stamp(model::BoloStamp& out) noexcept {
    if(!owner||sequence==UINT64_MAX)return false;
    float seconds{};
    if(!field(parent.world,L"TimeSeconds",L"FloatProperty",seconds)||!std::isfinite(seconds)||seconds<0||seconds>1e9f)return false;
    const auto now=weaponhand::clock_ns(nullptr);if(now<=0)return false;
    out={config.identity,owner->binding_token(),++sequence,observation.stamp.motion_epoch,now,
        std::int64_t(double(seconds)*1000000.)};return true;
}
template<std::size_t N>bool original_phase(hand::Frame const& f,wchar_t const (&expected)[N]) noexcept {
    // Do not adopt a stock throw/re-equip state whose animation could still
    // produce its own notification. This VR route starts no throw animation.
    playground::Args state(f.weapon.address,L"GetStateName");
    const auto fn=state.r.function,cls=fn?get<uptr>(fn+0x40):0,package=cls?get<uptr>(cls+0x40):0;
    if(!state.valid||!registered(cls)||!registered(package)||!playground::named(cls,L"Object")||
       !playground::named(package,L"Core")||get<uptr>(package+0x40)||
       !(get<unsigned>(fn+0xf4)&0x400)||!vrequip::result_property(state,L"NameProperty",8)||
       !nativeequipment::current(nullptr,f)||!state.run())return false;
    const auto value=state.output<Name>();meta::Table objects{},names{};meta::NameText text{};
    return !value.number&&meta::tables(base,objects,names)&&meta::fresh_name(names,value.index,text)&&
        meta::exact_text(text,expected)&&nativeequipment::current(nullptr,f);
}
bool active_original(hand::Frame const& f) noexcept {return original_phase(f,L"Active");}
bool preparation_retention_refusal(unsigned reason,unsigned lifetime_failure=~0u) noexcept {
    static unsigned preparation_retention_reports{};
    if(owner&&fragpreparation::owed()&&log_file&&preparation_retention_reports++<8){
        fwprintf(log_file,L"frag_preparation_retention_refusal reason=%u lifetime_failure=%u frame=%llu last_input=%llu hold=%llu observed_ns=%lld client_phase=%u disarm_reason=%u registration_alive=%u old_action_epoch=%llu current_metadata_generation=%llu\n",
            reason,lifetime_failure,parent.key.frame,last_input,held_generation,weaponhand::clock_ns(nullptr),
            unsigned(weaponhand::client.phase()),weaponhand::client.last_disarm_reason(),
            unsigned(weaponhand::client.registration_alive(parent.key.registration_cookie,parent.key.native_started_qpc)),
            fragpreparation::journal.lifetime.epoch,weaponhand::client.metadata_generation_on_gt());fflush(log_file);
    }
    return false;
}
bool retain_preparation_during_pose_gap(uptr pc) noexcept {
    unsigned lifetime_failure=~0u;
    if(!owner||owner->stage()!=model::BoloStage::prepared||owner->pending_request().token||returning.active||
       !fragpreparation::owed()||!fragpreparation::journal.prepared||fragpreparation::stop_requested.load()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||pc!=parent.controller.address||
       held_generation!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(parent.world.address,parent.pawn.address,parent.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(fragpreparation::journal.lifetime,&lifetime_failure))return preparation_retention_refusal(1,lifetime_failure);
    for(const auto& id:{parent.world,parent.controller,parent.pawn,parent.mesh,parent.weapon})if(!weapon_part_assets::object(id))return preparation_retention_refusal(2,lifetime_failure);
    if(playground::pointer(parent.controller.address,L"Pawn")!=parent.pawn.address||
       playground::pointer(parent.pawn.address,L"Weapon")!=parent.weapon.address||
       playground::pointer(parent.pawn.address,L"WorldInfo")!=parent.world.address||
       playground::pointer(parent.weapon.address,L"Instigator")!=parent.pawn.address)return preparation_retention_refusal(3,lifetime_failure);
    // A missing pose publication can also accompany a real pause. Keeping a
    // valid original action lifetime is not permission to carry that grasp
    // through pause. Failure to authenticate the original pause field revokes
    // this retention only; the normal original-stop debt remains payable.
    uptr pauser{};
    if(!field(parent.world,L"Pauser",L"ObjectProperty",pauser)||pauser)return preparation_retention_refusal(4,lifetime_failure);
    fragpreparation::State prepared_state{};Counts counts{};
    if(!fragpreparation::state(parent,prepared_state)||!prepared_state.key||!prepared_state.controller_target||!prepared_state.pawn_target||!prepared_state.spinning||
       !fragpreparation::inventory(parent,counts)||!(counts==fragpreparation::journal.before))return preparation_retention_refusal(5,lifetime_failure);
    vr::StereoSample stereo{};vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    const auto begin=weaponhand::clock_ns(nullptr);const bool sim=parent.stereo.source==vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,raw,publisher)!=gears_current_tracking::Failure::none)return preparation_retention_refusal(6,lifetime_failure);
    const auto end=weaponhand::clock_ns(nullptr);const auto& initial=fragpreparation::journal.source.controllers.hands[1];
    const auto& hand=raw.input.hands[1];
    const bool accepted=begin>0&&end>=begin&&end-begin<=50000000&&publisher.pid&&publisher.start&&
        publisher.pid==sampled_publisher.pid&&publisher.start==sampled_publisher.start&&
        raw.stream_epoch==sampled_input.stream_epoch&&raw.tracking_epoch[1]==sampled_input.tracking_epoch[1]&&
        gears_hand_continuity::detail::same_key(raw.input,sampled_input.input)&&
        raw.input.sequence>=sampled_input.input.sequence&&raw.input.sync_ns>=sampled_input.input.sync_ns&&
        vr::controller_frame_matches(raw.input,stereo,end,sim)&&end>=raw.input.sync_ns&&end-raw.input.sync_ns<=100000000&&
        vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)&&hand.squeeze_active==1&&
        hand.squeeze>vr::controller_analog_release_threshold&&
        hand.pressed[vr::control_squeeze]==initial.pressed[vr::control_squeeze]&&
        hand.released[vr::control_squeeze]==initial.released[vr::control_squeeze]&&
        weaponhand::client.existing_action_lifetime_on_gt(fragpreparation::journal.lifetime,&lifetime_failure);
    // Failure-only classification of the already-read values. Keep accepted and
    // its short-circuit lifetime check above unchanged; this grants no action.
    if(!accepted&&owner&&fragpreparation::owed()&&log_file){
        static unsigned predicate_reports{};
        if(predicate_reports++<8){
            unsigned failed{};
            if(!(begin>0&&end>=begin&&end-begin<=50000000))failed|=1u;
            if(!(publisher.pid&&publisher.start&&publisher.pid==sampled_publisher.pid&&publisher.start==sampled_publisher.start))failed|=2u;
            if(!(raw.stream_epoch==sampled_input.stream_epoch&&raw.tracking_epoch[1]==sampled_input.tracking_epoch[1]))failed|=4u;
            if(!gears_hand_continuity::detail::same_key(raw.input,sampled_input.input))failed|=8u;
            if(!(raw.input.sequence>=sampled_input.input.sequence&&raw.input.sync_ns>=sampled_input.input.sync_ns))failed|=16u;
            if(!vr::controller_frame_matches(raw.input,stereo,end,sim))failed|=32u;
            if(!(end>=raw.input.sync_ns&&end-raw.input.sync_ns<=100000000))failed|=64u;
            if(!vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active))failed|=128u;
            if(!(hand.squeeze_active==1&&hand.squeeze>vr::controller_analog_release_threshold))failed|=256u;
            if(!(hand.pressed[vr::control_squeeze]==initial.pressed[vr::control_squeeze]&&hand.released[vr::control_squeeze]==initial.released[vr::control_squeeze]))failed|=512u;
            fwprintf(log_file,L"frag_preparation_predicate_refusal failed_mask=%u begin_ns=%lld end_ns=%lld raw_sequence=%llu raw_sync_ns=%lld sampled_sequence=%llu sampled_sync_ns=%lld raw_stream=%llu sampled_stream=%llu raw_tracking=%llu sampled_tracking=%llu raw_press=%u initial_press=%u raw_release=%u initial_release=%u lifetime_failure=%u\n",
                failed,begin,end,raw.input.sequence,raw.input.sync_ns,sampled_input.input.sequence,sampled_input.input.sync_ns,
                raw.stream_epoch,sampled_input.stream_epoch,raw.tracking_epoch[1],sampled_input.tracking_epoch[1],
                hand.pressed[vr::control_squeeze],initial.pressed[vr::control_squeeze],hand.released[vr::control_squeeze],initial.released[vr::control_squeeze],lifetime_failure);fflush(log_file);
        }
    }
    return accepted?true:preparation_retention_refusal(7,lifetime_failure);
}
// Preserve only an already confirmed equipment hold while the ORIGINAL
// successful throw re-equips its next grenade. No Frame, pose, input edge,
// attachment or new hold is fabricated. Other source/identity loss still
// follows the normal equipment revocation path.
bool return_retention_refusal(unsigned reason) noexcept {
    static unsigned retention_refusal_reports{};
    if(returning.active&&log_file&&retention_refusal_reports++<8){
        fwprintf(log_file,L"frag_return_retention_refusal reason=%u frame=%llu epoch=%llu hold=%llu deadline_ns=%lld observed_ns=%lld\n",
            reason,returning.source.key.frame,returning.lifetime.epoch,returning.hold,
            returning.deadline,weaponhand::clock_ns(nullptr));fflush(log_file);
    }
    return false;
}
bool retain_return_during_pose_gap(uptr pc) noexcept {
    if(!returning.active||!weaponhand::current_gt(nullptr)||fragpreparation::stop_requested.load()||
       !nativeequipment::restoration_quiet()||pc!=returning.source.controller.address)return return_retention_refusal(1);
    const auto& f=returning.source;const auto now=weaponhand::clock_ns(nullptr);
    if(now<=0||now>returning.deadline||!returning.hold||returning.hold!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(returning.lifetime))return return_retention_refusal(2);
    for(const auto& id:{f.world,f.controller,f.pawn,f.mesh,f.weapon})if(!weapon_part_assets::object(id))return return_retention_refusal(3);
    const auto inv=playground::pointer(f.pawn.address,L"InvManager");uptr item{};
    if(playground::pointer(f.controller.address,L"Pawn")!=f.pawn.address||
       playground::pointer(f.pawn.address,L"Weapon")!=f.weapon.address||
       playground::pointer(f.pawn.address,L"WorldInfo")!=f.world.address||
       playground::pointer(f.pawn.address,L"Mesh")!=f.mesh.address||
       playground::pointer(f.weapon.address,L"Instigator")!=f.pawn.address||
       !vrequip::read_slot(inv,1,item)||item!=f.weapon.address)return return_retention_refusal(4);
    int health{};unsigned char mode{},special{};
    if(!field(f.pawn,L"Health",L"IntProperty",health)||health<=0||
       !field(f.pawn,L"SpecialMove",L"ByteProperty",special)||special||
       !field(f.weapon,L"CurrentFireMode",L"ByteProperty",mode)||(mode!=0&&mode!=5))return return_retention_refusal(5);
    vr::StereoSample stereo{};vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    const bool sim=f.stereo.source==vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,raw,publisher)!=gears_current_tracking::Failure::none)return return_retention_refusal(6);
    const auto end=weaponhand::clock_ns(nullptr);
    if(end<now||end-now>50000000||end>returning.deadline||!publisher.pid||!publisher.start||
       publisher.pid!=returning.publisher.pid||publisher.start!=returning.publisher.start||
       raw.stream_epoch!=returning.input.stream_epoch||raw.tracking_epoch[1]!=returning.input.tracking_epoch[1]||
       !gears_hand_continuity::detail::same_key(raw.input,returning.input.input)||
       raw.input.sequence<returning.input.input.sequence||raw.input.sync_ns<returning.input.input.sync_ns||
       !vr::controller_frame_matches(raw.input,stereo,end,sim)||end<raw.input.sync_ns||end-raw.input.sync_ns>100000000||
       !vr::controller_pose_usable(raw.input.hands[1].grip,raw.input.hands[1].grip_flags,raw.input.hands[1].grip_active)||
       !weaponhand::client.existing_action_lifetime_on_gt(returning.lifetime))return return_retention_refusal(7);
    returning.input=raw;returning.verified_ns=end;return true;
}
// Visual permission for the sole exact return continuation, never an action
// or attachment permit. Its release counters cannot survive a new Grip press.
bool return_visibility_current(hand::Frame const& f) noexcept {
    if(!returning.active||!same_parent(f,returning.source)||!nativeequipment::current(nullptr,f)||
       fragpreparation::stop_requested.load()||!returning.hold||
       returning.hold!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(returning.lifetime))return false;
    const auto now=weaponhand::clock_ns(nullptr);
    const auto& right=returning.input.input.hands[1];
    if(now<=0||now>returning.deadline||returning.verified_ns<=0||now<returning.verified_ns||
       now-returning.verified_ns>50000000||right.squeeze_active!=1||!std::isfinite(right.squeeze)||right.squeeze<0||
       right.squeeze>vr::controller_analog_release_threshold||
       right.pressed[vr::control_squeeze]!=returning.release_pressed||
       right.released[vr::control_squeeze]!=returning.release_released)return false;
    unsigned char mode{};
    return field(f.weapon,L"CurrentFireMode",L"ByteProperty",mode)&&mode==5&&
        original_phase(f,L"EquipNextGrenade")&&nativeequipment::current(nullptr,f);
}
bool pre_equipment(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    if(!returning.active)return false;
    if(!retain_return_during_pose_gap(f.controller.address)){returning={};report(36);return false;}
    // Preserve stock replenish timing, but hide its already reattached next
    // item while the physical Grip is still released. Existing visibility
    // ownership is also the only permission to inspect this hidden attachment.
    if(game.known&&game.main_hand_held&&game.kind==hand::WeaponKind::hand_thrown)
        (void)nativeequipment::stow_native::hide_released_frag_return(f);
    lancermain_asset::Evidence asset{};Counts loaded{};
    const bool attached=nativeequipment::stow_native::released_frag_attached(f)||
        (lancermain_asset::read(f,asset,lancermain_asset::AssetScope::frag_hold)==lancermain_asset::Status::ready&&
         nativeequipment::attached_frame(f,asset.component.address));
    if(game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&
       fragpreparation::inventory(f,loaded)&&loaded.capacity>loaded.used&&
       active_original(f)&&attached&&nativeequipment::current(nullptr,f)){
        if(log_file){fwprintf(log_file,L"frag_original_return weapon=%llx hold=%llu frame=%llu attached=1 active=1 actual_input_sequence=%llu\n",
            f.weapon.address,returning.hold,f.key.frame,returning.input.input.sequence);fflush(log_file);}
        // Original replenishment is inventory availability, not a second
        // physical grasp. End only this exact released throwable hold; the
        // existing idle-unheld adapter owns its visibility/restoration.
        if(vrequip::model.release_throwable(f.world.address,f.pawn.address,f.weapon.address,returning.hold))
            weaponhand::held_binding={};
        returning={};report(35);return false;
    }
    return true;
}
bool projectile_type(hand::Frame const& f,hand::ObjectIdentity& out) noexcept {
    return frag_original::projectile_class(f,out);
}
bool make_config(hand::Frame const& f,gears_rig_tracking::Frame const& rig,model::BoloInstanceConfig& out) noexcept {
    lancermain_asset::Evidence asset{};vrequip::MeshPoseProbe mesh{};
    hand::ObjectIdentity projectile{};frag_original::Entry prepare{},stop{},notify{},empty{},finished{},origin{},velocity{},base_throw{},mode{},prediction{};
    sample_gate=10;
    if(lancermain_asset::read(f,asset,lancermain_asset::AssetScope::frag_hold)!=lancermain_asset::Status::ready||
       !nativeequipment::attached_frame(f,asset.component.address)||!vrequip::mesh_pose_probe(f.weapon.address,mesh)||
       mesh.mesh!=asset.component.address||mesh.asset!=asset.asset.address)return false;
    sample_gate=11;if(!projectile_type(f,projectile))return false;
    sample_gate=12;if(
       !frag_original::bind(f.weapon.address,frag_original::Method::prepare,prepare)||
       !frag_original::bind(f.weapon.address,frag_original::Method::stop,stop)||
       !frag_original::bind(f.weapon.address,frag_original::Method::notify,notify)||
       !frag_original::bind(f.weapon.address,frag_original::Method::empty,empty)||
       !frag_original::bind(f.weapon.address,frag_original::Method::finished,finished)||
       !frag_original::bind(f.weapon.address,frag_original::Method::origin,origin)||
       !frag_original::bind(f.weapon.address,frag_original::Method::velocity,velocity)||
       !frag_original::bind(f.weapon.address,frag_original::Method::base_throw,base_throw)||
       !frag_original::bind(f.weapon.address,frag_original::Method::mode,mode)||
       !frag_original::bind(f.weapon.address,frag_original::Method::prediction,prediction))return false;
    sample_gate=13;playground::Args gravity(f.pawn.address,L"GetGravityZ");
    if(!weapon_part_assets::native_function(gravity,L"GetGravityZ",L"Actor")||
       !vrequip::result_property(gravity,L"FloatProperty",4)||!nativeequipment::current(nullptr,f)||!gravity.run())return false;
    const double g=-double(gravity.output<float>())/rig.units_per_metre;
    const auto* m=mesh.local_to_world;
    const double scale=std::hypot(std::hypot(double(m[0]),double(m[1])),double(m[2]));
    if(!std::isfinite(g)||g<=0||g>=40||!std::isfinite(scale)||scale<.1||scale>10)return false;
    for(unsigned axis=1;axis<3;++axis){const auto n=axis*4;
        if(std::abs(std::hypot(std::hypot(double(m[n]),double(m[n+1])),double(m[n+2]))-scale)>scale*.001)return false;}
    sample_gate=14;gears_weapon_frag_grasp::Profile fit{};
    if(!gears_weapon_frag_grasp::marcus115(gears_hand_authored_mount::marcus115_signature,1,rig.units_per_metre,fit,scale))return false;
    const auto rotation=vr::multiply(vr::rotation_matrix(fit.hand.calibration.grip_to_wrist_rotation),vr::transpose(fit.wrist_in_mesh));
    const auto shift=vr::transform(rotation,{-fit.palm_in_mesh.x*scale,-fit.palm_in_mesh.y*scale,-fit.palm_in_mesh.z*scale});
    model::BoloInstanceConfig c{};c.identity=identity(f);c.input_source=f.stereo.source;
    c.centre_in_right_grip_metres={shift.y/rig.units_per_metre,shift.z/rig.units_per_metre,-shift.x/rig.units_per_metre};
    c.motion.gravity_mps2=g;c.motion.virtual_mass_kg=2.;c.original_projectile_type=projectile.address;
    c.prepare_entry=prepare.function.address;c.launch_entry=notify.function.address;c.safety_entry=stop.function.address;
    c.original_no_ammo_cue=empty.function.address;
    unified_weapon_fire::Fingerprint fingerprint;
    for(const auto* e:{&prepare,&stop,&notify,&empty,&finished,&origin,&velocity,&base_throw,&mode,&prediction})fingerprint.word(e->fingerprint);
    fingerprint.word(projectile.address);fingerprint.word(asset.asset.address);fingerprint.word(rig.reference_generation);
    c.config_serial=fingerprint.value;c.grip_geometry_serial=asset.asset.address;
    c.release_rule_serial=notify.fingerprint;c.fuse_rule_serial=projectile.address;c.audio_rule_serial=empty.fingerprint;
    c.release_input=model::BoloReleaseInput::right_grip_release_after_hold;c.fuse_owner=model::BoloFuseOwner::original_projectile;
    // Admission evidence is current original metadata plus the audited fixed
    // adapter contracts. It is NOT a game/headset acceptance result. Each
    // execution still needs actual counts, state, once-only and return receipts.
    c.current_instance_verified=c.original_frag_classes_verified=c.grip_geometry_verified=true;
    c.release_input_verified=c.fuse_rule_verified=c.original_prepare_no_ammo_change_verified=true;
    c.original_launch_once_verified=c.original_initial_pose_velocity_override_verified=true;
    c.original_effects_preserved_verified=c.non_throwing_safety_entry_verified=true;
    c.original_gravity_and_units_verified=c.original_no_ammo_cue_verified=true;
    if(!c.valid()||!nativeequipment::current(nullptr,f))return false;
    out=c;return true;
}
bool sample(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    sample_cost={};sample_cost.begin_ns=weaponhand::clock_ns(nullptr);
    sample_cost.frame_now_ns=f.now_ns;sample_cost.controller_sync_ns=f.controllers.sync_ns;
    sample_gate=1;
    if(!nativeequipment::current(nullptr,f)||!game.known||!game.main_hand_held||
       game.kind!=hand::WeaponKind::hand_thrown||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       vrequip::model.held_slot()!=*model::original_inventory_slot(model::BodySlot::grenade))return false;
    sample_cost.current_held_ns=weaponhand::clock_ns(nullptr);
    sample_gate=2;gears_rig_tracking::Frame rig{};
    rig_mismatch=0;
    if(handrig::read(rig,0,unsigned(f.stereo.source)).reply!=gears_rig_tracking::Reply::accepted){rig_mismatch=1;return false;}
    if(rig.native.registration_cookie!=f.key.registration_cookie||rig.native.native_started_qpc!=f.key.native_started_qpc)rig_mismatch|=2;
    if(rig.world_generation!=f.key.world_generation||rig.reference_generation!=f.reference_generation)rig_mismatch|=4;
    if(!hand::same(rig.world,f.world)||!hand::same(rig.controller,f.controller)||!hand::same(rig.pawn,f.pawn)||
       !hand::same(rig.mesh,f.mesh)||!hand::same(rig.weapon,f.weapon))rig_mismatch|=8;
    if(!vr::stereo_same_pose(rig.reference,f.reference))rig_mismatch|=16;
    if(!gears_hand_continuity::same_input(rig.controllers,f.controllers))rig_mismatch|=32;
    if(rig_mismatch)return false;
    if(owner&&rig.units_per_metre!=binding_units)return false;
    sample_cost.rig_ns=weaponhand::clock_ns(nullptr);
    sample_gate=3;gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    observation={};
    if(!nativeequipment::read_input_continuity(f,raw,publisher,nullptr,nullptr,&observation.motion_history)||
       !observation.motion_history.count)return false;
    sample_cost.history_ns=weaponhand::clock_ns(nullptr);
    const auto forward=vr::rig_tracking::map_vector(rig.base_view,{0,0,-1});
    const auto right=vr::rig_tracking::map_vector(rig.base_view,{1,0,0});
    const auto up=vr::rig_tracking::map_vector(rig.base_view,{0,1,0});
    const vr::Matrix basis{{{forward.x,right.x,up.x},{forward.y,right.y,up.y},{forward.z,right.z,up.z}}};
    sample_gate=4;playground::Vector velocity{};
    if(!field(f.pawn,L"Velocity",L"StructProperty",velocity)||!inventory(f,observation.inventory))return false;
    sample_cost.velocity_inventory_ns=weaponhand::clock_ns(nullptr);
    unified_weapon_fire::Fingerprint epoch;epoch.word(raw.stream_epoch);epoch.word(raw.tracking_epoch[1]);epoch.word(f.reference_generation);
    observation.mapping={epoch.value,f.reference_generation,rig.reference,
        {vr::stereo_camera_position(rig.base_view),vr::ue_angles(basis)},
        {velocity.x,velocity.y,velocity.z},rig.units_per_metre,true};
    observation.hands=f.controllers;observation.stereo=f.stereo;
    observation.right_pocket_weapon=f.weapon.address;observation.equipment_hold_generation=vrequip::model.hold_generation();
    observation.original_equipped=true;observation.original_interaction_allowed=!game.busy;
    observation.original_owns_hands=game.animation_owns_hands;
    parent=f;
    if(!owner){
        sample_gate=5;
        if(!active_original(f)||!make_config(f,rig,config))return false;
        owner.emplace();if(!owner->bind(config,observation.inventory)){owner.reset();return false;}
        trigger.clear();held_generation=observation.equipment_hold_generation;last_input=0;binding_units=rig.units_per_metre;
    }
    sample_gate=6;
    if(!(config.identity==identity(f))||held_generation!=observation.equipment_hold_generation||
       !stamp(observation.stamp)||!nativeequipment::current(nullptr,f))return false;
    sample_cost.stamp_final_current_ns=weaponhand::clock_ns(nullptr);
    observation.stamp.motion_epoch=epoch.value;sampled_input=raw;sampled_publisher=publisher;return true;
}
bool receipt(model::BoloRequest const& request,model::BoloOriginalReceipt& out) noexcept {
    out={};out.request_token=request.token;out.original_preparation_serial=request.original_preparation_serial;
    if(event_serial==UINT64_MAX||!stamp(out.stamp)||!inventory(parent,out.inventory))return false;
    out.original_event_serial=++event_serial;out.fuse=owner->original_fuse();return true;
}
void report_trigger_refusal(model::BoloTriggerObservation status) noexcept {
    if(!log_file||trigger_refusal_reports>=8)return;
    const auto ticket=++trigger_refusal_reports;
    const auto& d=trigger_diagnostic;const auto& f=observation;const auto& h=f.hands.hands[1];
    fwprintf(log_file,L"frag_trigger_refusal ticket=%u status=%u reason=%u known=%u cohort=%u binding=%llu hold=%llu sample_started_ns=%lld clock_ns=%lld previous_sequence=%llu previous_sync_ns=%lld previous_stream=%llu previous_tracking=%llu previous_pressed=%u previous_released=%u previous_active=%u previous_level=%.9g current_sequence=%llu current_sync_ns=%lld current_pressed=%u current_released=%u current_active=%u current_level=%.9g history_count=%u\n",
        ticket,unsigned(status),d.reason,unsigned(d.known),unsigned(d.cohort),owner?owner->binding_token():0,
        held_generation,trigger_sample_started_ns,f.stamp.monotonic_ns,d.sequence,d.sync_ns,d.stream,d.tracking,d.pressed,d.released,d.active,
        double(d.level),f.hands.sequence,f.hands.sync_ns,h.pressed[vr::control_squeeze],h.released[vr::control_squeeze],
        h.squeeze_active,double(h.squeeze),f.motion_history.count);
    const auto count=(std::min)(std::size_t(f.motion_history.count),
                               std::size_t(gears_hand_continuity::motion_history_capacity));
    for(std::size_t i=0;i<count;++i){
        const auto& row=f.motion_history.frames[i];const auto& input=row.input;const auto& right=input.hands[1];
        fwprintf(log_file,L"frag_trigger_history ticket=%u index=%u sequence=%llu sync_ns=%lld received_ns=%lld stream=%llu tracking=%llu input_epoch=%llu space_epoch=%llu pressed=%u released=%u active=%u level=%.9g grip_active=%u grip_flags=%llu position=%.9g,%.9g,%.9g\n",
            ticket,unsigned(i),input.sequence,input.sync_ns,input.received_ns,row.stream_epoch,row.tracking_epoch[1],
            input.input_epoch,input.space_epoch,right.pressed[vr::control_squeeze],right.released[vr::control_squeeze],
            right.squeeze_active,double(right.squeeze),right.grip_active,right.grip_flags,
            right.grip.position.x,right.grip.position.y,right.grip.position.z);
    }
    fflush(log_file);
}
void forget() noexcept {
    if(fragpreparation::owed())return;
    owner.reset();trigger.clear();config={};parent={};observation={};held_generation=last_input=0;
    sampled_input={};sampled_publisher={};
    counts_known=false;faulted=finishing=false;binding_units=0;
}
bool cancel() noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(!owner)return fragpreparation::stop();
    // Recovery while still squeezing must not silently arm the old grasp.
    // Only a subsequent real press (or new equipment generation) may prepare.
    disarmed_hold=held_generation;
    disarmed_press=parent.controllers.hands[1].pressed[vr::control_squeeze];
    owner->cancel();
    if(!fragpreparation::stop())return false;
    const auto completed_ns=weaponhand::clock_ns(nullptr);
    const auto request=owner->pending_request();
    if(request.action==model::BoloAction::preserve_original_on_interrupt){
        const auto taken=owner->take_request();model::BoloOriginalReceipt r{};
        if(!taken||taken->token!=request.token||
           !request.original_preparation_serial||request.original_preparation_serial!=fragpreparation::journal.token||
           !receipt(request,r))return false;
        // A failed current tracking read clears observation before it can
        // supply a motion stamp. This is the already-owned, actually stopped
        // preparation's negative receipt, not a fresh motion/action permit.
        // Preserve its exact request epoch; never borrow an incomplete sample
        // or manufacture a new tracked pose to acknowledge cancellation.
        r.stamp.motion_epoch=request.motion_epoch;
        r.executed_ns=completed_ns;r.accepted=r.original_effect_applied_once=true;
        r.original_fuse_preserved=r.original_effects_preserved=r.native_safety_complete=true;
        if(owner->retire_stopped_after_external_debit(r)){
            // The original melee route spent its own item. Native stop above
            // proved the old targeting/spin obligations neutral; this grants
            // no VR launch and never acknowledges a debit as our own action.
            if(vrequip::model.release_throwable(parent.world.address,parent.pawn.address,
                                              parent.weapon.address,held_generation))
                weaponhand::held_binding={};
            report(14,model::BoloEvent::interrupted);return true;
        }
        const auto event=owner->acknowledge(r);
        if(event!=model::BoloEvent::safety_completed){faulted=true;report(12,event);return false;}
    }else if(request.token){faulted=true;return false;} // An ambiguous dispatched operation is never retried.
    return true;
}
void pre_original_tick(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||
       parent.controller.address!=pc||!fragpreparation::outstanding.load(std::memory_order_acquire)||
       !nativeequipment::stow_native::original_action_pending())return;
    // Negative authority only: cancel this old physical grasp and stop its
    // exact original preparation BEFORE a real original button can replace
    // inventory. cancel keeps anti-replay/disarmed-press and receipt checks.
    const bool clean=cancel();report(clean?60:61,model::BoloEvent::interrupted);
    if(clean)forget();
}
void unavailable(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr)||(!owner&&!fragpreparation::owed()))return;
    if(parent.controller.address&&pc!=parent.controller.address)return;
    // Only the existing original held preparation may wait through a soft
    // render/rig publication gap. The independent current input must still
    // prove the same press, held generation, tracking epoch and real actors.
    // Release, new Grip, source change or actual tracking loss still cancels;
    // no throw can execute without a new authentic current Frame/mapping.
    if(retain_preparation_during_pose_gap(pc))return;
    const bool clean=cancel();report(clean?10:11,model::BoloEvent::interrupted);
    // A missing render association is never an opportunity to throw from an
    // old pose. Cleanup may still settle the existing original obligation.
    if(clean)forget();
}
bool finish_original(hand::Frame const& f,fraglaunch::Result const& launch) noexcept {
    frag_original::Entry entry{};bool invoked{};
    if(!launch.original_once||finishing||!fragpreparation::stop()||
       !frag_original::bind(f.weapon.address,frag_original::Method::finished,entry))return false;
    // The authored physical release replaces the stock throw animation; call
    // its original completion once, after accounting for the spawn debit.
    // Pin the current producer only when actual spare ammunition can produce
    // another grenade. With no spare, original Active.BeginState owns the
    // normal empty-weapon auto-switch; retaining this old hold would block it.
    // A missing live pin grants no hold continuation, but never suppresses the
    // already owed original completion of an actually spawned projectile.
    Counts counts{};const bool known=fragpreparation::inventory(f,counts);
    const bool next_expected=known&&(counts.infinite||counts.reserve>0);
    returning={};ReturnTransition next{};next.source=f;next.lifetime=fraglaunch::journal.lifetime;
    next.hold=vrequip::model.hold_generation();const auto now=weaponhand::clock_ns(nullptr);
    vr::StereoSample stereo{};vr::ControllerInputFrame input{};
    const bool sim=f.stereo.source==vr::StereoSource::MetaOperatorSimulated;
    if(next_expected&&now>0&&now<=INT64_MAX-2000000000&&next.hold&&
       vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
       nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
           sim,stereo,input,next.input,next.publisher)==gears_current_tracking::Failure::none&&
       observation.motion_history.count&&
       next.input.stream_epoch==observation.motion_history.frames[observation.motion_history.count-1].stream_epoch&&
       next.input.tracking_epoch[1]==observation.motion_history.frames[observation.motion_history.count-1].tracking_epoch[1]&&
       gears_hand_continuity::detail::same_key(input,f.controllers)&&
       vr::controller_pose_usable(input.hands[1].grip,input.hands[1].grip_flags,input.hands[1].grip_active)){
        next.deadline=now+2000000000;next.active=true;
        next.release_pressed=f.controllers.hands[1].pressed[vr::control_squeeze];
        next.release_released=f.controllers.hands[1].released[vr::control_squeeze];
        returning=next;
    }
    finishing=true;const bool okay=frag_original::complete_existing(f,fraglaunch::journal.lifetime,entry,launch.projectile,&invoked);
    if(!okay||!invoked)returning={};
    if(log_file){fwprintf(log_file,L"frag_original_completion weapon=%llx counts_known=%u capacity=%d used=%d reserve=%d infinite=%u next_expected=%u continuation=%u invoked=%u completed=%u\n",
        f.weapon.address,unsigned(known),counts.capacity,counts.used,counts.reserve,unsigned(counts.infinite),
        unsigned(next_expected),unsigned(returning.active),unsigned(invoked),unsigned(okay&&invoked));fflush(log_file);}
    if(invoked)finishing=false;return okay&&invoked;
}
void dispatch(hand::Frame const& f) noexcept {
    if(!owner||faulted)return;
    const auto request=owner->pending_request();if(!request.token)return;
    model::BoloOriginalReceipt r{};model::BoloEvent event{};
    if(request.action==model::BoloAction::prepare_original){
        const auto actual=fragpreparation::begin(f,*owner);if(!actual.token_taken)return;
        if(!receipt(request,r)){faulted=true;cancel();report(21);return;}
        r.executed_ns=actual.executed_ns;r.accepted=actual.prepared;
        r.original_preparation_serial=actual.preparation_serial;
        r.original_effect_applied_once=actual.prepared;
        r.original_fuse_preserved=r.original_effects_preserved=actual.prepared;
        if(!actual.prepared&&!actual.cleanup_complete&&actual.dispatched){faulted=true;cancel();report(22);return;}
        event=owner->acknowledge(r);
    }else if(request.action==model::BoloAction::launch_original){
        if(!active_original(f)){cancel();return;}
        const auto begin_ns=weaponhand::clock_ns(nullptr);
        const auto actual=fraglaunch::invoke_once(f,*owner);
        if(!actual.token_taken){
            if(log_file&&launch_reject_reports++<8){
                fwprintf(log_file,L"frag_original_launch_preflight stage=%u token=%llu frame=%llu sequence=%llu begin_ns=%lld finish_ns=%lld input_sync_ns=%lld authored_ns=%lld valid_until_ns=%lld\n",
                    actual.preflight_stage,request.token,f.key.frame,f.controllers.sequence,begin_ns,weaponhand::clock_ns(nullptr),
                    request.original_input_sync_ns,request.authored_ns,request.valid_until_ns);fflush(log_file);
            }
            return;
        }
        if(!receipt(request,r)){faulted=true;cancel();report(31);return;}
        if(log_file&&launch_reports++<24){
            const auto& p=actual.actual_position;const auto& v=actual.actual_velocity;
            fwprintf(log_file,L"frag_original_launch token=%llu weapon=%llx projectile=%llx projectile_class=%llx once=%u dispatched=%u returned=%u before=%u after=%u throws=%u consumes=%u notifications=%u inits=%u origin_returns=%u velocity_returns=%u initial_matches=%u direction_restored=%u position=%.9g,%.9g,%.9g velocity=%.9g,%.9g,%.9g planned_position=%.9g,%.9g,%.9g planned_velocity=%.9g,%.9g,%.9g executed_ns=%lld\n",
                request.token,f.weapon.address,actual.projectile.address,actual.projectile.klass,
                unsigned(actual.original_once),unsigned(actual.dispatched),unsigned(actual.returned),
                request.before.count,r.inventory.count,actual.throws,actual.consumes,actual.notifications,actual.inits,
                actual.origins,actual.velocities,unsigned(actual.initial_state_matches),unsigned(actual.direction_restored),
                double(p.x),double(p.y),double(p.z),double(v.x),double(v.y),double(v.z),
                request.launch.position.x,request.launch.position.y,request.launch.position.z,
                request.launch.velocity.x,request.launch.velocity.y,request.launch.velocity.z,actual.executed_ns);fflush(log_file);
        }
        r.executed_ns=actual.executed_ns;r.accepted=actual.original_once;
        r.original_projectile=actual.projectile.address;r.original_projectile_type=actual.projectile.klass;
        r.original_projectiles_spawned=actual.projectile.address?1u:0u;
        r.original_effect_applied_once=actual.original_once;
        r.original_fuse_preserved=r.original_effects_preserved=r.tracked_initial_pose_and_velocity_applied=actual.original_once;
        // A failed original dispatch that generated effects is uncertain,
        // never a rejected-no-effect receipt or a second attempt.
        if(!actual.original_once&&(actual.dispatched||actual.projectile.address)){
            faulted=true;fragpreparation::stop();report(32,model::BoloEvent::safety_violation);return;
        }
        event=owner->acknowledge(r);
        if(event==model::BoloEvent::thrown){
            if(finish_original(f,actual)){report(33,event);forget();return;}
            faulted=true;report(34,event);return;
        }
    }else if(request.action==model::BoloAction::preserve_original_on_interrupt){cancel();return;}
    else return;
    report(20,event);
    if(event==model::BoloEvent::safety_violation||event==model::BoloEvent::rejected){faulted=true;cancel();}
}
// Pure old-frame validation for the negative stale-pose wait. This mirrors the
// production tracked_cohort() predicate except for the 60ms positive age bound;
// the native 100ms upper bound is still enforced by the caller.
bool stale_pose_old_frame_valid(model::BoloFrame const& old,model::BoloInstanceConfig const& c,std::uint64_t previous_epoch) noexcept {
    if(!previous_epoch||!old.stamp.motion_epoch||old.stamp.motion_epoch!=previous_epoch)return false;
    if(old.mapping.motion_epoch!=old.stamp.motion_epoch||!old.mapping.reference_generation)return false;
    if(!old.stamp.identity.valid()||!old.stamp.binding)return false;
    if(!old.inventory.valid())return false;
    const auto& h=old.hands;
    if(h.session!=c.identity.xr_session||h.space_epoch!=c.identity.space_epoch||
       h.input_epoch!=c.identity.input_epoch||h.profile_generation!=c.identity.profile_generation)return false;
    if(h.source!=c.input_source||old.stereo.source!=c.input_source)return false;
    const bool sim=c.input_source==vr::StereoSource::MetaOperatorSimulated;
    if(!vr::controller_frame_matches(h,old.stereo,old.stamp.monotonic_ns,sim))return false;
    return vr::controller_pose_usable(h.hands[1].grip,h.hands[1].grip_flags,h.hands[1].grip_active);
}
// Negative-only classification for an already-authorized preparation whose
// copied old motion sample is older than the positive model's 60ms tracking
// bound but still within the existing 100ms producer/lifetime ceiling. It
// grants no authority; unavailable() still calls the unchanged live retention
// function exactly once and otherwise performs the ordinary original cancel.
bool stale_pose_wait_eligible(hand::Frame const& f,hand::GripGameState const& game,
                              model::BoloFrame const& old,std::uint64_t previous_epoch) noexcept {
    if(!owner||owner->stage()!=model::BoloStage::prepared||owner->pending_request().token)return false;
    if(faulted||finishing||returning.active||fragpreparation::stop_requested.load())return false;
    if(!stale_pose_old_frame_valid(old,config,previous_epoch))return false;
    if(!old.original_equipped||!old.original_interaction_allowed||old.original_owns_hands)return false;
    if(!game.known||!game.main_hand_held||game.kind!=hand::WeaponKind::hand_thrown||game.busy||game.animation_owns_hands)return false;
    if(!(config.identity==identity(f))||old.stamp.binding!=owner->binding_token()||
       held_generation!=old.equipment_hold_generation||vrequip::model.hold_generation()!=held_generation)return false;
    if(old.inventory.count!=owner->inventory().count||old.inventory.revision!=owner->inventory().revision||
       old.right_pocket_weapon!=config.identity.weapon)return false;
    const auto age=old.stamp.monotonic_ns-old.hands.sync_ns;
    if(!(old.hands.sync_ns>0&&age>config.motion.maximum_sample_gap_ns&&age<=100000000))return false;
    const auto& right=old.hands.hands[1];
    return right.squeeze_active==1&&std::isfinite(right.squeeze)&&
        right.squeeze>vr::controller_analog_release_threshold;
}
struct SoftPoseSnapshot {
    std::uint64_t frame{},sequence{},binding{};
    std::int64_t stamp_ns{},sync_ns{},age_ns{};
    SampleCostMarks cost{};
    std::int64_t before_ns{};
};
SoftPoseSnapshot snapshot_soft_pose(model::BoloFrame const& old) noexcept {
    SoftPoseSnapshot out{};
    out.frame=parent.key.frame;out.sequence=old.hands.sequence;out.binding=old.stamp.binding;
    out.stamp_ns=old.stamp.monotonic_ns;out.sync_ns=old.hands.sync_ns;out.age_ns=out.stamp_ns-out.sync_ns;
    out.cost=sample_cost;return out;
}
void report_soft_pose_wait(SoftPoseSnapshot const& s,bool retained) noexcept {
    if(!log_file)return;
    static unsigned soft_pose_wait_reports{};
    if(soft_pose_wait_reports++>=8)return;
    const auto after_ns=weaponhand::clock_ns(nullptr);
    fwprintf(log_file,L"frag_soft_pose_wait ticket=%u frame=%llu sequence=%llu binding=%llu stamp_ns=%lld sync_ns=%lld sample_age_ns=%lld sample_begin_ns=%lld sample_current_held_ns=%lld sample_rig_ns=%lld sample_history_ns=%lld sample_velocity_inventory_ns=%lld sample_stamp_final_current_ns=%lld before_ns=%lld after_ns=%lld retained=%u\n",
        soft_pose_wait_reports,s.frame,s.sequence,s.binding,s.stamp_ns,s.sync_ns,s.age_ns,
        s.cost.begin_ns,s.cost.current_held_ns,s.cost.rig_ns,s.cost.history_ns,
        s.cost.velocity_inventory_ns,s.cost.stamp_final_current_ns,s.before_ns,after_ns,unsigned(retained));
    fflush(log_file);
}
void tick(hand::Frame const& f,hand::GripGameState const& game) noexcept try {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready())return;
    if(returning.active)return;
    if(fragpreparation::stop_requested.load(std::memory_order_acquire)){unavailable(f.controller.address);return;}
    const bool held=game.known&&game.kind==hand::WeaponKind::hand_thrown&&game.main_hand_held&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address);
    if(!held){if(owner||fragpreparation::owed())unavailable(parent.controller.address);return;}
    if(owner&&(!same_parent(f,parent)||!(config.identity==identity(f))||held_generation!=vrequip::model.hold_generation())){
        if(faulted){
            // A new confirmed equipment incarnation may observe actual stock
            // counts afresh. Retire only the local failed mirror; never retry
            // its old token, invent a receipt, or clear a native cleanup debt.
            if(!fragpreparation::stop()||fraglaunch::outstanding.load(std::memory_order_acquire))return;
            report(13,model::BoloEvent::interrupted);forget();
        }else{if(!cancel())return;forget();}
    }
    if(owner&&owner->stage()==model::BoloStage::interrupted&&!owner->pending_request().token&&!fragpreparation::owed())forget();
    if(faulted){fragpreparation::stop();return;}
    if(!owner){
        // Rejection hint only: a known Smoke class must not pay for Frag's
        // rig/history/inventory sampling before failing its full asset scope.
        // Never skip an outstanding preparation, or grant Frag admission here.
        if(!fragpreparation::owed()&&playground::named(f.weapon.klass,L"Weap_SmokeGrenade"))return;
        const auto& right=f.controllers.hands[1];const auto hold=vrequip::model.hold_generation();
        if(right.squeeze_active!=1||right.squeeze<=vr::controller_analog_release_threshold){
            // hand_thrown also includes Smoke. Before any model mutation,
            // authenticate this current Frag class/mesh/package specifically.
            // Unknown/other throwable observations retain their own owner.
            lancermain_asset::Evidence frag{};
            if(lancermain_asset::read(f,frag,lancermain_asset::AssetScope::frag_hold)!=lancermain_asset::Status::ready||
               !nativeequipment::current(nullptr,f))return;
            // Released during original equip: no preparation/motion was owned,
            // so discard only the exact equipment hold, never queue a throw.
            if(vrequip::model.release_throwable(f.world.address,f.pawn.address,f.weapon.address,hold))
                weaponhand::held_binding={};
            return;
        }
        if(hold==disarmed_hold&&right.pressed[vr::control_squeeze]<=disarmed_press)return;
    }
    const auto previous_motion_epoch=observation.stamp.motion_epoch;
    trigger_sample_started_ns=weaponhand::clock_ns(nullptr);
    if(!sample(f,game)){report(100+sample_gate);if(owner)unavailable(f.controller.address);return;}
    if(stale_pose_wait_eligible(f,game,observation,previous_motion_epoch)){
        auto snapshot=snapshot_soft_pose(observation);snapshot.before_ns=weaponhand::clock_ns(nullptr);
        unavailable(f.controller.address);
        const bool retained=owner&&owner->stage()==model::BoloStage::prepared&&fragpreparation::owed();
        report_soft_pose_wait(snapshot,retained);
        return;
    }
    frag_authored_main::publish(f); // Same certified current Frag sample; cosmetic only.
    if(observation.hands.sequence==last_input)return;
    if(fragpreparation::owed()&&f.controllers.hands[1].pressed[vr::control_squeeze]!=
        fragpreparation::journal.source.controllers.hands[1].pressed[vr::control_squeeze]){
        cancel();forget();return; // Yield before equipment consumes that real new Grip.
    }
    model::BoloInput input{};const auto seen=trigger.observe(observation,input,config.release_input,&trigger_diagnostic);
    last_input=observation.hands.sequence;
    if(seen==model::BoloTriggerObservation::invalid||seen==model::BoloTriggerObservation::ambiguous||
       seen==model::BoloTriggerObservation::exhausted){report_trigger_refusal(seen);cancel();report(3);return;}
    model::BoloDecision decision{};
    if(owner->stage()==model::BoloStage::idle){
        if(!observation.inventory.count&&seen==model::BoloTriggerObservation::edge&&input.kind==model::BoloInputKind::right_grip_press){
            frag_original::Entry empty{};
            if(frag_original::bind(f.weapon.address,frag_original::Method::empty,empty))frag_original::call(f,empty);
            report(5,model::BoloEvent::no_ammo);return;
        }
        decision=owner->adopt_equipment_hold(observation);
        if(owner->stage()==model::BoloStage::interrupted&&log_file&&adoption_refusal_reports<8){
            const auto& o=observation;const auto& h=o.hands;const auto& c=config;
            const bool history_valid=o.motion_history.count&&o.motion_history.count<=gears_hand_continuity::motion_history_capacity&&
                gears_hand_continuity::same_input(o.motion_history.frames[o.motion_history.count-1].input,h)&&
                gears_hand_continuity::motion_history_valid(o.motion_history,o.motion_history.frames[o.motion_history.count-1],1,c.motion.maximum_sample_gap_ns);
            fwprintf(log_file,L"frag_adoption_refusal ticket=%u event=%u allowed=%u original_owns_hands=%u equipped=%u count=%u count_revision=%llu hold=%llu stamp_ns=%lld input_sync_ns=%lld sample_age_ns=%lld sample_limit_ns=%lld controller_match=%u history_valid=%u history_count=%u session_match=%u space_match=%u input_epoch_match=%u profile_match=%u source_match=%u pose_usable=%u mapping_epoch_match=%u binding=%llu sequence=%llu\n",
                ++adoption_refusal_reports,unsigned(decision.event),unsigned(o.original_interaction_allowed),unsigned(o.original_owns_hands),unsigned(o.original_equipped),o.inventory.count,o.inventory.revision,o.equipment_hold_generation,
                o.stamp.monotonic_ns,h.sync_ns,o.stamp.monotonic_ns-h.sync_ns,c.motion.maximum_sample_gap_ns,
                unsigned(vr::controller_frame_matches(h,o.stereo,o.stamp.monotonic_ns,c.input_source==vr::StereoSource::MetaOperatorSimulated)),unsigned(history_valid),o.motion_history.count,
                unsigned(h.session==c.identity.xr_session),unsigned(h.space_epoch==c.identity.space_epoch),unsigned(h.input_epoch==c.identity.input_epoch),unsigned(h.profile_generation==c.identity.profile_generation),
                unsigned(h.source==c.input_source&&o.stereo.source==c.input_source),unsigned(vr::controller_pose_usable(h.hands[1].grip,h.hands[1].grip_flags,h.hands[1].grip_active)),unsigned(o.mapping.motion_epoch==o.stamp.motion_epoch),o.stamp.binding,h.sequence);
            fflush(log_file);
        }
    }
    else decision=owner->update(observation,seen==model::BoloTriggerObservation::edge?input:model::BoloInput{});
    if(decision.event==model::BoloEvent::safety_required||decision.event==model::BoloEvent::interrupted){
        static unsigned update_refusal_reports{};
        if(log_file&&update_refusal_reports++<8){
            fwprintf(log_file,L"frag_update_safety reason=%u boundary_flags=%u event=%u stage=%u frame=%llu sequence=%llu stamp_ns=%lld sync_ns=%lld sample_age_ns=%lld history_count=%u input_kind=%u input_sequence=%llu input_sync_ns=%lld squeeze=%.9g squeeze_active=%u pressed=%u released=%u mapping_epoch=%llu frame_now_ns=%lld sample_begin_ns=%lld sample_current_held_ns=%lld sample_rig_ns=%lld sample_history_ns=%lld sample_velocity_inventory_ns=%lld sample_stamp_final_current_ns=%lld sample_total_ns=%lld\n",
                owner->last_update_refusal(),owner->last_boundary_flags(),unsigned(decision.event),unsigned(owner->stage()),f.key.frame,
                observation.hands.sequence,observation.stamp.monotonic_ns,observation.hands.sync_ns,
                observation.stamp.monotonic_ns-observation.hands.sync_ns,observation.motion_history.count,
                unsigned(input.kind),input.hand_sequence,input.sync_ns,double(observation.hands.hands[1].squeeze),
                observation.hands.hands[1].squeeze_active,observation.hands.hands[1].pressed[vr::control_squeeze],
                observation.hands.hands[1].released[vr::control_squeeze],observation.mapping.motion_epoch,
                sample_cost.frame_now_ns,sample_cost.begin_ns,sample_cost.current_held_ns,sample_cost.rig_ns,
                sample_cost.history_ns,sample_cost.velocity_inventory_ns,sample_cost.stamp_final_current_ns,
                sample_cost.stamp_final_current_ns-sample_cost.begin_ns);fflush(log_file);
            const auto& throw_read=owner->last_throw_observation();
            fwprintf(log_file,L"frag_throw_solution_observation failure=%u flags=%u samples=%u span_ns=%lld cleared=%u clear_flags=%u gap_ns=%lld sync_gap_ns=%lld step_m=%.9g speed_mps=%.9g previous_sequence=%llu target_sequence=%llu current_sequence=%llu\n",
                throw_read.failure,throw_read.flags,throw_read.sample_count,throw_read.span_ns,
                throw_read.cleared_samples,throw_read.clear_flags,throw_read.last_gap_ns,throw_read.last_sync_gap_ns,
                throw_read.last_step_m,throw_read.computed_speed_mps,throw_read.previous_sequence,
                throw_read.target_sequence,throw_read.current_sequence);fflush(log_file);
            const auto& rotation=owner->last_rotation_observation();
            fwprintf(log_file,L"frag_rotation_observation valid=%u history=%u current=%u count=%llu gap_ns=%llu old=%.9g,%.9g,%.9g new=%.9g,%.9g,%.9g yaw_delta=%.9g pitch_changed=%u roll_changed=%u yaw_changed=%u stick_intent=%u current_stick_intent=%u continuous=%u episode_before=%u episode_after=%u episode_continuation=%u episode_reset=%u\n",
                unsigned(rotation.valid),unsigned(rotation.history_supplied),unsigned(rotation.history_current),
                rotation.history_count,rotation.consumer_gap_ns,rotation.old_pitch,rotation.old_yaw,rotation.old_roll,
                rotation.new_pitch,rotation.new_yaw,rotation.new_roll,rotation.yaw_delta,
                unsigned(rotation.pitch_changed),unsigned(rotation.roll_changed),unsigned(rotation.yaw_changed),
                unsigned(rotation.stick_intent),unsigned(rotation.current_stick_intent),unsigned(rotation.continuous),
                unsigned(rotation.episode_before),unsigned(rotation.episode_after),unsigned(rotation.episode_continuation),
                unsigned(rotation.episode_reset));
            fwprintf(log_file,L"frag_rotation_pitch pitch_delta=%.9g rotation_magnitude=%.9g locomotion_intent=%u pitch_continuous=%u\n",
                rotation.pitch_delta,rotation.rotation_magnitude,unsigned(rotation.locomotion_intent),unsigned(rotation.pitch_continuous));fflush(log_file);
        }
        cancel();report(4,decision.event);return;
    }
    // Snapshot passive continuity evidence before dispatch can retire the owner.
    // Emit only after dispatch, and only for an observation with no request.
    const auto pitch_snapshot=owner->last_rotation_observation();
    const auto pitch_frame=f.key.frame,pitch_sequence=observation.hands.sequence;
    const bool report_pitch=log_file&&decision.event==model::BoloEvent::observed&&
        !owner->pending_request().token&&pitch_snapshot.pitch_continuous&&pitch_reports<8;
    const auto& mapping_request=owner->pending_request();
    const bool report_mapping=log_file&&mapping_request.action==model::BoloAction::launch_original&&mapping_reports<24;
    std::uint64_t mapping_token{},mapping_current_sequence{},mapping_release_sequence{};
    std::int64_t mapping_input_sync_ns{};
    vr::Vector mapping_base_position{},mapping_base_velocity{},mapping_hand_velocity{};
    vr::Rotator mapping_base_rotation{};
    double mapping_units{};
    if(report_mapping){
        // Snapshot before the one original dispatch. Logging must not reread
        // live owner/request values that dispatch may commit or reset.
        const auto& m=observation.mapping;const auto& v=m.pawn_velocity_world_units_per_second;
        const auto& t=mapping_request.tracked;const auto& b=m.game_base;
        mapping_token=mapping_request.token;mapping_current_sequence=observation.hands.sequence;
        mapping_release_sequence=input.hand_sequence;mapping_input_sync_ns=mapping_request.original_input_sync_ns;
        mapping_base_position=b.position;mapping_base_rotation=b.rotation;mapping_base_velocity=v;
        mapping_hand_velocity=t.velocity_mps;mapping_units=m.units_per_metre;
    }
    dispatch(f);report(1,decision.event);
    if(report_pitch&&pitch_reports++<8){
        fwprintf(log_file,L"frag_pitch_continuity frame=%llu sequence=%llu history=%u current=%u gap_ns=%llu pitch_delta=%.9g yaw_delta=%.9g rotation_magnitude=%.9g locomotion_intent=%u turn_intent=%u episode_before=%u episode_after=%u\n",
            pitch_frame,pitch_sequence,unsigned(pitch_snapshot.history_supplied),unsigned(pitch_snapshot.history_current),
            pitch_snapshot.consumer_gap_ns,pitch_snapshot.pitch_delta,pitch_snapshot.yaw_delta,pitch_snapshot.rotation_magnitude,
            unsigned(pitch_snapshot.locomotion_intent),unsigned(pitch_snapshot.stick_intent),
            unsigned(pitch_snapshot.episode_before),unsigned(pitch_snapshot.episode_after));fflush(log_file);
    }
    if(report_mapping&&mapping_reports++<24){
        fwprintf(log_file,L"frag_release_mapping token=%llu current_sequence=%llu release_sequence=%llu input_sync_ns=%lld base_position=%.12g,%.12g,%.12g base_rotation=%.12g,%.12g,%.12g base_velocity=%.12g,%.12g,%.12g hand_velocity=%.12g,%.12g,%.12g units=%.12g\n",
            mapping_token,mapping_current_sequence,mapping_release_sequence,mapping_input_sync_ns,
            mapping_base_position.x,mapping_base_position.y,mapping_base_position.z,
            mapping_base_rotation.pitch,mapping_base_rotation.yaw,mapping_base_rotation.roll,
            mapping_base_velocity.x,mapping_base_velocity.y,mapping_base_velocity.z,
            mapping_hand_velocity.x,mapping_hand_velocity.y,mapping_hand_velocity.z,mapping_units);fflush(log_file);
    }
}catch(...){faulted=true;if(weaponhand::current_gt(nullptr))cancel();}
void reset_after_drain() noexcept {
    if(!fragpreparation::owed()&&!fraglaunch::outstanding.load(std::memory_order_acquire)){
        forget();returning={};reports=0;launch_reports=0;mapping_reports=0;pitch_reports=0;launch_reject_reports=0;previous_gate=~0u;
        disarmed_hold=0;disarmed_press=0;adoption_refusal_reports=0;
    }
}
} // namespace fragruntime
