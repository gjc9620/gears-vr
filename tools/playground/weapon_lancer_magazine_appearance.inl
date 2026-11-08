// Same original Lancer magazine geometry/UV and current weapon material.
// Prepare while idle, swap ONLY the leased retail MagazineMesh's asset, and
// restore it before releasing the transient resource. No new gameplay writer.
namespace lancer_magazine_appearance {
namespace hand=gears_weapon_hand;namespace asset=weapon_part_assets;
namespace visual=lancerreload_visual;namespace geometry=gears_weapon::part_geometry;
asset::Handle handle{};asset::WeaponPoseOwner owner{};
hand::FrameKey key{};hand::ObjectIdentity component{},original{},surface{},installed_mesh{};
std::uint64_t generation{},next_generation{1};
bool ready{},installed{};unsigned failure{};
bool same(hand::Frame const& f) noexcept {
    return generation&&key.registration_cookie==f.key.registration_cookie&&key.lifecycle==f.key.lifecycle&&
        key.game_start==f.key.game_start&&key.native_started_qpc==f.key.native_started_qpc&&
        key.world_generation==f.key.world_generation&&asset::same_weapon(owner,f)&&
        asset::object(component)&&asset::object(original)&&asset::object(surface)&&
        playground::pointer(f.weapon.address,L"MagazineMesh")==component.address;
}
bool set_mesh(hand::ObjectIdentity const& c,hand::ObjectIdentity const& mesh) noexcept {
    if(!asset::gt()||!asset::object(c)||!asset::object(mesh))return false;
    playground::Args call(c.address,L"SetStaticMesh");const auto p=call.prop(L"NewMesh");
    if(!asset::native_function(call,L"SetStaticMesh",L"StaticMeshComponent")||!p||
       !playground::named(get<uptr>(p+0x50),L"ObjectProperty")||get<unsigned>(p+0x74)!=8||
       !call.set(L"NewMesh",mesh.address).run())return false;
    return asset::object(c)&&asset::object(mesh)&&playground::pointer(c.address,L"StaticMesh")==mesh.address;
}
bool uninstall(hand::ObjectIdentity const& c,hand::ObjectIdentity& current_asset) noexcept {
    if(!installed)return true;
    if(!hand::same(c,component)||!asset::object(c)||!asset::object(original))return false;
    const auto current=playground::pointer(c.address,L"StaticMesh");
    if(current!=original.address&&current!=installed_mesh.address)return false;
    if(current!=original.address&&!set_mesh(c,original))return false;
    current_asset=original;installed=false;installed_mesh={};return true;
}
bool retire() noexcept {
    if(installed)return false;
    if(handle.generation&&asset::find(handle)&&!asset::destroy(handle,generation))return false;
    handle={};owner={};key={};component=original=surface={};generation=0;ready=false;return true;
}
// The caller already proves registry retirement, including the component's
// own old identity. Do not leave a stale "installed" bit blocking a later
// weapon, and never restore through a recycled component address.
bool retired_weapon(hand::ObjectIdentity const& weapon,bool component_gone) noexcept {
    if(!installed)return true;
    if(!hand::same(weapon,owner.weapon))return false;
    if(component_gone){installed=false;installed_mesh={};return retire();}
    auto old=installed_mesh;
    return uninstall(component,old)&&retire();
}
bool scale(hand::ObjectIdentity const& c,double& out) noexcept {
    playground::Vector xyz{};const auto p=playground::property(c.address,L"Scale");
    if(!asset::object(c)||!p||!playground::named(get<uptr>(p+0x50),L"FloatProperty")||
       get<unsigned>(c.klass+0x88)<4||get<unsigned>(p+0x74)!=4||get<unsigned>(p+0x94)>get<unsigned>(c.klass+0x88)-4||
       !visual::read_struct(c.address,L"Scale3D",xyz))return false;
    float uniform{};if(!copy(&uniform,c.address+get<unsigned>(p+0x94),4))return false;
    out=double(uniform)*xyz.x;
    return std::isfinite(out)&&out>=.1&&out<=10&&std::abs(xyz.x-xyz.y)<1e-5&&std::abs(xyz.x-xyz.z)<1e-5;
}
bool no_overrides(hand::ObjectIdentity const& c) noexcept {
    const auto p=playground::property(c.address,L"Materials");Array a{};
    return asset::object(c)&&p&&playground::named(get<uptr>(p+0x50),L"ArrayProperty")&&
        get<unsigned>(c.klass+0x88)>=sizeof a&&get<unsigned>(p+0x74)==sizeof a&&get<unsigned>(p+0x94)<=get<unsigned>(c.klass+0x88)-sizeof a&&
        copy(&a,c.address+get<unsigned>(p+0x94),sizeof a)&&a.count==0;
}
// Optional visual preparation never withholds a player's fresh grasp. All
// package decode is pre-launch; these existing resource phases run once per
// held weapon while neither hand is starting/manipulating a small part.
void prepare(hand::Frame const& f) noexcept try {
    if(!asset::gt()||visual::active||f.left_grip_down||!nativelancerfollow::bound_is_lancer||
       !weapon_part_inputs::original_source_enabled.load(std::memory_order_acquire))return;
    if(generation&&!same(f)){if(!retire())return;}
    if(generation){
        const auto result=asset::advance(f,handle,generation);
        ready=result==asset::Creation::ready;failure=ready?0:2;return;
    }
    failure=1;asset::WeaponPoseOwner selected{};vrequip::MagazineVisualProbe mag{};
    if(!asset::weapon_owner(f,selected)||!nativeequipment::current(nullptr,f)||
       !vrequip::magazine_visual_probe(f.weapon.address,f.pawn.address,mag)||!mag.present||mag.attached_to_pawn||
       std::wcscmp(mag.asset_name,L"Assault_Rifle_Magazine"))return;
    hand::ObjectIdentity c{},a{},material{};
    if(!nativeequipment::identify(nullptr,mag.component,c)||!nativeequipment::identify(nullptr,mag.asset,a)||!no_overrides(c))return;
    double mag_scale{};std::array<float,16> gun{};vrequip::MeshPoseProbe measured{};
    if(!scale(c,mag_scale)||!vrequip::mesh_pose_probe(f.weapon.address,measured)||
       measured.mesh!=selected.component.address||measured.asset!=selected.asset.address||measured.hidden)return;
    std::memcpy(gun.data(),measured.local_to_world,sizeof measured.local_to_world);
    const double gun_scale=std::hypot(std::hypot(double(gun[0]),double(gun[1])),double(gun[2]));
    if(!std::isfinite(gun_scale)||gun_scale<.1||gun_scale>10)return;
    geometry::Conversion conversion{{{{1,0,0},{0,1,0},{0,0,1}}},100.*gun_scale/mag_scale,f.reference_generation,true};
    geometry::Prepared data{};failure=3;
    if(weapon_part_inputs::load("lancer-magazine",conversion,data)!=weapon_part_inputs::Result::ready)return;
    uptr material_address{};failure=4;
    if(!visual::read_material(selected.component.address,material_address)||
       !nativeequipment::identify(nullptr,material_address,material)||!nativeequipment::current(nullptr,f)||
       !next_generation||next_generation==UINT64_MAX)return;
    const auto next=next_generation++;asset::Handle created{};failure=5;
    if(asset::create(f,next,data,{material},created)==asset::Creation::rejected)return;
    handle=created;generation=next;owner=selected;key=f.key;component=c;original=a;surface=material;
    ready=false;failure=0;
}catch(...){failure=6;}
bool install(hand::Frame const& f,hand::ObjectIdentity const& c,hand::ObjectIdentity& current_asset) noexcept {
    // Missing preparation preserves the previous visual, never fabricates an
    // asset or delays the actual input. Such a run cannot pass appearance QA.
    if(!ready){if(log_file)fwprintf(log_file,L"lancer_magazine_appearance ready=0 gate=%u fallback_original=1\n",failure);return true;}
    const auto r=asset::find(handle);
    if(installed||!same(f)||!r||r->stage!=asset::Stage::ready||!hand::same(c,component)||
       !hand::same(current_asset,original)||!no_overrides(c)||!asset::object(r->mesh)||
       playground::pointer(c.address,L"StaticMesh")!=original.address||!nativeequipment::current(nullptr,f))return false;
    // Journal before the setter; on ambiguous failure both the original and
    // exact replacement are recognized for restoration, never a foreign mesh.
    installed=true;installed_mesh=r->mesh;
    const bool result=set_mesh(c,installed_mesh);
    const auto actual_asset=playground::pointer(c.address,L"StaticMesh");
    if(actual_asset==installed_mesh.address)current_asset=installed_mesh;
    if(log_file){fwprintf(log_file,L"lancer_magazine_appearance ready=1 installed=%u component=%llx original=%llx replacement=%llx weapon_material=%llx same_original_uv=1 vertices=120 triangles=156\n",
        unsigned(result),c.address,original.address,installed_mesh.address,surface.address);fflush(log_file);}
    return result;
}
}
