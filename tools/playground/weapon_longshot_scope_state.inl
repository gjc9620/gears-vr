// Read-only stock scope state. Visibility is not inferred from bZoomed:
// SetOwnerNoSee also touches the body/head components and controller flag.
namespace longshot_scope_state {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
struct Component {hand::ObjectIdentity identity{};bool owner_no_see{};};
struct State {
    std::array<Component,4> components{}; // Gun, body, head1, head2.
    bool controller_target{},pawn_target{},target_key{},zoom{},ignore_controllers{};
    float base_fov{};
};
bool same(State const& a,State const& b) noexcept {
    if(a.controller_target!=b.controller_target||a.pawn_target!=b.pawn_target||a.target_key!=b.target_key||
       a.zoom!=b.zoom||a.ignore_controllers!=b.ignore_controllers||a.base_fov!=b.base_fov)return false;
    for(unsigned i=0;i<a.components.size();++i)if(!hand::same(a.components[i].identity,b.components[i].identity)||
        a.components[i].owner_no_see!=b.components[i].owner_no_see)return false;
    return true;
}
bool component(uptr at,Component& out) noexcept {
    out={};if(!at)return true;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField flag{};Component value{};
    if(!meta::tables(base,schema.objects,schema.names)||!longshot_scope_original::identify(r,at,value.identity)||
       !weaponhand::boolean(r,at,L"bOwnerNoSee",flag)||!unified_weapon_fire::retained_metadata(r))return false;
    value.owner_no_see=(flag.bits&flag.mask)!=0;out=value;return true;
}
bool sample(hand::Frame const& source,State& out) noexcept {
    out={};
    if(!weaponhand::current_gt(nullptr)||!longshot_scope_original::actors(source))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    weaponhand::BoolField pc{},pawn{},key{},zoom{},ignore{},fov{};
    meta::PointerField gun{},body{},head1{},head2{};State value{};
    if(!meta::tables(base,schema.objects,schema.names)||
       !weaponhand::boolean(r,source.controller.address,L"bIsTargeting",pc)||
       !weaponhand::boolean(r,source.pawn.address,L"bIsTargeting",pawn)||
       !weaponhand::boolean(r,source.controller.address,L"bPressingTargetingKey",key)||
       !weaponhand::boolean(r,source.weapon.address,L"bZoomed",zoom)||
       !weaponhand::field(r,source.controller.address,L"DefaultFOV",L"FloatProperty",4,fov)||
       !r.at(source.controller.address,fov.property.offset,value.base_fov)||
       !std::isfinite(value.base_fov)||value.base_fov<2||value.base_fov>179||
       !weaponhand::read_pointer(r,source.weapon.address,L"Mesh",gun,true)||!gun.value||
       !weaponhand::read_pointer(r,source.pawn.address,L"Mesh",body,true)||!body.value||
       !weaponhand::read_pointer(r,source.pawn.address,L"HeadSlotStaticMesh",head1,true)||
       !weaponhand::read_pointer(r,source.pawn.address,L"HeadSlotStaticMesh2",head2,true)||
       !weaponhand::boolean(r,body.value,L"bIgnoreControllersWhenNotRendered",ignore)||
       !unified_weapon_fire::retained_metadata(r))return false;
    const uptr addresses[]={gun.value,body.value,head1.value,head2.value};
    for(unsigned i=0;i<4;++i)if(!component(addresses[i],value.components[i]))return false;
    value.controller_target=(pc.bits&pc.mask)!=0;value.pawn_target=(pawn.bits&pawn.mask)!=0;
    value.target_key=(key.bits&key.mask)!=0;value.zoom=(zoom.bits&zoom.mask)!=0;
    value.ignore_controllers=(ignore.bits&ignore.mask)!=0;
    if(!longshot_scope_original::actors(source))return false;
    out=value;return true;
}
bool read(hand::Frame const& source,State& out) noexcept {
    out={};State a{},b{};
    if(!sample(source,a)||!sample(source,b)||!same(a,b))return false;
    out=a;return true;
}
bool neutral(State const& s) noexcept {
    // First integration accepts only the stock fully neutral baseline. A
    // differently-owned body/head state is rejected BEFORE any setter; it
    // must not be silently reset by stock SetZoomed(false) during cleanup.
    if(s.controller_target||s.pawn_target||s.target_key||s.zoom||!s.ignore_controllers)return false;
    for(const auto& c:s.components)if(c.owner_no_see)return false;
    return hand::identity_valid(s.components[0].identity)&&hand::identity_valid(s.components[1].identity);
}
bool scoped(State const& s) noexcept {
    if(!s.controller_target||!s.pawn_target||s.target_key||s.ignore_controllers)return false;
    for(const auto& c:s.components)if(c.identity.address&&!c.owner_no_see)return false;
    return hand::identity_valid(s.components[0].identity)&&hand::identity_valid(s.components[1].identity);
}
} // namespace longshot_scope_state
