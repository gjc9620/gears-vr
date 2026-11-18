// Single-round visual resources use the existing GT resource owner. Preparing
// this hidden component never reserves or changes game ammunition.
namespace longshot_round {
void reset_presentation() noexcept;
namespace hand=gears_weapon_hand;
namespace asset=weapon_part_assets;
namespace inv=lancerreload_inventory;
namespace geom=gears_weapon::part_geometry;
asset::Handle handle{};
inv::Identity identity{};
nativelancerfollow::Key gun_key{};
std::uint64_t generation{},next_generation{1};
bool ready{},authored_pinch{};
hand::Lease held_lease{};
std::uint64_t item_serial{},last_pose_sequence{};
bool has_item() noexcept {return item_serial!=0;}
gears_vr::Pose item_in_grip{};
unsigned failure{},reported{~0u},report_count{};
bool eligible(hand::Frame const& f) noexcept {
    return unified_weapon_fire::manual_profile()&&nativelancerfollow::bound_key_known&&
        nativelancerfollow::bound_is_longshot&&nativelancerfollow::mount.bound()&&
        hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
        nativeequipment::current(nullptr,f);
}
bool current(hand::Frame const& f) noexcept {
    return generation&&eligible(f)&&identity==inv::identity(f)&&
        gun_key==nativelancerfollow::bound_key&&asset::find(handle);
}
bool restore() noexcept {
    if(handle.generation&&asset::find(handle)&&!asset::destroy(handle,generation))return false;
    handle={};identity={};gun_key={};generation=0;ready=false;
    held_lease={};item_serial=last_pose_sequence=0;item_in_grip={};authored_pinch=false;reset_presentation();return true;
}
bool lighting(hand::Frame const& f) noexcept {
    namespace meta=gripdamage_metadata;
    if(!current(f))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField value{};hand::ObjectIdentity environment{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,gun_key.component)||
       !weaponhand::read_pointer(r,gun_key.component.address,L"LightEnvironment",value,true)||
       (value.value&&(!lancermain_asset::identify(r,value.value,environment)||
           !playground::instance(value.value,L"LightEnvironmentComponent")))||
       !unified_weapon_fire::retained_metadata(r)||!current(f))return false;
    return asset::bind_light_environment(f,handle,generation,environment,!value.value);
}
void prepare(hand::Frame const& f) {
    failure=1;
    if(generation&&!current(f)){restore();return;}
    if(!eligible(f))return;
    if(generation){
        const auto state=asset::advance(f,handle,generation);
        failure=2;if(state==asset::Creation::rejected){restore();return;}
        ready=state==asset::Creation::ready;
        if(ready){const auto resource=asset::find(handle);
            failure=3;if(!resource||(!resource->lighting_bound&&!lighting(f))){ready=false;return;}}
        failure=0;return;
    }
    geom::Prepared data{};
    hammerburst_animation::Parts parts{};
    failure=8;if(!longshot_parts::read(f,parts))return;
    // Authored cartridge +Z becomes engine-local +Y. The right-handed axis
    // conversion and actual rig units are baked once into the rigid vertices.
    const double units=nativelancerfollow::bound_units;
    geom::Conversion conversion{{{{1,0,0},{0,0,1},{0,-1,0}}},units,f.reference_generation,true};
    failure=4;
    if(weapon_part_inputs::load("longshot-round-lod0",conversion,data)!=weapon_part_inputs::Result::ready)return;
    hand::ObjectIdentity material{};
    failure=5;if(!snub_visual::original_material(f,material)||!eligible(f)||!next_generation||next_generation==UINT64_MAX)return;
    // Reuse the current original material for the initial renderable asset.
    // Brass/projectile/primer appearance requires its own visual acceptance.
    std::vector<hand::ObjectIdentity> materials(3,material);
    const auto next=next_generation++;
    asset::Handle created{};
    failure=6;if(asset::create(f,next,data,materials,created)==asset::Creation::rejected)return;
    handle=created;generation=next;identity=inv::identity(f);gun_key=nativelancerfollow::bound_key;
    failure=0;
}
void tick(hand::Frame const& f) noexcept {
    const bool selected=nativelancerfollow::bound_key_known&&nativelancerfollow::bound_is_longshot&&
        hand::same(nativelancerfollow::bound_key.weapon,f.weapon);
    try{prepare(f);}catch(...){failure=7;restore();}
    const unsigned state=failure|(unsigned(ready)<<8);
    if((selected||generation||reported!=~0u)&&state!=reported&&log_file&&report_count<12){
        fwprintf(log_file,L"longshot_round_resource frame=%llu generation=%llu ready=%u gate=%u asset_gate=%u inventory_write=0\n",
            f.key.frame,generation,unsigned(ready),failure,asset::failure);fflush(log_file);reported=state;++report_count;
    }
}
}
