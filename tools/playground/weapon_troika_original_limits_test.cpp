#define GEARS_TROIKA_AXES_FIXTURE_ONLY 1
#define GEARS_TROIKA_LIMITS_FIXTURE_ONLY 1
#include "weapon_troika_control_axes_test.cpp"
namespace {
namespace troika_mounted_parts {
#include "weapon_troika_original_limits.inl"
}
namespace limits=troika_mounted_parts::original_limits;
struct LimitMemory:AxisMemory {
    uptr point_fn{},rotation_fn{},point_field{},return_field{},view_field{},delta_field{},time_field{};
    std::array<uptr,3> debug_fields{};bool point_result{true},dispatch_failure{},remaining_delta{};
    unsigned calls{};bool inputs_zero{};std::function<void()> after_call;
    std::array<std::int32_t,3> last_requested{};
    LimitMemory(){
        const auto function_type=cls(L"Function",core),int_type=cls(L"IntProperty",core);
        point_fn=node(L"CanLookAtPoint",function_type,control_class);rotation_fn=node(L"ProcessViewRotation",function_type,turret_class);
        point_field=property(point_fn,L"PointLoc",struct_class,12,12);put(point_field,0xd0,vector_type);put(point_field,0x78,std::uint64_t(0x80));
        const wchar_t* debug_names[]={L"bDrawDebugInfo",L"bDebugUsePersistentLines",L"bDebugFlushLinesFirst"};
        for(unsigned i=0;i<3;++i){debug_fields[i]=property(point_fn,debug_names[i],bool_type,0,4,1u<<i);put(debug_fields[i],0x78,std::uint64_t(0x90));}
        return_field=property(point_fn,L"ReturnValue",bool_type,0,4,8);put(return_field,0x78,std::uint64_t(0x480));
        const auto rotator=node(L"Rotator",cls(L"ScriptStruct",core),object_class);put(rotator,0x88,12u);
        property(rotator,L"Pitch",int_type,0,4);property(rotator,L"Yaw",int_type,4,4);property(rotator,L"Roll",int_type,8,4);
        time_field=property(rotation_fn,L"DeltaTime",float_class,0,4);put(time_field,0x78,std::uint64_t(0x80));
        view_field=property(rotation_fn,L"OutViewRot",struct_class,16,12);put(view_field,0xd0,rotator);put(view_field,0x78,std::uint64_t(0x180));
        delta_field=property(rotation_fn,L"OutDeltaRot",struct_class,4,12);put(delta_field,0xd0,rotator);put(delta_field,0x78,std::uint64_t(0x180));
        for(auto fn:{point_fn,rotation_fn}){put(fn,0x88,28u);put(fn,0x106,std::uint16_t(28));}
        put(point_fn,0x108,std::uint16_t(0));put(rotation_fn,0x108,std::uint16_t(0xffff));tables();
    }
    struct Call {LimitMemory* memory{};bool point{},valid{true};struct Request {uptr function{};unsigned size{28};unsigned char params[64]{};} r;
        bool run(){auto& m=*memory;++m.calls;if(m.dispatch_failure)return false;
            if(point){unsigned debug{};std::memcpy(&debug,r.params,4);m.inputs_zero=(debug&7u)==0;
                const unsigned result=m.point_result?8u:0u;std::memcpy(r.params,&result,4);
            }else {float dt{};std::array<int,3> delta{};std::memcpy(&dt,r.params,4);std::memcpy(delta.data(),r.params+4,12);
                m.inputs_zero=dt==0&&delta==std::array<int,3>{};std::memcpy(m.last_requested.data(),r.params+16,12);
                auto result=m.last_requested;result[0]=(std::max)(-6000,(std::min)(4400,result[0]));
                result[1]=(std::max)(-10000,(std::min)(10000,result[1]));std::memcpy(r.params+16,result.data(),12);
                if(m.remaining_delta){delta[0]=1;std::memcpy(r.params+4,delta.data(),12);}}
            if(m.after_call)m.after_call();return true;
        }
    };
    auto factory(bool point){return [this,point](uptr receiver){Call c{};c.memory=this;c.point=point;c.valid=receiver==(point?latitude:turret);
        c.r.function=point?point_fn:rotation_fn;std::memset(c.r.params,0xcd,sizeof c.r.params);return c;};}
};
}
#if !defined(GEARS_TROIKA_LIMITS_REUSE_FIXTURE)
int main(){
    {
        auto m=std::make_unique<LimitMemory>();bool result=false;
        CHECK(limits::point(m->identity(m->latitude),{1,2,3},m->factory(true),[](){return true;},result));CHECK(result&&m->inputs_zero&&m->calls==1);
        m->point_result=false;CHECK(limits::point(m->identity(m->latitude),{1,2,3},m->factory(true),[](){return true;},result));CHECK(!result);
        std::array<int,3> rotation{};CHECK(limits::rotation(m->identity(m->turret),{8000,12000,7},m->factory(false),[](){return true;},rotation));
        CHECK((rotation==std::array<int,3>{4400,10000,7})&&m->inputs_zero&&m->last_requested[0]==8000);
    }
    for(unsigned fault=0;fault<17;++fault){auto m=std::make_unique<LimitMemory>();bool result=true;
        if(fault==0)m->put(m->point_fn,0x40,m->actor_class);
        if(fault==1)m->put(m->point_fn,0x106,std::uint16_t(24));
        if(fault==2)m->put(m->point_field,0xd0,m->pawn_class);
        if(fault==3)m->put(m->point_field,0x94,0u);
        if(fault==4)m->put(m->debug_fields[1],0xd0,1u);
        if(fault==5)m->put(m->return_field,0xd0,3u);
        if(fault==6)m->put(m->return_field,0x78,std::uint64_t(0x80));
        if(fault==7)m->put(m->return_field,0x68,m->point_field);
        if(fault==8)m->dispatch_failure=true;
        if(fault==9)m->after_call=[&](){m->put(m->return_field,0xd0,16u);};
        if(fault==10)m->after_call=[&](){m->put(m->point_field,0x94,16u);};
        if(fault==11)m->put(m->point_fn,0x108,std::uint16_t(4));
        if(fault==12)m->put(m->point_field,0x70,2u);
        if(fault==13)m->put(m->point_field,0x74,8u);
        if(fault==14)m->put(m->debug_fields[0],0x50,m->float_class);
        if(fault==15)m->put(m->point_fn,0x50,m->class_type);
        if(fault==16)m->put(m->debug_fields[0],0x78,std::uint64_t(0x180));
        CHECK(!limits::point(m->identity(m->latitude),{1,2,3},m->factory(true),[](){return true;},result));CHECK(result);
    }
    for(unsigned fault=0;fault<9;++fault){auto m=std::make_unique<LimitMemory>();const std::array<int,3> saved{7,8,9};auto result=saved;
        if(fault==0)m->put(m->rotation_fn,0x40,m->control_class);
        if(fault==1)m->put(m->view_field,0x78,std::uint64_t(0x80));
        if(fault==2)m->put(m->delta_field,0x94,16u);
        if(fault==3)m->put(m->rotation_fn,0x108,std::uint16_t(0));
        if(fault==4)m->remaining_delta=true;
        if(fault==5)m->dispatch_failure=true;
        if(fault==6)m->after_call=[&](){m->put(m->time_field,0x50,m->object_type);};
        if(fault==7)m->put(m->view_field,0xd0,m->vector_type);
        if(fault==8)m->put(m->view_field,0x94,20u);
        CHECK(!limits::rotation(m->identity(m->turret),{8000,12000,0},m->factory(false),[](){return true;},result));CHECK(result==saved);
    }
    {
        auto m=std::make_unique<LimitMemory>();bool result=true,live=true;m->after_call=[&](){live=false;};
        CHECK(!limits::point(m->identity(m->latitude),{1,2,3},m->factory(true),[&](){return live;},result));CHECK(result);
        CHECK(!limits::point(m->identity(m->latitude),{1,2,3},m->factory(true),[](){return false;},result));
        CHECK(m->calls==1);
    }
    std::printf("Troika original limit schemas: %u checks passed; actual Reader and emulated original calls, no game/aim write proof.\n",checks);
}
#endif
