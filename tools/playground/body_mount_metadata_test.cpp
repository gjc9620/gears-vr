#define main original_asset_fixture
#include "weapon_lancer_asset_read_test.cpp"
#undef main
namespace {
#include "body_mount_game_read.inl"
#include "body_mount_asset_read.inl"
abi::EquipmentIdentity identity(Memory const& m){
    abi::EquipmentIdentity i{};i.key=m.frame.key;i.world=m.frame.world;i.controller=m.frame.controller;
    i.pawn=m.frame.pawn;i.mesh=m.frame.mesh;i.weapon=m.frame.weapon;return i;
}
unsigned typed_checks{};
#define TYPED_CHECK(x) do{++typed_checks;if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(false)
}
int main(){
    for(bool object_mesh:{false,true}){
        auto m=std::make_unique<AssetMemory>(object_mesh);const auto i=identity(*m);
        abi::GripGameState game{},legacy{};TYPED_CHECK(weaponhand::observe_equipment(i,game));
        TYPED_CHECK(m->read(legacy)&&abi::same_game(game,legacy));
        la::Evidence a{},b{};TYPED_CHECK(la::read_equipment(i,a)==la::Status::ready);
        TYPED_CHECK(m->read_asset(b)==la::Status::ready&&abi::same(a.component,b.component)&&abi::same(a.asset,b.asset));
        // The metadata reader accepts identities, never a patched/retimed
        // Frame. Qualification belongs to Client and the separate raw source.
        m->frame.now_ns=-1;m->frame.controllers.sync_ns=-1;m->frame.stereo.received_ns=-1;
        TYPED_CHECK(weaponhand::observe_equipment(i,game)&&la::read_equipment(i,a)==la::Status::ready);
    }
    for(unsigned changed=0;changed<7;++changed){
        auto m=std::make_unique<AssetMemory>();auto i=identity(*m);abi::GripGameState game{};
        switch(changed){case 0:++i.pawn.name_number;break;case 1:fail_read=m->pawn+0x128;break;
        case 2:Memory::put(m->weapon,0x110,m->pc);break;
        case 3:mutate_read=m->pawn+0x120;mutation=[&]{Memory::put(m->pawn,0x120,uptr(0));};break;
        case 4:Memory::put(m->switch_property,0x50,m->object_type);break;
        case 5:Memory::put(m->pawn,0x108,4u);break;case 6:game_thread.store(0);break;}
        TYPED_CHECK(!weaponhand::observe_equipment(i,game)&&!game.known);
    }
    for(unsigned changed=0;changed<5;++changed){
        auto m=std::make_unique<AssetMemory>();const auto i=identity(*m);la::Evidence a{};
        switch(changed){case 0:Memory::put(m->component,0x110,2u);break;
        case 1:fail_read=m->weapon+0x118;break;
        case 2:m->final_mutation([&]{Memory::put(m->component,0x120,uptr(0));});break;
        case 3:m->final_mutation([&]{Memory::put(m->asset_property,0x94,0x128u);});break;
        case 4:game_thread.store(0);break;}
        TYPED_CHECK(la::read_equipment(i,a)!=la::Status::ready);
    }
    // Grouping metadata searches must retain the independently closing live
    // relationships/delete flags and first same-name field's type checks.
    for(unsigned changed=0;changed<6;++changed){
        auto m=std::make_unique<AssetMemory>();auto i=identity(*m);abi::GripGameState game{};
        if(changed<3){
            const uptr field=changed==0?m->pc+0x110:changed==1?m->pc+0x100:m->weapon+0x110;
            mutate_read=field;mutation=[field]{Memory::put(field,0,uptr(0));};
        }else if(changed==3)Memory::put(m->pc,0x108,4u);
        else if(changed==4)Memory::put(m->weapon,0x108,4u);
        else {
            const auto cls=m->cls(L"WrongFirstPawnController",m->warfare,m->pc_class);
            m->property(cls,L"Pawn",m->bool_type,0x190,8);
            Memory::put(m->pc,0x50,cls);m->tables();i.controller=m->identity(m->pc);
        }
        TYPED_CHECK(!weaponhand::observe_equipment(i,game)&&!game.known);
    }
    {auto m=std::make_unique<AssetMemory>();auto i=identity(*m);abi::GripGameState a{},b{};
        TYPED_CHECK(weaponhand::observe_equipment(i,a));Memory::put(m->pawn,0x128,1u);
        TYPED_CHECK(weaponhand::observe_equipment(i,b)&&!abi::same_game(a,b));}
    std::printf("BodyMountMetadataTest: %u checks; actual identity-only Game/typed-asset readers, owned metadata; attachment/Client/provider tested separately\n",typed_checks);
}
