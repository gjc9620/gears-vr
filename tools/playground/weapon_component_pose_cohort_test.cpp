#include "weapon_component_pose_cohort.h"
#include <cassert>
#include <cstdio>
#include <functional>
#include <map>
#include <vector>
namespace p=gears_weapon_component_pose;
struct Fixture {
    std::map<p::Address,std::vector<unsigned char>> memory;
    std::function<void(p::Address,unsigned)> mutate;
    std::map<p::Address,unsigned> reads;
    static constexpr p::Address mesh=0x10000,asset=0x20000,space=0x30000,local=0x40000,ref=0x50000;
    p::Matrix ctw{1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1};
    template<class T>void put(p::Address address,T const& value){
        auto& v=memory[address];v.resize(sizeof value);std::memcpy(v.data(),&value,sizeof value);
    }
    Fixture(){
        put(mesh+0x218,asset);put(mesh+0x338,12u);put(mesh+0xa0,ctw);
        std::array<unsigned char,56> header{};p::Array s{space,115,115},l{local,115,115},d{};
        std::memcpy(header.data(),&s,16);std::memcpy(header.data()+16,&l,16);std::memcpy(header.data()+32,&d,16);put(mesh+0x26c,header);
        put(asset+0xa4,p::Array{ref,115,115});
        std::array<p::Matrix,115> values{};
        for(auto& m:values)m={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        values[52][12]=3;put(space,values);
        put(local,std::array<unsigned char,115*32>{});put(ref,std::array<unsigned char,115*68>{});
    }
    bool read(p::Address address,void* out,std::size_t size){
        const auto n=++reads[address];if(mutate)mutate(address,n);
        const auto it=memory.find(address);if(it==memory.end()||it->second.size()!=size)return false;
        std::memcpy(out,it->second.data(),size);return true;
    }
    bool capture(p::Snapshot& out){return p::capture([&](p::Address a,void* o,std::size_t n){return read(a,o,n);},mesh,asset,ctw,out);}
};
int main(){
    unsigned cases{};
    {Fixture f;p::Snapshot a{},b{};assert(f.capture(a)&&f.capture(b)&&p::same(a,b));
     const auto w=p::world(a,52);assert(w[12]==13&&w[13]==20&&w[14]==30);++cases;}
    for(unsigned kind=0;kind<11;++kind){Fixture f;p::Snapshot out{};out.epoch=777;
        switch(kind){
        case 0:f.memory.erase(Fixture::space);break;
        case 1:f.put(Fixture::mesh+0x218,p::Address(0));break;
        case 2:{auto& h=f.memory[Fixture::mesh+0x26c];p::Address master=0x80000;std::memcpy(h.data()+48,&master,8);break;}
        case 3:f.put(Fixture::asset+0xa4,p::Array{Fixture::ref,114,115});break;
        case 4:f.put(Fixture::asset+0xa4,p::Array{Fixture::ref,115,513});break;
        case 5:f.put(Fixture::asset+0xa4,p::Array{UINT64_MAX-8,115,115});break;
        case 6:{auto& v=f.memory[Fixture::space];const float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(v.data()+52*64,&nan,4);break;}
        case 7:f.mutate=[&](p::Address a,unsigned n){if(a==Fixture::mesh+0x338&&n==2)f.put(a,13u);};break;
        case 8:f.mutate=[&](p::Address a,unsigned n){if(a==Fixture::mesh+0x26c&&n==2)f.memory[a][0]^=1;};break;
        case 9:f.mutate=[&](p::Address a,unsigned n){if(a==Fixture::mesh+0x218&&n==2)f.put(a,p::Address(0));};break;
        case 10:f.mutate=[&](p::Address a,unsigned){if(a==Fixture::mesh+0xa0)f.memory[a][0]^=1;};break;
        }
        assert(!f.capture(out)&&out.epoch==777);++cases;
    }
    for(unsigned kind=0;kind<6;++kind){Fixture f;p::Snapshot a{},b{};assert(f.capture(a));
        switch(kind){case 0:f.put(Fixture::mesh+0x338,13u);break;
        case 1:f.memory[Fixture::space][52*64+48]^=1;break;
        case 2:f.memory[Fixture::space][35*64+48]^=1;break;
        case 3:f.memory[Fixture::local][0]^=1;break;
        case 4:f.memory[Fixture::ref][52*68]^=1;break;
        case 5:f.ctw[12]+=1;f.put(Fixture::mesh+0xa0,f.ctw);break;}
        assert(f.capture(b)&&!p::same(a,b));++cases;
    }
    std::printf("WeaponComponentPoseCohortTest: %u cases; actual read-only Ref115 cohort, no engine calls or pose authority\n",cases);
}
