#define main existing_asset_fixture
#include "weapon_lancer_asset_read_test.cpp"
#undef main
#include <cmath>
#include <limits>
namespace {
using Name=gripdamage_metadata::RawName;
namespace playground {struct Vector{float x{},y{},z{};};struct Rotation{std::int32_t pitch{},yaw{},roll{};};}
namespace unified_weapon_fire {
namespace metadata=gripdamage_metadata;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
}
#include "weapon_boomshot_socket_metadata.inl"
namespace socket=boomshot_socket;
struct SocketFixture:AssetMemory {
    uptr name_type{},array_type{},struct_type{},script_type{},socket_class{},magazine{},muzzle{},location{},scale_field{},rotation{};
    uptr sockets_property{},inner{},bone{},vector{},rotator{};std::array<uptr,2> sockets{};la::Evidence evidence{};
    SocketFixture(){
        name_type=node(L"NameProperty",class_type,core);array_type=node(L"ArrayProperty",class_type,core);
        struct_type=node(L"StructProperty",class_type,core);script_type=node(L"ScriptStruct",class_type,core);
        const auto original=cls(L"Weap_Boomer_base",warfare,weapon_class),content=node(L"warfaregamecontentweapons",package_type);
        const auto boomer=cls(L"Weap_Boomer",content,original),package=node(L"Locust_Boomshot",package_type),group=node(L"Mesh",package_type,package);
        put(weapon,0x50,boomer);put(asset,0x48,text(L"Locust_Boomshot"));put(asset,0x40,group);
        socket_class=cls(L"SkeletalMeshSocket",engine);magazine=node(L"MagazineSocket",socket_class,asset);muzzle=node(L"MuzzleSocket",socket_class,asset);
        property(socket_class,L"SocketName",name_type,0x100,8);bone=property(socket_class,L"BoneName",name_type,0x108,8);
        location=property(socket_class,L"RelativeLocation",struct_type,0x110,12);
        rotation=property(socket_class,L"RelativeRotation",struct_type,0x120,12);scale_field=property(socket_class,L"RelativeScale",struct_type,0x130,12);
        const auto object=cls(L"Object",core);vector=node(L"Vector",script_type,object);rotator=node(L"Rotator",script_type,object);
        put(vector,0x88,12u);put(rotator,0x88,12u);put(location,0xd0,vector);put(rotation,0xd0,rotator);put(scale_field,0xd0,vector);
        put(magazine,0x100,text(L"Magazine"));put(magazine,0x108,text(L"Boomshot_Magazine"));put(magazine,0x130,std::array<float,3>{1,1,1});
        put(muzzle,0x100,text(L"Muzzle"));sockets={muzzle,magazine};
        sockets_property=property(asset_class,L"Sockets",array_type,0x140,16);inner=node(L"SocketInner",object_type,sockets_property);
        put(inner,0x70,1u);put(inner,0x74,8u);put(sockets_property,0xd0,inner);
        put(asset,0x140,meta::Table{reinterpret_cast<uptr>(sockets.data()),2,2});frame.weapon=identity(weapon);tables();
        assert(la::read(frame,evidence,la::AssetScope::boomshot_projectile)==la::Status::ready);
    }
    bool inspect(socket::Source& out){
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader reader{snapshot,meta::Stage::final_check};socket::Reads reads{};
        socket::Source before{},after{};
        if(!meta::tables(base,snapshot.objects,snapshot.names)||!socket::metadata(frame,evidence,reader,reads,before))return false;
        reads.repeat=true;
        if(!socket::metadata(frame,evidence,reader,reads,after)||reads.cursor!=reads.count||reads.field_cursor!=reads.field_count||
           reads.name_cursor!=reads.name_count||!abi::same(before.socket,after.socket)||!meta::same_property(before.inner,after.inner))return false;
        out=before;return true;
    }
};
}
int main(){unsigned cases_count{};
 {auto m=std::make_unique<SocketFixture>();socket::Source out{};assert(m->inspect(out));assert(out.socket.address==m->magazine&&out.scale.x==1&&out.rotation.yaw==0);++cases_count;}
 for(unsigned reason=0;reason<14;++reason){auto m=std::make_unique<SocketFixture>();
    switch(reason){
      case 0:m->sockets[0]=m->magazine;break;
      case 1:Memory::put(m->magazine,0x108,m->text(L"b_AR_Magazine"));break;
      case 2:Memory::put(m->magazine,0x130,std::array<float,3>{0,1,1});break;
      case 3:Memory::put(m->magazine,0x130,std::array<float,3>{std::numeric_limits<float>::quiet_NaN(),1,1});break;
      case 4:Memory::put(m->magazine,0x110,std::array<float,3>{0,std::numeric_limits<float>::infinity(),0});break;
      case 5:Memory::put(m->scale_field,0xd0,m->rotator);break;
      case 6:Memory::put(m->inner,0x74,4u);break;
      case 7:Memory::put(m->socket_class,0x40,m->warfare);break;
      case 8:Memory::put(m->asset,0x140,meta::Table{reinterpret_cast<uptr>(m->sockets.data()),33,33});break;
      case 9:{auto raw=m->text(L"Magazine");raw.number=1;Memory::put(m->magazine,0x100,raw);break;}
      case 10:fail_read=m->magazine+0x120;break;
      case 11:mutate_read=m->magazine+0x130;mutation=[&]{Memory::put(m->magazine,0x110,std::array<float,3>{1,0,0});};break;
      case 12:mutate_read=m->magazine+0x130;mutation=[&]{m->sockets[0]=m->magazine;};break;
      case 13:Memory::put(m->magazine,0x40,m->engine);break;
    }
    socket::Source out{};out.socket.address=0x1234;assert(!m->inspect(out)&&out.socket.address==0x1234);++cases_count;
 }
 std::printf("Boomshot original Magazine socket metadata: %u cases passed; typed fields, exact bone, duplicate/foreign/changed/invalid rejection; owned memory only\n",cases_count);
}
