#define main component_matrix_asset_fixture
#include "weapon_lancer_asset_read_test.cpp"
#undef main
namespace {
#include "weapon_component_sample_budget.h"
#include "weapon_component_matrix_read.inl"
struct MatrixMemory:AssetMemory {
    uptr descriptor{},definition{};std::array<float,16> value{1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1};
    MatrixMemory(){
        const auto type=node(L"StructProperty",class_type,core);const auto structure_type=cls(L"ScriptStruct",core);
        definition=node(L"Matrix",structure_type,engine);put(definition,0x88,64u);
        descriptor=property(mesh_class,L"LocalToWorld",type,0x140,64);put(descriptor,0xd0,definition);
        put(component,0x140,value);tables();
    }
};
}
int main(){
    unsigned n{};
    for(unsigned changed=0;changed<8;++changed){auto m=std::make_unique<MatrixMemory>();const auto id=m->identity(m->component);
        switch(changed){case 1:Memory::put(m->definition,0x88,60u);break;
        case 2:Memory::put(m->descriptor,0x50,m->object_type);break;
        case 3:fail_read=m->component+0x140;break;
        case 4:mutate_read=m->component+0x140;mutation=[&]{auto v=m->value;v[12]+=1;Memory::put(m->component,0x140,v);};break;
        case 5:{auto v=m->value;v[1]=std::numeric_limits<float>::quiet_NaN();Memory::put(m->component,0x140,v);break;}
        case 6:Memory::put(m->definition,0x50,m->class_type);break;
        case 7:mutate_read=m->component+0x140;mutation=[&]{Memory::put(m->descriptor,0x94,0x180u);};break;}
        nativeequipment_component::Matrix out{};out.fill(-9);const auto previous=out;
        const auto okay=nativeequipment_component::matrix(id,L"LocalToWorld",out);assert(okay==(changed==0));
        assert(okay?out==m->value:out==previous);++n;
    }
    std::printf("WeaponComponentMatrixReadTest: %u cases; actual typed LocalToWorld reader on owned metadata, no world-pose/shot authority\n",n);
}
