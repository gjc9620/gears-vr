// Current same-ticket ocular/eye observation on the normal GT. This does not
// change targeting, consume the game's R3, or claim a rendered scope image.
namespace longshot_eyebox {
namespace hand=gears_weapon_hand;namespace control=gears_weapon::longshot_scope_control;
struct Observation {
    hand::FrameKey key{};lancerreload_inventory::Identity identity{};
    std::uint64_t reference_generation{},profile_generation{};std::int64_t observed_ns{};
    std::array<control::EyeContact,2> eyes{};gears_vr::Pose ocular_world_m{};
    bool valid{},near_eye{};int active_eye{-1};
    gears_vr::Pose model_world{};double model_scale{};
    bool switch_requested{};
};
Observation current{};control::Control controller;
bool matches(hand::Frame const& f,std::int64_t now) noexcept {
    return current.valid&&hand::same(current.key,f.key)&&current.identity==lancerreload_inventory::identity(f)&&
        current.reference_generation==f.reference_generation&&current.profile_generation==f.controllers.profile_generation&&
        now>=current.observed_ns&&now-current.observed_ns<=100000000&&nativeequipment::current(nullptr,f);
}
bool view_request(hand::Frame const& f,double original_fov,gears_weapon_longshot_scope_view::Request& out) noexcept {
    out={};
    if(!matches(f,weaponhand::clock_ns(nullptr))||!current.near_eye||current.active_eye<0||current.active_eye>1||
       !std::isfinite(original_fov)||original_fov<2||original_fov>90)return false;
    out.source=f;out.component=nativelancerfollow::bound_key.component;out.asset=nativelancerfollow::bound_key.asset;
    out.model_world=current.model_world;out.model_scale=current.model_scale;
    out.original_targeting_fov_degrees=original_fov;out.active=1;out.eye=static_cast<unsigned>(current.active_eye);return true;
}
bool possibly_near(hand::Frame const& f) noexcept {
    // Rejection-only broad phase from the existing rigid mount. The full
    // current mesh/rig checks below still decide all accepted observations.
    std::array<float,16> matrix{};
    const double units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500||!nativelancerfollow::mount.model_matrix({{},{}},matrix))return false;
    const auto p=gears_weapon::longshot_eye_contact::ocular_center_raw;
    const gears_vr::Vector offset{matrix[12]+p.x*matrix[0]+p.y*matrix[4]+p.z*matrix[8],
        matrix[13]+p.x*matrix[1]+p.y*matrix[5]+p.z*matrix[9],matrix[14]+p.x*matrix[2]+p.y*matrix[6]+p.z*matrix[10]};
    // Existing mount only: bounded planning POD, no rig/object query and no
    // eye-box/action permission. This same fixed transform drives holding.
    static unsigned mount_rows{};static uptr reported_weapon{};static std::uint64_t reported_hold{};
    if(log_file&&mount_rows<8&&(reported_weapon!=f.weapon.address||reported_hold!=nativelancerfollow::bound_key.hold_generation)){
        gears_vr::Pose mesh_in_grip{},ocular_in_grip{};double scale{};
        if(hammerburst_runtime::matrix(matrix,mesh_in_grip,scale)&&
           gears_weapon::longshot_eye_contact::ocular_world_m(mesh_in_grip,scale,units,ocular_in_grip)){
            const auto pos=ocular_in_grip.position;const auto q=ocular_in_grip.orientation;
            const auto& left=f.stereo.eyes[0].pose;const auto& right=f.stereo.eyes[1].pose;
            fwprintf(log_file,L"longshot_scope_mount weapon=%llx hold=%llu frame=%llu source_sync_ns=%lld units=%.9g lens_in_grip_m=%.9g,%.9g,%.9g lens_rotation=%.9g,%.9g,%.9g,%.9g eye0_position_m=%.9g,%.9g,%.9g eye0_rotation=%.9g,%.9g,%.9g,%.9g eye1_position_m=%.9g,%.9g,%.9g eye1_rotation=%.9g,%.9g,%.9g,%.9g planning_only=1\n",
                static_cast<unsigned long long>(f.weapon.address),nativelancerfollow::bound_key.hold_generation,
                f.key.frame,f.controllers.sync_ns,units,pos.x,pos.y,pos.z,q.x,q.y,q.z,q.w,
                left.position.x,left.position.y,left.position.z,left.orientation.x,left.orientation.y,left.orientation.z,left.orientation.w,
                right.position.x,right.position.y,right.position.z,right.orientation.x,right.orientation.y,right.orientation.z,right.orientation.w);fflush(log_file);
            reported_weapon=f.weapon.address;reported_hold=nativelancerfollow::bound_key.hold_generation;++mount_rows;
        }
    }
    const auto delta=longshot_bolt::sub(f.controllers.hands[1].grip.position,f.stereo.head.position);
    return snub_visual::length(delta)<=snub_visual::length(offset)/units+.13+.04+.08;
}
void observe(hand::Frame const& f,hand::GripGameState const& game,bool scope_ready) noexcept {
    current={};
    control::Frame input{};
    const auto now=weaponhand::clock_ns(nullptr);
    const auto reject=[&]() noexcept {controller.observe(input,now);};
    if(!longshot_runtime::selected(f)||!game.known||!game.main_hand_held||
       !nativeequipment::current(nullptr,f)||(f.stereo.view_flags&15)!=15||!possibly_near(f)){reject();return;}
    gears_rig_tracking::Frame rig{};const auto source=static_cast<std::uint32_t>(f.stereo.source);
    if(handrig::read(rig,0,source).reply!=gears_rig_tracking::Reply::accepted||
       gears_equipment_rig::match(f,rig,now,source)!=gears_equipment_rig::Status::matched||
       std::abs(rig.units_per_metre-nativelancerfollow::bound_units)>1e-6){reject();return;}
    vrequip::MeshPoseProbe measured{};gears_vr::Pose mesh{};double scale{};
    if(!vrequip::mesh_pose_probe(f.weapon.address,measured)||measured.mesh!=nativelancerfollow::bound_key.component.address||
       measured.asset!=nativelancerfollow::bound_key.asset.address){reject();return;}
    std::array<float,16> raw{};std::memcpy(raw.data(),measured.local_to_world,sizeof measured.local_to_world);
    gears_vr::Pose ocular{};
    if(!hammerburst_runtime::matrix(raw,mesh,scale)){reject();return;}
    // Like the authoritative muzzle adapter, use the SAME current held-model
    // plan instead of the ordinary component pose restored around game calls.
    // The ordinary matrix still certifies actual model scale/rigidity above.
    const auto& held=nativelancerfollow::plan;
    gears_vr::Quaternion rotation{};
    if(!held.valid||!hand::same_frame(held.source,f)||
       !hand::same(held.key.weapon,f.weapon)||!hand::same(held.key.component,nativelancerfollow::bound_key.component)||
       !hand::same(held.key.asset,nativelancerfollow::bound_key.asset)||
       held.key.hold_generation!=nativelancerfollow::bound_key.hold_generation||
       !gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(held.target.rotation),rotation)){reject();return;}
    mesh={held.target.position,rotation};
    if(!gears_weapon::longshot_eye_contact::ocular_world_m(mesh,scale,rig.units_per_metre,ocular)){reject();return;}
    for(unsigned i=0;i<2;++i){gears_vr::Pose eye{};
        if(!gears_vr::rig_tracking::pose(rig.base_view,rig.reference,f.stereo.eyes[i].pose,rig.units_per_metre,eye)){reject();return;}
        eye.position=longshot_bolt::mul(eye.position,1/rig.units_per_metre);
        input.eyes[i]=gears_weapon::longshot_eye_contact::contact_ue(ocular,eye,true);
    }
    const auto& right=f.controllers.hands[1];
    input.stamp={longshot_fire::reload_identity(f),f.controllers.sequence,f.controllers.sync_ns,0,
        bool(rig.world_pose_mask&gears_rig_tracking::right_grip_pose),bool(f.left_tracked),true};
    input.pressed=right.pressed[gears_vr::control_stick];input.released=right.released[gears_vr::control_stick];
    input.click_down=(right.buttons&gears_vr::controller_stick_click)!=0;
    input.click_active=(right.buttons_active&gears_vr::controller_stick_click)!=0;
    input.menu=game.busy||game.animation_owns_hands;input.reloading=longshot_runtime::stage!=longshot_runtime::Stage::idle;
    input.scope_ready=scope_ready;
    if(!nativeequipment::current(nullptr,f)){input={};reject();return;}
    const auto result=controller.observe(input,now);
    current={f.key,lancerreload_inventory::identity(f),f.reference_generation,f.controllers.profile_generation,
        f.controllers.sync_ns,input.eyes,ocular,true,result.near_eye,result.eye,mesh,scale,result.switch_magnification};
}
// Until the complete owner is composed, the existing caller remains purely
// observational and cannot request a zoom operation.
void tick(hand::Frame const& f,hand::GripGameState const& game) noexcept {observe(f,game,false);}
}
