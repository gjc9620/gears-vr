// Original Snub rigid magazine, prepared privately from the purchased mesh.
// This owns one hidden transient component and a fixed wrist-relative mount.
// Original-bone visibility remains Native's single writer; ammo is separate.
namespace snub_visual {
namespace hand=gears_weapon_hand;namespace inv=lancerreload_inventory;namespace asset=weapon_part_assets;
namespace input=weapon_part_inputs;namespace geom=gears_weapon::part_geometry;namespace anim=hammerburst_animation;
using V=gears_vr::Vector;using P=gears_vr::Pose;using Q=gears_vr::Quaternion;
asset::Handle handle{};std::uint64_t generation{},next_generation{1};
inv::Identity identity{};nativelancerfollow::Key gun_key{};
hand::ObjectIdentity material{};double object_scale{},units{};
gears_weapon::SnubMagazineGeometry geometry;geom::Bounds bounds{};
gears_weapon::SnubMagazineMount mount;
bool ready{},held{};unsigned failure{};std::uint64_t last_move_sequence{};
V add(V a,V b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
V sub(V a,V b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
V mul(V a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
double length(V v) noexcept {return std::hypot(std::hypot(v.x,v.y),v.z);}
// Metadata/binding comparison only: safe inside the hand observer's gate.
// It must never call nativeequipment::current or any HandClient reader.
bool same_binding(hand::Frame const& f) noexcept {
    return input::original_source_enabled.load(std::memory_order_acquire)&&
        playground::instance(f.weapon.address,L"Weap_COGPistol")&&nativelancerfollow::bound_key_known&&
        hand::same(f.weapon,nativelancerfollow::bound_key.weapon)&&nativelancerfollow::mount.bound()&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation()&&
        nativelancerfollow::bound_session==f.stereo.session&&nativelancerfollow::bound_space==f.stereo.space_epoch&&
        nativelancerfollow::bound_reference==f.reference_generation&&nativelancerfollow::bound_world==f.key.world_generation&&
        nativelancerfollow::bound_input==f.controllers.input_epoch;
}
bool eligible(hand::Frame const& f) noexcept {
    return same_binding(f)&&nativeequipment::current(nullptr,f);
}
bool current(hand::Frame const& f) noexcept {
    return generation&&identity==inv::identity(f)&&gun_key==nativelancerfollow::bound_key&&eligible(f)&&
        units==nativelancerfollow::bound_units;
}
bool gun_pose(hand::Frame const& f,P& pose,double& scale) noexcept {
    vrequip::MeshPoseProbe measured{};
    if(!eligible(f)||!vrequip::mesh_pose_probe(f.weapon.address,measured)||
       measured.mesh!=nativelancerfollow::bound_key.component.address||
       measured.asset!=nativelancerfollow::bound_key.asset.address)return false;
    std::array<float,16> raw{};std::memcpy(raw.data(),measured.local_to_world,sizeof(measured.local_to_world));
    return hammerburst_runtime::matrix(raw,pose,scale)&&nativeequipment::current(nullptr,f);
}
bool original_material(hand::Frame const& f,hand::ObjectIdentity& out) noexcept {
    out={};lancermain_asset::Evidence mesh{};
    if(lancermain_asset::read(f,mesh,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready)return false;
    playground::Args call(mesh.component.address,L"GetMaterial");
    const auto in=call.prop(L"ElementIndex"),result=call.prop(L"ReturnValue");
    if(!asset::native_function(call,L"GetMaterial",L"MeshComponent")||
       !anim::input_field(call,in,L"IntProperty",4)||!vrequip::result_property(call,L"ObjectProperty",8)||
       !(get<unsigned>(in+0x94)+4<=get<unsigned>(result+0x94)||get<unsigned>(result+0x94)+8<=get<unsigned>(in+0x94))||
       !call.set(L"ElementIndex",0).run()||!nativeequipment::current(nullptr,f))return false;
    const auto address=call.output<uptr>();
    return playground::instance(address,L"MaterialInterface")&&nativeequipment::identify(nullptr,address,out)&&
        nativeequipment::current(nullptr,f);
}
bool lighting(hand::Frame const& f) noexcept {
    if(!current(f))return false;
    namespace meta=gripdamage_metadata;meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField value{};hand::ObjectIdentity environment{};
    const auto component=gun_key.component.address;
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,gun_key.component)||
       !weaponhand::read_pointer(r,component,L"LightEnvironment",value,true)||
       (value.value&&(!lancermain_asset::identify(r,value.value,environment)||!playground::instance(value.value,L"LightEnvironmentComponent")))||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,f))return false;
    return asset::bind_light_environment(f,handle,generation,environment,!value.value);
}
bool restore() noexcept {
    held=false;last_move_sequence=0;mount.reset();
    // The shared post-tick drain may already have retired this exact private
    // handle after a world/stop boundary. Never dereference or destroy a newer
    // generation occupying its old slot.
    if(handle.generation&&asset::find(handle)&&!asset::destroy(handle,generation))return false;
    handle={};generation=0;identity={};gun_key={};material={};geometry={};bounds={};ready=false;return true;
}
enum class Preparation {unavailable,pending,ready,recovery_required};
Preparation prepare(hand::Frame const& f) noexcept try {
    failure=1;if(!eligible(f))return Preparation::unavailable;
    if(generation&&!current(f))return restore()?Preparation::pending:Preparation::recovery_required;
    if(generation){
        const auto state=asset::advance(f,handle,generation);
        failure=2;if(state==asset::Creation::rejected)return Preparation::recovery_required;
        ready=state==asset::Creation::ready;
        if(ready){const auto resource=asset::find(handle);
            if(!resource||(!resource->lighting_bound&&!lighting(f))){ready=false;return Preparation::pending;}}
        failure=0;return ready?Preparation::ready:Preparation::pending;
    }
    if(!snub_part_visibility::function<gears_weapon_part_visibility::Begin>(gears_weapon_part_visibility::begin_export))
        return Preparation::unavailable;
    P gun{};double scale{};failure=3;if(!gun_pose(f,gun,scale))return Preparation::pending;
    snub_animation::Parts parts{};failure=4;if(!snub_animation::read_parts(f,parts))return Preparation::pending;
    P original_mag{};double mag_scale{};
    failure=5;if(!hammerburst_runtime::matrix(parts.world[4],original_mag,mag_scale)||std::abs(mag_scale-scale)>1e-3)
        return Preparation::pending;
    const auto inverse=gears_vr::inverse(gun.orientation);
    const V feed=gears_weapon::rotate(inverse,sub(original_mag.position,gun.position));
    const V outward=gears_weapon::rotate(inverse,gears_weapon::rotate(original_mag.orientation,{0,0,-1}));
    geom::Prepared data{};
    // The cache is original raw mesh/100, independent of the user's world
    // tracking scale. Bake this actual original component's object scale once
    // so the new rigid component uses unit scale and the same seated vertices.
    geom::Conversion conversion{{{{1,0,0},{0,1,0},{0,0,1}}},100.*scale,f.reference_generation,true};
    failure=6;if(input::load("snub-magazine",conversion,data)!=input::Result::ready)return Preparation::unavailable;
    gears_weapon::SnubMagazineGeometry fitted{};
    failure=7;if(!gears_weapon::snub_magazine_geometry(data.vertices.size(),[&](std::size_t i){
        const auto p=data.vertices[i].position;return V{p[0],p[1],p[2]};},feed,outward,true,fitted))return Preparation::unavailable;
    hand::ObjectIdentity surface{};failure=8;if(!original_material(f,surface))return Preparation::pending;
    P after{};double after_scale{};
    failure=9;if(!gun_pose(f,after,after_scale)||scale!=after_scale||!nativeequipment::current(nullptr,f)||
       !next_generation||next_generation==UINT64_MAX)return Preparation::pending;
    const auto next=next_generation++;
    asset::Handle created{};
    failure=10;if(asset::create(f,next,data,{surface},created)==asset::Creation::rejected)return Preparation::recovery_required;
    handle=created;generation=next;identity=inv::identity(f);gun_key=nativelancerfollow::bound_key;
    material=surface;object_scale=scale;units=nativelancerfollow::bound_units;geometry=fitted;bounds=data.bounds;
    ready=held=false;failure=0;return Preparation::pending;
}catch(...){return Preparation::recovery_required;}
gears_equipment_names::Lookup wrist_names({L"b_MF_Hand_L",L"b_MF_Hand_L",L"b_MF_Hand_L"});
bool wrist_name(hand::Frame const& f,std::array<unsigned,3>& names) noexcept {
    std::int64_t started{};
    return wrist_names.step(
        [](gears_equipment_names::Table& t){const auto a=get<Array>(base+0x1c34800);t={a.data,unsigned(a.count),unsigned(a.capacity)};return a.count>=0&&a.capacity>=0;},
        [](gears_equipment_names::Table t,unsigned i,std::array<wchar_t,32>& text){const auto e=get<uptr>(t.data+std::size_t(i)*8);unsigned index{};
            return e&&copy(&index,e,4)&&index==i&&copy(text.data(),e+20,sizeof(text));},
        [&](){return nativeequipment::current(nullptr,f);},[&](){const auto now=weaponhand::clock_ns(nullptr);
            if(!started)started=now;return now>=started&&now-started<3000000;},names,8192)==gears_equipment_names::State::ready;
}
bool wrist(hand::Frame const& f,P& out) noexcept {
    if(!eligible(f)||!f.left_tracked)return false;
    // Reuse Native's already validated actual C0 wrist when it belongs to
    // this EXACT stereo/controller ticket. Do not label an older endpoint
    // with f's new sequence/time; all other cases keep the original getter.
    gears_committed_interaction::Frame actual{};
    if(handrig::read_interaction(actual)&&
       gears_committed_interaction::matches(actual,f,vrequip::model.hold_generation(),weaponhand::clock_ns(nullptr),
           f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)&&
       gears_vr::stereo_same(actual.right.stereo,f.stereo)&&
       gears_hand_continuity::same_input(actual.right.controllers,f.controllers)&&nativeequipment::current(nullptr,f)){
        out=actual.left_wrist;return true;
    }
    std::array<unsigned,3> names{};if(!wrist_name(f,names))return false;
    playground::Args name(f.mesh.address,L"MatchRefBone"),matrix(f.mesh.address,L"GetBoneMatrix");
    if(!anim::component_getter(name,L"MatchRefBone")||!anim::component_getter(matrix,L"GetBoneMatrix")||
       !anim::input_field(name,name.prop(L"BoneName"),L"NameProperty",8)||
       !anim::input_field(matrix,matrix.prop(L"BoneIndex"),L"IntProperty",4)||
       !vrequip::result_property(name,L"IntProperty",4)||!vrequip::result_property(matrix,L"StructProperty",64))return false;
    const auto structure=get<uptr>(matrix.prop(L"ReturnValue")+0xd0);
    const auto disjoint=[](playground::Args& c,const wchar_t* input_name,unsigned width,unsigned output_width){
        const auto a=get<unsigned>(c.prop(input_name)+0x94),b=get<unsigned>(c.prop(L"ReturnValue")+0x94);
        return a+width<=b||b+output_width<=a;};
    if(!registered(structure)||!playground::named(structure,L"Matrix")||get<unsigned>(structure+0x88)!=64||
       !disjoint(name,L"BoneName",8,4)||!disjoint(matrix,L"BoneIndex",4,64)||
       !name.set(L"BoneName",Name{names[0],0}).run())return false;
    const auto bone=name.output<int>();
    if(bone<0||bone>=115||!matrix.set(L"BoneIndex",bone).run()||!nativeequipment::current(nullptr,f))return false;
    const auto raw=matrix.output<std::array<float,16>>();double scale{};
    return hammerburst_runtime::matrix(raw,out,scale)&&nativeequipment::current(nullptr,f);
}
bool granted(hand::Frame const& f,hand::Decision const& d) noexcept {
    return hand::same(f.key,d.key)&&!d.release_required&&
        (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
        (d.lease.kind==hand::LeaseKind::magazine||d.lease.kind==hand::LeaseKind::ammo)&&
        hand::lease_valid(d.lease,f.key.lifecycle)&&d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_down;
}
V nearest(V p) noexcept {
    return {std::clamp(p.x,double(bounds.centre[0]-bounds.extent[0]),double(bounds.centre[0]+bounds.extent[0])),
        std::clamp(p.y,double(bounds.centre[1]-bounds.extent[1]),double(bounds.centre[1]+bounds.extent[1])),
        std::clamp(p.z,double(bounds.centre[2]-bounds.extent[2]),double(bounds.centre[2]+bounds.extent[2]))};
}
bool capture(hand::Frame const& f,hand::Decision const& d) noexcept {
    failure=20;
    if(!ready||held||!current(f)||!granted(f,d)||mount.known())return false;
    P left{},gun{};double scale{};hand::ObjectIdentity surface{};
    failure=21;if(!wrist(f,left))return false;
    failure=22;if(!gun_pose(f,gun,scale)||std::abs(scale-object_scale)>1e-6)return false;
    failure=23;if(!original_material(f,surface)||!hand::same(material,surface))return false;
    const auto local=gears_weapon::rotate(gears_vr::inverse(gun.orientation),sub(left.position,gun.position));
    const auto contact=add(gun.position,gears_weapon::rotate(gun.orientation,nearest(local)));
    failure=24;if(!mount.capture(left,gun,contact,.045*units)||!nativeequipment::current(nullptr,f))return false;
    held=true;last_move_sequence=0;failure=0;return true;
}
bool hide() noexcept {
    held=false;last_move_sequence=0;
    if(!handle.generation)return true;
    const auto resource=asset::find(handle);
    if(!resource)return false;
    if(resource->stage!=asset::Stage::ready&&resource->stage!=asset::Stage::visible)return false;
    return asset::set_visible(handle,generation,false);
}
bool reacquire(hand::Frame const& f,hand::Decision const& d) noexcept {
    if(!ready||held||!mount.known()||!current(f)||!granted(f,d))return false;
    held=true;last_move_sequence=0;return true;
}
bool poses(hand::Frame const& f,P& gun,P& left,P& item) noexcept {
    double scale{};return current(f)&&mount.known()&&wrist(f,left)&&gun_pose(f,gun,scale)&&
        std::abs(scale-object_scale)<=1e-6&&mount.apply(left,item)&&nativeequipment::current(nullptr,f);
}
bool show(hand::Frame const& f,hand::Decision const& d,std::uint64_t serial) noexcept {
    if(!held||!ready||!granted(f,d)||!current(f)||!snub_part_visibility::hidden(f,serial))return false;
    P gun{},left{},item{};if(!poses(f,gun,left,item))return false;
    const gears_vr::Viewpoint target{item.position,gears_vr::ue_angles(gears_vr::rotation_matrix(item.orientation))};
    if(!asset::set_pose(f,handle,generation,d,target,true))return false;
    last_move_sequence=f.controllers.sequence;return true;
}
// Metre-scale feed observations use the ACTUAL wrist and same actual gun
// transform as the rendered rigid magazine. They do not use an assisted pose.
bool feed_sample(hand::Frame const& f,std::uint64_t item_id,gears_weapon::FeedPortMount& feed,gears_weapon::FeedSample& sample) noexcept {
    if(!held||!item_id||!mount.known())return false;
    P gun{},left{},item{};if(!poses(f,gun,left,item))return false;
    auto relative=mount.relative();relative.position=mul(relative.position,1/units);
    const auto anchor=mul(geometry.feed,1/units);
    feed={};feed.port_in_weapon={anchor,{}};feed.outward_axis_in_weapon=geometry.outward;
    feed.item_in_left_grip={add(relative.position,gears_weapon::rotate(relative.orientation,anchor)),relative.orientation};
    if(!feed.valid())return false;
    gun.position=mul(gun.position,1/units);left.position=mul(left.position,1/units);
    sample={f.weapon.address,item_id,f.world.address,f.stereo.space_epoch,f.controllers.sequence,
        f.controllers.sync_ns,gun,left,bool(f.left_tracked),bool(f.left_grip_down)};
    return nativeequipment::current(nullptr,f);
}
} // namespace snub_visual
