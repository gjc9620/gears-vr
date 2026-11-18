// Original possession observation only. Deliberately NOT a hand Frame and
// never substituted into the ordinary WarPawn participant/currentness path.
namespace troika_mounted_context {
namespace hand=gears_weapon_hand;
namespace meta=weaponhand::meta;
struct Context {
    hand::ObjectIdentity world{},controller{},driver{},driver_mesh{},turret{},turret_mesh{},weapon{};
    std::uint64_t entry_binding{},entry_token{};
    std::int64_t observed_ns{};
};
template<std::size_t N>bool pointer(meta::Reader& reader,uptr object,wchar_t const (&label)[N],
    hand::ObjectIdentity& result,bool component=false) noexcept {
    meta::PointerField field{};
    return weaponhand::read_pointer(reader,object,label,field,component)&&field.value&&
        nativeequipment::identify(nullptr,field.value,result)&&weaponhand::object(reader,result);
}
// Camera.ViewTarget is an Engine.Camera.TViewTarget STRUCT, not an
// ObjectProperty. Validate its Target member and container bounds separately.
bool camera_target(uptr camera,uptr expected) noexcept {
    const auto raw=playground::property(camera,L"ViewTarget");
    const auto bytes=raw?get<unsigned>(raw+0x74):0;
    if(!bytes||bytes>4096)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField container{};troika_reflection::ScriptStruct structure{};
    if(!meta::tables(base,schema.objects,schema.names)||
       !weaponhand::field(r,camera,L"ViewTarget",L"StructProperty",bytes,container)||
       !troika_reflection::read(r,container.property,L"TViewTarget",L"Camera",L"Engine",bytes,structure)||
       r.result.identities[structure.layout.identity].outer!=r.result.identities[container.owner].address)return false;
    if(camera>UINTPTR_MAX-container.property.offset)return false;
    const auto value=camera+container.property.offset;
    return troika_reflection::object_member(r,structure,value,L"Target",expected)&&
        troika_reflection::current(r,structure,L"TViewTarget",L"Camera",L"Engine")&&
        troika_reflection::object_member(r,structure,value,L"Target",expected)&&unified_weapon_fire::retained_metadata(r);
}
bool observe(troika_original::EntryOwner const& owner,Context& out) noexcept {
    out={};
    if(!enabled.load(std::memory_order_acquire)||!weaponhand::current_gt(nullptr)||!owner.outstanding||
       owner.receipt.result!=troika_original::EntryResult::mounted)return false;
    const auto& entry=owner.receipt;
    if(entry.binding!=owner.binding||entry.token!=owner.last_token)return false;
    Context value{};value.world=entry.world;value.controller=entry.controller;
    value.driver=entry.driver;value.turret=entry.turret;
    value.entry_binding=entry.binding;value.entry_token=entry.token;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,value.world)||
       !weaponhand::object(reader,value.controller)||!weaponhand::object(reader,value.driver)||
       !weaponhand::object(reader,value.turret)||
       !weaponhand::derives(reader,value.world.address,L"WorldInfo",L"Engine")||
       !weaponhand::derives(reader,value.controller.address,L"WarPC",L"WarfareGame")||
       !weaponhand::derives(reader,value.driver.address,L"WarPawn",L"WarfareGame")||
       !weaponhand::derives(reader,value.turret.address,L"Turret_TroikaCabal_base",L"WarfareGame")||
       !pointer(reader,value.driver.address,L"Mesh",value.driver_mesh,true)||
       !pointer(reader,value.turret.address,L"Mesh",value.turret_mesh,true)||
       !pointer(reader,value.turret.address,L"Weapon",value.weapon)||
       !weaponhand::derives(reader,value.driver_mesh.address,L"SkeletalMeshComponent",L"Engine")||
       !weaponhand::derives(reader,value.turret_mesh.address,L"SkeletalMeshComponent",L"Engine")||
       !weaponhand::derives(reader,value.weapon.address,L"Weap_Troika_base",L"WarfareGame"))return false;
    hand::ObjectIdentity manager{},camera{};
    if(!pointer(reader,value.driver.address,L"InvManager",manager)||
       !pointer(reader,value.controller.address,L"PlayerCamera",camera)||
       !weaponhand::derives(reader,manager.address,L"WarInventoryManager",L"WarfareGame")||
       !weaponhand::derives(reader,camera.address,L"Camera",L"Engine"))return false;
    const auto links=[&](){
        using troika_original::link;
        return link(value.controller.address,L"Pawn",value.turret.address)&&
            link(value.turret.address,L"Controller",value.controller.address)&&
            link(value.turret.address,L"Driver",value.driver.address)&&
            link(value.driver.address,L"DrivenVehicle",value.turret.address)&&
            link(value.driver.address,L"Controller",0)&&
            link(value.driver.address,L"InvManager",manager.address)&&link(manager.address,L"Owner",value.driver.address)&&
            link(value.controller.address,L"PlayerCamera",camera.address)&&
            link(camera.address,L"PCOwner",value.controller.address)&&camera_target(camera.address,value.turret.address)&&
            link(value.controller.address,L"WorldInfo",value.world.address)&&
            link(value.driver.address,L"WorldInfo",value.world.address)&&
            link(value.turret.address,L"WorldInfo",value.world.address)&&
            link(camera.address,L"WorldInfo",value.world.address)&&
            link(value.weapon.address,L"Instigator",value.turret.address)&&
            link(value.driver.address,L"Mesh",value.driver_mesh.address,true)&&
            link(value.turret.address,L"Mesh",value.turret_mesh.address,true)&&
            link(value.turret.address,L"Weapon",value.weapon.address);
    };
    if(!links()||!unified_weapon_fire::retained_metadata(reader)||!links()||
       !troika_original::same_object(value.world)||!troika_original::same_object(value.controller)||
       !troika_original::same_object(value.driver)||!troika_original::same_object(value.turret))return false;
    value.observed_ns=weaponhand::clock_ns(nullptr);out=value;return true;
}
} // namespace troika_mounted_context
