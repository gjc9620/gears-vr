// Controller-relative geometry from the SAME authenticated immutable model
// mount used by the right-hand follower. No left-hand pose defines the socket.
namespace support_skin {
namespace authored=gears_hand_authored_grasp;namespace owner=gears_boltok_owner_profiles;
bool geometry(hand::Frame const& f,P& grip,V& centre,double& physical) noexcept {
 if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_boltok||!nativelancerfollow::mount.bound()||
    !hand::same(nativelancerfollow::bound_key.weapon,f.weapon)||
    nativelancerfollow::bound_key.hold_generation!=vrequip::model.hold_generation()||
    nativelancerfollow::bound_reference!=f.reference_generation||nativelancerfollow::bound_world!=f.key.world_generation||
    nativelancerfollow::bound_session!=f.stereo.session||nativelancerfollow::bound_space!=f.stereo.space_epoch||
    nativelancerfollow::bound_input!=f.controllers.input_epoch)return false;
 const double units=nativelancerfollow::bound_units;if(!std::isfinite(units)||units<10||units>500)return false;
 std::array<float,16> matrix{};P model{};double scale{};
 if(!nativelancerfollow::mount.model_matrix({{},{}},matrix)||!hammerburst_runtime::matrix(matrix,model,scale))return false;
 gears_hand_authored_mount::Mount left{};
 if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,gears_hand_authored_mount::Side::left,1,100,left))return false;
 const auto wrist=owner::support.wrist_raw_object;
 const auto local_rotation=gears_vr::normalized(gears_vr::multiply(wrist.orientation,gears_vr::inverse(left.calibration.grip_to_wrist_rotation)));
 const auto marker=gears_weapon::rotate(local_rotation,gears_weapon::scaled(left.calibration.grip_to_wrist_metres,100));
 const auto local=gears_weapon::difference(wrist.position,marker);
 grip.position=gears_weapon::scaled(gears_weapon::sum(model.position,gears_weapon::rotate(model.orientation,gears_weapon::scaled(local,scale))),1/units);
 grip.orientation=gears_vr::normalized(gears_vr::multiply(model.orientation,local_rotation));
 centre=gears_weapon::scaled(gears_weapon::sum(model.position,gears_weapon::rotate(model.orientation,gears_weapon::scaled(owner::support.volume_center_raw,scale))),1/units);
 physical=100*scale/units;
 return gears_vr::stereo_pose_valid(grip)&&gears_vr::finite(centre)&&std::isfinite(physical)&&physical>0;
}
// Cosmetic only: no arbiter proposal, lease, physical-input change or second C0 writer.
void publish_hover(hand::Frame const& f,hand::GripGameState const& game,
    hand::Decision const& d) noexcept {
    if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
       f.left_grip_down||hand::index_pinch(f)||!hand::same(d.key,f.key)||
       d.lease.kind!=hand::LeaseKind::none||d.release_required)return;
    const auto& left=f.controllers.hands[0];
    if(left.squeeze_active!=1||left.trigger_active!=1||
       gears_weapon_hand::control_held(left,gears_vr::control_squeeze)||
       gears_weapon_hand::control_held(left,gears_vr::control_trigger))return;

    if(!nativelancerfollow::plan.valid||nativelancerfollow::plan.departing||
       !(nativelancerfollow::plan.key==nativelancerfollow::bound_key)||
       !hand::same_frame(nativelancerfollow::plan.source,f))return;
    const auto& prepared=nativelancerfollow::plan.prepared;
    const auto grips=gears_rig_tracking::left_grip_pose|
        gears_rig_tracking::right_grip_pose;
    if((prepared.world_pose_mask&grips)!=grips)return;
    const double units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500||prepared.units_per_metre!=units)
        return;

    P relative{};V centre{};double physical{};
    if(!geometry(f,relative,centre,physical))return;
    const P& right=prepared.world_grip[1];
    const P& world_left=prepared.world_grip[0];
    if(!gears_vr::stereo_pose_valid(right)||!gears_vr::stereo_pose_valid(world_left))return;
    const V local=gears_weapon::scaled(
        gears_weapon::rotate(gears_vr::inverse(right.orientation),
            gears_weapon::difference(world_left.position,right.position)),
        1/units);
    if(!gears_vr::finite(local))return;

    const auto& profile=owner::support;
    const double radius=profile.volume_radius_metres*physical;
    const double snap=profile.snap_distance_metres*physical;
    const double radius_distance=gears_weapon::length(
        gears_weapon::difference(local,centre));
    const double snap_distance=gears_weapon::length(
        gears_weapon::difference(local,relative.position));
    if(!std::isfinite(radius_distance)||!std::isfinite(snap_distance)||
       radius_distance>radius+1e-5||snap_distance>snap+1e-5)return;

    const auto local_orientation=gears_vr::normalized(gears_vr::multiply(
        gears_vr::inverse(right.orientation),world_left.orientation));
    const double dot=std::abs(
        local_orientation.x*relative.orientation.x+
        local_orientation.y*relative.orientation.y+
        local_orientation.z*relative.orientation.z+
        local_orientation.w*relative.orientation.w);
    const double hover_angle=2.0*std::acos((std::min)(1.0,(std::max)(0.0,dot)));
    if(!std::isfinite(hover_angle)||hover_angle>profile.snap_angle_radians+1e-5)return;

    P avatar_relative{};
    if(!gears_boltok_support_avatar::target(relative,physical,avatar_relative))return;
    authored::Request q{};
    q.source=f;
    q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
    q.profile_id=profile.profile_id;
    q.side=authored::Side::left;
    q.mode=authored::Mode::left_part;
    q.kind=authored::object::Kind::support;
    q.proximity=1;
    q.grip.position=gears_weapon::sum(right.position,
        gears_weapon::rotate(right.orientation,
            gears_weapon::scaled(avatar_relative.position,units)));
    q.grip.orientation=gears_vr::normalized(
        gears_vr::multiply(right.orientation,avatar_relative.orientation));
    q.closed=gears_boltok_support_avatar::closed;
    if(!gears_hand_marcus115_reference::read(q.geometry_signature,q.side,
           q.reference)||!authored::structural(q))return;
    const auto call=function<authored::Publish>(authored::publish_export);
    if(!call||!nativeequipment::current(nullptr,f))return;
    const bool accepted=call(&q);
    static unsigned rows{};static bool known{},previous{};
    if(log_file&&rows<12&&(!known||previous!=accepted)){
        ++rows;fwprintf(log_file,L"boltok_support_hover accepted=%u profile=%llu frame=%llu sequence=%llu proximity=1 radius_distance=%.9g snap_distance=%.9g angle=%.9g observation_only=1 derived_avatar=1\n",
            unsigned(accepted),q.profile_id,f.key.frame,f.controllers.sequence,radius_distance,snap_distance,hover_angle);fflush(log_file);
    }
    known=true;previous=accepted;
}

void publish(hand::Frame const& f,hand::Decision const& d) noexcept {
 if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||hand::index_pinch(f)||d.release_required||
    (d.status!=hand::DecisionStatus::granted&&d.status!=hand::DecisionStatus::retained)||
    d.lease.kind!=hand::LeaseKind::support||!hand::same(f.key,d.key)||d.press_token!=f.press_token||
    d.lease.id!=f.press_token||!hand::lease_valid(d.lease,f.key.lifecycle))return;
 P grip{};V centre{};double physical{};if(!geometry(f,grip,centre,physical))return;
 authored::Request q{};q.source=f;q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
 q.profile_id=owner::support.profile_id;q.side=authored::Side::left;q.mode=authored::Mode::left_part;
 q.kind=authored::object::Kind::support;
 if(!gears_boltok_support_avatar::target(grip,physical,q.grip))return;
 q.closed=gears_boltok_support_avatar::closed;
 if(!gears_hand_marcus115_reference::read(q.geometry_signature,q.side,q.reference)||!authored::structural(q))return;
 const auto call=function<authored::Publish>(authored::publish_export);if(!call)return;
 const bool accepted=call(&q);static unsigned rows{};static std::uint64_t last_token{};static bool last_accepted{},known{};
 if(log_file&&rows<12&&(!known||last_token!=f.press_token||last_accepted!=accepted)){++rows;fwprintf(log_file,L"boltok_support accepted=%u profile=%llu frame=%llu token=%llu capture=1794485865263-1 derived_avatar=1\n",unsigned(accepted),q.profile_id,f.key.frame,f.press_token);fflush(log_file);}
 known=true;last_token=f.press_token;last_accepted=accepted;
}
}
namespace part_skin {
namespace authored=gears_hand_authored_grasp;namespace owner=gears_boltok_owner_profiles;
// Current loading lease only. Feed geometry supplies a cosmetic Grip target;
// this is the same Native C0 publisher, never a physical input or bone writer.
bool publish_loading_target(hand::Frame const& f,hand::Decision const& d,
    hand::Lease const& owned_lease,P const& grip) noexcept {
 if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||d.release_required||
    (d.status!=hand::DecisionStatus::granted&&d.status!=hand::DecisionStatus::retained)||
    !hand::same(f.key,d.key)||!f.press_token||d.press_token!=f.press_token||
    !hand::same(owned_lease,d.lease)||owned_lease.kind!=hand::LeaseKind::ammo||
    owned_lease.id!=f.press_token||!hand::lease_valid(owned_lease,f.key.lifecycle)||
    !gears_vr::stereo_pose_valid(grip))return false;
 authored::Request q{};q.source=f;q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
 q.profile_id=gears_boltok_ammo_avatar::profile_id;q.side=authored::Side::left;
 q.mode=authored::Mode::left_part;q.kind=authored::object::Kind::loading;
 q.grip=grip;q.closed=gears_boltok_ammo_avatar::closed;
 if(!gears_hand_marcus115_reference::read(q.geometry_signature,q.side,q.reference)||
    !authored::structural(q))return false;
 const auto call=function<authored::Publish>(authored::publish_export);
 const bool accepted=call&&nativeequipment::current(nullptr,f)&&call(&q);
 static unsigned rows{};static std::uint64_t token{};static bool known{},prior{};
 if(log_file&&rows<12&&(!known||token!=f.press_token||prior!=accepted)){
  ++rows;fwprintf(log_file,L"boltok_loading_target accepted=%u profile=%llu source_profile=%llu frame=%llu sequence=%llu token=%llu proximity=0 cosmetic_only=1\n",
   unsigned(accepted),q.profile_id,owner::ammo.profile_id,f.key.frame,f.controllers.sequence,f.press_token);fflush(log_file);
 }
 known=true;token=f.press_token;prior=accepted;return accepted;
}
// Finger-only registration. The actual part lease and fresh Native C0 frame,
// never this fixed local15 descriptor, own every later cosmetic application.
bool publish(hand::Frame const& f,hand::Decision const& d,hand::Lease const& owned_lease,bool ammo) noexcept {
 const auto lease_kind=ammo?hand::LeaseKind::ammo:hand::LeaseKind::magazine;
 if(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||d.release_required||
    (d.status!=hand::DecisionStatus::granted&&d.status!=hand::DecisionStatus::retained)||
    !hand::same(f.key,d.key)||!f.press_token||d.press_token!=f.press_token||
    !hand::same(owned_lease,d.lease)||owned_lease.kind!=lease_kind||owned_lease.id!=f.press_token||
    !hand::lease_valid(owned_lease,f.key.lifecycle))return false;
 const auto& profile=ammo?owner::ammo:owner::mechanism;
 authored::Request q{};q.source=f;q.geometry_signature=gears_hand_authored_mount::marcus115_signature;
 q.profile_id=ammo?gears_boltok_ammo_avatar::profile_id:profile.profile_id;q.side=authored::Side::left;q.mode=authored::Mode::left_part;
 q.kind=ammo?authored::object::Kind::loading:authored::object::Kind::magazine;
 q.closed=ammo?gears_boltok_ammo_avatar::closed:profile.fingers_native;
 if(!gears_hand_marcus115_reference::read(q.geometry_signature,q.side,q.reference)||!authored::structural(q))return false;
 const auto call=function<authored::Publish>(authored::part_fingers_export);
 const bool accepted=call&&call(&q);
 struct Trace {unsigned rows{};std::uint64_t token{};bool accepted{},known{};};
 static Trace traces[2];auto& trace=traces[ammo?0:1];
 if(log_file&&trace.rows<12&&(!trace.known||trace.token!=f.press_token||trace.accepted!=accepted)){
  ++trace.rows;fwprintf(log_file,L"%ls accepted=%u profile=%llu frame=%llu sequence=%llu token=%llu capture=%ls source_profile=%llu derived_avatar_thumb=%u\n",
   ammo?L"boltok_round_skin":L"boltok_mechanism_skin",unsigned(accepted),q.profile_id,f.key.frame,f.controllers.sequence,
   f.press_token,ammo?L"1794500066511-3":L"1794486092117-4",profile.profile_id,unsigned(ammo));fflush(log_file);
 }
 trace.known=true;trace.token=f.press_token;trace.accepted=accepted;return accepted;
}
}
bool fill_support_mount(hand::Frame const& f,hand::GripGameState& game) noexcept {
 if(!game.known||!game.main_hand_held||game.busy||game.animation_owns_hands||
    game.kind!=hand::WeaponKind::firearm||!f.left_tracked||!f.left_grip_known||hand::index_pinch(f))return false;
 P grip{};V centre{};double physical{};if(!support_skin::geometry(f,grip,centre,physical))return false;
 const auto& profile=gears_boltok_owner_profiles::support;
 const auto xr=[](V p) noexcept {return V{p.y,p.z,-p.x};};
 game.mount={};game.mount.maximum_support_correction_radians=0;
 game.mount.support_grip_in_weapon=xr(grip.position);
 game.mount.support_length_tolerance_metres=profile.release_distance_metres*physical;
 game.acquire_metres=profile.snap_distance_metres*physical;game.release_metres=profile.release_distance_metres*physical;
 if(game.acquire_metres<=0||game.acquire_metres>.15||game.release_metres<game.acquire_metres||game.release_metres>.25||!game.mount.valid())return false;
 game.support_region={};game.support_region.enabled=true;
 game.support_region.volume.local.position=xr(centre);game.support_region.volume.radius=profile.volume_radius_metres*physical;
 game.support_region.origin_in_object=xr(grip.position);
 game.mount_asset=nativelancerfollow::bound_key.asset;game.mount_revision=nativelancerfollow::bound_key.hold_generation;
 game.mount_known=game.support_region.valid();
 static std::uint64_t logged_hold{};
 if(game.mount_known&&log_file&&logged_hold!=game.mount_revision){
  logged_hold=game.mount_revision;
  fwprintf(log_file,L"boltok_support_geometry frame=%llu sequence=%llu hold=%llu grip=%.9g,%.9g,%.9g rotation=%.9g,%.9g,%.9g,%.9g acquire=%.9g release=%.9g observation_only=1\n",f.key.frame,f.controllers.sequence,game.mount_revision,
   grip.position.x,grip.position.y,grip.position.z,grip.orientation.x,grip.orientation.y,grip.orientation.z,grip.orientation.w,game.acquire_metres,game.release_metres);fflush(log_file);
 }
 return game.mount_known;
}
void publish_normal_support(uptr pc) noexcept {
 if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_boltok)return;
 hand::Frame f{};hand::GripGameState game{};hand::Decision d{};
 if(weaponhand::client.copy_frame_on_gt(f,game,d)&&f.controller.address==pc){main_skin::publish(f);support_skin::publish_hover(f,game,d);support_skin::publish(f,d);}
}
