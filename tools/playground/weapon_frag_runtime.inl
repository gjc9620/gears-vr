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
double binding_units{};
unsigned sample_gate{};
unsigned rig_mismatch{};
model::BoloFrame observation{}; // Caller-owned motion workspace, not another publication.
std::uint64_t reports{};unsigned previous_gate{~0u};
unsigned launch_reports{};
struct ReturnTransition {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};
    gears_hand_continuity::TrackedInput input{};gears_hand_continuity::Publisher publisher{};
    std::uint64_t hold{};std::int64_t deadline{};bool active{};
};
ReturnTransition returning{};
gears_hand_continuity::TrackedInput sampled_input{};
gears_hand_continuity::Publisher sampled_publisher{};
void report(unsigned gate,model::BoloEvent event=model::BoloEvent::observed) noexcept {
    if(!log_file||(gate==previous_gate&&event==model::BoloEvent::observed)||reports>=96)return;
    fwprintf(log_file,L"frag_original_runtime gate=%u event=%u stage=%u binding=%llu input=%llu count_revision=%llu fault=%u preparation_owed=%u rig_mismatch=%u\n",
        gate,unsigned(event),owner?unsigned(owner->stage()):0,owner?owner->binding_token():0,
        last_input,revision,unsigned(faulted),unsigned(fragpreparation::owed()),rig_mismatch);
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
bool active_original(hand::Frame const& f) noexcept {
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
        meta::exact_text(text,L"Active")&&nativeequipment::current(nullptr,f);
}
bool retain_preparation_during_pose_gap(uptr pc) noexcept {
    if(!owner||owner->stage()!=model::BoloStage::prepared||owner->pending_request().token||returning.active||
       !fragpreparation::owed()||!fragpreparation::journal.prepared||fragpreparation::stop_requested.load()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||pc!=parent.controller.address||
       held_generation!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(parent.world.address,parent.pawn.address,parent.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(fragpreparation::journal.lifetime))return false;
    for(const auto& id:{parent.world,parent.controller,parent.pawn,parent.mesh,parent.weapon})if(!weapon_part_assets::object(id))return false;
    if(playground::pointer(parent.controller.address,L"Pawn")!=parent.pawn.address||
       playground::pointer(parent.pawn.address,L"Weapon")!=parent.weapon.address||
       playground::pointer(parent.pawn.address,L"WorldInfo")!=parent.world.address||
       playground::pointer(parent.weapon.address,L"Instigator")!=parent.pawn.address)return false;
    fragpreparation::State prepared_state{};Counts counts{};
    if(!fragpreparation::state(parent,prepared_state)||!prepared_state.key||!prepared_state.controller_target||!prepared_state.pawn_target||!prepared_state.spinning||
       !fragpreparation::inventory(parent,counts)||!(counts==fragpreparation::journal.before))return false;
    vr::StereoSample stereo{};vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    const auto begin=weaponhand::clock_ns(nullptr);const bool sim=parent.stereo.source==vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,raw,publisher)!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);const auto& initial=fragpreparation::journal.source.controllers.hands[1];
    const auto& hand=raw.input.hands[1];
    return begin>0&&end>=begin&&end-begin<=50000000&&publisher.pid&&publisher.start&&
        publisher.pid==sampled_publisher.pid&&publisher.start==sampled_publisher.start&&
        raw.stream_epoch==sampled_input.stream_epoch&&raw.tracking_epoch[1]==sampled_input.tracking_epoch[1]&&
        gears_hand_continuity::detail::same_key(raw.input,sampled_input.input)&&
        raw.input.sequence>=sampled_input.input.sequence&&raw.input.sync_ns>=sampled_input.input.sync_ns&&
        vr::controller_frame_matches(raw.input,stereo,end,sim)&&end>=raw.input.sync_ns&&end-raw.input.sync_ns<=100000000&&
        vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)&&hand.trigger_active==1&&
        hand.trigger>vr::controller_analog_release_threshold&&
        hand.pressed[vr::control_trigger]==initial.pressed[vr::control_trigger]&&
        hand.released[vr::control_trigger]==initial.released[vr::control_trigger]&&
        hand.pressed[vr::control_squeeze]==initial.pressed[vr::control_squeeze]&&
        hand.released[vr::control_squeeze]==initial.released[vr::control_squeeze]&&
        weaponhand::client.existing_action_lifetime_on_gt(fragpreparation::journal.lifetime);
}
// Preserve only an already confirmed equipment hold while the ORIGINAL
// successful throw re-equips its next grenade. No Frame, pose, input edge,
// attachment or new hold is fabricated. Other source/identity loss still
// follows the normal equipment revocation path.
bool retain_return_during_pose_gap(uptr pc) noexcept {
    if(!returning.active||!weaponhand::current_gt(nullptr)||fragpreparation::stop_requested.load()||
       !nativeequipment::restoration_quiet()||pc!=returning.source.controller.address)return false;
    const auto& f=returning.source;const auto now=weaponhand::clock_ns(nullptr);
    if(now<=0||now>returning.deadline||!returning.hold||returning.hold!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(returning.lifetime))return false;
    for(const auto& id:{f.world,f.controller,f.pawn,f.mesh,f.weapon})if(!weapon_part_assets::object(id))return false;
    const auto inv=playground::pointer(f.pawn.address,L"InvManager");uptr item{};
    if(playground::pointer(f.controller.address,L"Pawn")!=f.pawn.address||
       playground::pointer(f.pawn.address,L"Weapon")!=f.weapon.address||
       playground::pointer(f.pawn.address,L"WorldInfo")!=f.world.address||
       playground::pointer(f.pawn.address,L"Mesh")!=f.mesh.address||
       playground::pointer(f.weapon.address,L"Instigator")!=f.pawn.address||
       !vrequip::read_slot(inv,1,item)||item!=f.weapon.address)return false;
    int health{};unsigned char mode{},special{};
    if(!field(f.pawn,L"Health",L"IntProperty",health)||health<=0||
       !field(f.pawn,L"SpecialMove",L"ByteProperty",special)||special||
       !field(f.weapon,L"CurrentFireMode",L"ByteProperty",mode)||(mode!=0&&mode!=5))return false;
    vr::StereoSample stereo{};vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    const bool sim=f.stereo.source==vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,raw,publisher)!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);
    if(end<now||end-now>50000000||end>returning.deadline||!publisher.pid||!publisher.start||
       publisher.pid!=returning.publisher.pid||publisher.start!=returning.publisher.start||
       raw.stream_epoch!=returning.input.stream_epoch||raw.tracking_epoch[1]!=returning.input.tracking_epoch[1]||
       !gears_hand_continuity::detail::same_key(raw.input,returning.input.input)||
       raw.input.sequence<returning.input.input.sequence||raw.input.sync_ns<returning.input.input.sync_ns||
       !vr::controller_frame_matches(raw.input,stereo,end,sim)||end<raw.input.sync_ns||end-raw.input.sync_ns>100000000||
       !vr::controller_pose_usable(raw.input.hands[1].grip,raw.input.hands[1].grip_flags,raw.input.hands[1].grip_active)||
       !weaponhand::client.existing_action_lifetime_on_gt(returning.lifetime))return false;
    returning.input=raw;return true;
}
bool pre_equipment(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    if(!returning.active)return false;
    if(!retain_return_during_pose_gap(f.controller.address)){returning={};report(36);return false;}
    lancermain_asset::Evidence asset{};Counts loaded{};
    if(game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&
       fragpreparation::inventory(f,loaded)&&loaded.capacity>loaded.used&&
       active_original(f)&&lancermain_asset::read(f,asset,lancermain_asset::AssetScope::frag_hold)==lancermain_asset::Status::ready&&
       nativeequipment::attached_frame(f,asset.component.address)&&nativeequipment::current(nullptr,f)){
        if(log_file){fwprintf(log_file,L"frag_original_return weapon=%llx hold=%llu frame=%llu attached=1 active=1 actual_input_sequence=%llu\n",
            f.weapon.address,returning.hold,f.key.frame,returning.input.input.sequence);fflush(log_file);}
        returning={};report(35);return false;
    }
    return true;
}
bool projectile_type(hand::Frame const& f,hand::ObjectIdentity& out) noexcept {
    return frag_original::projectile_class(f,out);
}
bool make_config(hand::Frame const& f,gears_rig_tracking::Frame const& rig,model::BoloInstanceConfig& out) noexcept {
    lancermain_asset::Evidence asset{};vrequip::MeshPoseProbe mesh{};
    hand::ObjectIdentity projectile{};frag_original::Entry prepare{},stop{},notify{},empty{},finished{},origin{},velocity{},base_throw{},mode{};
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
       !frag_original::bind(f.weapon.address,frag_original::Method::mode,mode))return false;
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
    c.motion.gravity_mps2=g;c.original_projectile_type=projectile.address;
    c.prepare_entry=prepare.function.address;c.launch_entry=notify.function.address;c.safety_entry=stop.function.address;
    c.original_no_ammo_cue=empty.function.address;
    unified_weapon_fire::Fingerprint fingerprint;
    for(const auto* e:{&prepare,&stop,&notify,&empty,&finished,&origin,&velocity,&base_throw,&mode})fingerprint.word(e->fingerprint);
    fingerprint.word(projectile.address);fingerprint.word(asset.asset.address);fingerprint.word(rig.reference_generation);
    c.config_serial=fingerprint.value;c.grip_geometry_serial=asset.asset.address;
    c.release_rule_serial=notify.fingerprint;c.fuse_rule_serial=projectile.address;c.audio_rule_serial=empty.fingerprint;
    c.release_input=model::BoloReleaseInput::right_trigger_release_after_press;c.fuse_owner=model::BoloFuseOwner::original_projectile;
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
    sample_gate=1;
    if(!nativeequipment::current(nullptr,f)||!game.known||!game.main_hand_held||
       game.kind!=hand::WeaponKind::hand_thrown||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       vrequip::model.held_slot()!=*model::original_inventory_slot(model::BodySlot::grenade))return false;
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
    sample_gate=3;gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    observation={};
    if(!nativeequipment::read_input_continuity(f,raw,publisher,nullptr,nullptr,&observation.motion_history)||
       !observation.motion_history.count)return false;
    const auto forward=vr::rig_tracking::map_vector(rig.base_view,{0,0,-1});
    const auto right=vr::rig_tracking::map_vector(rig.base_view,{1,0,0});
    const auto up=vr::rig_tracking::map_vector(rig.base_view,{0,1,0});
    const vr::Matrix basis{{{forward.x,right.x,up.x},{forward.y,right.y,up.y},{forward.z,right.z,up.z}}};
    sample_gate=4;playground::Vector velocity{};
    if(!field(f.pawn,L"Velocity",L"StructProperty",velocity)||!inventory(f,observation.inventory))return false;
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
    observation.stamp.motion_epoch=epoch.value;sampled_input=raw;sampled_publisher=publisher;return true;
}
bool receipt(model::BoloRequest const& request,model::BoloOriginalReceipt& out) noexcept {
    out={};out.request_token=request.token;out.original_preparation_serial=request.original_preparation_serial;
    if(event_serial==UINT64_MAX||!stamp(out.stamp)||!inventory(parent,out.inventory))return false;
    out.original_event_serial=++event_serial;out.fuse=owner->original_fuse();return true;
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
        const auto event=owner->acknowledge(r);
        if(event!=model::BoloEvent::safety_completed){faulted=true;report(12,event);return false;}
    }else if(request.token){faulted=true;return false;} // An ambiguous dispatched operation is never retried.
    return true;
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
        next.deadline=now+2000000000;next.active=true;returning=next;
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
        const auto actual=fraglaunch::invoke_once(f,*owner);if(!actual.token_taken)return;
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
    if(!sample(f,game)){report(100+sample_gate);if(owner)unavailable(f.controller.address);return;}
    if(observation.hands.sequence==last_input)return;
    if(fragpreparation::owed()&&f.controllers.hands[1].pressed[vr::control_squeeze]!=
        fragpreparation::journal.source.controllers.hands[1].pressed[vr::control_squeeze]){
        cancel();forget();return; // Yield before equipment consumes that real new Grip.
    }
    model::BoloInput input{};const auto seen=trigger.observe(observation,input);
    last_input=observation.hands.sequence;
    if(seen==model::BoloTriggerObservation::invalid||seen==model::BoloTriggerObservation::ambiguous||
       seen==model::BoloTriggerObservation::exhausted){cancel();report(3);return;}
    model::BoloDecision decision{};
    if(owner->stage()==model::BoloStage::idle){
        if(!observation.inventory.count&&seen==model::BoloTriggerObservation::edge&&input.kind==model::BoloInputKind::right_trigger_press){
            frag_original::Entry empty{};
            if(frag_original::bind(f.weapon.address,frag_original::Method::empty,empty))frag_original::call(f,empty);
            report(5,model::BoloEvent::no_ammo);return;
        }
        decision=owner->adopt_equipment_hold(observation);
    }
    else decision=owner->update(observation,seen==model::BoloTriggerObservation::edge?input:model::BoloInput{});
    if(decision.event==model::BoloEvent::safety_required||decision.event==model::BoloEvent::interrupted){cancel();report(4,decision.event);return;}
    dispatch(f);report(1,decision.event);
}catch(...){faulted=true;if(weaponhand::current_gt(nullptr))cancel();}
void reset_after_drain() noexcept {
    if(!fragpreparation::owed()&&!fraglaunch::outstanding.load(std::memory_order_acquire)){
        forget();returning={};reports=0;launch_reports=0;previous_gate=~0u;
    }
}
} // namespace fragruntime
