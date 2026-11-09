#include "weapon_hammerburst_first.h"
#include <cassert>
#include <cstdio>
#ifdef NDEBUG
#error Assertions required
#endif
int main(){
    // Independent package191-byte fixture, SHA saved in hb-burst-static-
    // contract.json. Decode operand indices by value/token, NOT by the
    // production matcher's relocation positions or emitted expected array.
    constexpr char package[]="142d013294000082772e1e0e000001c5f6ffff2a16182100192e1e0e000001c5f6ffff0a00042d01f7040000160f013594000024001b7a7f00000000000020115a01001e0000803f1607f90082772e596900001901c5f6ffff0900080184f6ffff2a161818001901c5f6ffff0a00041c81f9ffff161607ef00811b505b0000000000001616192e596900001901c5f6ffff0900080184f6ffff1300001c2ff9ffff01c9590000161b026a000000000000161c86f7ffff005794000016040b53";
    std::array<unsigned char,191> raw{};
    const auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
    for(unsigned i=0;i<raw.size();++i)raw[i]=static_cast<unsigned char>(16*digit(package[2*i])+digit(package[2*i+1]));
    constexpr std::array<std::int32_t,13> indices{{-2428,-2363,-2170,-1745,-1663,1271,3614,22985,26969,37938,37941,37975,88593}};
    constexpr std::array<unsigned,3> name_indices{{32634,23376,27138}};
    gears_hammerburst_first::Bindings bindings{};
    for(unsigned i=0;i<13;++i)bindings.objects[i]=0x120000000ull+0x1000*i;
    for(unsigned i=0;i<3;++i)bindings.names[i]={0x8100+i,0};
    std::array<unsigned char,271> actual{};unsigned out{},objects{},names{};
    for(unsigned at=0;at<raw.size();){
        bool handled{};
        if(at&&at+4<=raw.size()&&(raw[at-1]==0||raw[at-1]==1||raw[at-1]==0x1c||raw[at-1]==0x20||raw[at-1]==0x2e)){
            std::int32_t index{};std::memcpy(&index,raw.data()+at,4);
            for(unsigned slot=0;slot<indices.size();++slot)if(index==indices[slot]){
                std::memcpy(actual.data()+out,&bindings.objects[slot],8);out+=8;at+=4;++objects;handled=true;break;
            }
        }
        if(handled)continue;
        if(at&&at+8<=raw.size()&&raw[at-1]==0x1b){
            unsigned index{},number{};std::memcpy(&index,raw.data()+at,4);std::memcpy(&number,raw.data()+at+4,4);
            for(unsigned slot=0;slot<3;++slot)if(index==name_indices[slot]&&!number){
                std::memcpy(actual.data()+out,&bindings.names[slot],8);out+=8;at+=8;++names;handled=true;break;
            }
        }
        if(!handled)actual[out++]=raw[at++];
    }
    unsigned checks{};
    assert(out==271&&objects==20&&names==3);++checks;
    const auto accepts=[&](unsigned char const* p,std::size_t n){return gears_hammerburst_first::entry(p,n,bindings);};
    assert(accepts(actual.data(),actual.size()));++checks;
    for(unsigned i=0;i<271;++i){auto mutated=actual;mutated[i]^=1;assert(!accepts(mutated.data(),271));++checks;}
    for(unsigned n=0;n<271;++n){assert(!accepts(actual.data(),n));++checks;}
    assert(!accepts(nullptr,271));++checks;
    std::array<unsigned char,272> extra{};std::memcpy(extra.data(),actual.data(),271);
    assert(!accepts(extra.data(),272));++checks;
    for(unsigned i=0;i<13;++i){auto b=bindings;b.objects[i]=0;assert(!gears_hammerburst_first::entry(actual.data(),271,b));++checks;}
    for(unsigned i=1;i<13;++i){auto b=bindings;b.objects[i]=b.objects[0];assert(!gears_hammerburst_first::entry(actual.data(),271,b));++checks;}
    for(unsigned i=0;i<3;++i){auto b=bindings;b.names[i].number=1;assert(!gears_hammerburst_first::entry(actual.data(),271,b));++checks;}
    std::printf("HB complete original outer-first matcher: %u recorded-fixture/mutation/truncation/reference checks passed; CPU only\n",checks);
}
