#define main lancer_asset_fixture_main
#include "weapon_lancer_asset_read_test.cpp"
#undef main
#include "weapon_lancer_muzzle_observation.h"
#include <cmath>
#include <limits>
namespace {
#include "weapon_lancer_muzzle_socket_read.inl"
namespace muzzle=gears_weapon_lancer_muzzle;
unsigned socket_cases{};
struct SocketMemory:AssetMemory {
    uptr name_type{},array_type{},struct_type{},script_type{},socket_class{},socket{},second{};
    uptr configured_property{},sockets_property{},inner{},socket_name_property{},bone_property{},location_property{},rotation_property{};
    uptr object_owner{},vector{},rotator{};std::array<uptr,2> socket_array{};la::Evidence evidence{};
    SocketMemory(){
        name_type=node(L"NameProperty",class_type,core);array_type=node(L"ArrayProperty",class_type,core);
        struct_type=node(L"StructProperty",class_type,core);script_type=node(L"ScriptStruct",class_type,core);
        socket_class=cls(L"SkeletalMeshSocket",engine);
        configured_property=property(weapon_class,L"MuzzleSocketName",name_type,0x130,8);
        sockets_property=property(asset_class,L"Sockets",array_type,0x140,16);
        inner=node(L"SocketsInner",object_type,sockets_property);put(inner,0x70,1u);put(inner,0x74,8u);
        put(sockets_property,0xd0,inner);
        socket_name_property=property(socket_class,L"SocketName",name_type,0x100,8);
        bone_property=property(socket_class,L"BoneName",name_type,0x108,8);
        location_property=property(socket_class,L"RelativeLocation",struct_type,0x110,12);
        rotation_property=property(socket_class,L"RelativeRotation",struct_type,0x120,12);
        object_owner=cls(L"Object",core);
        vector=node(L"Vector",script_type,object_owner);rotator=node(L"Rotator",script_type,object_owner);
        put(vector,0x88,12u);put(rotator,0x88,12u);put(location_property,0xd0,vector);put(rotation_property,0xd0,rotator);
        socket=node(L"LiveMuzzleSocket",socket_class,asset);second=node(L"LiveMagazineSocket",socket_class,asset);
        put(weapon,0x130,text(L"Muzzle"));put(socket,0x100,text(L"Muzzle"));put(socket,0x108,text(L"b_AR_Root"));
        put(socket,0x110,std::array<float,3>{1.25f,2.5f,-3.0f});put(socket,0x120,std::array<std::int32_t,3>{10,-20,30});
        put(second,0x100,text(L"Magazine"));socket_array={socket,second};
        put(asset,0x140,meta::Table{reinterpret_cast<uptr>(socket_array.data()),2,2});tables();
        assert(read_asset(evidence)==la::Status::ready);
    }
    bool read_socket(muzzle::Socket& out,muzzle::SocketDiagnostic* diagnostic=nullptr){
        return nativeequipment_component::muzzle_socket(frame,evidence,out,diagnostic);}
    void after_first(std::function<void()> action){mutate_read=socket+0x120;mutation=std::move(action);}
};
void socket_reject(SocketMemory& memory){muzzle::Socket out{};out.object=0x1234;out.location={9,8,7};const auto before=out;
    assert(!memory.read_socket(out)&&muzzle::same(before,out));++socket_cases;}
template<class Change>void socket_rejection(Change change){auto memory=std::make_unique<SocketMemory>();change(*memory);socket_reject(*memory);}
}
int main(){
    {auto memory=std::make_unique<SocketMemory>();muzzle::Socket out{};muzzle::SocketDiagnostic diagnostic{};
        assert(memory->read_socket(out,&diagnostic)&&out.object==memory->socket&&out.configured_name.index==out.socket_name.index&&
            out.bone_name.index==memory->text(L"b_AR_Root").index&&out.location[0]==1.25f&&out.rotation[1]==-20);++socket_cases;}
    {auto memory=std::make_unique<SocketMemory>();muzzle::Socket out{};muzzle::SocketDiagnostic diagnostic{};
        assert(memory->read_socket(out,&diagnostic)&&diagnostic.step==muzzle::SocketStep::completed&&diagnostic.pass==2&&
            diagnostic.metadata_reason==static_cast<unsigned>(meta::Reason::none));++socket_cases;}
    {auto memory=std::make_unique<SocketMemory>();muzzle::Socket out{};out.object=17;const auto before=out;
        muzzle::SocketDiagnostic diagnostic{};game_thread.store(0);
        assert(!memory->read_socket(out,&diagnostic)&&muzzle::same(out,before)&&diagnostic.step==muzzle::SocketStep::entry&&
            diagnostic.pass==0&&diagnostic.metadata_reason==UINT32_MAX);++socket_cases;}
    {auto memory=std::make_unique<SocketMemory>();muzzle::Socket out{};out.object=17;const auto before=out;
        muzzle::SocketDiagnostic diagnostic{};auto name=memory->text(L"Muzzle");name.number=1;Memory::put(memory->weapon,0x130,name);
        assert(!memory->read_socket(out,&diagnostic)&&muzzle::same(out,before)&&diagnostic.step==muzzle::SocketStep::configured_name&&
            diagnostic.pass==1&&diagnostic.metadata_reason==static_cast<unsigned>(meta::Reason::none));++socket_cases;}
    {auto memory=std::make_unique<SocketMemory>();muzzle::Socket out{};out.object=17;const auto before=out;
        muzzle::SocketDiagnostic diagnostic{};fail_read=memory->socket+0x120;
        assert(!memory->read_socket(out,&diagnostic)&&muzzle::same(out,before)&&diagnostic.step==muzzle::SocketStep::rotation&&
            diagnostic.pass==1&&diagnostic.metadata_reason==static_cast<unsigned>(meta::Reason::unreadable));++socket_cases;}
    {auto memory=std::make_unique<SocketMemory>();muzzle::Socket out{};out.object=17;const auto before=out;
        muzzle::SocketDiagnostic diagnostic{};memory->after_first([&memory]{Memory::put(memory->socket,0x110,std::array<float,3>{9,2,3});});
        assert(!memory->read_socket(out,&diagnostic)&&muzzle::same(out,before)&&diagnostic.step==muzzle::SocketStep::location&&
            diagnostic.pass==2&&diagnostic.metadata_reason==static_cast<unsigned>(meta::Reason::none));++socket_cases;}
    socket_rejection([](auto& m){++m.frame.weapon.registry_index;});
    socket_rejection([](auto& m){++m.evidence.component.name_number;});
    socket_rejection([](auto& m){++m.evidence.asset.registry_index;});
    socket_rejection([](auto& m){Memory::put(m.weapon,0x108,4u);});
    socket_rejection([](auto& m){Memory::put(m.weapon,0x130,m.text(L"Other"));m.tables();});
    socket_rejection([](auto& m){auto name=m.text(L"Muzzle");name.number=1;Memory::put(m.weapon,0x130,name);});
    socket_rejection([](auto& m){Memory::put(m.configured_property,0x50,m.object_type);});
    socket_rejection([](auto& m){Memory::put(m.inner,0x40,m.asset);});
    socket_rejection([](auto& m){Memory::put(m.inner,0x74,4u);});
    socket_rejection([](auto& m){Memory::put(m.inner,0x50,m.component_type);});
    socket_rejection([](auto& m){Memory::put(m.asset,0x140,meta::Table{reinterpret_cast<uptr>(m.socket_array.data()),33,33});});
    socket_rejection([](auto& m){m.socket_array[1]=m.socket;});
    socket_rejection([](auto& m){Memory::put(m.socket_class,0x40,m.warfare);});
    socket_rejection([](auto& m){Memory::put(m.socket,0x48,m.text(L"Default__Muzzle"));m.tables();});
    socket_rejection([](auto& m){Memory::put(m.socket,0x108,meta::RawName{999999,0});});
    socket_rejection([](auto& m){Memory::put(m.vector,0x50,m.class_type);});
    socket_rejection([](auto& m){Memory::put(m.vector,0x40,m.core);});
    socket_rejection([](auto& m){Memory::put(m.object_owner,0x40,m.engine);});
    socket_rejection([](auto& m){Memory::put(m.object_owner,0x48,m.text(L"OtherObject"));m.tables();});
    socket_rejection([](auto& m){Memory::put(m.object_owner,0x50,m.script_type);});
    socket_rejection([](auto& m){const auto fake_core=m.node(L"FakeCore",m.package_type);
        const auto fake_class=m.node(L"Class",m.class_type,fake_core);
        Memory::put(m.object_owner,0x50,fake_class);m.tables();});
    socket_rejection([](auto& m){auto name=m.text(L"Object");name.number=1;Memory::put(m.object_owner,0x48,name);});
    socket_rejection([](auto& m){Memory::put(m.rotator,0x40,m.engine);});
    socket_rejection([](auto& m){Memory::put(m.vector,0x88,16u);});
    socket_rejection([](auto& m){Memory::put(m.socket,0x110,std::array<float,3>{std::numeric_limits<float>::quiet_NaN(),0,0});});
    socket_rejection([](auto& m){fail_read=m.socket+0x120;});
    socket_rejection([](auto& m){m.after_first([&m]{Memory::put(m.socket,0x110,std::array<float,3>{9,2,3});});});
    socket_rejection([](auto& m){m.after_first([&m]{Memory::put(m.socket,0x108,m.text(L"b_AR_Magazine"));m.tables();});});
    socket_rejection([](auto& m){m.after_first([&m]{m.socket_array[1]=m.socket;});});
    socket_rejection([](auto& m){m.after_first([&m]{Memory::put(m.inner,0x74,4u);});});
    socket_rejection([](auto& m){m.after_first([&m]{Memory::put(m.location_property,0xd0,m.rotator);});});
    socket_rejection([](auto& m){m.after_first([&m]{Memory::put(m.socket_class,0x88,500u);});});
    socket_rejection([](auto& m){const auto entry=m.names[m.text(L"b_AR_Root").index];
        m.after_first([entry]{Memory::put(entry,20,wchar_t(L'X'));});});
    socket_rejection([](auto& m){m.after_first([&m]{m.objects[m.identity(m.socket).registry_index]=0;});});
    socket_rejection([](auto& m){m.after_first([]{auto table=Memory::get<meta::Table>(base,0x1c34800);--table.capacity;Memory::put(base,0x1c34800,table);});});
    socket_rejection([](auto& m){m.after_first([&m]{const auto previous=Memory::get<uptr>(m.socket_class,0x80);
        const auto replacement=m.node(L"SocketName",m.name_type,m.socket_class);Memory::put(replacement,0x70,1u);
        Memory::put(replacement,0x74,8u);Memory::put(replacement,0x94,0x108u);Memory::put(replacement,0x68,previous);
        Memory::put(m.socket_class,0x80,replacement);m.tables();});});
    std::printf("WeaponLancerMuzzleSocketReadTest: %u cases passed (owned memory only)\n",socket_cases);return 0;
}
