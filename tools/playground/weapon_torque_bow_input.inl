// Bow's one coordinator/backend on the existing GT R2 path. Its entry points
// do nothing until that dispatcher explicitly routes Bow here. No new hook,
// raw-input producer, skeletal writer, worker or launch path is installed.
namespace torquebow_input {
namespace hand=gears_weapon_hand;namespace driver_api=gears_torque_bow_fire_driver;
using Result=gears_weapon::TorqueBowCallResult;
hand::EquipmentIdentity parent(hand::Frame const& f) noexcept {
    return {f.key,f.world,f.controller,f.pawn,f.mesh,f.weapon,f.reference,f.reference_generation};
}
unsigned game_clock_gate{},snapshot_gate{};
lancerreload_inventory::Counts snapshot_counts{};
bool game_clock(hand::Frame const& f,std::int64_t& tick) noexcept {
    // Actual paused GAME time, not QPC or a renewed input clock. This query
    // does not grant actor/pose/input permission or write a reload timestamp.
    namespace meta=gripdamage_metadata;
    game_clock_gate=1;if(!weaponhand::current_gt(nullptr))return false;
    game_clock_gate=2;if(!torquebow_cleanup_context::current(parent(f)))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField field{};float first{},second{};unsigned package{};
    game_clock_gate=3;if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.world)||
       !weaponhand::derives(r,f.world.address,L"WorldInfo",L"Engine")||
       !weaponhand::field(r,f.world.address,L"TimeSeconds",L"FloatProperty",4,field)||
       !r.named(field.owner,L"WorldInfo")||!r.path(schema.identities[field.owner].outer,package)||
       !r.named(package,L"Engine")||schema.identities[package].outer||
       !r.at(f.world.address,field.property.offset,first)||!std::isfinite(first)||first<0||
       schema.field_count==schema.fields.size())return false;
    schema.fields[schema.field_count++]=field.property;
    const double microseconds=double(first)*1000000.;
    game_clock_gate=4;if(microseconds>9e15||!unified_weapon_fire::retained_metadata(r)||
       !r.at(f.world.address,field.property.offset,second)||second!=first)return false;
    game_clock_gate=5;if(!torquebow_cleanup_context::current(parent(f)))return false;
    tick=static_cast<std::int64_t>(microseconds);game_clock_gate=0;return true;
}
bool snapshot(hand::Frame const& f,torquebow_state::Observed const& actual,driver_api::Snapshot& out) noexcept {
    const auto& counts=actual.inventory;snapshot_counts=counts;snapshot_gate=1;
    if(counts.capacity!=1||counts.used<0||counts.used>1||counts.reserve<0||counts.reserve>1000000||counts.infinite)return false;
    driver_api::Snapshot value{};
    value.original={actual.active&&actual.stopped(),actual.charge,actual.pending_end,actual.input_down,false,
        actual.pending_fire,actual.fire};
    value.loaded=unsigned(1-counts.used);value.reserve=unsigned(counts.reserve);value.fully_stopped=actual.stopped();
    snapshot_gate=2;if(!game_clock(f,value.game_tick))return false;out=value;snapshot_gate=0;return true;
}
bool shoulder(hand::Frame const& f) noexcept {
    const auto& left=f.controllers.hands[0];
    const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
    return !hand::index_pinch(f)&&f.left_tracked&&gears_vr::controller_pose_usable(left.grip,left.grip_flags,left.grip_active)&&
        gears_weapon::body_slot(nativeequipment::calibration,body,left.grip.position,0)==gears_weapon::BodySlot::shoulder_ammo;
}
struct Backend {
    unsigned observe_gate{},configure_gate{},warm_result{};
    unsigned preparation_attempts{},preparation_gate{},press_attempts{};bool press_reserved{};Result press_result{Result::rejected};
    unsigned timing_observations{};std::int64_t timing_state_ns{},timing_snapshot_ns{};
    void reset_timing() noexcept {timing_observations=0;timing_state_ns=timing_snapshot_ns=0;}
    driver_api::Snapshot last{};std::uint64_t config_serial{1};
    hand::Frame last_source{};bool last_known{};
    std::uint64_t next_arrow{1},reload_arrow{},reload_press{};hand::Lease reload_lease{};
    hand::Frame geometry_parent{};std::uint64_t geometry_hold{};double geometry_avatar_scale{},geometry_bow_scale{};
    hand::ObjectIdentity geometry_component{},geometry_asset{};
    torquebow_arrow_appearance::Appearance geometry_appearance{};
    gears_torque_bow_arrow_mount::Mount arrow_mount{};gears_weapon::TorqueBowFeedCalibration feed_calibration{};
    gears_vr::Pose support_grip{};bool support_known{},geometry_known{};
    std::int64_t now() noexcept {return weaponhand::clock_ns(nullptr);}
    std::uint64_t hold(hand::Frame const& f) noexcept {
        return vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)?vrequip::model.hold_generation():0;
    }
    bool owned() const noexcept {return torquebow_input_owner::pending();}
    bool has_spawn() const noexcept {torquebow_fire::SpawnReceipt receipt{};return torquebow_fire::peek_spawn(receipt);}
    bool geometry_current(hand::Frame const& f) const noexcept {
        auto a=geometry_parent.key,b=f.key;a.frame=b.frame=0;
        return geometry_known&&hand::same(a,b)&&geometry_hold==vrequip::model.hold_generation()&&
            hand::same(geometry_parent.world,f.world)&&hand::same(geometry_parent.controller,f.controller)&&
            hand::same(geometry_parent.pawn,f.pawn)&&hand::same(geometry_parent.mesh,f.mesh)&&hand::same(geometry_parent.weapon,f.weapon)&&
            geometry_parent.reference_generation==f.reference_generation&&gears_vr::stereo_same_pose(geometry_parent.reference,f.reference)&&
            geometry_parent.controllers.source==f.controllers.source&&geometry_parent.controllers.session==f.controllers.session&&
            geometry_parent.controllers.space_epoch==f.controllers.space_epoch&&geometry_parent.controllers.input_epoch==f.controllers.input_epoch&&
            geometry_parent.controllers.profile_generation==f.controllers.profile_generation&&
            nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_torque_bow&&nativelancerfollow::mount.bound()&&
            hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&nativelancerfollow::bound_key.hold_generation==geometry_hold&&
            hand::same(nativelancerfollow::bound_key.component,geometry_component)&&hand::same(nativelancerfollow::bound_key.asset,geometry_asset)&&
            nativelancerfollow::bound_units==arrow_mount.units_per_metre&&
            torquebow_arrow_visual::warmed()&&torquebow_arrow_visual::same_parent(f)&&
            torquebow_arrow_appearance::same(geometry_appearance,torquebow_arrow_visual::owner.original);
    }
    bool prepare_geometry(hand::Frame const& f) noexcept {
        if(owned()||has_spawn()||reload_owned()||!torquebow_arrow_visual::warmed()||
           !torquebow_arrow_visual::same_parent(f)||!nativeequipment::current(nullptr,f))return false;
        nativelancerfollow::tick(f,nativelancerfollow::Phase::prepare_only);
        gears_rig_tracking::Frame raw{};gears_committed_interaction::Frame avatar{};
        torquebow_arrow_appearance::Appearance appearance{};lancermain_asset::Evidence asset{};
        gears_torque_bow_arrow_tracking::RootSample root{};gears_hand_authored_mount::Mount wrist{};
        gears_torque_bow_arrow_mount::Mount candidate_arrow{};gears_weapon::TorqueBowFeedCalibration candidate_feed{};
        if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_torque_bow||
           lancermain_asset::read(f,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready||
           !hand::same(asset.component,nativelancerfollow::bound_key.component)||!hand::same(asset.asset,nativelancerfollow::bound_key.asset)||
           nativeequipment_rig::acquire(f,raw).status!=nativeequipment_rig::Status::ready||
           !gears_torque_bow_arrow_tracking::root_sample(f,raw,nativelancerfollow::mount,now(),root)||
           !handrig::read_interaction(avatar)||!torquebow_arrow_appearance::read(f,avatar,appearance)||
           !gears_hand_authored_mount::marcus115(avatar.right.geometry,gears_hand_authored_mount::Side::left,
               avatar.model_scale,raw.units_per_metre,wrist)||
           !gears_torque_bow_arrow_mount::author_original(wrist.calibration,avatar.model_scale,appearance.world_scale,raw.units_per_metre,candidate_arrow)||
           !gears_weapon::author_torque_bow_feed(config_serial,root.scale,candidate_arrow,candidate_feed)||
           !nativeequipment::current(nullptr,f))return false;
        if(!torquebow_arrow_appearance::same(appearance,torquebow_arrow_visual::owner.original)){
            geometry_known=false;torquebow_arrow_visual::retire();return false;
        }
        geometry_parent=f;geometry_hold=hold(f);geometry_avatar_scale=avatar.model_scale;geometry_bow_scale=root.scale;
        geometry_component=asset.component;geometry_asset=asset.asset;
        geometry_appearance=appearance;arrow_mount=candidate_arrow;feed_calibration=candidate_feed;geometry_known=true;
        support_known=gears_torque_bow_grasp_pose::support_grip(nativelancerfollow::mount,root.scale,avatar.model_scale,
            raw.units_per_metre,wrist.calibration,support_grip);
        return geometry_current(f);
    }
    bool reload_owned() const noexcept {return torquebow_reload_live::pending();}
    bool current_decision(hand::Frame const& f,hand::Decision& decision) noexcept {
        hand::Frame current{};hand::GripGameState game{};
        return weaponhand::client.copy_frame_on_gt(current,game,decision)&&hand::same_frame(current,f)&&
            game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands&&nativeequipment::current(nullptr,f);
    }
    bool reload_sample(hand::Frame const& f,driver_api::Snapshot& out) noexcept {
        if(!torquebow_reload_live::matches(f))return false;
        torquebow_state::Observed actual{};
        return torquebow_state::read(f,actual)&&snapshot(f,actual,out)&&nativeequipment::current(nullptr,f);
    }
    bool reload_begin(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& d,
        gears_weapon::TorqueBowInventory const& inventory) noexcept {
        if(owned()||has_spawn()||reload_owned()||!geometry_current(f)||!game.known||game.busy||game.animation_owns_hands||
           !shoulder(f)||!torquebow_arrow_visual::warmed()||!torquebow_arrow_visual::same_parent(f)||
           !next_arrow||next_arrow==UINT64_MAX)return false;
        gears_committed_interaction::Frame avatar{};torquebow_arrow_appearance::Appearance appearance{};
        if(!handrig::read_interaction(avatar)||avatar.model_scale!=geometry_avatar_scale||
           !torquebow_arrow_appearance::read(f,avatar,appearance)||!torquebow_arrow_appearance::same(appearance,geometry_appearance)||
           !torquebow_reload_live::reserve_before_begin(f,d,inventory))return false;
        reload_arrow=next_arrow++;reload_press=f.press_token;reload_lease=d.lease;
        torquebow_reload_live::begin_original(f,d);
        return reload_owned(); // an unknown dispatched begin remains owned
    }
    Result reload_accept(hand::Frame const& f,hand::GripGameState const&,driver_api::ReloadAcceptance& out) noexcept {
        out={};
        if(torquebow_reload_live::owner.phase==torquebow_reload_live::Phase::recovering||
           torquebow_reload_live::owner.phase==torquebow_reload_live::Phase::conflict)return Result::rejected;
        if(!nativeequipment::current(nullptr,f))return Result::unknown;
        hand::Decision d{};
        if(!current_decision(f,d)||!torquebow_reload_live::ammo_grant(f,d)||
           !hand::same(reload_lease,d.lease)||f.press_token!=reload_press)return Result::rejected;
        std::uint64_t serial{};std::int64_t started{};
        if(!torquebow_reload_live::accepted(serial,started)||!reload_sample(f,out.actual))return Result::unknown;
        out.original_serial=serial;out.original_start_tick=started;out.arrow_serial=reload_arrow;out.left_grip_edge=reload_press;
        out.automatic_fill_suppressed=out.fresh_left_grip=out.left_shoulder_contact=true;
        return Result::accepted; // retained facts from the original fresh shoulder begin
    }
    bool reload_regrab(hand::Frame const& f,hand::Decision const& d,gears_weapon::TorqueBowArrowReceipt& out) noexcept {
        out={};
        if(!reload_owned()||!shoulder(f)||!d.new_grant||f.press_token<=reload_press||
           !torquebow_reload_live::ammo_grant(f,d)||!torquebow_reload_live::owns_reload(f,torquebow_reload_live::owner.original_serial)||
           !next_arrow||next_arrow==UINT64_MAX)return false;
        reload_arrow=next_arrow++;reload_press=f.press_token;reload_lease=d.lease;
        out={reload_arrow,gears_weapon::TorqueBowAmmoSource::left_shoulder,true,true,reload_press,true,true};return true;
    }
    bool reload_complete(hand::Frame const& f,hand::GripGameState const&,gears_weapon::TorqueBowRequest const& request) noexcept {
        hand::Decision decision{};
        return current_decision(f,decision)&&torquebow_reload_live::begin_completion(f,decision,request);
    }
    bool reload_reject(gears_weapon::TorqueBowRequest const& request) noexcept {return torquebow_reload_live::reject_completion(request);}
    void reload_stop() noexcept {torquebow_reload_live::stop();}
    void reload_service(hand::Frame const* f) noexcept {
        if(torquebow_reload_live::owner.phase!=torquebow_reload_live::Phase::holding)torquebow_reload_live::service(f);
    }
    bool reload_receipt(hand::Frame const* f,gears_weapon::TorqueBowActionReceipt& receipt,driver_api::Snapshot& actual) noexcept {
        if(!(f&&torquebow_reload_live::receipt(*f,receipt))&&!torquebow_reload_live::receipt_owned(receipt))return false;
        actual={receipt.original,receipt.inventory.loaded,receipt.inventory.reserve,0,true};
        return game_clock(torquebow_reload_live::owner.source,actual.game_tick);
    }
    bool reload_retire(std::uint64_t token,gears_weapon::TorqueBowInteraction const& model) noexcept {
        if(!torquebow_reload_live::retire_acknowledged(token,model))return false;
        reload_arrow=reload_press=0;reload_lease={};return true;
    }
    bool read_continuity(hand::Frame const& f,gears_torque_bow_controls::Continuity& out) noexcept {
        out={};if(!nativeequipment::current(nullptr,f))return false;
        gears_torque_bow_controls::Continuity candidate{};
        if(!nativeequipment::read_input_continuity(f,candidate.current,candidate.publisher,nullptr,nullptr,&candidate.history)||
           !nativeequipment::current(nullptr,f))return false;
        out=candidate;return true;
    }
    bool observe(hand::Frame const& f,hand::GripGameState const& game,driver_api::Snapshot& out) noexcept {
        last_known=false;observe_gate=1;
        if(!weaponhand::current_gt(nullptr)||!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
           !nativeequipment::current(nullptr,f)||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return false;
        torquebow_state::Observed actual{};
        observe_gate=2;++timing_observations;
        const auto state_begin=now();const bool state_known=torquebow_state::read(f,actual);
        timing_state_ns+=now()-state_begin;if(!state_known)return false;
        observe_gate=3;const auto snapshot_begin=now();const bool snapshot_known=snapshot(f,actual,out);
        timing_snapshot_ns+=now()-snapshot_begin;if(!snapshot_known)return false;
        observe_gate=4;if(!nativeequipment::current(nullptr,f))return false;
        last=out;last_source=f;last_known=true;observe_gate=0;return true;
    }
    bool configure(hand::Frame const& f,hand::GripGameState const&,driver_api::Snapshot const& observed_state,
        gears_weapon::TorqueBowConfig& config) noexcept {
        configure_gate=1;if(config_serial==UINT64_MAX)return false;
        configure_gate=2;if(!geometry_current(f))return false;
        configure_gate=3;if(feed_calibration.instance_serial!=config_serial)return false;
        configure_gate=4;if(!observed_state.original.ready)return false;
        configure_gate=5;if(!torquebow_input_fence::available())return false;
        configure_gate=6;if(!torquebow_cleanup_context::current(parent(f)))return false;
        torquebow_original::Preparation prepared{};torquebow_buttons::Entry press_entry{},release_entry{};
        configure_gate=7;if(!torquebow_original::prepare(f,prepared))return false;
        configure_gate=8;if(!torquebow_buttons::bind(f.controller,true,press_entry))return false;
        configure_gate=9;if(!torquebow_buttons::bind(f.controller,false,release_entry))return false;
        configure_gate=10;
        for(auto method:{torquebow_cancel_live::Method::clear_pending,torquebow_cancel_live::Method::charge_ended,
                       torquebow_cancel_live::Method::active,torquebow_cancel_live::Method::put_down}){
            torquebow_cancel_live::Entry entry{};if(!torquebow_cancel_live::bind_entry(f.weapon.address,method,entry))return false;
        }
        Name active{};bool tick_probe{};
        configure_gate=11;
        if(!torquebow_cancel_live::active_name(f.weapon.address,active))return false;
        configure_gate=12;if(!torquebow_state::tick_enabled(f.weapon.address,tick_probe))return false;
        configure_gate=13;if(!torquebow_cleanup_context::current(parent(f)))return false;
        configure_gate=14;if(!nativeequipment::current(nullptr,f))return false;
        config={};config.identity={f.world.address,f.pawn.address,f.controller.address,f.weapon.address,f.key.world_generation,
            f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch};
        config.instance_config_serial=config_serial++;config.capacity=1;config.game_ticks_per_second=1000000;
        config.original_class_verified=config.original_charge_entry_verified=config.original_end_fire_verified=config.non_firing_abort_verified=true;
        config.manual_reload_phase_verified=config.reload_plan_verified=true;
        config.reload_step_count=1;config.reload_steps[0]={gears_weapon::TorqueBowReloadAction::insert_arrow,feed_calibration.instance_serial};
        configure_gate=0;return true;
    }
    bool prepare(hand::Frame const& f,hand::GripGameState const& game) noexcept {
        ++preparation_attempts;preparation_gate=1;
        if(torquebow_fire::ready_for(f,20000000)&&torquebow_fire::reload_veto_bound(f)){
            preparation_gate=2;const bool accepted=torquebow_input_fence::update(f,true);if(accepted)preparation_gate=0;return accepted;
        }
        preparation_gate=3;lancerreload_manual::prepare(f);
        preparation_gate=4;nativelancerfollow::tick(f,nativelancerfollow::Phase::prepare_only);
        preparation_gate=5;if(!torquebow_fire::prepare(f,game))return false;
        preparation_gate=6;if(!torquebow_input_fence::update(f,true))return false;
        preparation_gate=7;if(!torquebow_fire::ready_for(f,20000000))return false;
        preparation_gate=0;return true;
    }
    bool guard(hand::Frame const& f) noexcept {
        return torquebow_input_owner::same_hold(f)&&torquebow_input_fence::update(f,true);
    }
    Result press(hand::Frame const& f) noexcept {
        ++press_attempts;press_reserved=torquebow_input_owner::reserve_before_press(f);
        press_result=press_reserved?torquebow_input_owner::press(f):Result::rejected;return press_result;
    }
    Result release(hand::Frame const& f) noexcept {return torquebow_input_owner::normal_release(f);}
    bool read_owned(driver_api::Snapshot& out) noexcept {
        torquebow_state::Observed actual{};
        if(!torquebow_input_owner::read(actual)||!snapshot(torquebow_input_owner::journal.source,actual,out))return false;
        last=out;return true;
    }
    void fence() noexcept {
        const auto serial=torquebow_input_owner::serial();
        if(serial)torquebow_input_owner::stop_local(serial);else torquebow_fire::clear_source();
    }
    bool cancel(gears_weapon::TorqueBowCancelRoute route,driver_api::Snapshot& out) noexcept {
        if(!owned()){
            // No original input call can be replayed/adopted here. Only the
            // exact prior observed neutral/no-call result can finish a request.
            if(has_spawn()||!last.fully_stopped)return false;out=last;out.original.ready=false;return true;
        }
        driver_api::Snapshot before{};
        if(!read_owned(before))return false;
        torquebow_input_owner::StopReceipt receipt{};
        if(!torquebow_input_owner::cancel(route,receipt)||!receipt.complete)return false;
        out=before;out.original={};out.fully_stopped=true;
        if(receipt.after.capacity){
            if(receipt.after.capacity!=1||receipt.after.used<0||receipt.after.used>1||receipt.after.reserve<0||receipt.after.infinite)return false;
            out.loaded=unsigned(1-receipt.after.used);out.reserve=unsigned(receipt.after.reserve);
        }
        last=out;return true;
    }
    bool settle(driver_api::Snapshot& out) noexcept {
        driver_api::Snapshot actual{};
        if(!read_owned(actual)||!actual.fully_stopped||has_spawn()||!torquebow_input_owner::retire_neutral())return false;
        out=actual;last=out;return true;
    }
    driver_api::Shot account(gears_weapon::TorqueBowInteraction& model,gears_weapon::TorqueBowStamp const& stamp,
        std::uint64_t serial,gears_weapon::TorqueBowEvent& event) noexcept {
        using S=torquebow_input_owner::ShotStatus;
        torquebow_fire::SpawnReceipt observed_spawn{};const bool had_spawn=torquebow_fire::peek_spawn(observed_spawn);
        const auto previous_projectile=model.last_original_projectile_serial();const auto previous_inventory=model.inventory();
        const auto accounted=torquebow_input_owner::record_original_shot(model,stamp,serial,event);
        // Durable evidence only AFTER the existing model accepts the exact
        // original returned projectile and its one-arrow inventory delta.
        // Queue-retirement retries leave that serial unchanged, so cannot
        // duplicate this row. No callback I/O or new projectile ownership.
        static unsigned shot_rows{};
        if(log_file&&shot_rows<16&&had_spawn&&model.last_original_projectile_serial()!=previous_projectile&&
           model.last_original_projectile_serial()==observed_spawn.shot){
            ++shot_rows;const auto after=model.inventory();
            fwprintf(log_file,L"torquebow_original_shot weapon=%llx world_generation=%llu input_serial=%llu call=%llu shot=%llu projectile=%llx projectile_class=%llx loaded_before=%u loaded_after=%u reserve_before=%u reserve_after=%u event=%u original_return_and_inventory=1 normal_gt=1\n",
                observed_spawn.source.weapon.address,observed_spawn.source.key.world_generation,observed_spawn.input_serial,
                observed_spawn.call,observed_spawn.shot,observed_spawn.projectile.address,observed_spawn.projectile_class.address,
                previous_inventory.loaded,after.loaded,previous_inventory.reserve,after.reserve,unsigned(event));fflush(log_file);
        }
        switch(accounted){
            case S::none:return driver_api::Shot::none;
            case S::recorded:return driver_api::Shot::recorded;
            case S::waiting_for_ammo:case S::receipt_retirement:return driver_api::Shot::waiting;
            case S::unavailable:return driver_api::Shot::unavailable;
            default:return driver_api::Shot::conflict;
        }
    }
};
Backend backend;driver_api::Driver driver;
// Bounded packet evidence at the two EXISTING dispatch boundaries. This does
// not decode, renew, replay, publish or authorize input.
void trace_packet(hand::Frame const& f,unsigned phase,bool available) noexcept {
    static gears_vr::ControllerInputFrame previous{};static gears_vr::StereoSample previous_stereo{};
    static bool known{};static unsigned missing_rows{},repeat_rows{},edge_rows{},first_rows{};
    if(!log_file)return;
    const auto difference=[](void const* a,void const* b,std::size_t bytes) noexcept {
        const auto x=static_cast<unsigned char const*>(a),y=static_cast<unsigned char const*>(b);
        for(std::size_t i=0;i<bytes;++i)if(x[i]!=y[i])return unsigned(i);return UINT_MAX;
    };
    const auto& right=f.controllers.hands[1];
    const auto dc=known&&available?difference(&previous,&f.controllers,sizeof(previous)):UINT_MAX;
    const auto ds=known&&available?difference(&previous_stereo,&f.stereo,sizeof(previous_stereo)):UINT_MAX;
    const bool edge=available&&known&&(right.pressed[gears_vr::control_trigger]!=previous.hands[1].pressed[gears_vr::control_trigger]||
        right.released[gears_vr::control_trigger]!=previous.hands[1].released[gears_vr::control_trigger]||right.trigger!=previous.hands[1].trigger);
    const bool changed_repeat=available&&known&&dc==UINT_MAX&&ds!=UINT_MAX;
    if((!available&&missing_rows++<16)||(changed_repeat&&repeat_rows++<8)||(edge&&edge_rows++<16)||(available&&!known&&first_rows++<1)){
        fwprintf(log_file,L"torquebow_packet phase=%u available=%u bound=%u pending=%u frame=%llu sequence=%llu previous_sequence=%llu sync_ns=%lld previous_sync_ns=%lld input_epoch=%llu profile=%llu trigger=%.6f trigger_active=%u pressed=%u released=%u stick_active_mask=%u controller_difference=%u stereo_difference=%u stereo_received_ns=%lld previous_stereo_received_ns=%lld observation_only=1\n",
            phase,unsigned(available),unsigned(driver.bound()),unsigned(driver.pending()||backend.owned()||backend.reload_owned()),
            f.key.frame,f.controllers.sequence,previous.sequence,f.controllers.sync_ns,previous.sync_ns,f.controllers.input_epoch,f.controllers.profile_generation,
            double(right.trigger),right.trigger_active,right.pressed[gears_vr::control_trigger],right.released[gears_vr::control_trigger],right.buttons_active,dc,ds,
            f.stereo.received_ns,previous_stereo.received_ns);fflush(log_file);
    }
    if(available){previous=f.controllers;previous_stereo=f.stereo;known=true;}
}
void trace_boundary(hand::Frame const& source,unsigned phase,driver_api::Status status) noexcept {
    static unsigned rows{};static std::uint64_t losses{};
    const auto d=driver.diagnostic();
    if(!log_file||rows>=32||d.losses==losses)return;
    losses=d.losses;++rows;
    fwprintf(log_file,L"torquebow_boundary phase=%u frame=%llu sequence=%llu status=%u losses=%llu reason=%u decoded=%u decode_sequence=%llu decode_now=%lld sync_ns=%lld received_ns=%lld decoder_rejection=%u previous_known=%u previous_sequence=%llu previous_sync_ns=%lld observation_only=1\n",
        phase,source.key.frame,source.controllers.sequence,unsigned(status),d.losses,d.reason,d.decoded,d.sequence,d.decode_now,d.sync_ns,d.received_ns,d.decoder_rejection,unsigned(d.decoder_previous_known),d.decoder_previous_sequence,d.decoder_previous_sync_ns);fflush(log_file);
}
void trace_dispatch(hand::Frame const& source,unsigned phase) noexcept {
    static unsigned rows{},previous_preparations{},previous_presses{};static std::uint32_t previous_down{},previous_up{};
    const auto& right=source.controllers.hands[1];const auto down=right.pressed[gears_vr::control_trigger],up=right.released[gears_vr::control_trigger];
    static std::uint64_t previous_waits{};const auto waits=driver.diagnostic().same_frame_begin_waits;
    const bool changed=previous_waits!=waits||previous_preparations!=backend.preparation_attempts||previous_presses!=backend.press_attempts||down!=previous_down||up!=previous_up;
    previous_waits=waits;previous_preparations=backend.preparation_attempts;previous_presses=backend.press_attempts;previous_down=down;previous_up=up;
    if(!log_file||!changed||rows>=8)return;++rows;const auto diagnostic=driver.diagnostic();
    fwprintf(log_file,L"torquebow_dispatch phase=%u frame=%llu sequence=%llu decoded=%u stage=%u preparation_attempts=%u preparation_gate=%u press_attempts=%u press_reserved=%u press_result=%u original_sync_ns=%lld same_frame_begin_waits=%llu history_verified_decodes=%llu decoded_count=%u model_observation=%u model_edge=%u model_edge_ns=%lld model_can_fire=%u fire_prepare_gate=%u muzzle_prepare_stage=%u observation_only=1\n",
        phase,source.key.frame,source.controllers.sequence,diagnostic.decoded,unsigned(driver.stage()),backend.preparation_attempts,backend.preparation_gate,
        backend.press_attempts,unsigned(backend.press_reserved),unsigned(backend.press_result),source.controllers.sync_ns,
        diagnostic.same_frame_begin_waits,diagnostic.history_verified_decodes,diagnostic.decoded_count,diagnostic.model_observation,diagnostic.model_edge,diagnostic.model_edge_ns,unsigned(diagnostic.model_can_fire),
        torquebow_fire::prepare_gate,unsigned(unified_weapon_fire::prepare_stage));fflush(log_file);
    if(backend.preparation_attempts&&torquebow_fire::prepare_gate==7){
        const auto a=torquebow_muzzle_clear::query_start,b=torquebow_muzzle_clear::query_end;
        fwprintf(log_file,L"torquebow_clearance frame=%llu sequence=%llu gate=%u called=%u clear=%u start=%.9g,%.9g,%.9g end=%.9g,%.9g,%.9g observation_only=1\n",
            source.key.frame,source.controllers.sequence,torquebow_muzzle_clear::query_gate,unsigned(torquebow_muzzle_clear::query_called),unsigned(torquebow_muzzle_clear::query_clear),
            a.x,a.y,a.z,b.x,b.y,b.z);fflush(log_file);
        const auto& o=torquebow_muzzle_clear::query_schema;
        fwprintf(log_file,L"torquebow_clearance_schema frame=%llu sequence=%llu gate=%u seen=%u children=%u bytes=%u native=%u flags=%x return_offset=%u offsets=%u,%u,%u,%u,%u type_gate=%u field_index=%u field_flags=%llx element_bytes=%u mask_read=%u field_mask=%x observation_only=1\n",
            source.key.frame,source.controllers.sequence,unsigned(o.gate),o.seen,o.children,o.bytes,o.native,o.flags,o.return_offset,
            o.offsets[0],o.offsets[1],o.offsets[2],o.offsets[3],o.offsets[4],unsigned(o.type_gate),o.field_index,
            static_cast<unsigned long long>(o.field_flags),o.element_bytes,o.mask_read,o.mask);fflush(log_file);
    }
}
void trace_timing(hand::Frame const& f,unsigned phase,std::int64_t begin,std::int64_t copied,
    std::int64_t driver_begin,std::int64_t driver_end,std::int64_t end) noexcept {
    static std::array<unsigned,2> rows{};static std::array<std::uint32_t,2> down{},up{};
    if(phase>1||!driver.bound())return;
    const auto& right=f.controllers.hands[1];
    const bool edge=down[phase]!=right.pressed[gears_vr::control_trigger]||up[phase]!=right.released[gears_vr::control_trigger];
    down[phase]=right.pressed[gears_vr::control_trigger];up[phase]=right.released[gears_vr::control_trigger];
    if(!log_file||(!edge&&rows[phase]>=12)||rows[phase]>=20)return;++rows[phase];
    fwprintf(log_file,L"torquebow_timing phase=%u frame=%llu sequence=%llu begin_ns=%lld end_ns=%lld copy_ns=%lld before_driver_ns=%lld driver_ns=%lld after_driver_ns=%lld state_calls=%u state_ns=%lld snapshot_ns=%lld source_sync_ns=%lld source_age_end_ns=%lld observation_only=1\n",
        phase,f.key.frame,f.controllers.sequence,begin,end,copied-begin,driver_begin-copied,driver_end-driver_begin,end-driver_end,
        backend.timing_observations,backend.timing_state_ns,backend.timing_snapshot_ns,f.controllers.sync_ns,end-f.controllers.sync_ns);fflush(log_file);
}
void report_driver(hand::Frame const& source,driver_api::Status status) noexcept {
    // Bounded normal-GT outcome transitions. Historical receipts are never
    // turned into new input or a new success flag by this observer.
    static unsigned rows{};static std::array<std::uint64_t,19> previous{};
    auto const& model=driver.interaction();const auto inventory=model.inventory();
    const std::array<std::uint64_t,19> state{{unsigned(status),unsigned(driver.stage()),unsigned(driver.bound()),
        model.last_loaded_token(),model.last_confirmed_cancel_edge(),model.last_original_projectile_serial(),
        backend.observe_gate,backend.configure_gate,backend.warm_result,unsigned(backend.geometry_known),torquebow_arrow_visual::owner.handle.generation,
        torquebow_state::read_gate,std::uint64_t(torquebow_state::pending_count),std::uint64_t(torquebow_state::pending_capacity),
        torquebow_arrow_appearance::read_gate,torquebow_arrow_appearance::material_gate,torquebow_arrow_source::asset_gate,
        torquebow_arrow_source::attributes_gate,torquebow_arrow_source::parent_gate}};
    if(!log_file||rows>=64||state==previous)return;previous=state;++rows;
    fwprintf(log_file,L"torquebow_driver frame=%llu sequence=%llu status=%u stage=%u bound=%u loaded=%u reserve=%u loaded_token=%llu cancel_edge=%llu projectile_serial=%llu original_owned=%u reload_owned=%u input_age_ns=%lld normal_gt=1\n",
        source.key.frame,source.controllers.sequence,unsigned(status),unsigned(driver.stage()),unsigned(driver.bound()),inventory.loaded,inventory.reserve,
        state[3],state[4],state[5],unsigned(backend.owned()),unsigned(backend.reload_owned()),backend.now()-source.controllers.sync_ns);fflush(log_file);
    fwprintf(log_file,L"torquebow_binding frame=%llu observe_gate=%u configure_gate=%u warm_result=%u geometry_known=%u arrow_generation=%llu arrow_lit=%u config_serial=%llu feed_serial=%llu original_ready=%u stopped=%u normal_gt=1\n",
        source.key.frame,backend.observe_gate,backend.configure_gate,backend.warm_result,unsigned(backend.geometry_known),state[10],
        unsigned(torquebow_arrow_visual::owner.lit),backend.config_serial,backend.feed_calibration.instance_serial,
        unsigned(backend.last.original.ready),unsigned(backend.last.fully_stopped));fflush(log_file);
    fwprintf(log_file,L"torquebow_snapshot frame=%llu gate=%u game_clock_gate=%u capacity=%u used=%lld reserve=%lld infinite=%u observation_only=1\n",
        source.key.frame,snapshot_gate,game_clock_gate,unsigned(snapshot_counts.capacity),static_cast<long long>(snapshot_counts.used),
        static_cast<long long>(snapshot_counts.reserve),unsigned(snapshot_counts.infinite));fflush(log_file);
    fwprintf(log_file,L"torquebow_state_read frame=%llu gate=%u failed_field=%ls pending_count=%d pending_capacity=%d normal_gt=1\n",
        source.key.frame,torquebow_state::read_gate,torquebow_state::failed_field,torquebow_state::pending_count,torquebow_state::pending_capacity);fflush(log_file);
    fwprintf(log_file,L"torquebow_appearance_read frame=%llu gate=%u material_gate=%u asset_gate=%u attributes_gate=%u parent_gate=%u normal_gt=1\n",
        source.key.frame,torquebow_arrow_appearance::read_gate,torquebow_arrow_appearance::material_gate,
        torquebow_arrow_source::asset_gate,torquebow_arrow_source::attributes_gate,torquebow_arrow_source::parent_gate);fflush(log_file);
}
gears_weapon::TorqueBowArrowFeed arrow_feed;
std::uint64_t feed_arrow{},feed_sequence{},feed_begin{};std::int64_t feed_sync{};
void reset_feed() noexcept {arrow_feed.reset();feed_arrow=feed_sequence=feed_begin=0;feed_sync=0;}
void held_arrow(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& decision) noexcept {
    const auto arrow=driver.interaction().held_arrow_serial();
    if(driver.stage()!=gears_weapon::TorqueBowStage::reload_steps||!arrow){
        reset_feed();torquebow_arrow_visual::withdraw();return;
    }
    const auto refuse=[&]() noexcept {reset_feed();torquebow_arrow_visual::withdraw();};
    if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       !backend.geometry_current(f)||!torquebow_reload_live::ammo_grant(f,decision)||!nativeequipment::current(nullptr,f)){
        refuse();return;
    }
    gears_committed_interaction::Frame avatar{};gears_rig_tracking::Frame raw{};
    torquebow_arrow_appearance::Appearance appearance{};
    gears_torque_bow_arrow_tracking::Sample arrow_pose{};gears_torque_bow_arrow_tracking::RootSample root{};
    if(!handrig::read_interaction(avatar)||!torquebow_arrow_appearance::read(f,avatar,appearance)){
        refuse();return;
    }
    if(avatar.model_scale!=backend.geometry_avatar_scale||!torquebow_arrow_appearance::same(appearance,backend.geometry_appearance)){
        backend.geometry_known=false;driver.stop(backend,gears_weapon::TorqueBowCancelRoute::active);
        reset_feed();torquebow_arrow_visual::retire();return;
    }
    if(
       nativeequipment_rig::acquire(f,raw).status!=nativeequipment_rig::Status::ready||
       !gears_torque_bow_arrow_tracking::sample(f,raw,backend.arrow_mount,backend.now(),arrow_pose)||
       !gears_torque_bow_arrow_tracking::root_sample(f,raw,nativelancerfollow::mount,backend.now(),root)||
       std::abs(root.scale-backend.geometry_bow_scale)>1e-6||!nativeequipment::current(nullptr,f)){
        refuse();return;
    }
    // The recognizer uses the original controller sync, never GT time or a
    // displayed/snapped palm. Both hands are authenticated by the SAME Rig.
    if(feed_arrow!=arrow){
        reset_feed();
        if(!arrow_feed.arm(f.weapon.address,arrow,f.world.address,f.controllers.space_epoch,backend.feed_calibration))return;
        feed_arrow=arrow;
    }
    if(feed_sequence!=f.controllers.sequence||feed_sync!=f.controllers.sync_ns){
        if(feed_sequence&&(f.controllers.sequence<=feed_sequence||f.controllers.sync_ns<=feed_sync)){refuse();return;}
        const double units=raw.units_per_metre;
        auto gun=root.world,left=raw.world_grip[0];
        gun.position=gears_torque_bow_arrow_mount::mul(gun.position,1/units);
        left.position=gears_torque_bow_arrow_mount::mul(left.position,1/units);
        const gears_weapon::FeedSample sample{f.weapon.address,arrow,f.world.address,f.controllers.space_epoch,
            f.controllers.sequence,f.controllers.sync_ns,gun,left,true,true};
        const auto result=arrow_feed.observe_raw(sample,backend.feed_calibration);
        feed_sequence=f.controllers.sequence;feed_sync=f.controllers.sync_ns;
        if(result==gears_weapon::FeedResult::approaching&&!feed_begin)feed_begin=f.controllers.sequence;
        if(result==gears_weapon::FeedResult::rejected){
            feed_begin=0;
            // A discontinuity requires a new complete approach. It cannot
            // resume an earlier partial stroke or complete from proximity.
            arrow_feed.arm(f.weapon.address,arrow,f.world.address,f.controllers.space_epoch,backend.feed_calibration);
        }
        if(result==gears_weapon::FeedResult::inserted){
            const auto serial=backend.feed_calibration.instance_serial;
            const driver_api::ReloadMotion motion{serial,torquebow_reload_live::owner.original_serial,serial,arrow,feed_begin,
                true,true,true,true,true};
            const bool accepted=driver.complete_reload_motion(backend,f,game,motion);
            reset_feed();torquebow_arrow_visual::withdraw();
            if(accepted)return;
            // An unknown original completion stays in its existing journal;
            // this consumed geometric stroke is never replayed as a receipt.
            return;
        }
    }
    gears_torque_bow_arrow_tracking::Sample visible{};
    const gears_torque_bow_arrow_presentation::Config guide{backend.feed_calibration,nativelancerfollow::mount,arrow};
    gears_torque_bow_arrow_presentation::Result displayed{};
    if(torquebow_arrow_visual::show(f,avatar,decision,driver,backend.arrow_mount,visible,&guide,&displayed))
        torquebow_grasp::left_part(f,displayed.feed_assisted?gears_torque_bow_grasp_pose::Part::arrow_feed:
            gears_torque_bow_grasp_pose::Part::arrow_shaft,displayed.grip_world,false);
}
bool selected() noexcept {
    const auto candidate=uptr(vrequip::model.held_weapon());
    return candidate&&playground::instance(candidate,L"Weap_Bow_base");
}
bool pending() noexcept {return driver.pending()||torquebow_input_owner::pending()||torquebow_reload_live::pending();}
bool copy(uptr pc,hand::Frame& f,hand::GripGameState& game,hand::Decision* out=nullptr) noexcept {
    hand::Decision decision{};
    const bool copied=weaponhand::current_gt(nullptr)&&nativeequipment::domain_ready()&&
        weaponhand::client.copy_frame_on_gt(f,game,decision)&&f.controller.address==pc&&
        f.weapon.address==vrequip::model.held_weapon()&&playground::instance(f.weapon.address,L"Weap_Bow_base");
    if(out)*out=decision;return copied;
}
bool tick(uptr pc,bool running) noexcept {
    if(!weaponhand::current_gt(nullptr)||(!selected()&&!pending()&&!torquebow_arrow_visual::pending()))return false;
    if(driver.controller()&&driver.controller()!=pc)return false;
    if(torquebow_arrow_visual::controller()&&torquebow_arrow_visual::controller()!=pc)return false;
    if(!running||!selected()){
        torquebow_feedback::lost();torquebow_guide::lost();
        driver.stop(backend,running?gears_weapon::TorqueBowCancelRoute::put_down:gears_weapon::TorqueBowCancelRoute::active);
        torquebow_arrow_visual::retire();
        if(!pending())driver.reset_drained(backend);return true;
    }
    const auto timing_begin=backend.now();backend.reset_timing();
    hand::Frame source{};hand::GripGameState game{};hand::Decision decision{};
    const bool available=copy(pc,source,game,&decision);trace_packet(source,1,available);
    const auto timing_copied=backend.now();
    if(!available){
        torquebow_feedback::lost();torquebow_guide::lost();
        driver.stop(backend,gears_weapon::TorqueBowCancelRoute::active);torquebow_arrow_visual::withdraw();return true;
    }
    if(game.known&&game.main_hand_held&&!game.busy&&!game.animation_owns_hands)torquebow_grasp::main_hand(source);
    if(driver.stage()==gears_weapon::TorqueBowStage::interrupted&&!pending())driver.reset_drained(backend);
    if(driver.bound()&&!backend.geometry_current(source)){
        torquebow_feedback::lost();torquebow_guide::lost();
        driver.stop(backend,gears_weapon::TorqueBowCancelRoute::active);reset_feed();torquebow_arrow_visual::withdraw();
        if(!pending())driver.reset_drained(backend);return true;
    }
    const auto timing_driver_begin=backend.now();
    const auto driven=driver.tick(backend,source,game);const auto timing_driver_end=backend.now();trace_boundary(source,1,driven);trace_dispatch(source,1);
    torquebow_feedback::sample(source,game,decision,driven==driver_api::Status::observed||driven==driver_api::Status::baseline);
    if(running&&driver.bound()){
        if(driver.interaction().can_begin_reload(backend.now())&&decision.new_grant&&
           torquebow_reload_live::ammo_grant(source,decision)&&shoulder(source))driver.begin_reload(backend,source,game,decision);
        if(driver.stage()==gears_weapon::TorqueBowStage::reload_steps){
            if(!source.left_grip_down)driver.release_reload_arrow(backend,source);
            else if(!driver.interaction().held_arrow_serial()&&decision.new_grant&&shoulder(source))driver.regrab_reload_arrow(backend,source,decision);
        }
        held_arrow(source,game,decision);
    }
    if(!backend.reload_owned()&&!driver.interaction().held_arrow_serial()&&
       driver.stage()!=gears_weapon::TorqueBowStage::interrupted)torquebow_grasp::support_hand(source,game,decision);
    torquebow_guide::tick(source,game,driven==driver_api::Status::observed||driven==driver_api::Status::baseline||driven==driver_api::Status::owned);
    report_driver(source,driven);
    trace_timing(source,1,timing_begin,timing_copied,timing_driver_begin,timing_driver_end,backend.now());
    return true;
}
bool before_original_tick(uptr pc) noexcept {
    if(!weaponhand::current_gt(nullptr)||(!selected()&&!pending())||(driver.controller()&&driver.controller()!=pc))return false;
    const auto timing_begin=backend.now();backend.reset_timing();
    hand::Frame source{};hand::GripGameState game{};
    const bool available=selected()&&copy(pc,source,game);trace_packet(source,0,available);
    const auto timing_copied=backend.now();
    if(!available){driver.stop(backend,gears_weapon::TorqueBowCancelRoute::active);return true;}
    const auto driven=driver.before_original_tick(backend,source,game);const auto timing_driver_end=backend.now();
    trace_boundary(source,0,driven);trace_dispatch(source,0);
    trace_timing(source,0,timing_begin,timing_copied,timing_copied,timing_driver_end,backend.now());return true;
}
bool yield_for_equipment(hand::Frame const& source) noexcept {
    torquebow_feedback::lost();torquebow_guide::lost();
    // Fence/settle an owned original input BEFORE any display cleanup may
    // dispatch its own Engine component calls. A visual is never a fire debt.
    if(pending()){
        if(!weaponhand::current_gt(nullptr)||driver.controller()!=source.controller.address)return false;
        driver.stop(backend,gears_weapon::TorqueBowCancelRoute::put_down);
        if(pending()||!driver.reset_drained(backend))return false;
    }
    if(torquebow_arrow_visual::pending()){
        if(!weaponhand::current_gt(nullptr)||torquebow_arrow_visual::controller()!=source.controller.address||
           !torquebow_arrow_visual::retire())return false;
    }
    return true;
}
}
