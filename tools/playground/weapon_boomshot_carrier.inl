// Hidden original carrier preparation. Uses the shared resource owner; taking
// ammunition, visibility, inventory commit and fitted hands are separate.
namespace boomshot_carrier {
namespace hand=gears_weapon_hand;namespace asset=weapon_part_assets;
namespace inv=lancerreload_inventory;
asset::Handle handle{};inv::Identity owner_identity{};
hand::ObjectIdentity original_component{},original_mesh{};
std::uint64_t generation{},next_generation{1};bool ready{};
float original_scale{};
enum class Preparation {unavailable,pending,ready,recovery_required};
bool selected(hand::Frame const& f) noexcept {
    return unified_weapon_fire::manual_profile()&&nativelancerfollow::bound_key_known&&
        nativelancerfollow::mount.bound()&&hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
        playground::instance(f.weapon.address,L"Weap_Boomer_base")&&nativeequipment::current(nullptr,f);
}
bool current(hand::Frame const& f) noexcept {
    return generation&&selected(f)&&owner_identity==inv::identity(f)&&asset::find(handle);
}
bool restore() noexcept {
    if(handle.generation&&asset::find(handle)&&!asset::destroy(handle,generation))return false;
    handle={};owner_identity={};original_component={};original_mesh={};generation=0;original_scale=0;ready=false;return true;
}
bool source_material(hand::Frame const& f,boomshot_original::Source const& source,hand::ObjectIdentity& out) noexcept {
    out={};if(!selected(f)||!asset::object(source.carrier_component)||!asset::object(source.carrier))return false;
    playground::Args call(source.carrier_component.address,L"GetMaterial");
    const auto in=call.prop(L"ElementIndex"),result=call.prop(L"ReturnValue");
    if(!asset::native_function(call,L"GetMaterial",L"MeshComponent")||
       !hammerburst_animation::input_field(call,in,L"IntProperty",4)||!vrequip::result_property(call,L"ObjectProperty",8)||
       !(get<unsigned>(in+0x94)+4<=get<unsigned>(result+0x94)||get<unsigned>(result+0x94)+8<=get<unsigned>(in+0x94))||
       !call.set(L"ElementIndex",0).run()||!selected(f))return false;
    const auto value=call.output<uptr>();
    if(!value||!playground::instance(value,L"MaterialInterface")||!nativeequipment::identify(nullptr,value,out))return false;
    namespace meta=gripdamage_metadata;
    meta::Snapshot context{};context.module_base=base;meta::Reader r{context,meta::Stage::final_check};meta::PointerField component{},mesh{};
    return meta::tables(base,context.objects,context.names)&&weaponhand::object(r,f.weapon)&&
        weaponhand::object(r,source.carrier_component)&&weaponhand::object(r,source.carrier)&&
        weaponhand::pointer(r,f.weapon.address,L"MagazineMesh",source.carrier_component.address,component,true)&&
        weaponhand::pointer(r,source.carrier_component.address,L"StaticMesh",source.carrier.address,mesh)&&
        unified_weapon_fire::retained_metadata(r)&&selected(f);
}
bool lighting(hand::Frame const& f) noexcept {
    if(!current(f))return false;
    namespace meta=gripdamage_metadata;
    meta::Snapshot context{};context.module_base=base;meta::Reader r{context,meta::Stage::final_check};
    meta::PointerField value{};hand::ObjectIdentity environment{};
    if(!meta::tables(base,context.objects,context.names)||!weaponhand::object(r,original_component)||
       !weaponhand::read_pointer(r,original_component.address,L"LightEnvironment",value,true)||
       (value.value&&(!lancermain_asset::identify(r,value.value,environment)||
           !playground::instance(value.value,L"LightEnvironmentComponent")))||
       !unified_weapon_fire::retained_metadata(r)||!current(f))return false;
    return asset::bind_light_environment(f,handle,generation,environment,!value.value);
}
Preparation prepare(hand::Frame const& f) noexcept try {
    if(!selected(f))return Preparation::unavailable;
    if(generation&&!current(f))return restore()?Preparation::pending:Preparation::recovery_required;
    if(generation){
        const auto state=asset::advance(f,handle,generation);
        if(state==asset::Creation::rejected)return Preparation::recovery_required;
        ready=state==asset::Creation::ready;
        if(ready){const auto resource=asset::find(handle);
            if(!resource||(!resource->lighting_bound&&!lighting(f))){ready=false;return Preparation::pending;}}
        return ready?Preparation::ready:Preparation::pending;
    }
    boomshot_original::Source source{};
    if(!boomshot_original::prepare(f,source))return Preparation::pending;
    const double units=nativelancerfollow::bound_units;
    if(!std::isfinite(units)||units<10||units>500)return Preparation::unavailable;
    gears_weapon::part_geometry::Conversion conversion{{{{1,0,0},{0,1,0},{0,0,1}}},units,f.reference_generation,true};
    gears_weapon::part_geometry::Prepared input{};
    if(weapon_part_inputs::load("boomshot-carrier",conversion,input)!=weapon_part_inputs::Result::ready)return Preparation::unavailable;
    const bool geist_boomer=playground::instance(f.pawn.address,L"Pawn_GeistBoomer");
    if(!gears_weapon::boomshot_carrier_geometry::apply_original_scale(input,geist_boomer))return Preparation::unavailable;
    hand::ObjectIdentity material{};
    if(!source_material(f,source,material)||!next_generation||next_generation==UINT64_MAX)return Preparation::pending;
    const auto next=next_generation++;asset::Handle created{};
    if(asset::create(f,next,input,{material},created)==asset::Creation::rejected)return Preparation::recovery_required;
    handle=created;generation=next;owner_identity=inv::identity(f);original_component=source.carrier_component;original_mesh=source.carrier;
    original_scale=gears_weapon::boomshot_carrier_geometry::original_scale(geist_boomer);
    return Preparation::pending;
}catch(...){return restore()?Preparation::unavailable:Preparation::recovery_required;}
}
