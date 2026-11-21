#define GEARS_TROIKA_AXES_FIXTURE_ONLY 1
#define GEARS_TROIKA_PHYSICAL_START_FIXTURE_ONLY 1
#include "weapon_troika_control_axes_test.cpp"
namespace {
namespace troika_mounted_parts {
#include "weapon_troika_physical_start.inl"
}
namespace physical=troika_mounted_parts::physical_start;
struct PhysicalMemory:AxisMemory {
    uptr function{},input{},answer{};unsigned calls{};bool zero{};std::function<void()> after_call;
    PhysicalMemory(){
        function=node(L"GetPhysicalFireStartLoc",cls(L"Function",core),turret_class);
        input=property(function,L"FireOffset",struct_class,16,12);put(input,0xd0,vector_type);put(input,0x78,std::uint64_t(0x80));
        answer=property(function,L"ReturnValue",struct_class,0,12);put(answer,0xd0,vector_type);put(answer,0x78,std::uint64_t(0x480));
        put(function,0x106,std::uint16_t(28));put(function,0x108,std::uint16_t(0));tables();
    }
    struct Call {struct Request {uptr function{};unsigned size{28};unsigned char params[64]{};} r;bool valid{};};
    bool read(std::array<float,3>& out,bool returned=true,std::function<bool()> current=[](){return true;}){
        return physical::read(identity(turret),[&](uptr receiver){Call call{};call.valid=receiver==turret;call.r.function=function;return call;},
            [&](Call& call){++calls;std::array<float,3> offset{};std::memcpy(offset.data(),call.r.params+16,12);zero=offset==std::array<float,3>{};
                const std::array<float,3> value{1,2,3};std::memcpy(call.r.params,value.data(),12);if(after_call)after_call();return returned;},current,out);
    }
};
}
int main(){
    {auto m=std::make_unique<PhysicalMemory>();std::array<float,3> out{};CHECK(m->read(out));CHECK((out==std::array<float,3>{1,2,3})&&m->zero&&m->calls==1);}
    {auto m=std::make_unique<PhysicalMemory>();m->put(m->input,0x78,std::uint64_t(0x90));std::array<float,3> out{};
        CHECK(m->read(out));CHECK((out==std::array<float,3>{1,2,3})&&m->zero);}
    const std::array<float,3> sentinel{8,9,10};
    for(unsigned fault=0;fault<12;++fault){auto m=std::make_unique<PhysicalMemory>();auto out=sentinel;
        if(fault==0)m->put(m->function,0x40,m->pawn_class);
        if(fault==1)m->put(m->input,0xd0,m->control_class);
        if(fault==2)m->put(m->answer,0x94,16u);
        if(fault==3)m->put(m->answer,0x78,std::uint64_t(0x80));
        if(fault==4)m->put(m->input,0x74,8u);
        if(fault==5)m->put(m->input,0x68,m->input);
        if(fault==6)m->put(m->function,0x106,std::uint16_t(29));
        if(fault==7)m->put(m->function,0x108,std::uint16_t(4));
        if(fault==8)m->after_call=[&](){m->put(m->input,0x94,12u);};
        CHECK(!m->read(out,fault!=9,[&](){return fault!=10&&(fault!=11||!m->calls);}));CHECK(out==sentinel);
        if(fault==8||fault==9||fault==11)CHECK(m->calls==1);
    }
    std::printf("Troika original physical-start getter: %u checks passed; actual Reader/schema and emulated call only.\n",checks);
}
