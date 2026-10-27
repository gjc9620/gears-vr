#define main component_pose_asset_fixture
#include "weapon_lancer_asset_read_test.cpp"
#undef main
namespace {
#include "weapon_component_sample_budget.h"
#include "weapon_component_matrix_read.inl"
#include "weapon_component_pose_read.inl"
struct PoseMemory:AssetMemory {
    alignas(16)std::array<unsigned char,1024> body{};
    std::array<gears_weapon_component_pose::Matrix,115> space{};
    std::array<unsigned char,115*32> local{};
    std::array<unsigned char,115*68> reference{};
    nativeequipment_component::Matrix ctw{1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1};
    PoseMemory(){
        auto original=identity(mesh);std::memcpy(body.data(),reinterpret_cast<void const*>(mesh),512);
        mesh=reinterpret_cast<uptr>(body.data());objects[original.registry_index]=mesh;
        Memory::put(mesh_class,0x88,1024u);frame.mesh=identity(mesh);
        Memory::put(asset_property,0x94,0x218u);Memory::put(mesh,0x218,asset);
        Memory::put(mesh,0x338,12u);Memory::put(mesh,0xa0,ctw);
        for(auto& m:space)m={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        using A=gears_weapon_component_pose::Array;
        std::array<unsigned char,56> header{};const A s{reinterpret_cast<uptr>(space.data()),115,115},l{reinterpret_cast<uptr>(local.data()),115,115};
        std::memcpy(header.data(),&s,16);std::memcpy(header.data()+16,&l,16);Memory::put(mesh,0x26c,header);
        Memory::put(asset,0xa4,A{reinterpret_cast<uptr>(reference.data()),115,115});tables();
    }
};
}
int main(){
    unsigned checked{};
    for(unsigned kind=0;kind<5;++kind){auto m=std::make_unique<PoseMemory>();
        switch(kind){case 1:Memory::put(m->asset_property,0x94,0x120u);break;
        case 2:Memory::put(m->asset,0x50,m->mesh_class);break;
        case 3:fail_read=m->mesh+0x26c;break;
        case 4:mutate_read=m->mesh+0x218;mutation=[&]{Memory::put(m->asset_property,0x94,0x120u);};break;}
        gears_weapon_component_pose::Snapshot out{};out.epoch=777;
        const auto okay=nativeequipment_component::pose_cohort(m->identity(m->mesh),m->ctw,out);
        assert(okay==(kind==0));assert(okay?out.epoch==12:out.epoch==777);++checked;
    }
    std::printf("WeaponComponentPoseReadTest: %u cases; production typed adapter, owned registered memory only\n",checked);
}
