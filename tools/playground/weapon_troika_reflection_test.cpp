// Owned synthetic reflection. Uses the real Reader and Troika ScriptStruct
// helper; no game process, original engine call, input or skeleton write.
#include "weapon_hand_offline_fixture.h"
namespace {
#include "weapon_troika_reflection.inl"
struct CameraMemory:Memory {
    uptr camera_class{},camera{},script_type{},view{},target{},container{};
    CameraMemory(){
        camera_class=cls(L"Camera",engine);script_type=cls(L"ScriptStruct",core);
        camera=node(L"Camera_0",camera_class,world);
        view=node(L"TViewTarget",script_type,camera_class);put(view,0x88,32u);
        target=property(view,L"Target",object_type,8,8);
        container=property(camera_class,L"ViewTarget",cls(L"StructProperty",core),0x100,32);put(container,0xd0,view);
        put(camera,0x108,pawn);tables();
    }
};
struct NumericMemory:Memory {
    uptr value{},x{},y{},z{},container{},float_type{},int_type{};
    explicit NumericMemory(bool rotator=false){
        const auto object_owner=cls(L"Object",core),script_type=cls(L"ScriptStruct",core);
        float_type=cls(L"FloatProperty",core);int_type=cls(L"IntProperty",core);
        value=node(rotator?L"Rotator":L"Vector2D",script_type,object_owner);put(value,0x88,rotator?12u:8u);
        x=property(value,rotator?L"Pitch":L"X",rotator?int_type:float_type,0,4);
        y=property(value,rotator?L"Yaw":L"Y",rotator?int_type:float_type,4,4);
        if(rotator)z=property(value,L"Roll",int_type,8,4);
        container=property(actor_class,L"GeometryValue",cls(L"StructProperty",core),0x100,rotator?12u:8u);put(container,0xd0,value);tables();
    }
};
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::printf("failed %d: %s\n",__LINE__,#x);return 1;}}while(0)
}
int main(){
    auto memory=std::make_unique<CameraMemory>();
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};CHECK(meta::tables(base,schema.objects,schema.names));
    meta::Property container{};CHECK(reader.property(memory->container,container));
    troika_reflection::ScriptStruct value{};CHECK(troika_reflection::read(reader,container,L"TViewTarget",L"Camera",L"Engine",32,value));
    CHECK(troika_reflection::object_member(reader,value,memory->camera+0x100,L"Target",memory->pawn));
    CHECK(troika_reflection::current(reader,value,L"TViewTarget",L"Camera",L"Engine"));
    CHECK(schema.structure_count==0); // No ScriptStruct masquerades as Class/Function.
    for(unsigned fault=0;fault<8;++fault){
        auto m=std::make_unique<CameraMemory>();meta::Snapshot s{};s.module_base=base;meta::Reader r{s,meta::Stage::final_check};
        CHECK(meta::tables(base,s.objects,s.names));meta::Property p{};CHECK(r.property(m->container,p));
        troika_reflection::ScriptStruct before{};CHECK(troika_reflection::read(r,p,L"TViewTarget",L"Camera",L"Engine",32,before));
        if(fault==0)m->put(m->view,80,m->class_type);
        if(fault==1)m->put(m->view,64,m->pawn_class);
        if(fault==2)m->put(m->view,0x88,64u);
        if(fault==3)m->put(m->view,0x60,m->pawn_class);
        if(fault==4)m->put(m->view,0x80,uptr(0));
        if(fault==5)m->put(m->container,0xd0,m->pawn_class);
        if(fault==6)m->put(m->container,0x94,0x120u);
        if(fault==7)m->put(m->view,72,m->text(L"OtherStruct"));
        CHECK(!troika_reflection::current(r,before,L"TViewTarget",L"Camera",L"Engine"));
    }
    for(unsigned fault=0;fault<5;++fault){
        auto m=std::make_unique<CameraMemory>();
        if(fault==4){m->property(m->view,L"Target",m->object_type,16,8);m->put(m->camera,0x110,m->pawn);m->tables();}
        meta::Snapshot s{};s.module_base=base;meta::Reader r{s,meta::Stage::final_check};
        CHECK(meta::tables(base,s.objects,s.names));meta::Property p{};CHECK(r.property(m->container,p));
        troika_reflection::ScriptStruct layout{};CHECK(troika_reflection::read(r,p,L"TViewTarget",L"Camera",L"Engine",32,layout));
        if(fault==0)m->put(m->camera,0x108,uptr(0));
        if(fault==1)m->put(m->target,0x74,4u);
        if(fault==2)m->put(m->target,0x94,28u);
        if(fault==3)m->put(m->target,0x68,m->target);
        CHECK(!troika_reflection::object_member(r,layout,m->camera+0x100,L"Target",m->pawn));
    }
    for(bool rotator:{false,true}){
        auto m=std::make_unique<NumericMemory>(rotator);meta::Snapshot s{};s.module_base=base;meta::Reader r{s,meta::Stage::final_check};
        CHECK(meta::tables(base,s.objects,s.names));meta::Property p{};CHECK(r.property(m->container,p));troika_reflection::ScriptStruct layout{};
        CHECK(rotator?troika_reflection::numeric(r,p,L"Rotator",L"IntProperty",std::array<wchar_t const*,3>{L"Pitch",L"Yaw",L"Roll"},layout):
            troika_reflection::numeric(r,p,L"Vector2D",L"FloatProperty",std::array<wchar_t const*,2>{L"X",L"Y"},layout));
    }
    for(unsigned fault=0;fault<7;++fault){
        auto m=std::make_unique<NumericMemory>();
        if(fault==0)m->put(m->x,80,m->int_type);
        if(fault==1)m->put(m->y,0x94,0u);
        if(fault==2)m->put(m->y,0x74,8u);
        if(fault==3)m->put(m->x,0x70,2u);
        if(fault==4)m->put(m->y,0x68,m->x);
        if(fault==5)m->put(m->y,64,m->actor_class);
        if(fault==6){m->property(m->value,L"X",m->float_type,0,4);m->tables();}
        meta::Snapshot s{};s.module_base=base;meta::Reader r{s,meta::Stage::final_check};
        CHECK(meta::tables(base,s.objects,s.names));meta::Property p{};CHECK(r.property(m->container,p));troika_reflection::ScriptStruct layout{};
        CHECK(!troika_reflection::numeric(r,p,L"Vector2D",L"FloatProperty",std::array<wchar_t const*,2>{L"X",L"Y"},layout));
    }
    std::printf("Troika ScriptStruct/category, declaration/Target/numeric layout races: %u checks passed.\n",checks);
}
