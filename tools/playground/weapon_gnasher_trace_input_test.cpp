#include "weapon_gnasher_trace_input.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <limits>
#include "weapon_gnasher_trace_input_observed_fixture.inl"

int main(){
    using namespace gears_gnasher_trace_input;using Address=std::uintptr_t;
    constexpr Address caller=0x1000,locals=0x2000;
    Layout layout{0x7fc6d8a0,0x166eeb1a80,0x3000,0x100000,4096,320,2471,0x2efb,
        {0x7fcb5f00,0x7fcb5b80,0x7fcb6ec0}};
    std::map<Address,unsigned char> bytes;unsigned checks{};
    const auto put=[&](Address p,auto const& value){auto raw=reinterpret_cast<unsigned char const*>(&value);
        for(unsigned i=0;i<sizeof(value);++i)bytes[p+i]=raw[i];};
    const auto reset=[&](){bytes.clear();put(caller+0x18,layout.function);put(caller+0x20,layout.weapon);
        put(caller+0x28,layout.script+layout.code_offset);put(caller+0x30,locals);put(caller+0x38,layout.parent);
        put(layout.script+layout.code_offset-9,observed_call);
        put(locals,std::array<float,3>{-4065.69385f,-425.524872f,610.710754f});
        put(locals+112,std::array<float,3>{-4506.60693f,-3277.25586f,-443.049011f});
        put(locals+160,std::array<float,3>{-4065.69385f,-425.524872f,610.710754f});};
    const auto read=[&](void* out,void const* from,std::size_t count){auto p=reinterpret_cast<Address>(from);
        for(std::size_t i=0;i<count;++i){auto it=bytes.find(p+i);if(it==bytes.end())return false;
            static_cast<unsigned char*>(out)[i]=it->second;}return true;};
    const auto check=[&](bool value){assert(value);++checks;};
    for(unsigned code_offset:{2471u,2520u}){layout.code_offset=code_offset;reset();gears_weapon_call_trace::FireInput value{};
        check(direct(read,caller,layout,value));check(value.start[0]==-4065.69385f&&value.end[1]==-3277.25586f);}
    for(unsigned reason=0;reason<17;++reason){layout.code_offset=2471;reset();auto test=layout;
        switch(reason){
            case 0:put(caller+0x18,Address(1));break;case 1:put(caller+0x20,Address(1));break;
            case 2:put(caller+0x38,Address(1));break;case 3:put(caller+0x28,layout.script+2472);break;
            case 4:put(caller+0x30,locals+1);break;case 5:put(caller+0x30,Address{});break;
            case 6:bytes[layout.script+layout.code_offset-9]=0;break;
            case 7:bytes[layout.script+layout.code_offset-8]^=1;break;
            case 8:bytes[layout.script+layout.code_offset+1]^=1;break;
            case 9:bytes[layout.script+layout.code_offset+18]=1;break;
            case 10:bytes[layout.script+layout.code_offset+27]=0;break;
            case 11:bytes.erase(locals+112);break;
            case 12:put(locals+112,std::numeric_limits<float>::quiet_NaN());break;
            case 13:test.locals_bytes=319;break;case 14:test.code_offset=99;break;
            case 15:test.arguments[2]=test.arguments[0];break;case 16:test.script=UINTPTR_MAX-4;break;
        }
        gears_weapon_call_trace::FireInput value{};value.start[0]=99;check(!direct(read,caller,test,value));check(value.start[0]==0);
    }
    reset();unsigned vector_reads{};
    const auto changing=[&](void* out,void const* from,std::size_t count){
        if(reinterpret_cast<Address>(from)==locals&&++vector_reads==2)put(caller+0x38,Address(1));
        return read(out,from,count);};
    gears_weapon_call_trace::FireInput value{};check(!direct(changing,caller,layout,value));
    std::printf("WeaponGnasherTraceInputTest: %u checks passed; observed retail code offsets2471/2520 and locals0/112/160\n",checks);return 0;
}
