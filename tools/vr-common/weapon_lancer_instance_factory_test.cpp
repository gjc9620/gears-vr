#include "weapon_lancer_instance_factory.h"
#include <cstdio>
#include <cstdlib>
#include <type_traits>
namespace f=gears_lancer_instance_factory;
unsigned checks{};
#define C(v) do{++checks;if(!(v)){std::printf("FAIL line%d\n",__LINE__);std::exit(1);}}while(false)
struct Memory {
    std::uintptr_t base{0x140000000},gpu{0x10000},value{base+f::table_rva};
    std::array<std::uintptr_t,14> original{};
    bool readable{true},ambiguous{};unsigned writes{};
    Memory(){original[0]=base+0x577e3b;for(unsigned i=0;i<f::slot_rvas.size();++i)original[i+1]=base+f::slot_rvas[i];}
    bool read(std::uintptr_t at,std::uintptr_t& out) noexcept {
        if(!readable)return false;
        if(at==gpu){out=value;return true;}
        const auto first=base+f::table_rva-8;
        if(at>=first&&at<first+sizeof original&&(at-first)%8==0){out=original[(at-first)/8];return true;}
        return false;
    }
    bool compare(std::uintptr_t at,std::uintptr_t before,std::uintptr_t after,bool& changed) noexcept {
        changed=false;if(at!=gpu)return false;
        if(value==before){value=after;changed=true;++writes;}
        return !ambiguous;
    }
};
int main(){
    static_assert(!std::is_copy_constructible_v<f::Lease>&&!std::is_move_constructible_v<f::Lease>);
    Memory m;f::Lease lease;const auto original=m.original;
    auto read=[&](auto a,auto& v){return m.read(a,v);};
    auto cas=[&](auto a,auto b,auto c,auto& d){return m.compare(a,b,c,d);};
    C(lease.install(m.base,m.gpu,0x20000,0x30000,read,cas));C(lease.active()&&m.value==lease.table());
    C(m.original==original&&m.writes==1);
    for(unsigned i=0;i<14;++i)C(lease.entries()[i]==(i==6?0x20000:i==7?0x30000:original[i]));
    C(lease.install(m.base,m.gpu,0x20000,0x30000,read,cas)&&m.writes==1);
    C(!lease.install(m.base,m.gpu+8,0x20000,0x30000,read,cas));
    C(lease.restore(read,cas)&&!lease.active()&&m.value==m.base+f::table_rva&&m.writes==2);
    C(lease.restore(read,cas)&&m.writes==2);
    m.original[6]+=8;C(!lease.install(m.base,m.gpu,0x20000,0x30000,read,cas)&&!lease.active());m.original=original;
    C(!lease.install(m.base,m.gpu,m.base+f::slot_rvas[5],0x30000,read,cas));
    C(lease.install(m.base,m.gpu,0x20000,0x30000,read,cas));
    m.value=0x77700;const auto before=m.writes;C(lease.restore(read,cas)&&m.value==0x77700&&m.writes==before);
    m.value=m.base+f::table_rva;m.ambiguous=true;
    C(!lease.install(m.base,m.gpu,0x20000,0x30000,read,cas)&&lease.active()&&m.value==lease.table());
    m.readable=false;C(!lease.restore(read,cas)&&lease.active());m.readable=true;m.ambiguous=false;
    C(lease.restore(read,cas)&&!lease.active()&&m.value==m.base+f::table_rva);
    std::printf("Instance factory table: %u ownership/clone/CAS/restoration checks; CPU only\n",checks);
}
