// Original SetPawnAmmoAttachment uses WarPawn's final left socket selection.
// Read its actual relative scale; a missing serialized default is no witness
// that the socket contributes unit scale. No socket/component is modified.
namespace torquebow_arrow_source {
unsigned parent_gate{};
bool vector_type(meta::Reader& r,meta::Property const& p,uptr& type) noexcept {
    unsigned item{},kind{},owner{},owner_kind{},package{},extent{};
    return p.element_bytes==12&&weaponhand::core_type(r,p.type,L"StructProperty")&&
        r.at(r.result.identities[p.identity].address,0xd0,type)&&r.path(type,item)&&r.named(item,L"Vector")&&
        r.path(r.result.identities[item].klass,kind)&&weaponhand::core_type(r,kind,L"ScriptStruct")&&
        r.path(r.result.identities[item].outer,owner)&&r.named(owner,L"Object")&&
        r.path(r.result.identities[owner].klass,owner_kind)&&weaponhand::core_type(r,owner_kind,L"Class")&&
        r.path(r.result.identities[owner].outer,package)&&r.named(package,L"Core")&&!r.result.identities[package].outer&&
        r.at(type,0x88,extent)&&extent==12;
}
struct Parent {hand::ObjectIdentity asset{},socket{};std::array<float,3> xyz{};};
bool same(Parent const& a,Parent const& b) noexcept {
    return hand::same(a.asset,b.asset)&&hand::same(a.socket,b.socket)&&a.xyz==b.xyz;
}
bool parent(meta::Reader& r,hand::Frame const& f,Parent& out) noexcept {
    out={};weaponhand::BoolField mirror{},socket_name{},array_field{};meta::PointerField mesh_asset{};
    parent_gate=1;if(!weaponhand::derives(r,f.pawn.address,L"WarPawn",L"WarfareGame")||
       !weaponhand::boolean(r,f.pawn.address,L"bIsMirrored",mirror)||
       !weaponhand::read_pointer(r,f.mesh.address,L"SkeletalMesh",mesh_asset)||
       !weaponhand::derives(r,mesh_asset.value,L"SkeletalMesh",L"Engine"))return false;
    const bool mirrored=(mirror.bits&mirror.mask)!=0;
    parent_gate=2;if(!(mirrored?weaponhand::field(r,f.pawn.address,L"RightHandSocketName",L"NameProperty",8,socket_name):
                  weaponhand::field(r,f.pawn.address,L"LeftHandSocketName",L"NameProperty",8,socket_name)))return false;
    meta::RawName selected{};
    parent_gate=3;if(!r.at(f.pawn.address,socket_name.property.offset,selected)||!selected.index||selected.number||
       !weaponhand::field(r,mesh_asset.value,L"Sockets",L"ArrayProperty",16,array_field))return false;
    uptr inner{};meta::Property element{};meta::Table sockets{},again_sockets{};
    parent_gate=4;if(!r.at(r.result.identities[array_field.property.identity].address,0xd0,inner)||
       !r.property(inner,element)||!weaponhand::core_type(r,element.type,L"ObjectProperty")||
       element.dimension!=1||element.element_bytes!=8||element.offset||
       r.result.identities[element.identity].outer!=r.result.identities[array_field.property.identity].address||
       !r.at(mesh_asset.value,array_field.property.offset,sockets)||!sockets.data||
       sockets.count<1||sockets.count>64||sockets.capacity<sockets.count||sockets.capacity>1024||
       sockets.data>UINTPTR_MAX-uptr(sockets.count)*8)return false;
    std::array<uptr,64> addresses{};Parent value{};unsigned matches{};
    for(int i=0;i<sockets.count;++i){
        auto& address=addresses[unsigned(i)];weaponhand::BoolField name_field{};meta::RawName name{};
        parent_gate=5;if(!r.at(sockets.data,uptr(i)*8,address)||!address||
           !weaponhand::derives(r,address,L"SkeletalMeshSocket",L"Engine")||
           !weaponhand::field(r,address,L"SocketName",L"NameProperty",8,name_field)||
           !r.at(address,name_field.property.offset,name)||!retain(r,name_field.property))return false;
        if(name.index!=selected.index||name.number!=selected.number)continue;
        weaponhand::BoolField relative{};uptr type{};
        parent_gate=6;if(++matches!=1||!lancermain_asset::identify(r,mesh_asset.value,value.asset)||
           !lancermain_asset::identify(r,address,value.socket)||value.socket.outer!=mesh_asset.value||
           !weaponhand::field(r,address,L"RelativeScale",L"StructProperty",12,relative)||
           !vector_type(r,relative.property,type)||!r.at(address,relative.property.offset,value.xyz)||
           !retain(r,relative.property))return false;
    }
    parent_gate=7;if(matches!=1||!retain(r,mirror.property)||!retain(r,socket_name.property)||!retain(r,array_field.property)||
       !retain(r,mesh_asset.property)||!retain(r,element)||
       !r.at(mesh_asset.value,array_field.property.offset,again_sockets)||!meta::same_table(sockets,again_sockets))return false;
    for(int i=0;i<sockets.count;++i){uptr again{};
        if(!r.at(sockets.data,uptr(i)*8,again)||again!=addresses[unsigned(i)])return false;}
    for(float axis:value.xyz)if(!std::isfinite(axis)||axis<=0)return false;
    out=value;parent_gate=0;return true;
}
}
