// First playable checkpoint only: original shot -> physical lever -> next shot.
// Reload routes additionally require real feed geometry and a matching round
// mount. The current authored installer supplies phase1 geometry only.
namespace gnasher_runtime {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace weapon=gears_weapon;namespace vr=gears_vr;namespace rig=gears_rig_tracking;
namespace tracks=weapon::gnasher_tracks;
struct Geometry {
    hand::ObjectIdentity component{},asset{};
    weapon::GnasherMechanicalConfig mechanical{};
    weapon::WeaponMount support{};weapon::SupportBox box{};
    gears_gnasher_round_pose::Mount round{};
    double units{},component_scale{},acquire{},release{};
    std::uint64_t curve_content{},timing_evidence{},contact_evidence{},feed_evidence{};
    tracks::Timing timing{tracks::Timing::uncertified};
    bool valid() const noexcept {
        if(!hand::identity_valid(component)||!hand::identity_valid(asset)||!mechanical.valid()||
           mechanical.game_ticks_per_second!=1000000||!support.valid()||
           (mechanical.feed_calibrated?(!feed_evidence||!round.matches_feed(mechanical)):
               (feed_evidence||round.calibration_id||round.reference_generation))||
           !box.enabled||!box.valid()||!vr::stereo_same_pose(support.right_grip_in_weapon,vr::Pose{})||
           !curve_content||!timing_evidence||!contact_evidence||
           (timing!=tracks::Timing::keys&&timing!=tracks::Timing::keys_minus_one)||
           !std::isfinite(units)||units<10||units>500||!std::isfinite(component_scale)||component_scale<=.1||component_scale>=10||
           !std::isfinite(acquire)||!std::isfinite(release)||acquire<=0||acquire>.12||release<acquire||release>.3)return false;
        // The route is in raw right-grip coordinates, not a different gun basis.
        const auto relative=weapon::sum(mechanical.right_contact_in_grip,weapon::rotate(
            vr::inverse(mechanical.right_grip_home_in_main),weapon::difference(
                mechanical.lever.left_front_grip_in_main.position,mechanical.lever.right_handle_home_in_main)));
        return weapon::length(weapon::difference(support.support_grip_in_weapon,{relative.y,relative.z,-relative.x}))<1e-8;
    }
};
struct Profile {
    Geometry geometry{};inv::Identity identity{};hand::Frame source{};bool known{};
    std::uint64_t retained_shot{};bool retained_down{};
} profile;
std::array<Profile,16> saved_profiles{};
struct Route {hand::Frame source{};hand::GripGameState before{},after{};bool known{};} proposal,publication;
struct Main {hand::Frame source{};vr::Pose component{};std::uint64_t rig_publication{};bool known{};} main;
gnasher_curve::Curve curve{};gnasher_cycle::Reference closed{};vr::Pose main_in_component{};
gnasher_parts::FrameGeometry observed_geometry{};bool basis_known{};
std::uint64_t animation_serial{},next_animation{1},shot{},last_sequence{},last_frame{};
bool curve_known{},down_seen{},main_required{};
hand::Frame decision_source{};hand::Decision committed_decision{};bool decision_known{};
hand::FrameKey route_deferred_key{};

#include "weapon_gnasher_profile_cache.inl"

// Bounded first-failure observations for the normal instance bootstrap.
// These read existing local state only; no engine query or source renewal.
void bootstrap_trace(hand::Frame const& f,char const* stage,double first=-1,double second=-1,double third=-1) noexcept {
    const auto* e=gnasher_fire::entry(f,false);if(!e||!log_file)return;
    static char const* previous{};static std::uint64_t previous_weapon{};static unsigned rows{};
    if(rows>=16||(previous==stage&&previous_weapon==f.weapon.address))return;
    previous=stage;previous_weapon=f.weapon.address;++rows;
    const auto now=weaponhand::clock_ns(nullptr);
    const auto age=f.controllers.sync_ns>0&&now>=f.controllers.sync_ns?now-f.controllers.sync_ns:-1;
    fwprintf(log_file,L"gnasher_bootstrap stage=%hs frame=%llu weapon=%llx profile=%u entry_prepared=%u mechanical=%u cycle_blocked=%u sync_age_ns=%lld first=%.9g second=%.9g third=%.9g\n",
        stage,f.key.frame,f.weapon.address,unsigned(profile.known),unsigned(e->prepared),unsigned(e->mechanical.stage()),
        unsigned(e->cycle.blocked()),age,first,second,third);fflush(log_file);
}

bool context(hand::Frame const& f) noexcept {
    const auto& s=profile.source;
    return profile.known&&inv::same_instance_across_hold(profile.identity,inv::identity(f))&&
        hand::same(s.controller,f.controller)&&s.stereo.session==f.stereo.session&&s.stereo.space_epoch==f.stereo.space_epoch&&
        s.controllers.input_epoch==f.controllers.input_epoch&&s.controllers.profile_generation==f.controllers.profile_generation&&
        s.reference_generation==f.reference_generation&&hand::frame_active(f,weaponhand::clock_ns(nullptr));
}
bool live_asset(hand::Frame const& f) noexcept {
    if(!context(f))return false;
    lancermain_asset::Evidence actual{};lancermain_asset::HeldAsset kind{};
    return lancermain_asset::read(f,actual,lancermain_asset::AssetScope::basic_hold,&kind)==lancermain_asset::Status::ready&&
        kind==lancermain_asset::HeldAsset::gnasher&&hand::same(actual.component,profile.geometry.component)&&
        hand::same(actual.asset,profile.geometry.asset)&&nativelancerfollow::bound_key_known&&
        hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        hand::same(nativelancerfollow::bound_key.component,actual.component)&&hand::same(nativelancerfollow::bound_key.asset,actual.asset)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
        nativelancerfollow::bound_units==profile.geometry.units&&nativelancerfollow::bound_session==f.stereo.session&&
        nativelancerfollow::bound_space==f.stereo.space_epoch&&nativelancerfollow::bound_input==f.controllers.input_epoch&&
        nativelancerfollow::bound_reference==f.reference_generation&&nativelancerfollow::bound_world==f.key.world_generation;
}
bool same_route(hand::GripGameState const& a,hand::GripGameState const& b) noexcept {
    const auto vector=[](vr::Vector p,vr::Vector q){return p.x==q.x&&p.y==q.y&&p.z==q.z;};
    const bool mount=vr::stereo_same_pose(a.mount.right_grip_in_weapon,b.mount.right_grip_in_weapon)&&
        vector(a.mount.support_grip_in_weapon,b.mount.support_grip_in_weapon)&&
        vector(a.mount.muzzle_in_weapon,b.mount.muzzle_in_weapon)&&vector(a.mount.bore_forward_in_weapon,b.mount.bore_forward_in_weapon)&&
        a.mount.support_length_tolerance_metres==b.mount.support_length_tolerance_metres&&
        a.mount.maximum_support_correction_radians==b.mount.maximum_support_correction_radians;
    return a.known==b.known&&a.main_hand_held==b.main_hand_held&&a.busy==b.busy&&a.animation_owns_hands==b.animation_owns_hands&&a.kind==b.kind&&
        a.mount_known==b.mount_known&&hand::same(a.mount_asset,b.mount_asset)&&a.mount_revision==b.mount_revision&&
        mount&&a.support_box==b.support_box&&a.acquire_metres==b.acquire_metres&&a.release_metres==b.release_metres&&
        a.request_ammo==b.request_ammo&&a.request_magazine==b.request_magazine&&a.magazine_observation_pending==b.magazine_observation_pending&&
        hand::same(a.object_incumbents[0],b.object_incumbents[0])&&hand::same(a.object_incumbents[1],b.object_incumbents[1]);
}
#include "weapon_gnasher_source_route.inl"
bool route_asset(hand::Frame const& f) noexcept;
bool project_source_observation(hand::Frame const& f,hand::GripGameState& game) noexcept {
    const bool route_current=context(f)&&profile_route_ready(f);unsigned route_failure{};
    const bool route_asset_current=route_current&&route_asset(f);
    const bool route_applied=project_source_route(f,publication.source,publication.before,publication.after,
        decision_source,committed_decision,publication.known&&decision_known,
        route_current,game,&route_failure,route_asset_current);
    // Read existing local failure state only. No engine query, clock renewal,
    // input, proposal or alternate permission path is introduced here.
    static unsigned route_rows{},route_previous{},route_previous_fields{};
    if(!route_applied&&log_file&&route_rows<12&&profile.known&&hand::same(profile.identity.weapon,f.weapon)&&
       f.controllers.hands[1].trigger_active==1&&f.controllers.hands[1].trigger>.35f){
        const auto& prior_route=publication.before;
        unsigned route_fields=unsigned(game.known!=prior_route.known)|
            (unsigned(game.main_hand_held!=prior_route.main_hand_held)<<1)|
            (unsigned(game.busy!=prior_route.busy)<<2)|(unsigned(game.animation_owns_hands!=prior_route.animation_owns_hands)<<3)|
            (unsigned(game.kind!=prior_route.kind)<<4)|(unsigned(game.mount_known!=prior_route.mount_known)<<5)|
            (unsigned(!hand::same(game.mount_asset,prior_route.mount_asset))<<6)|(unsigned(game.mount_revision!=prior_route.mount_revision)<<7)|
            (unsigned(game.acquire_metres!=prior_route.acquire_metres||game.release_metres!=prior_route.release_metres)<<9)|
            (unsigned(!(game.support_box==prior_route.support_box))<<10);
        auto route_mount_probe=prior_route;route_mount_probe.mount=game.mount;
        if(!hand::same_game(route_mount_probe,prior_route))route_fields|=256;
        if(!route_rows||route_failure!=route_previous||route_fields!=route_previous_fields){
            ++route_rows;route_previous=route_failure;route_previous_fields=route_fields;
            fwprintf(log_file,L"gnasher_source_route_rejected stage=%u current=%u fresh_asset=%u publication=%u decision=%u base_difference=%u base_fields=%u frame=%llu publication_frame=%llu source_sequence=%llu publication_sequence=%llu source_now_ns=%lld publication_now_ns=%lld actual_revision=%llu base_revision=%llu route_revision=%llu\n",
                route_failure,unsigned(route_current),unsigned(route_asset_current),unsigned(publication.known),unsigned(decision_known),
                hand::game_difference(game,prior_route),route_fields,f.key.frame,publication.source.key.frame,
                f.controllers.sequence,publication.source.controllers.sequence,f.now_ns,publication.source.now_ns,
                game.mount_revision,prior_route.mount_revision,publication.after.mount_revision);fflush(log_file);
        }
    }
    return route_applied;
}
#include "weapon_gnasher_route_asset.inl"
bool route_asset(hand::Frame const& f) noexcept {
    // Callback-local reflected metadata only: no Args.run, Native query or
    // HandClient.current recursion. The enclosing reader closes player flags.
    if(!context(f)||!nativelancerfollow::bound_key_known||
       !hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       !hand::same(nativelancerfollow::bound_key.component,profile.geometry.component)||
       !hand::same(nativelancerfollow::bound_key.asset,profile.geometry.asset)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       nativelancerfollow::bound_units!=profile.geometry.units||nativelancerfollow::bound_session!=f.stereo.session||
       nativelancerfollow::bound_space!=f.stereo.space_epoch||nativelancerfollow::bound_input!=f.controllers.input_epoch||
       nativelancerfollow::bound_reference!=f.reference_generation||nativelancerfollow::bound_world!=f.key.world_generation)return false;
    return route_asset_metadata(f,profile.geometry.component,profile.geometry.asset)&&context(f);
}
#include "weapon_gnasher_reload_dispatch.inl"
bool round_route_enabled() noexcept {return reload::enabled();}
void propose(hand::Frame const& f,hand::GripGameState& game) noexcept {
    // Asset reads are reflected metadata only; no engine call or state transition.
    if(!profile_route_ready(f)||!route_asset(f)||!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||game.kind!=hand::WeaponKind::firearm)return;
    if(publication.known&&hand::same_frame(publication.source,f)&&same_route(game,publication.before)){game=publication.after;return;}
    const auto before=game;game.mount=profile.geometry.support;game.mount_known=true;
    game.mount_asset=profile.geometry.asset;game.mount_revision=profile.geometry.mechanical.calibration_id;
    game.acquire_metres=profile.geometry.acquire;game.release_metres=profile.geometry.release;game.support_box=profile.geometry.box;
    game.request_magazine=false;game.request_ammo=reload::requests_ammo(f);
    proposal={f,before,game,true};
}
void committed(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(context(f)&&proposal.known&&hand::same_frame(proposal.source,f)&&hand::same(d.key,f.key)){
        publication=proposal;decision_source=f;committed_decision=d;decision_known=true;
    }
}
bool raw(hand::Frame const& f,hand::GripGameState const& game,bool ready,
         rig::Frame& prepared,weapon::GnasherMechanicalSample& sample) noexcept {
    const auto source=static_cast<std::uint32_t>(f.stereo.source);const auto now=weaponhand::clock_ns(nullptr);
    const auto* e=gnasher_fire::entry(f,false);
    hand::GripSnapshot ownership{};
    const auto owned=(!e||gears_gnasher_raw::ownership_required(f,e->mechanical))&&
        weaponhand::client.snapshot_on_gt(ownership);
    float seconds{};
    if(handrig::read(prepared,0,source).reply!=rig::Reply::accepted||prepared.units_per_metre!=profile.geometry.units||
       !lancerreload_state::time_field(f.world.address,L"TimeSeconds",seconds)||!std::isfinite(seconds)||seconds<0||double(seconds)*1000000.>9e15)return false;
    auto item=reload::enabled()&&e?e->mechanical.held_round_serial():0;
    if(item&&!gnasher_round::captured_for(f,item))item=0;
    return gears_gnasher_raw::sample(f,prepared,game,owned?&ownership:nullptr,now,
        std::int64_t(double(seconds)*1000000.),item,game.animation_owns_hands,ready,sample);
}
bool prepare_authored(hand::Frame const& f,std::uint64_t curve_content,tracks::Timing timing,
    std::uint64_t timing_evidence,Geometry& out) noexcept {
    out={};if(!curve_content||!timing_evidence||
       (timing!=tracks::Timing::keys&&timing!=tracks::Timing::keys_minus_one)||
       !nativelancerfollow::bound_key_known||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
       nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
       !nativeequipment::current(nullptr,f)){bootstrap_trace(f,"prepare_context");return false;}
    gnasher_parts::FrameGeometry actual_geometry{};const auto units=nativelancerfollow::bound_units;
    auto* e=gnasher_fire::entry(f,false);inv::Inventory inventory{};
    if(!e||!(e->identity==inv::identity(f))||!e->observer.sample(f,inventory)||!inventory.valid()||inventory.infinite_reserve){
        bootstrap_trace(f,"prepare_inventory",inventory.capacity,inventory.loaded,inventory.reserve);return false;}
    if(!gnasher_parts::frame_geometry(f,units,actual_geometry)){bootstrap_trace(f,"prepare_frame_geometry",units);return false;}
    gears_weapon_gnasher_hold::Authored authored{};
    if(!gears_weapon_gnasher_hold::authored(actual_geometry.component_scale,units,inventory.capacity,authored)){
        bootstrap_trace(f,"prepare_authored_geometry",actual_geometry.component_scale,units,inventory.capacity);return false;}
    // The normal sole writer and this recognizer must share this SAME authored
    // raw grip. A former attachment/switching-pose mount cannot be borrowed.
    std::array<float,16> mount_matrix{};vr::Pose actual{},expected{};double mount_scale{};
    if(!nativelancerfollow::mount.model_matrix({{},{}},mount_matrix)||
       !hammerburst_runtime::matrix(mount_matrix,actual,mount_scale)||
       !weapon::gnasher_live_basis::component_for_main({},authored.right_in_component_metres,expected)){
        bootstrap_trace(f,"prepare_shared_mount");return false;}
    actual.position=weapon::scaled(actual.position,1/units);
    const auto a=actual.orientation,b=expected.orientation;
    if(std::abs(mount_scale-actual_geometry.component_scale)>1e-5||weapon::length(weapon::difference(actual.position,expected.position))>1e-6||
       std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)<1-1e-7||
       weapon::length(weapon::difference(actual_geometry.lever_in_main.position,authored.mechanical.lever.pivot_in_main))>.002||
       std::abs(actual_geometry.lever_in_main.orientation.w)<std::cos(.005)||!nativeequipment::current(nullptr,f)){
        bootstrap_trace(f,"prepare_shared_closure",weapon::length(weapon::difference(actual.position,expected.position)),
            std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w),
            weapon::length(weapon::difference(actual_geometry.lever_in_main.position,authored.mechanical.lever.pivot_in_main)));return false;}
    Geometry value{};value.component=actual_geometry.source.component;value.asset=actual_geometry.source.asset;
    value.mechanical=authored.mechanical;value.support=authored.support;value.box=authored.box;
    value.units=units;value.component_scale=actual_geometry.component_scale;
    value.acquire=authored.acquire_metres;value.release=authored.release_metres;
    value.contact_evidence=gears_weapon_gnasher_hold::contact_id;
    value.curve_content=curve_content;value.timing=timing;value.timing_evidence=timing_evidence;
    if(!value.valid()){bootstrap_trace(f,"prepare_output_geometry");return false;}out=value;return true;
}
// Internal calibration installer. No default geometry/evidence is supplied,
// no external flag bypasses it, and a historical unpaid shot cannot be reset.
bool install(hand::Frame const& f,Geometry const& measured) noexcept {
    if(profile.known||!measured.valid()||
       (measured.mechanical.feed_calibrated&&measured.round.reference_generation!=f.reference_generation)||
       !nativeequipment::current(nullptr,f)||
       gnasher_animation::pending_work()||gnasher_cycle::pending_work()){bootstrap_trace(f,"install_context");return false;}
    auto* e=gnasher_fire::entry(f,false);if(!e||!(e->identity==inv::identity(f))||e->cycle.blocked()||
       e->mechanical.stage()!=weapon::GnasherMechanicalStage::unbound){bootstrap_trace(f,"install_entry");return false;}
    // raw() revalidates the already committed hand snapshot. Exposing this
    // provisional route in that SAME Native Frame would change its mount and
    // make the strict snapshot reader withdraw its own bootstrap source.
    // Next Frame's original observation may publish it coherently; clocks,
    // press/lease state and the current committed observation stay unchanged.
    const auto previous=profile;route_deferred_key=f.key;profile={measured,e->identity,f,true};
    inv::Inventory actual{};hand::GripGameState game{};rig::Frame prepared{};weapon::GnasherMechanicalSample sample{};
    hammerburst_animation::Parts parts{};vr::Pose body{};double scale{};
    bool firing{},reloading{};
    if(!live_asset(f)||!e->observer.sample(f,actual)||!actual.valid()||actual.infinite_reserve||actual.capacity!=measured.mechanical.capacity||
       !weaponhand::observe_game(f,game,false,false)||!lancerreload_state::bool_call(f.weapon.address,L"IsFiring",firing)||firing||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading)||reloading||
       !gnasher_parts::read(f,parts)||!hammerburst_runtime::matrix(parts.world[1],body,scale)||
       std::abs(scale-measured.component_scale)>1e-5||!raw(f,game,true,prepared,sample)){
        bootstrap_trace(f,"install_source_pose");profile=previous;return false;
    }
    auto mechanical=e->mechanical;auto fire=e->cycle;const auto counts=gnasher_fire::counts(f,actual);
    if(!mechanical.bind(counts.identity,measured.mechanical,counts)||
       mechanical.observe(sample).event==weapon::GnasherMechanicalEvent::rejected||
       (fire.inventory().revision?!fire.prepare(counts):!fire.bind(counts))||!nativeequipment::current(nullptr,f)){
        bootstrap_trace(f,"install_mechanical_bind");profile=previous;return false;
    }
    e->mechanical=mechanical;e->cycle=fire;last_sequence=sample.sequence;last_frame=0;
    curve={};closed={};curve_known=down_seen=main_required=false;main={};shot=animation_serial=0;
    gnasher_fire::service(f);bootstrap_trace(f,"installed");return true;
}
bool install_authored(hand::Frame const& f,std::uint64_t curve_content,tracks::Timing timing,
    std::uint64_t timing_evidence) noexcept {
    Geometry measured{};return prepare_authored(f,curve_content,timing,timing_evidence,measured)&&install(f,measured);
}
bool bind_curve(hand::Frame const& f) noexcept {
    hammerburst_animation::Observation selected{};gnasher_curve::Curve value{};
    if(!gnasher_animation::current(f,selected)||!gnasher_curve::read_curve(f,selected,value)||
       value.content!=profile.geometry.curve_content||!gnasher_curve::closed_reference(f,value,
           profile.geometry.component_scale,profile.geometry.units,closed))return false;
    // Raw stored Main keys use a different convention/offset from the mesh
    // reference. They certify the curve, never a live component transform.
    if(weapon::length(weapon::difference(
        profile.geometry.mechanical.lever.pivot_in_main,closed.closed.in_main.position))>.002)return false;
    curve=value;curve_known=true;return true;
}
bool refresh_basis(hand::Frame const& f) noexcept {
    basis_known=false;gnasher_parts::FrameGeometry measured{};
    if(!gnasher_parts::frame_geometry(f,profile.geometry.units,measured)||!hand::same(measured.source.key,f.key)||
       !hand::same(measured.source.component,profile.geometry.component)||!hand::same(measured.source.asset,profile.geometry.asset))return false;
    if(std::abs(measured.component_scale-profile.geometry.component_scale)>1e-5||!nativeequipment::current(nullptr,f))return false;
    main_in_component=measured.main_in_component;observed_geometry=measured;basis_known=true;return true;
}
void observe_down(hand::Frame const& f) noexcept {
    if(!curve_known||!gnasher_animation::owner.active||!basis_known||!hand::same(observed_geometry.source.key,f.key))return;
    unsigned peak{};if(!tracks::chamber_shape(curve.samples,peak))return;
    const auto q=vr::normalized(observed_geometry.lever_in_main.orientation);
    const auto actual=2*std::atan2(-q.x,std::abs(q.w));
    const auto p=curve.samples.bones[2][peak].orientation;
    const auto endpoint=2*std::atan2(-p.x,std::abs(p.w));
    if(actual>=endpoint-.003&&std::abs(q.y)<.003&&std::abs(q.z)<.003)down_seen=true;
}
bool original_cocking(hand::Frame const& f) noexcept {
    playground::Args query(f.weapon.address,L"GetStateName");
    if(query.r.size!=8||!vrequip::result_property(query,L"NameProperty",8)||
       !nativeequipment::current(nullptr,f)||!query.run()||!nativeequipment::current(nullptr,f))return false;
    const auto value=query.output<Name>();
    namespace meta=gripdamage_metadata;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};meta::NameText text{};
    return !value.number&&meta::tables(base,schema.objects,schema.names)&&
        meta::fresh_name(schema.names,value.index,text)&&meta::exact_text(text,L"ShotgunCocking")&&
        unified_weapon_fire::retained_metadata(r)&&nativeequipment::current(nullptr,f);
}
void cache_main(hand::Frame const& f,rig::Frame const& prepared,vr::Pose const& body) noexcept {
    if(!basis_known||!hand::same(observed_geometry.source.key,f.key)||
       !weapon::gnasher_live_basis::component_for_main(body,main_in_component,main.component)){main={};return;}
    main.source=f;main.rig_publication=prepared.publication_sequence;main.known=vr::stereo_pose_valid(main.component);
}
void tick(hand::Frame const& f,hand::GripGameState const& game,hand::Decision const& grant) noexcept {
    main={};main_required=basis_known=false;
    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire))return;
    if(!select_profile(f))return;
    if(!profile.known){
        // Normal production entry, not an operator-export/input path. A real
        // reviewed original timing record is still required before binding.
        constexpr auto timing=gears_weapon_gnasher_calibration::current;
        if(!timing.valid()||!install_authored(f,timing.curve_content,timing.law,timing.evidence))return;
    }
    if(!live_asset(f)||!nativeequipment::current(nullptr,f))return;
    auto* e=gnasher_fire::entry(f,false);if(!e||!(e->identity==inv::identity(f))||e->mechanical.hold_suspended()||
       e->mechanical.stage()==weapon::GnasherMechanicalStage::unbound||e->cycle.pending()||e->cycle.faulted())return;
    if(e->cycle.confirmed()&&e->mechanical_shot!=e->cycle.receipt().original_event_sequence)return;
    if(e->mechanical_shot!=shot){shot=e->mechanical_shot;animation_serial=0;curve_known=down_seen=false;curve={};closed={};}
    if(reload::withdraw_unused(f,*e))return;
    if(!e->mechanical.mechanism_pending()&&!e->cycle.blocked()&&gnasher_animation::owner.active&&
       animation_serial==gnasher_animation::owner.serial&&gnasher_animation::release_after_exit(f,animation_serial))animation_serial=0;
    bool firing{},reloading{};
    if(!lancerreload_state::bool_call(f.weapon.address,L"IsFiring",firing)||
       !lancerreload_state::bool_call(f.weapon.address,L"IsReloading",reloading))return;
    // Stock ShotgunCocking belongs to this confirmed shot. It may retain its
    // original ROF timer while physical motion controls the weapon animation;
    // cycle ACK still independently requires actual !IsFiring/!IsReloading.
    const bool physical_ready=(!firing&&reload::owns_reload(f))||(!reloading&&(!firing||
        (e->cycle.confirmed()&&e->mechanical_shot==e->cycle.receipt().original_event_sequence&&original_cocking(f))));
    rig::Frame prepared{};weapon::GnasherMechanicalSample sample{};
    if(!raw(f,game,physical_ready,prepared,sample))return;
    main_required=e->mechanical.mechanism_pending()&&sample.left_front_held;
    if(e->mechanical.mechanism_pending()&&physical_ready&&!gnasher_animation::owner.active){
        if(!animation_serial){if(!next_animation||next_animation==UINT64_MAX)return;animation_serial=next_animation++;}
        (void)gnasher_animation::hold_after_original_shot(f,animation_serial);
    }
    if(main_required&&physical_ready){
        if(gnasher_animation::owner.active){
            hammerburst_animation::Observation current{};
            if(!gnasher_animation::current(f,current)){
                if(firing||reloading||!gnasher_animation::release_after_exit(f,animation_serial))return;
                animation_serial=0;curve_known=false;
            }
        }
        if(!animation_serial){if(!next_animation||next_animation==UINT64_MAX)return;animation_serial=next_animation++;}
        if(!gnasher_animation::begin(f,animation_serial)){
            if(!gnasher_animation::owner.active)animation_serial=0;return;
        }
        if((!curve_known&&!bind_curve(f))||!refresh_basis(f))return;
        observe_down(f);
        vr::Pose body{};const auto& contact=profile.geometry.mechanical.lever.left_front_grip_in_main;
        body.orientation=vr::normalized(vr::multiply(sample.left_grip.orientation,vr::inverse(contact.orientation)));
        body.position=weapon::difference(sample.left_grip.position,weapon::rotate(body.orientation,contact.position));
        cache_main(f,prepared,body);
    }
    if(f.key.frame==last_frame)return;
    if(sample.sequence<=last_sequence){
        // A new arbiter grant can arrive on a later GT Frame with the SAME
        // still-fresh XR sample. Do not observe/replay its physical movement,
        // but do let that exact already-consumed source admit the new intent.
        if(sample.sequence==last_sequence&&e->mechanical.source_matches(sample.sequence,sample.monotonic_ns))
            (void)reload::dispatch(f,grant,*e);
        return;
    }
    last_frame=f.key.frame;last_sequence=sample.sequence;
    const auto decision=e->mechanical.observe(sample);
    if(reload::dispatch(f,grant,*e))return;
    if(decision.drive_lever){
        if(!curve_known)return;
        auto visual=decision;unsigned peak{};
        if(!tracks::chamber_shape(curve.samples,peak))return;
        const auto q=curve.samples.bones[2][peak].orientation;
        const auto maximum=2*std::atan2(-q.x,std::abs(q.w));
        // Rendering stops at the actual asset endpoint; raw mechanics never
        // reads this clamped value. Home is snapped only after the calibrated
        // physical return gesture, and still needs actual original readback.
        visual.lever_angle_radians=(std::min)(visual.lever_angle_radians,maximum);
        if(decision.event==weapon::GnasherMechanicalEvent::request_original_cycle)visual.lever_angle_radians=0;
        if(!gnasher_curve::seek_angle(f,animation_serial,curve,visual,profile.geometry.timing)){main.known=false;return;}
        cache_main(f,prepared,decision.main_pose);
    }
    if(decision.event==weapon::GnasherMechanicalEvent::request_original_cycle){
        if(!down_seen){
            if(!gnasher_cycle::pending_work()&&e->cycle.can_request_cycle())
                (void)e->mechanical.reject_unpublished_cycle(e->mechanical.cycle_request().token);
        }else (void)gnasher_cycle::begin(f,e->mechanical,closed);
    }
    if(gnasher_cycle::pending_work()&&gnasher_cycle::tick(f,e->mechanical)){
        // Retire this shot's stopped clip on its actual closed/readiness ACK.
        // Waiting for another stale pre-tick mechanical sample can otherwise
        // keep the animation fire blocker alive indefinitely at low cadence.
        if(gnasher_animation::release_after_exit(f,animation_serial))animation_serial=0;
    }
    // Preparation follows consumption of this exact raw sample, never an
    // older sample that happened to remain inside100ms.
    gnasher_fire::service(f);
}
MainResult correct_main(hand::Frame const& f,rig::Frame const& prepared,vr::Viewpoint& target) noexcept {
    if(!context(f))return MainResult::not_applicable;
    const auto* e=gnasher_fire::entry(f,false);
    const bool support=decision_known&&hand::same_frame(decision_source,f)&&
        !committed_decision.release_required&&(committed_decision.status==hand::DecisionStatus::granted||committed_decision.status==hand::DecisionStatus::retained)&&
        committed_decision.lease.kind==hand::LeaseKind::support&&hand::lease_valid(committed_decision.lease,f.key.lifecycle);
    if(!main_required&&!(support&&e&&e->mechanical.mechanism_pending()))return MainResult::not_applicable;
    if(!main.known||!hand::same_frame(main.source,f)||prepared.publication_sequence!=main.rig_publication||
       gears_equipment_rig::match(f,prepared,weaponhand::clock_ns(nullptr),static_cast<std::uint32_t>(f.stereo.source))!=gears_equipment_rig::Status::matched||
       prepared.units_per_metre!=profile.geometry.units)return MainResult::rejected;
    target={weapon::scaled(main.component.position,profile.geometry.units),vr::ue_angles(vr::rotation_matrix(main.component.orientation))};
    return vr::finite(target.position)?MainResult::applied:MainResult::rejected;
}
} // namespace gnasher_runtime
