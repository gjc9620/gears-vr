// One ordinary PostPlayerTick dispatcher. The existing equipment owner grants
// the held Smoke; raw producer history supplies motion. Only original methods
// prepare/spawn/consume/notify/re-equip. No second skeleton or input writer.
namespace smokeruntime {
namespace hand=gears_weapon_hand;namespace vr=gears_vr;namespace model=gears_weapon;
namespace meta=gripdamage_metadata;
using Counts=lancerreload_inventory::Counts;
std::optional<model::SmokeGrenadeInteraction> owner;
model::SmokeTriggerObserver trigger;
model::SmokeInstanceConfig config{};
smoke_metadata::Evidence certified{};
hand::Frame parent{};
Counts observed_counts{};std::uint64_t revision{},sequence{},event_serial{},last_input{},held_generation{};
bool counts_known{},faulted{},finishing{};
std::uint64_t disarmed_hold{};std::uint32_t disarmed_press{};
double binding_units{};
unsigned sample_gate{};
unsigned rig_mismatch{};
unsigned rig_read_refusal_reports{};
unsigned metadata_refusal_reports{};
model::SmokeFrame observation{}; // Caller-owned motion workspace, not another publication.
std::uint64_t reports{};unsigned previous_gate{~0u};
unsigned launch_reports{};
unsigned mapping_reports{},motion_reports{};
unsigned launch_reject_reports{};
unsigned trigger_refusal_reports{};
unsigned adoption_refusal_reports{};
unsigned interruption_reports{};
model::SmokeTriggerDiagnostic trigger_diagnostic{};
std::int64_t trigger_sample_started_ns{};
struct ReturnTransition {
    hand::Frame source{};hand::ExistingActionLifetime lifetime{};
    gears_hand_continuity::TrackedInput input{};gears_hand_continuity::Publisher publisher{};
    std::uint64_t hold{};std::int64_t deadline{},verified_ns{};
    std::uint32_t release_pressed{},release_released{};bool active{};
};
ReturnTransition returning{};
gears_hand_continuity::TrackedInput sampled_input{};
gears_hand_continuity::Publisher sampled_publisher{};
void report(unsigned gate,model::SmokeEvent event=model::SmokeEvent::observed) noexcept {
    if(!log_file||(gate==previous_gate&&event==model::SmokeEvent::observed)||reports>=96)return;
    fwprintf(log_file,L"smoke_original_runtime gate=%u event=%u stage=%u binding=%llu input=%llu count_revision=%llu fault=%u preparation_owed=%u rig_mismatch=%u grip_release=1 virtual_mass_kg=2 preview_suppressed=%llu\n",
        gate,unsigned(event),owner?unsigned(owner->stage()):0,owner?owner->binding_token():0,
        last_input,revision,unsigned(faulted),unsigned(smokepreparation::owed()),rig_mismatch,smokepreview::suppressed);
    fflush(log_file);previous_gate=gate;++reports;
}
model::SmokeIdentity identity(hand::Frame const& f) noexcept {
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
bool inventory(hand::Frame const& f,model::SmokeInventory& out,Counts* raw=nullptr) noexcept {
    Counts c{};if(!smokepreparation::inventory(f,c))return false;
    const auto total=std::int64_t(c.capacity)-c.used+c.reserve;
    if(total<0||total>10000)return false;
    if(!counts_known||!(c==observed_counts)){
        if(revision==UINT64_MAX)return false;
        ++revision;observed_counts=c;counts_known=true;
    }
    out={revision,std::uint32_t(total),true};if(raw)*raw=c;return true;
}
bool stamp(model::SmokeStamp& out) noexcept {
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
bool retain_preparation_during_pose_gap(uptr pc) noexcept {
    if(!owner||owner->stage()!=model::SmokeStage::prepared||owner->pending_request().token||returning.active||
       !smokepreparation::owed()||!smokepreparation::journal.prepared||smokepreparation::stop_requested.load()||
       !weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||pc!=parent.controller.address||
       held_generation!=vrequip::model.hold_generation()||
       !vrequip::model.held_for(parent.world.address,parent.pawn.address,parent.weapon.address)||
       !weaponhand::client.existing_action_lifetime_on_gt(smokepreparation::journal.lifetime))return false;
    for(const auto& id:{parent.world,parent.controller,parent.pawn,parent.mesh,parent.weapon})if(!weapon_part_assets::object(id))return false;
    if(playground::pointer(parent.controller.address,L"Pawn")!=parent.pawn.address||
       playground::pointer(parent.pawn.address,L"Weapon")!=parent.weapon.address||
       playground::pointer(parent.pawn.address,L"WorldInfo")!=parent.world.address||
       playground::pointer(parent.weapon.address,L"Instigator")!=parent.pawn.address)return false;
    // Pause is an interruption, even while the independent Grip/actor witness
    // is still valid. Authenticate the stock IsPaused field without granting
    // action authority; unreadable pause state also ends soft-gap retention.
    uptr pauser{};
    if(!field(parent.world,L"Pauser",L"ObjectProperty",pauser)||pauser)return false;
    smokepreparation::State prepared_state{};Counts counts{};
    if(!smokepreparation::state(parent,prepared_state)||!prepared_state.key||!prepared_state.controller_target||!prepared_state.pawn_target||!prepared_state.spinning||
       !smokepreparation::inventory(parent,counts)||!(counts==smokepreparation::journal.before))return false;
    vr::StereoSample stereo{};vr::ControllerInputFrame input{};
    gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    const auto begin=weaponhand::clock_ns(nullptr);const bool sim=parent.stereo.source==vr::StereoSource::MetaOperatorSimulated;
    if(nativeequipment::input_continuity_reader.read(GetCurrentProcessId(),[](){return weaponhand::clock_ns(nullptr);},
       sim,stereo,input,raw,publisher)!=gears_current_tracking::Failure::none)return false;
    const auto end=weaponhand::clock_ns(nullptr);const auto& initial=smokepreparation::journal.source.controllers.hands[1];
    const auto& hand=raw.input.hands[1];
    return begin>0&&end>=begin&&end-begin<=50000000&&publisher.pid&&publisher.start&&
        publisher.pid==sampled_publisher.pid&&publisher.start==sampled_publisher.start&&
        raw.stream_epoch==sampled_input.stream_epoch&&raw.tracking_epoch[1]==sampled_input.tracking_epoch[1]&&
        gears_hand_continuity::detail::same_key(raw.input,sampled_input.input)&&
        raw.input.sequence>=sampled_input.input.sequence&&raw.input.sync_ns>=sampled_input.input.sync_ns&&
        vr::controller_frame_matches(raw.input,stereo,end,sim)&&end>=raw.input.sync_ns&&end-raw.input.sync_ns<=100000000&&
        vr::controller_pose_usable(hand.grip,hand.grip_flags,hand.grip_active)&&hand.squeeze_active==1&&
        hand.squeeze>vr::controller_analog_release_threshold&&
        hand.pressed[vr::control_squeeze]==initial.pressed[vr::control_squeeze]&&
        hand.released[vr::control_squeeze]==initial.released[vr::control_squeeze]&&
        weaponhand::client.existing_action_lifetime_on_gt(smokepreparation::journal.lifetime);
}
// Preserve only an already confirmed equipment hold while the ORIGINAL
// successful throw re-equips its next grenade. No Frame, pose, input edge,
// attachment or new hold is fabricated. Other source/identity loss still
// follows the normal equipment revocation path.
bool retain_return_during_pose_gap(uptr pc) noexcept {
    if(!returning.active||!weaponhand::current_gt(nullptr)||smokepreparation::stop_requested.load()||
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
    returning.input=raw;returning.verified_ns=end;return true;
}
// Visual permission for the sole exact return continuation, never an action
// or attachment permit. Its release counters cannot survive a new Grip press.
bool return_visibility_current(hand::Frame const& f) noexcept {
    if(!returning.active||!same_parent(f,returning.source)||!nativeequipment::current(nullptr,f)||
       smokepreparation::stop_requested.load()||!returning.hold||
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
        (void)nativeequipment::stow_native::hide_released_smoke_return(f);
    lancermain_asset::Evidence asset{};Counts loaded{};
    const bool attached=nativeequipment::stow_native::released_smoke_attached(f)||
        (lancermain_asset::read(f,asset,lancermain_asset::AssetScope::smoke_hold)==lancermain_asset::Status::ready&&
         nativeequipment::attached_frame(f,asset.component.address));
    if(game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&
       smokepreparation::inventory(f,loaded)&&loaded.capacity>loaded.used&&
       active_original(f)&&attached&&nativeequipment::current(nullptr,f)){
        if(log_file){fwprintf(log_file,L"smoke_original_return weapon=%llx hold=%llu frame=%llu attached=1 active=1 actual_input_sequence=%llu\n",
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
    return smoke_original::projectile_class(f,out);
}
bool make_config(hand::Frame const& f,gears_rig_tracking::Frame const& rig,model::SmokeInstanceConfig& out) noexcept {
    lancermain_asset::Evidence asset{};vrequip::MeshPoseProbe mesh{};
    hand::ObjectIdentity projectile{};smoke_original::Entry prepare{},stop{},notify{},empty{},finished{},origin{},velocity{},base_throw{},mode{},prediction{},smoke_throw{};
    sample_gate=10;
    if(lancermain_asset::read(f,asset,lancermain_asset::AssetScope::smoke_hold)!=lancermain_asset::Status::ready||
       !nativeequipment::attached_frame(f,asset.component.address)||!vrequip::mesh_pose_probe(f.weapon.address,mesh)||
       mesh.mesh!=asset.component.address||mesh.asset!=asset.asset.address)return false;
    sample_gate=11;if(!projectile_type(f,projectile))return false;
    sample_gate=12;if(
       !smoke_original::bind(f.weapon.address,smoke_original::Method::prepare,prepare)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::stop,stop)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::notify,notify)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::empty,empty)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::finished,finished)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::origin,origin)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::velocity,velocity)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::base_throw,base_throw)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::mode,mode)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::prediction,prediction)||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::smoke_throw,smoke_throw))return false;
    sample_gate=13;playground::Args gravity(f.pawn.address,L"GetGravityZ");
    if(!weapon_part_assets::native_function(gravity,L"GetGravityZ",L"Actor")||
       !vrequip::result_property(gravity,L"FloatProperty",4)||!nativeequipment::current(nullptr,f)||!gravity.run())return false;
    const double g=-double(gravity.output<float>())/rig.units_per_metre;
    const auto* m=mesh.local_to_world;
    const double scale=std::hypot(std::hypot(double(m[0]),double(m[1])),double(m[2]));
    if(!std::isfinite(g)||g<=0||g>=40||!std::isfinite(scale)||scale<.1||scale>10)return false;
    for(unsigned axis=1;axis<3;++axis){const auto n=axis*4;
        if(std::abs(std::hypot(std::hypot(double(m[n]),double(m[n+1])),double(m[n+2]))-scale)>scale*.001)return false;}
    sample_gate=14;gears_weapon_smoke_grasp::Profile fit{};
    if(!gears_weapon_smoke_grasp::marcus115(gears_hand_authored_mount::marcus115_signature,1,rig.units_per_metre,fit,scale))return false;
    smoke_metadata::Evidence types{};
    if(!smoke_metadata::read(f,types)||!hand::same(types.projectile_type,projectile))return false;
    model::SmokeInstanceConfig c{};c.identity=identity(f);c.input_source=f.stereo.source;
    if(!gears_weapon_smoke_grasp::release_centre(fit,scale,rig.units_per_metre,c.centre_in_right_grip_metres))return false;
    c.original_weapon_type=types.weapon_type.address;c.original_ammo_type=types.ammo_type.address;
    c.original_emitter_type=types.emitter_type.address;
    c.motion.gravity_mps2=g;c.motion.virtual_mass_kg=2.;c.original_projectile_type=projectile.address;
    c.prepare_entry=prepare.function.address;c.launch_entry=notify.function.address;c.safety_entry=stop.function.address;
    c.original_no_ammo_cue=empty.function.address;
    unified_weapon_fire::Fingerprint fingerprint;
    for(const auto* e:{&prepare,&stop,&notify,&empty,&finished,&origin,&velocity,&base_throw,&mode,&prediction,&smoke_throw})fingerprint.word(e->fingerprint);
    fingerprint.word(types.weapon_type.address);fingerprint.word(types.ammo_type.address);
    fingerprint.word(types.emitter_type.address);fingerprint.word(types.effects);fingerprint.word(types.audio);
    fingerprint.word(projectile.address);fingerprint.word(asset.asset.address);fingerprint.word(rig.reference_generation);
    c.config_serial=fingerprint.value;c.grip_geometry_serial=asset.asset.address;
    c.release_rule_serial=notify.fingerprint;c.fuse_rule_serial=projectile.address;c.audio_rule_serial=types.audio;
    c.release_input=model::SmokeReleaseInput::right_grip_release_after_hold;c.fuse_owner=model::SmokeFuseOwner::original_projectile;
    // Admission evidence is current original metadata plus the audited fixed
    // adapter contracts. It is NOT a game/headset acceptance result. Each
    // execution still needs actual counts, state, once-only and return receipts.
    c.current_instance_verified=c.original_smoke_classes_verified=c.grip_geometry_verified=true;
    c.release_input_verified=c.fuse_rule_verified=c.original_prepare_no_ammo_change_verified=true;
    c.original_launch_once_verified=c.original_initial_pose_velocity_override_verified=true;
    c.original_effects_preserved_verified=c.non_throwing_safety_entry_verified=true;
    c.original_gravity_and_units_verified=c.original_no_ammo_cue_verified=true;
    c.original_smoke_emitter_path_verified=c.original_throw_and_smoke_audio_verified=true;
    if(!c.valid()||!nativeequipment::current(nullptr,f))return false;
    certified=types;out=c;return true;
}
bool sample(hand::Frame const& f,hand::GripGameState const& game) noexcept {
    // Bounded timing only: preserve every Engine read and admission predicate.
    // Actual56780 exceeded current-input age before any launch; distinguish
    // rig/history/inventory/config work without refreshing the Frame or input.
    struct SampleCost {
        hand::Frame const& frame;std::int64_t times[7]{};unsigned last{};bool had_owner{};
        explicit SampleCost(hand::Frame const& value) noexcept:frame(value),had_owner(bool(owner)){times[0]=weaponhand::clock_ns(nullptr);}
        void mark(unsigned stage) noexcept {last=stage;times[stage]=weaponhand::clock_ns(nullptr);}
        ~SampleCost() noexcept {
            static unsigned sample_cost_reports{};const auto end=weaponhand::clock_ns(nullptr);
            if(!log_file||sample_cost_reports>=8||end<=times[0]||end-times[0]<20000000)return;
            ++sample_cost_reports;
            fwprintf(log_file,L"smoke_sample_cost frame=%llu sequence=%llu had_owner=%u gate=%u last=%u begin_age_ns=%lld total_ns=%lld initial_ns=%lld rig_ns=%lld history_ns=%lld inventory_ns=%lld config_ns=%lld final_ns=%lld\n",
                frame.key.frame,frame.controllers.sequence,unsigned(had_owner),sample_gate,last,times[0]-frame.controllers.sync_ns,end-times[0],
                last>=1?times[1]-times[0]:-1,last>=2?times[2]-times[1]:-1,last>=3?times[3]-times[2]:-1,
                last>=4?times[4]-times[3]:-1,last>=5?times[5]-times[4]:-1,last>=6?times[6]-times[5]:-1);
            fflush(log_file);
        }
    }cost{f};
    sample_gate=1;
    if(!nativeequipment::current(nullptr,f)||!game.known||!game.main_hand_held||
       game.kind!=hand::WeaponKind::hand_thrown||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       vrequip::model.held_slot()!=*model::original_inventory_slot(model::BodySlot::grenade))return false;
    cost.mark(1);sample_gate=2;gears_rig_tracking::Frame rig{};
    rig_mismatch=0;
    const auto rig_read=handrig::read(rig,0,unsigned(f.stereo.source));
    if(rig_read.reply!=gears_rig_tracking::Reply::accepted){
        rig_mismatch=1;
        // Keep the exact failed read result. No second getter, actor read,
        // source refresh or action admission is performed for diagnostics.
        if(log_file&&rig_read_refusal_reports++<8){
            fwprintf(log_file,L"smoke_rig_read_refusal reply=%u failure=%u observed_ns=%lld frame=%llu sequence=%llu input_sync_ns=%lld input_received_ns=%lld sample_started_ns=%lld source=%u hold=%llu owed=%u\n",
                unsigned(rig_read.reply),unsigned(rig_read.failure),rig_read.observed_ns,f.key.frame,
                f.controllers.sequence,f.controllers.sync_ns,f.controllers.received_ns,trigger_sample_started_ns,
                unsigned(f.stereo.source),vrequip::model.hold_generation(),unsigned(smokepreparation::owed()));
            fflush(log_file);
        }
        return false;
    }
    if(rig.native.registration_cookie!=f.key.registration_cookie||rig.native.native_started_qpc!=f.key.native_started_qpc)rig_mismatch|=2;
    if(rig.world_generation!=f.key.world_generation||rig.reference_generation!=f.reference_generation)rig_mismatch|=4;
    if(!hand::same(rig.world,f.world)||!hand::same(rig.controller,f.controller)||!hand::same(rig.pawn,f.pawn)||
       !hand::same(rig.mesh,f.mesh)||!hand::same(rig.weapon,f.weapon))rig_mismatch|=8;
    if(!vr::stereo_same_pose(rig.reference,f.reference))rig_mismatch|=16;
    if(!gears_hand_continuity::same_input(rig.controllers,f.controllers))rig_mismatch|=32;
    if(rig_mismatch)return false;
    if(owner&&rig.units_per_metre!=binding_units)return false;
    cost.mark(2);sample_gate=3;gears_hand_continuity::TrackedInput raw{};gears_hand_continuity::Publisher publisher{};
    observation={};
    if(!nativeequipment::read_input_continuity(f,raw,publisher,nullptr,nullptr,&observation.motion_history)||
       !observation.motion_history.count)return false;
    cost.mark(3);const auto forward=vr::rig_tracking::map_vector(rig.base_view,{0,0,-1});
    const auto right=vr::rig_tracking::map_vector(rig.base_view,{1,0,0});
    const auto up=vr::rig_tracking::map_vector(rig.base_view,{0,1,0});
    const vr::Matrix basis{{{forward.x,right.x,up.x},{forward.y,right.y,up.y},{forward.z,right.z,up.z}}};
    sample_gate=4;playground::Vector velocity{};
    if(!field(f.pawn,L"Velocity",L"StructProperty",velocity)||!inventory(f,observation.inventory))return false;
    cost.mark(4);unified_weapon_fire::Fingerprint epoch;epoch.word(raw.stream_epoch);epoch.word(raw.tracking_epoch[1]);epoch.word(f.reference_generation);
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
    cost.mark(5);sample_gate=6;
    if(!(config.identity==identity(f))||held_generation!=observation.equipment_hold_generation||
       !stamp(observation.stamp)||!nativeequipment::current(nullptr,f))return false;
    smoke_metadata::Evidence current_types{};
    sample_gate=7;
    unsigned metadata_stage{};
    const bool metadata_read=smoke_metadata::read_after_current_check(f,current_types,&metadata_stage);
    const bool metadata_same=metadata_read&&smoke_metadata::same(current_types,certified);
    if(!metadata_same){
        if(log_file&&metadata_refusal_reports++<8){
            fwprintf(log_file,L"smoke_metadata_refusal stage=%u read=%u same=%u frame=%llu sequence=%llu input_sync_ns=%lld sample_started_ns=%lld observed_ns=%lld owed=%u\n",
                metadata_read?6:metadata_stage,unsigned(metadata_read),unsigned(metadata_same),f.key.frame,
                f.controllers.sequence,f.controllers.sync_ns,trigger_sample_started_ns,weaponhand::clock_ns(nullptr),unsigned(smokepreparation::owed()));
            fflush(log_file);
        }
        return false;
    }
    observation.weapon_type=current_types.weapon_type.address;observation.projectile_type=current_types.projectile_type.address;
    observation.ammo_type=current_types.ammo_type.address;observation.emitter_type=current_types.emitter_type.address;
    observation.stamp.motion_epoch=epoch.value;sampled_input=raw;sampled_publisher=publisher;cost.mark(6);return true;
}
bool receipt(model::SmokeRequest const& request,model::SmokeOriginalReceipt& out) noexcept {
    out={};out.request_token=request.token;out.original_preparation_serial=request.original_preparation_serial;
    if(event_serial==UINT64_MAX||!stamp(out.stamp)||!inventory(parent,out.inventory))return false;
    out.original_event_serial=++event_serial;out.fuse=owner->original_fuse();
    out.config_serial=request.config_serial;out.original_ammo_type=certified.ammo_type.address;
    out.original_emitter_type=certified.emitter_type.address;
    out.executed_motion_epoch=request.motion_epoch;out.executed_reference_generation=request.reference_generation;
    return true;
}
void report_trigger_refusal(model::SmokeTriggerObservation status) noexcept {
    if(!log_file||trigger_refusal_reports>=8)return;
    const auto ticket=++trigger_refusal_reports;
    const auto& d=trigger_diagnostic;const auto& f=observation;const auto& h=f.hands.hands[1];
    fwprintf(log_file,L"smoke_trigger_refusal ticket=%u status=%u reason=%u known=%u cohort=%u binding=%llu hold=%llu sample_started_ns=%lld clock_ns=%lld previous_sequence=%llu previous_sync_ns=%lld previous_stream=%llu previous_tracking=%llu previous_pressed=%u previous_released=%u previous_active=%u previous_level=%.9g current_sequence=%llu current_sync_ns=%lld current_pressed=%u current_released=%u current_active=%u current_level=%.9g history_count=%u\n",
        ticket,unsigned(status),d.reason,unsigned(d.known),unsigned(d.cohort),owner?owner->binding_token():0,
        held_generation,trigger_sample_started_ns,f.stamp.monotonic_ns,d.sequence,d.sync_ns,d.stream,d.tracking,d.pressed,d.released,d.active,
        double(d.level),f.hands.sequence,f.hands.sync_ns,h.pressed[vr::control_squeeze],h.released[vr::control_squeeze],
        h.squeeze_active,double(h.squeeze),f.motion_history.count);
    const auto count=(std::min)(std::size_t(f.motion_history.count),
                               std::size_t(gears_hand_continuity::motion_history_capacity));
    for(std::size_t i=0;i<count;++i){
        const auto& row=f.motion_history.frames[i];const auto& input=row.input;const auto& right=input.hands[1];
        fwprintf(log_file,L"smoke_trigger_history ticket=%u index=%u sequence=%llu sync_ns=%lld received_ns=%lld stream=%llu tracking=%llu input_epoch=%llu space_epoch=%llu pressed=%u released=%u active=%u level=%.9g grip_active=%u grip_flags=%llu position=%.9g,%.9g,%.9g\n",
            ticket,unsigned(i),input.sequence,input.sync_ns,input.received_ns,row.stream_epoch,row.tracking_epoch[1],
            input.input_epoch,input.space_epoch,right.pressed[vr::control_squeeze],right.released[vr::control_squeeze],
            right.squeeze_active,double(right.squeeze),right.grip_active,right.grip_flags,
            right.grip.position.x,right.grip.position.y,right.grip.position.z);
    }
    fflush(log_file);
}
void forget() noexcept {
    if(smokepreparation::owed())return;
    owner.reset();trigger.clear();config={};certified={};parent={};observation={};held_generation=last_input=0;
    sampled_input={};sampled_publisher={};
    counts_known=false;faulted=finishing=false;binding_units=0;
}
bool cancel() noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(!owner)return smokepreparation::stop();
    // Recovery while still squeezing must not silently arm the old grasp.
    // Only a subsequent real press (or new equipment generation) may prepare.
    disarmed_hold=held_generation;
    disarmed_press=parent.controllers.hands[1].pressed[vr::control_squeeze];
    owner->cancel();
    if(!smokepreparation::stop())return false;
    const auto completed_ns=weaponhand::clock_ns(nullptr);
    const auto request=owner->pending_request();
    if(request.action==model::SmokeAction::preserve_original_on_interrupt){
        const auto taken=owner->take_request();model::SmokeOriginalReceipt r{};
        if(!taken||taken->token!=request.token||
           !request.original_preparation_serial||request.original_preparation_serial!=smokepreparation::journal.token||
           !receipt(request,r))return false;
        // A failed current tracking read clears observation before it can
        // supply a motion stamp. This is the already-owned, actually stopped
        // preparation's negative receipt, not a fresh motion/action permit.
        // Preserve its exact request epoch; never borrow an incomplete sample
        // or manufacture a new tracked pose to acknowledge cancellation.
        r.stamp.motion_epoch=request.motion_epoch;
        r.executed_ns=completed_ns;r.accepted=r.original_effect_applied_once=true;
        r.original_entry=smokepreparation::journal.stop.function.address;
        r.original_smoke_emitter_path_preserved=r.original_audio_path_preserved=true;
        r.original_fuse_preserved=r.original_effects_preserved=r.native_safety_complete=true;
        if(owner->retire_stopped_after_external_debit(r)){
            // The original melee route spent its own item. Native stop above
            // proved the old targeting/spin obligations neutral; this grants
            // no VR launch and never acknowledges a debit as our own action.
            if(vrequip::model.release_throwable(parent.world.address,parent.pawn.address,
                                              parent.weapon.address,held_generation))
                weaponhand::held_binding={};
            report(14,model::SmokeEvent::interrupted);return true;
        }
        const auto event=owner->acknowledge(r);
        if(event!=model::SmokeEvent::safety_completed){faulted=true;report(12,event);return false;}
    }else if(request.token){faulted=true;return false;} // An ambiguous dispatched operation is never retried.
    return true;
}
void pre_original_tick(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::restoration_quiet()||
       parent.controller.address!=pc||!smokepreparation::outstanding.load(std::memory_order_acquire)||
       !nativeequipment::stow_native::original_action_pending())return;
    // Negative authority only: cancel this old physical grasp and stop its
    // exact original preparation BEFORE a real original button can replace
    // inventory. cancel keeps anti-replay/disarmed-press and receipt checks.
    const bool clean=cancel();report(clean?60:61,model::SmokeEvent::interrupted);
    if(clean)forget();
}
void unavailable(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr)||(!owner&&!smokepreparation::owed()))return;
    if(parent.controller.address&&pc!=parent.controller.address)return;
    // Only the existing original held preparation may wait through a soft
    // render/rig publication gap. The independent current input must still
    // prove the same press, held generation, tracking epoch and real actors.
    // Release, new Grip, source change or actual tracking loss still cancels;
    // no throw can execute without a new authentic current Frame/mapping.
    if(retain_preparation_during_pose_gap(pc))return;
    const bool clean=cancel();report(clean?10:11,model::SmokeEvent::interrupted);
    // A missing render association is never an opportunity to throw from an
    // old pose. Cleanup may still settle the existing original obligation.
    if(clean)forget();
}
bool finish_original(hand::Frame const& f,smokelaunch::Result const& launch) noexcept {
    smoke_original::Entry entry{};bool invoked{};
    if(!launch.original_once||finishing||!smokepreparation::stop()||
       !smoke_original::bind(f.weapon.address,smoke_original::Method::finished,entry))return false;
    // The authored physical release replaces the stock throw animation; call
    // its original completion once, after accounting for the spawn debit.
    // Pin the current producer only when actual spare ammunition can produce
    // another grenade. With no spare, original Active.BeginState owns the
    // normal empty-weapon auto-switch; retaining this old hold would block it.
    // A missing live pin grants no hold continuation, but never suppresses the
    // already owed original completion of an actually spawned projectile.
    Counts counts{};const bool known=smokepreparation::inventory(f,counts);
    const bool next_expected=known&&(counts.infinite||counts.reserve>0);
    returning={};ReturnTransition next{};next.source=f;next.lifetime=smokelaunch::journal.lifetime;
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
    finishing=true;const bool okay=smoke_original::complete_existing(f,smokelaunch::journal.lifetime,entry,launch.projectile,&invoked);
    if(!okay||!invoked)returning={};
    if(log_file){fwprintf(log_file,L"smoke_original_completion weapon=%llx counts_known=%u capacity=%d used=%d reserve=%d infinite=%u next_expected=%u continuation=%u invoked=%u completed=%u\n",
        f.weapon.address,unsigned(known),counts.capacity,counts.used,counts.reserve,unsigned(counts.infinite),
        unsigned(next_expected),unsigned(returning.active),unsigned(invoked),unsigned(okay&&invoked));fflush(log_file);}
    if(invoked)finishing=false;return okay&&invoked;
}
void dispatch(hand::Frame const& f) noexcept {
    if(!owner||faulted)return;
    const auto request=owner->pending_request();if(!request.token)return;
    model::SmokeOriginalReceipt r{};model::SmokeEvent event{};
    if(request.action==model::SmokeAction::prepare_original){
        const auto actual=smokepreparation::begin(f,*owner);if(!actual.token_taken)return;
        if(!receipt(request,r)){faulted=true;cancel();report(21);return;}
        r.executed_ns=actual.executed_ns;r.accepted=actual.prepared;
        r.original_entry=smokepreparation::journal.prepare.function.address;
        r.original_smoke_emitter_path_preserved=r.original_audio_path_preserved=actual.prepared;
        r.original_preparation_serial=actual.preparation_serial;
        r.original_effect_applied_once=actual.prepared;
        r.original_fuse_preserved=r.original_effects_preserved=actual.prepared;
        if(!actual.prepared&&!actual.cleanup_complete&&actual.dispatched){faulted=true;cancel();report(22);return;}
        event=owner->acknowledge(r);
    }else if(request.action==model::SmokeAction::launch_original){
        if(!active_original(f)){cancel();return;}
        const auto begin_ns=weaponhand::clock_ns(nullptr);
        const auto actual=smokelaunch::invoke_once(f,*owner);
        if(!actual.token_taken){
            if(log_file&&launch_reject_reports++<8){
                fwprintf(log_file,L"smoke_original_launch_preflight stage=%u token=%llu frame=%llu sequence=%llu begin_ns=%lld finish_ns=%lld input_sync_ns=%lld authored_ns=%lld valid_until_ns=%lld\n",
                    actual.preflight_stage,request.token,f.key.frame,f.controllers.sequence,begin_ns,weaponhand::clock_ns(nullptr),
                    f.controllers.sync_ns,request.authored_ns,request.valid_until_ns);fflush(log_file);
            }
            return;
        }
        if(!receipt(request,r)){faulted=true;cancel();report(31);return;}
        if(log_file&&launch_reports++<24){
            if(!actual.dispatched){
                fwprintf(log_file,L"smoke_original_launch_undispatched stage=%u token=%llu begin_ns=%lld finish_ns=%lld input_sync_ns=%lld authored_ns=%lld valid_until_ns=%lld\n",
                    actual.preflight_stage,request.token,begin_ns,weaponhand::clock_ns(nullptr),
                    f.controllers.sync_ns,request.authored_ns,request.valid_until_ns);
            }
            const auto& p=actual.actual_position;const auto& v=actual.actual_velocity;
            fwprintf(log_file,L"smoke_original_launch token=%llu weapon=%llx projectile=%llx projectile_class=%llx once=%u dispatched=%u returned=%u before=%u after=%u throws=%u smoke_throws=%u consumes=%u notifications=%u inits=%u origin_returns=%u velocity_returns=%u initial_matches=%u direction_restored=%u position=%.9g,%.9g,%.9g velocity=%.9g,%.9g,%.9g planned_position=%.9g,%.9g,%.9g planned_velocity=%.9g,%.9g,%.9g executed_ns=%lld\n",
                request.token,f.weapon.address,actual.projectile.address,actual.projectile.klass,
                unsigned(actual.original_once),unsigned(actual.dispatched),unsigned(actual.returned),
                request.before.count,r.inventory.count,actual.throws,actual.smoke_throws,actual.consumes,actual.notifications,actual.inits,
                actual.origins,actual.velocities,unsigned(actual.initial_state_matches),unsigned(actual.direction_restored),
                double(p.x),double(p.y),double(p.z),double(v.x),double(v.y),double(v.z),
                request.launch.position.x,request.launch.position.y,request.launch.position.z,
                request.launch.velocity.x,request.launch.velocity.y,request.launch.velocity.z,actual.executed_ns);fflush(log_file);
        }
        r.executed_ns=actual.executed_ns;r.accepted=actual.original_once;
        r.original_entry=smokelaunch::journal.notify.function.address;
        r.original_ammo_consumptions=actual.consumes;
        r.original_smoke_emitter_path_preserved=r.original_audio_path_preserved=actual.original_once;
        r.original_projectile=actual.projectile.address;r.original_projectile_type=actual.projectile.klass;
        r.original_projectiles_spawned=actual.projectile.address?1u:0u;
        r.original_effect_applied_once=actual.original_once;
        r.original_fuse_preserved=r.original_effects_preserved=r.tracked_initial_pose_and_velocity_applied=actual.original_once;
        // A failed original dispatch that generated effects is uncertain,
        // never a rejected-no-effect receipt or a second attempt.
        if(!actual.original_once&&(actual.dispatched||actual.projectile.address)){
            faulted=true;smokepreparation::stop();report(32,model::SmokeEvent::safety_violation);return;
        }
        event=owner->acknowledge(r);
        if(event==model::SmokeEvent::thrown){
            if(finish_original(f,actual)){report(33,event);forget();return;}
            faulted=true;report(34,event);return;
        }
    }else if(request.action==model::SmokeAction::preserve_original_on_interrupt){cancel();return;}
    else return;
    report(20,event);
    if(event==model::SmokeEvent::safety_violation||event==model::SmokeEvent::rejected){faulted=true;cancel();}
}
void tick(hand::Frame const& f,hand::GripGameState const& game) noexcept try {
    if(!weaponhand::current_gt(nullptr)||!nativeequipment::domain_ready())return;
    if(returning.active)return;
    if(smokepreparation::stop_requested.load(std::memory_order_acquire)){unavailable(f.controller.address);return;}
    const bool held=game.known&&game.kind==hand::WeaponKind::hand_thrown&&game.main_hand_held&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address);
    if(!held){if(owner||smokepreparation::owed())unavailable(parent.controller.address);return;}
    if(owner&&(!same_parent(f,parent)||!(config.identity==identity(f))||held_generation!=vrequip::model.hold_generation())){
        if(faulted){
            // A new confirmed equipment incarnation may observe actual stock
            // counts afresh. Retire only the local failed mirror; never retry
            // its old token, invent a receipt, or clear a native cleanup debt.
            if(!smokepreparation::stop()||smokelaunch::outstanding.load(std::memory_order_acquire))return;
            report(13,model::SmokeEvent::interrupted);forget();
        }else{if(!cancel())return;forget();}
    }
    if(owner&&owner->stage()==model::SmokeStage::interrupted&&!owner->pending_request().token&&!smokepreparation::owed())forget();
    if(faulted){smokepreparation::stop();return;}
    if(!owner){
        // An exact known Frag class is a cheap negative hint. The full Smoke
        // class/package/asset and current-input checks still admit every hold;
        // an outstanding original preparation is never bypassed by this hint.
        if(!smokepreparation::owed()&&playground::named(f.weapon.klass,L"Weap_FragGrenade"))return;
        const auto& right=f.controllers.hands[1];const auto hold=vrequip::model.hold_generation();
        if(right.squeeze_active!=1||right.squeeze<=vr::controller_analog_release_threshold){
            // hand_thrown also includes Frag. Before any model mutation,
            // authenticate this current Smoke class/mesh/package specifically.
            // Unknown/other throwable observations retain their own owner.
            lancermain_asset::Evidence smoke{};
            if(lancermain_asset::read(f,smoke,lancermain_asset::AssetScope::smoke_hold)!=lancermain_asset::Status::ready||
               !nativeequipment::current(nullptr,f))return;
            // Released during original equip: no preparation/motion was owned,
            // so discard only the exact equipment hold, never queue a throw.
            if(vrequip::model.release_throwable(f.world.address,f.pawn.address,f.weapon.address,hold))
                weaponhand::held_binding={};
            return;
        }
        if(hold==disarmed_hold&&right.pressed[vr::control_squeeze]<=disarmed_press)return;
    }
    trigger_sample_started_ns=weaponhand::clock_ns(nullptr);
    if(!sample(f,game)){report(100+sample_gate);if(owner)unavailable(f.controller.address);return;}
    if(observation.hands.sequence==last_input)return;
    if(smokepreparation::owed()&&f.controllers.hands[1].pressed[vr::control_squeeze]!=
        smokepreparation::journal.source.controllers.hands[1].pressed[vr::control_squeeze]){
        cancel();forget();return; // Yield before equipment consumes that real new Grip.
    }
    model::SmokeInput input{};const auto seen=trigger.observe(observation,input,config.release_input,&trigger_diagnostic);
    last_input=observation.hands.sequence;
    if(seen==model::SmokeTriggerObservation::invalid||seen==model::SmokeTriggerObservation::ambiguous||
       seen==model::SmokeTriggerObservation::exhausted){report_trigger_refusal(seen);cancel();report(3);return;}
    model::SmokeDecision decision{};
    if(owner->stage()==model::SmokeStage::idle){
        decision=owner->adopt_equipment_hold(observation);
        if(decision.audio_for(config.identity,owner->binding_token())){
            smoke_original::Entry empty{};
            if(smoke_original::bind(f.weapon.address,smoke_original::Method::empty,empty)&&
               empty.function.address==decision.original_cue)(void)smoke_original::call(f,empty);
            report(5,model::SmokeEvent::no_ammo);return;
        }
        if(owner->stage()==model::SmokeStage::interrupted&&log_file&&adoption_refusal_reports<8){
            const auto& o=observation;const auto& h=o.hands;const auto& c=config;
            const bool history_valid=o.motion_history.count&&o.motion_history.count<=gears_hand_continuity::motion_history_capacity&&
                gears_hand_continuity::same_input(o.motion_history.frames[o.motion_history.count-1].input,h)&&
                gears_hand_continuity::motion_history_valid(o.motion_history,o.motion_history.frames[o.motion_history.count-1],1,c.motion.maximum_sample_gap_ns);
            fwprintf(log_file,L"smoke_adoption_refusal ticket=%u event=%u allowed=%u original_owns_hands=%u equipped=%u count=%u count_revision=%llu hold=%llu stamp_ns=%lld input_sync_ns=%lld sample_age_ns=%lld sample_limit_ns=%lld controller_match=%u history_valid=%u history_count=%u session_match=%u space_match=%u input_epoch_match=%u profile_match=%u source_match=%u pose_usable=%u mapping_epoch_match=%u binding=%llu sequence=%llu\n",
                ++adoption_refusal_reports,unsigned(decision.event),unsigned(o.original_interaction_allowed),unsigned(o.original_owns_hands),unsigned(o.original_equipped),o.inventory.count,o.inventory.revision,o.equipment_hold_generation,
                o.stamp.monotonic_ns,h.sync_ns,o.stamp.monotonic_ns-h.sync_ns,c.motion.maximum_sample_gap_ns,
                unsigned(vr::controller_frame_matches(h,o.stereo,o.stamp.monotonic_ns,c.input_source==vr::StereoSource::MetaOperatorSimulated)),unsigned(history_valid),o.motion_history.count,
                unsigned(h.session==c.identity.xr_session),unsigned(h.space_epoch==c.identity.space_epoch),unsigned(h.input_epoch==c.identity.input_epoch),unsigned(h.profile_generation==c.identity.profile_generation),
                unsigned(h.source==c.input_source&&o.stereo.source==c.input_source),unsigned(vr::controller_pose_usable(h.hands[1].grip,h.hands[1].grip_flags,h.hands[1].grip_active)),unsigned(o.mapping.motion_epoch==o.stamp.motion_epoch),o.stamp.binding,h.sequence);
            fflush(log_file);
        }
    }
    else decision=owner->update(observation,seen==model::SmokeTriggerObservation::edge?input:model::SmokeInput{});
    if(decision.event==model::SmokeEvent::safety_required||decision.event==model::SmokeEvent::interrupted){
        // Existing sampled values only, before the normal cancellation. This
        // does not query another Frame, refresh input or grant an action.
        if(log_file&&interruption_reports++<8){const auto& o=observation;const auto& h=o.hands;const auto& right=h.hands[1];
            fwprintf(log_file,L"smoke_interruption reason=%u boundary_mask=%u event=%u sequence=%llu sync_ns=%lld stamp_ns=%lld sample_age_ns=%lld sample_limit_ns=%lld allowed=%u original_owns_hands=%u equipped=%u inventory_count=%u expected_count=%u motion_epoch=%llu reference=%llu history_count=%u squeeze_active=%u squeeze=%.9g pressed=%u released=%u input_kind=%u edge_sequence=%llu edge_sync_ns=%lld base=%.9g,%.9g,%.9g base_rotation=%.9g,%.9g,%.9g sample_started_ns=%lld before_sample_age_ns=%lld sampling_to_stamp_ns=%lld frame=%llu\n",
                unsigned(owner->last_interrupt_reason()),owner->last_boundary_mask(),unsigned(decision.event),h.sequence,h.sync_ns,o.stamp.monotonic_ns,
                o.stamp.monotonic_ns-h.sync_ns,config.motion.maximum_sample_gap_ns,unsigned(o.original_interaction_allowed),unsigned(o.original_owns_hands),
                unsigned(o.original_equipped),o.inventory.count,owner->inventory().count,o.stamp.motion_epoch,o.mapping.reference_generation,o.motion_history.count,
                right.squeeze_active,double(right.squeeze),right.pressed[vr::control_squeeze],right.released[vr::control_squeeze],unsigned(input.kind),input.hand_sequence,input.sync_ns,
                o.mapping.game_base.position.x,o.mapping.game_base.position.y,o.mapping.game_base.position.z,
                o.mapping.game_base.rotation.pitch,o.mapping.game_base.rotation.yaw,o.mapping.game_base.rotation.roll,
                trigger_sample_started_ns,trigger_sample_started_ns-h.sync_ns,o.stamp.monotonic_ns-trigger_sample_started_ns,f.key.frame);fflush(log_file);}
        if(log_file&&interruption_reports<=8){
            const auto& rot=owner->last_rotation_observation();
            fwprintf(log_file,L"smoke_rotation_observation valid=%u history=%u current=%u gap_ns=%llu old=%.9g,%.9g,%.9g new=%.9g,%.9g,%.9g pitch_delta=%.9g yaw_delta=%.9g magnitude=%.9g locomotion_intent=%u turn_intent=%u pitch_continuous=%u yaw_continuous=%u episode_before=%u episode_after=%u\n",
                unsigned(rot.valid),unsigned(rot.history_supplied),unsigned(rot.history_current),rot.consumer_gap_ns,
                rot.old_pitch,rot.old_yaw,rot.old_roll,rot.new_pitch,rot.new_yaw,rot.new_roll,
                rot.pitch_delta,rot.yaw_delta,rot.rotation_magnitude,unsigned(rot.locomotion_intent),unsigned(rot.stick_intent),
                unsigned(rot.pitch_continuous),unsigned(rot.continuous),unsigned(rot.episode_before),unsigned(rot.episode_after));fflush(log_file);
        }
        cancel();report(4,decision.event);return;
    }
    // Passive snapshots only. Dispatch may retire the owner; no live owner or
    // pending-request reference is used by the post-dispatch evidence below.
    const auto motion_snapshot=owner->last_rotation_observation();
    const auto motion_frame=f.key.frame,motion_sequence=observation.hands.sequence;
    const auto& pending_snapshot=owner->pending_request();
    const bool report_motion=log_file&&decision.event==model::SmokeEvent::observed&&!pending_snapshot.token&&
        (motion_snapshot.continuous||motion_snapshot.episode_continuation||motion_snapshot.pitch_continuous)&&motion_reports<8;
    const bool report_mapping=log_file&&pending_snapshot.action==model::SmokeAction::launch_original&&mapping_reports<24;
    const auto mapping_token=pending_snapshot.token,mapping_release_sequence=input.hand_sequence;
    const auto mapping_input_sync=input.sync_ns;
    const auto mapping_snapshot=observation.mapping;
    const auto hand_velocity=pending_snapshot.tracked.velocity_mps;
    dispatch(f);report(1,decision.event);
    if(report_motion&&motion_reports++<8){
        fwprintf(log_file,L"smoke_motion_continuity frame=%llu sequence=%llu history=%u current=%u gap_ns=%llu pitch_delta=%.9g yaw_delta=%.9g magnitude=%.9g locomotion_intent=%u turn_intent=%u pitch_continuous=%u yaw_continuous=%u episode_before=%u episode_after=%u\n",
            motion_frame,motion_sequence,unsigned(motion_snapshot.history_supplied),unsigned(motion_snapshot.history_current),
            motion_snapshot.consumer_gap_ns,motion_snapshot.pitch_delta,motion_snapshot.yaw_delta,motion_snapshot.rotation_magnitude,
            unsigned(motion_snapshot.locomotion_intent),unsigned(motion_snapshot.stick_intent),unsigned(motion_snapshot.pitch_continuous),
            unsigned(motion_snapshot.continuous),unsigned(motion_snapshot.episode_before),unsigned(motion_snapshot.episode_after));fflush(log_file);
    }
    if(report_mapping&&mapping_reports++<24){
        const auto& b=mapping_snapshot.game_base;const auto& v=mapping_snapshot.pawn_velocity_world_units_per_second;
        fwprintf(log_file,L"smoke_release_mapping token=%llu current_sequence=%llu release_sequence=%llu input_sync_ns=%lld base_position=%.12g,%.12g,%.12g base_rotation=%.12g,%.12g,%.12g base_velocity=%.12g,%.12g,%.12g hand_velocity=%.12g,%.12g,%.12g units=%.12g\n",
            mapping_token,motion_sequence,mapping_release_sequence,mapping_input_sync,
            b.position.x,b.position.y,b.position.z,b.rotation.pitch,b.rotation.yaw,b.rotation.roll,
            v.x,v.y,v.z,hand_velocity.x,hand_velocity.y,hand_velocity.z,mapping_snapshot.units_per_metre);fflush(log_file);
    }
}catch(...){faulted=true;if(weaponhand::current_gt(nullptr))cancel();}
void reset_after_drain() noexcept {
    if(!smokepreparation::owed()&&!smokelaunch::outstanding.load(std::memory_order_acquire)){
        forget();returning={};reports=0;launch_reports=0;mapping_reports=0;motion_reports=0;launch_reject_reports=0;previous_gate=~0u;
        disarmed_hold=0;disarmed_press=0;adoption_refusal_reports=0;interruption_reports=0;rig_read_refusal_reports=0;metadata_refusal_reports=0;
    }
}
} // namespace smokeruntime
