// One Bow-owned display component. The original asset/material remain game
// owned. No original reload, ammo, grip acquisition, snap or C0 write occurs.
namespace torquebow_arrow_visual {
namespace appearance=torquebow_arrow_appearance;namespace tracking=gears_torque_bow_arrow_tracking;
namespace mount=gears_torque_bow_arrow_mount;namespace vr=gears_vr;
struct Owner {
    pool::Handle handle{};std::uint64_t generation{},hold{};
    hand::Frame parent{};appearance::Appearance original{};bool retiring{},lit{};
    std::uint64_t pose_scene{},pose_sequence{};std::int64_t pose_sync_ns{};
    bool feed_assisted{};std::uint64_t assist_arrow{},assist_config{};
};
Owner owner;std::uint64_t next_generation{1};
bool pending() noexcept {return owner.handle.generation!=0;}
bool retire() noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    if(!pending())return true;
    owner.retiring=true;
    // The common stop/drain may already have retired this exact handle. A
    // reused pool slot has another generation and is never adopted/destroyed.
    if(pool::find(owner.handle)&&!pool::destroy(owner.handle,owner.generation))return false;
    owner={};return true;
}
bool withdraw() noexcept {
    if(!weaponhand::current_gt(nullptr))return false;
    owner.feed_assisted=false;owner.assist_arrow=owner.assist_config=0;
    if(!pending())return true;
    auto r=pool::find(owner.handle);
    if(!r){owner={};return true;}
    if(r->stage==pool::Stage::retiring||r->stage==pool::Stage::failed){owner.retiring=true;return retire();}
    if(r->stage!=pool::Stage::ready&&r->stage!=pool::Stage::visible)return true;
    if(pool::set_visible(owner.handle,owner.generation,false))return true;
    // Failed hiding retains the exact cleanup obligation; never reset it.
    owner.retiring=true;return retire();
}
bool same_parent(hand::Frame const& f) noexcept {
    const auto& p=owner.parent;
    return pending()&&!owner.retiring&&owner.hold==vrequip::model.hold_generation()&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        hand::same(p.world,f.world)&&hand::same(p.controller,f.controller)&&hand::same(p.pawn,f.pawn)&&
        hand::same(p.mesh,f.mesh)&&hand::same(p.weapon,f.weapon)&&p.key.game_pid==f.key.game_pid&&
        p.key.game_start==f.key.game_start&&p.key.native_started_qpc==f.key.native_started_qpc&&
        p.key.registration_cookie==f.key.registration_cookie&&p.key.thread_id==f.key.thread_id&&
        p.key.world_generation==f.key.world_generation&&p.key.lifecycle==f.key.lifecycle&&
        p.reference_generation==f.reference_generation&&vr::stereo_same_pose(p.reference,f.reference)&&
        p.stereo.source==f.stereo.source&&p.stereo.session==f.stereo.session&&p.stereo.space_epoch==f.stereo.space_epoch&&
        p.controllers.input_epoch==f.controllers.input_epoch&&p.controllers.profile_generation==f.controllers.profile_generation;
}
std::uint64_t controller() noexcept {return pending()?owner.parent.controller.address:0;}
bool warmed() noexcept {
    const auto r=pool::find(owner.handle);
    return pending()&&!owner.retiring&&owner.lit&&r&&r->owner.generation==owner.generation&&
        !r->publication_withdrawn&&r->lighting_bound&&(r->stage==pool::Stage::ready||r->stage==pool::Stage::visible);
}
// Existing equipment Frame notification only. No source read, Engine call,
// setter or destruction is allowed here; common GT drain performs cleanup.
void on_frame(hand::Frame const& f) noexcept {
    if(!weaponhand::current_gt(nullptr)||!pending())return;
    const auto r=pool::find(owner.handle);if(!r)return;
    if(!same_parent(f)){
        owner.retiring=true;r->publication_withdrawn=true;r->stage=pool::Stage::retiring;return;
    }
    if(r->stage==pool::Stage::visible&&(owner.pose_scene!=f.key.frame||
       owner.pose_sequence!=f.controllers.sequence||owner.pose_sync_ns!=f.controllers.sync_ns))r->visibility_revoke=true;
}
pool::Creation prepare(hand::Frame const& f,gears_committed_interaction::Frame const& avatar) noexcept {
    if(!weaponhand::current_gt(nullptr))return pool::Creation::rejected;
    if(pending()&&(!same_parent(f)||owner.retiring)){retire();return pool::Creation::rejected;}
    appearance::Appearance actual{};
    if(!appearance::read(f,avatar,actual)){withdraw();return pool::Creation::pending;}
    if(pending()&&!appearance::same(owner.original,actual)){retire();return pool::Creation::rejected;}
    if(!pending()){
        if(!next_generation||next_generation==UINT64_MAX)return pool::Creation::rejected;
        Owner candidate{};candidate.parent=f;candidate.original=actual;candidate.hold=vrequip::model.hold_generation();
        candidate.generation=next_generation++;
        const playground::Vector scale{float(actual.world_scale),float(actual.world_scale),float(actual.world_scale)};
        std::vector<hand::ObjectIdentity> materials;
        try{materials.push_back(actual.material);}catch(...){return pool::Creation::rejected;}
        const auto result=pool::create_existing_static(f,candidate.generation,actual.binding.asset,materials,scale,candidate.handle);
        if(result==pool::Creation::rejected)return result;
        owner=candidate;return pool::Creation::pending;
    }
    auto r=pool::find(owner.handle);
    if(!r){owner={};return pool::Creation::rejected;}
    const auto result=pool::advance(f,owner.handle,owner.generation);
    if(result==pool::Creation::rejected){retire();return result;}
    if(result!=pool::Creation::ready)return result;
    if(!owner.lit){
        if(!pool::bind_light_environment(f,owner.handle,owner.generation,actual.attributes.light,!actual.attributes.light.address))
            return pool::Creation::pending;
        owner.lit=true;
    }
    return nativeequipment::current(nullptr,f)?pool::Creation::ready:pool::Creation::pending;
}
bool held_arrow(hand::Frame const& f,hand::Decision const& d,gears_torque_bow_fire_driver::Driver const& driver) noexcept {
    const auto& model=driver.interaction();
    const gears_weapon::TorqueBowIdentity identity{f.world.address,f.pawn.address,f.controller.address,f.weapon.address,
        f.key.world_generation,f.controllers.session,f.controllers.space_epoch,f.controllers.input_epoch};
    return same_parent(f)&&driver.bound()&&driver.hold_generation()==owner.hold&&
        model.identity()==identity&&model.stage()==gears_weapon::TorqueBowStage::reload_steps&&
        model.held_arrow_serial()&&model.held_arrows()==1&&owner.lit&&
        hand::same(d.key,f.key)&&!d.release_required&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        hand::lease_valid(d.lease,f.key.lifecycle)&&d.lease.kind==hand::LeaseKind::ammo&&
        d.press_token==f.press_token&&d.lease.id==f.press_token&&!hand::index_pinch(f)&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
bool show(hand::Frame const& f,gears_committed_interaction::Frame const& avatar,hand::Decision const& d,
    gears_torque_bow_fire_driver::Driver const& driver,mount::Mount const& arrow_mount,tracking::Sample& out,
    gears_torque_bow_arrow_presentation::Config const* guide=nullptr,
    gears_torque_bow_arrow_presentation::Result* displayed=nullptr) noexcept {
    out={};if(displayed)*displayed={};
    const auto rejected=[&]() noexcept {withdraw();return false;};
    if(!weaponhand::current_gt(nullptr)||!held_arrow(f,d,driver)||
       arrow_mount.component_scale!=owner.original.world_scale||arrow_mount.units_per_metre!=owner.original.units_per_metre)
        return rejected();
    appearance::Appearance actual{};gears_rig_tracking::Frame raw{};tracking::Sample sample{};
    if(!appearance::read(f,avatar,actual))return rejected();
    if(!appearance::same(owner.original,actual)){retire();return false;}
    if(nativeequipment_rig::acquire(f,raw).status!=nativeequipment_rig::Status::ready||
       !tracking::sample(f,raw,arrow_mount,weaponhand::clock_ns(nullptr),sample))return rejected();
    gears_torque_bow_arrow_presentation::Result presentation{};
    presentation.raw=sample;presentation.source_world=sample.source_world;presentation.tip_world=sample.tip_world;presentation.grip_world=raw.world_grip[0];
    if(guide){
        if(guide->arrow_serial!=driver.interaction().held_arrow_serial()||
           guide->feed.instance_serial!=driver.interaction().instance_config_serial())return rejected();
        const bool keeping=owner.feed_assisted&&owner.assist_arrow==driver.interaction().held_arrow_serial()&&
            owner.assist_config==guide->feed.instance_serial&&owner.pose_sequence<=f.controllers.sequence&&
            owner.pose_sync_ns>0&&owner.pose_sync_ns<=f.controllers.sync_ns&&
            f.controllers.sync_ns-owner.pose_sync_ns<=guide->feed.mount.maximum_sample_gap_ns;
        if(!gears_torque_bow_arrow_presentation::make(f,raw,arrow_mount,*guide,weaponhand::clock_ns(nullptr),keeping,presentation))return rejected();
    }
    const auto angles=vr::ue_angles(vr::rotation_matrix(presentation.source_world.orientation));
    const vr::Viewpoint pose{presentation.source_world.position,angles};
    const playground::Rotation rotation{vr::to_rotator_units(angles.pitch),vr::to_rotator_units(angles.yaw),vr::to_rotator_units(angles.roll)};
    const vr::Vector scale{actual.world_scale,actual.world_scale,actual.world_scale};vr::Matrix engine_basis{};
    auto r=pool::find(owner.handle);
    if(!r||!pool::set_pose(f,owner.handle,owner.generation,d,pose,false)||
       !pool::engine_rotation_axes(f,*r,rotation,engine_basis)||!update(f,owner.handle,owner.generation,d))return rejected();
    const auto current=[&]() noexcept {
        return held_arrow(f,d,driver)&&!owner.retiring&&nativeequipment::current(nullptr,f)&&
            gears_equipment_rig::match(f,raw,weaponhand::clock_ns(nullptr),unsigned(f.stereo.source))==gears_equipment_rig::Status::matched;
    };
    std::array<float,16> matrix{};
    if(!pool::read_scene_matrix(f,owner.handle,owner.generation,matrix,current)||
       !gears_weapon::lancer_starter_matches_draw(matrix,pose,scale,&engine_basis)||!current()||
       !pool::set_pose(f,owner.handle,owner.generation,d,pose,true)||
       !pool::read_scene_matrix(f,owner.handle,owner.generation,matrix,current)||
       !gears_weapon::lancer_starter_matches_draw(matrix,pose,scale,&engine_basis)||!current())return rejected();
    // Observation only. A visible component is still no insertion, inventory
    // completion, authored palm/fingers fit or stereo-pixel acceptance.
    owner.pose_scene=sample.scene;owner.pose_sequence=sample.input_sequence;owner.pose_sync_ns=sample.input_sync_ns;
    owner.feed_assisted=presentation.feed_assisted;owner.assist_arrow=driver.interaction().held_arrow_serial();
    owner.assist_config=guide?guide->feed.instance_serial:0;
    if(displayed)*displayed=presentation;
    out=sample;return true;
}
}
