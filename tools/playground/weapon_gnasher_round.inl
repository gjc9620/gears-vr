// Hidden preparation and real held-shell display on the existing GT resource
// owner. No ammo setter/reservation, skeletal writer or original shell reuse.
namespace gnasher_runtime {bool round_route_enabled() noexcept;}
namespace gnasher_round {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;
namespace asset=weapon_part_assets;namespace geom=gears_weapon::part_geometry;
namespace pose=gears_gnasher_round_pose;
asset::Handle handle{};inv::Identity identity{},released_identity{},last_item_identity{};
hand::ObjectIdentity component{},mesh{};
std::uint64_t generation{},next_generation{1},item_serial{},last_item{},released_item{},last_sequence{},last_frame{};
std::uint64_t reference{},profile{};double units{};
bool ready{};hand::Lease held{};pose::Mount held_mount{};
unsigned failure{},reported{~0u},rows{};
bool held_for(hand::ObjectIdentity const& weapon) noexcept {
    return item_serial&&hand::same(identity.weapon,weapon);
}
bool retains_ammo_route(hand::Frame const& f) noexcept {
    // Source-reader metadata only. This retains the existing captured route;
    // it is not a new grant, current-source witness or insertion permission.
    return item_serial&&item_serial==f.press_token&&identity==inv::identity(f)&&
        reference==f.reference_generation&&profile==f.controllers.profile_generation&&
        held.kind==hand::LeaseKind::ammo&&held.id==item_serial&&hand::lease_valid(held,f.key.lifecycle)&&
        f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
void clear_held() noexcept {
    if(item_serial){released_item=item_serial;released_identity=identity;}
    item_serial=0;held={};held_mount={};last_sequence=last_frame=0;
}
bool restore() noexcept {
    if(handle.generation&&asset::find(handle)&&!asset::destroy(handle,generation))return false;
    clear_held();handle={};generation=0;identity={};component=mesh={};ready=false;return true;
}
bool eligible(hand::Frame const& f,lancermain_asset::Evidence& evidence) noexcept {
    lancermain_asset::HeldAsset kind{};
    return unified_weapon_fire::manual_profile()&&nativelancerfollow::bound_key_known&&
        nativelancerfollow::mount.bound()&&hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
        nativelancerfollow::bound_session==f.stereo.session&&nativelancerfollow::bound_space==f.stereo.space_epoch&&
        nativelancerfollow::bound_input==f.controllers.input_epoch&&nativelancerfollow::bound_world==f.key.world_generation&&
        nativelancerfollow::bound_reference==f.reference_generation&&
        lancermain_asset::read(f,evidence,lancermain_asset::AssetScope::basic_hold,&kind)==lancermain_asset::Status::ready&&
        kind==lancermain_asset::HeldAsset::gnasher&&hand::same(evidence.component,nativelancerfollow::bound_key.component)&&
        hand::same(evidence.asset,nativelancerfollow::bound_key.asset)&&nativeequipment::current(nullptr,f);
}
bool current(hand::Frame const& f) noexcept {
    lancermain_asset::Evidence evidence{};
    return generation&&identity==inv::identity(f)&&reference==f.reference_generation&&
        profile==f.controllers.profile_generation&&units==nativelancerfollow::bound_units&&
        asset::find(handle)&&eligible(f,evidence)&&hand::same(component,evidence.component)&&hand::same(mesh,evidence.asset);
}
bool captured_for(hand::Frame const& f,std::uint64_t item) noexcept {
    hand::GripSnapshot actual{};
    return item&&item_serial==item&&f.press_token==item&&held.kind==hand::LeaseKind::ammo&&
        held.id==item&&hand::lease_valid(held,f.key.lifecycle)&&f.left_tracked&&f.left_grip_known&&f.left_grip_down&&
        ready&&current(f)&&weaponhand::client.snapshot_on_gt(actual)&&hand::same(actual.left_hand,held)&&
        gears_gnasher_raw::ownership_matches(f,actual,weaponhand::clock_ns(nullptr));
}
bool lighting(hand::Frame const& f) noexcept {
    namespace meta=gripdamage_metadata;
    if(!current(f))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField value{};hand::ObjectIdentity environment{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,component)||
       !weaponhand::read_pointer(r,component.address,L"LightEnvironment",value,true)||
       (value.value&&(!lancermain_asset::identify(r,value.value,environment)||
                     !playground::instance(value.value,L"LightEnvironmentComponent")))||
       !unified_weapon_fire::retained_metadata(r)||!current(f))return false;
    return asset::bind_light_environment(f,handle,generation,environment,!value.value);
}
void prepare(hand::Frame const& f) {
    failure=1;
    if(asset::stop_requested.load(std::memory_order_acquire)||
       lancerreload_runtime::stop_requested.load(std::memory_order_acquire)){
        (void)restore();return;
    }
    if(generation&&!current(f)){(void)restore();return;}
    lancermain_asset::Evidence evidence{};if(!eligible(f,evidence))return;
    if(generation){
        const auto state=asset::advance(f,handle,generation);
        failure=2;if(state==asset::Creation::rejected){(void)restore();return;}
        ready=state==asset::Creation::ready;
        if(ready){auto* resource=asset::find(handle);failure=3;
            if(!resource||(!resource->lighting_bound&&!lighting(f))){ready=false;return;}}
        failure=0;return;
    }
    const double scale=nativelancerfollow::bound_units;
    if(!std::isfinite(scale)||scale<10||scale>500||!f.reference_generation)return;
    geom::Conversion conversion{{{{1,0,0},{0,0,1},{0,-1,0}}},scale,f.reference_generation,true};
    geom::Prepared data{};failure=4;
    if(weapon_part_inputs::load("gnasher-round-lod0",conversion,data)!=weapon_part_inputs::Result::ready)return;
    hand::ObjectIdentity material{};failure=5;
    if(!asset::held_weapon_material(f,material)||!eligible(f,evidence)||!next_generation||next_generation==UINT64_MAX)return;
    // Initial renderable geometry uses a compatible ORIGINAL material. This
    // does not import the GLB PBR colours: brass/red/crimp appearance is pending.
    std::vector<hand::ObjectIdentity> materials(pose::material_slots,material);
    const auto next=next_generation++;asset::Handle created{};failure=6;
    if(asset::create(f,next,data,materials,created)==asset::Creation::rejected)return;
    handle=created;generation=next;identity=inv::identity(f);component=evidence.component;mesh=evidence.asset;
    reference=f.reference_generation;profile=f.controllers.profile_generation;units=scale;ready=false;failure=0;
}
bool granted(hand::Frame const& f,hand::Decision const& d) noexcept {
    return hand::same(f.key,d.key)&&d.press_token==f.press_token&&!d.release_required&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        d.lease.kind==hand::LeaseKind::ammo&&hand::lease_valid(d.lease,f.key.lifecycle)&&
        d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_known&&f.left_grip_down;
}
bool capture(hand::Frame const& f,hand::Decision const& d,std::uint64_t item,
             pose::Mount const& mount,gears_weapon::GnasherMechanicalConfig const& config) noexcept {
    if(!item||item!=d.lease.id||(inv::same_instance_across_hold(last_item_identity,identity)&&item<=last_item)||
       item_serial||!ready||!current(f)||!d.new_grant||
       !granted(f,d)||mount.reference_generation!=f.reference_generation||!mount.matches_feed(config))return false;
    const gears_vr::Pose body{f.stereo.head.position,f.reference.orientation};
    if(gears_weapon::body_slot(nativeequipment::calibration,body,f.controllers.hands[0].grip.position,0)!=
       gears_weapon::BodySlot::shoulder_ammo)return false;
    held=d.lease;held_mount=mount;item_serial=last_item=item;last_item_identity=identity;
    last_sequence=last_frame=0;return true;
}
bool raw_pose(hand::Frame const& f,hand::Decision const& d,gears_vr::Pose& out) noexcept {
    out={};if(!item_serial||!ready||!current(f)||!granted(f,d)||!hand::same(held,d.lease))return false;
    gears_rig_tracking::Frame rig{};const auto source=static_cast<std::uint32_t>(f.stereo.source);
    return handrig::read(rig,0,source).reply==gears_rig_tracking::Reply::accepted&&
        gears_equipment_rig::match(f,rig,weaponhand::clock_ns(nullptr),source)==gears_equipment_rig::Status::matched&&
        (rig.world_pose_mask&gears_rig_tracking::left_grip_pose)&&rig.units_per_metre==units&&
        pose::world_root(held_mount,rig.world_grip[0],units,out)&&current(f);
}
bool show(hand::Frame const& f,hand::Decision const& d) noexcept {
    gears_vr::Pose value{};
    if(!raw_pose(f,d,value)){
        if(generation&&asset::find(handle))(void)asset::set_visible(handle,generation,false);
        last_sequence=last_frame=0;return false;
    }
    if(f.controllers.sequence<last_sequence||f.key.frame<last_frame)return false;
    if(f.controllers.sequence==last_sequence&&f.key.frame==last_frame)return true;
    if(!asset::set_pose(f,handle,generation,d,{value.position,gears_vr::ue_angles(gears_vr::rotation_matrix(value.orientation))},true))return false;
    last_sequence=f.controllers.sequence;last_frame=f.key.frame;return true;
}
// Metadata receipt survives a late private consumer: if withdrawal succeeded
// but its source expired, the next frame may retire that SAME held token only.
bool release_for_stow(inv::Identity const& owner,std::uint64_t item) noexcept {
    if(!item)return !item_serial;
    if(!item_serial)return released_item==item&&released_identity==owner;
    if(item_serial!=item||!(identity==owner))return false;
    if(generation&&asset::find(handle)&&!asset::set_visible(handle,generation,false))return false;
    clear_held();return true;
}
void tick(hand::Frame const& f) noexcept {
    // An unavailable feed route cannot capture a round. Do not repeatedly
    // prepare an unused hidden resource before firing. Existing resources or
    // held items keep the original validation/withdrawal/cleanup path.
    if(!generation&&!handle.generation&&!item_serial&&!gnasher_runtime::round_route_enabled())return;
    try{prepare(f);}catch(...){failure=7;(void)restore();}
    const unsigned state=failure|(unsigned(ready)<<8);
    if((generation||reported!=~0u)&&state!=reported&&log_file&&rows<12){
        fwprintf(log_file,L"gnasher_round_resource frame=%llu generation=%llu ready=%u gate=%u ammo_write=0 original_material_provisional=1\n",
            f.key.frame,generation,unsigned(ready),failure);fflush(log_file);reported=state;++rows;
    }
}
} // namespace gnasher_round
