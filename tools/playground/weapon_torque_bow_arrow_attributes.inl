// Exact original local component settings and current avatar light reference.
// No setters, attachment, resource construction or hand/ammo grant here.
namespace torquebow_arrow_source {
unsigned attributes_gate{};
struct Attributes {gears_torque_bow_arrow_scale::Local scale{};hand::ObjectIdentity light{};Parent socket{};};
bool same(Attributes const& a,Attributes const& b) noexcept {
    return a.scale.scalar==b.scale.scalar&&a.scale.xyz==b.scale.xyz&&
        a.scale.absolute==b.scale.absolute&&hand::same(a.light,b.light)&&same(a.socket,b.socket);
}
bool attributes(hand::Frame const& f,Binding const& source,Attributes& out) noexcept {
    out={};Binding first{},last{};
    attributes_gate=1;if(!read(f,first)||!same(source,first))return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    r.reuse_initial_identities=true; // Discovery only; existing fresh closing checks disable it.
    attributes_gate=2;if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.mesh)||
       !weaponhand::derives(r,f.mesh.address,L"SkeletalMeshComponent",L"Engine"))return false;
    weaponhand::BoolField scalar{},vector{},absolute{};meta::PointerField pawn_mesh{},lighting{};
    std::array<float,3> xyz{},again_xyz{};float uniform{},again_uniform{};
    attributes_gate=3;if(!weaponhand::pointer(r,f.pawn.address,L"Mesh",f.mesh.address,pawn_mesh,true)||
       !weaponhand::field(r,source.component.address,L"Scale",L"FloatProperty",4,scalar)||
       !weaponhand::field(r,source.component.address,L"Scale3D",L"StructProperty",12,vector)||
       !weaponhand::boolean(r,source.component.address,L"AbsoluteScale",absolute)||
       !weaponhand::read_pointer(r,f.mesh.address,L"LightEnvironment",lighting,true)||
       !r.at(source.component.address,scalar.property.offset,uniform)||
       !r.at(source.component.address,vector.property.offset,xyz))return false;
    uptr type{};
    const auto descriptor=schema.identities[vector.property.identity].address;
    attributes_gate=4;if(!vector_type(r,vector.property,type))return false;
    Attributes value{};value.scale={double(uniform),{{double(xyz[0]),double(xyz[1]),double(xyz[2])}},(absolute.bits&absolute.mask)!=0};
    attributes_gate=5;if(!std::isfinite(uniform)||uniform<=0)return false;
    for(float axis:xyz)if(!std::isfinite(axis)||axis<=0)return false;
    attributes_gate=6;if(lighting.value&&(!lancermain_asset::identify(r,lighting.value,value.light)||
       !weaponhand::derives(r,lighting.value,L"LightEnvironmentComponent",L"Engine")))return false;
    attributes_gate=7;if(!parent(r,f,value.socket))return false;
    for(auto const& field:{scalar.property,vector.property,absolute.property,pawn_mesh.property,lighting.property}){
        if(!retain(r,field))return false;
    }
    r.reuse_initial_identities=false;
    weaponhand::BoolField again_absolute{};meta::PointerField again_mesh{},again_light{};uptr again_type{};unsigned again_extent{};Parent again_parent{};
    attributes_gate=8;if(!r.at(source.component.address,scalar.property.offset,again_uniform)||again_uniform!=uniform||
       !r.at(source.component.address,vector.property.offset,again_xyz)||again_xyz!=xyz||
       !weaponhand::boolean(r,source.component.address,L"AbsoluteScale",again_absolute)||
       !meta::same_property(absolute.property,again_absolute.property)||
       absolute.mask!=again_absolute.mask||absolute.bits!=again_absolute.bits||
       !weaponhand::pointer(r,f.pawn.address,L"Mesh",f.mesh.address,again_mesh,true)||!meta::same_pointer(pawn_mesh,again_mesh)||
       !weaponhand::read_pointer(r,f.mesh.address,L"LightEnvironment",again_light,true)||!meta::same_pointer(lighting,again_light)||
       !r.at(descriptor,0xd0,again_type)||again_type!=type||!r.at(type,0x88,again_extent)||again_extent!=12||
       !parent(r,f,again_parent)||!same(value.socket,again_parent)||!retained(r)||
       !read(f,last)||!same(source,last))return false;
    out=value;attributes_gate=0;return true;
}
}
