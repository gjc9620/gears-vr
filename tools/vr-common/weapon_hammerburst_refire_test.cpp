#include "weapon_hammerburst_refire.h"
#ifdef NDEBUG
#error Assertions required
#endif
#include <cassert>
#include <cstdio>
template<std::size_t N>std::array<unsigned char,(N-1)/2> hex(char const(&s)[N]){
    static_assert((N-1)%2==0);std::array<unsigned char,(N-1)/2> out{};
    const auto digit=[](char c){return c>='0'&&c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
    for(std::size_t i=0;i<out.size();++i)out[i]=static_cast<unsigned char>(16*digit(s[2*i])+digit(s[2*i+1]));
    return out;
}
int main(){
 namespace hb=gears_hammerburst_refire;
 // Independent fixture: exact package71 bytes with only its two object refs
 // relocated to declared synthetic64-bit property identities. Not live proof.
 const std::array<unsigned char,79> actual{{0x07,0x19,0x00,0x2d,0x01,0x44,0x44,0x33,0x33,0x22,0x22,0x11,0x11,0x1b,0xbd,0x5d,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x04,0x0b,0x07,0x32,0x00,0x1b,0xa6,0x6b,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x1b,0xca,0x32,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x04,0x0b,0x14,0x2d,0x01,0x88,0x88,0x77,0x77,0x66,0x66,0x55,0x55,0x28,0x71,0x21,0xe5,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x50,0x50,0x50,0x16,0x04,0x0b,0x53}};
 constexpr std::uint64_t down=0x1111222233334444,target=0x5555666677778888;
 const hb::Name put{23997,0},should{27558,0},ammo{13002,0},burst{4581,0};
 unsigned checks{};
 auto accepts=[&](unsigned char const* p,std::size_t n){return hb::timer(p,n,down,target,put,should,ammo,burst);};
 assert(accepts(actual.data(),actual.size()));++checks;
 for(std::size_t i=0;i<actual.size();++i){auto changed=actual;changed[i]^=1;assert(!accepts(changed.data(),changed.size()));++checks;}
 for(std::size_t n=0;n<actual.size();++n){assert(!accepts(actual.data(),n));++checks;}
 auto extra=std::array<unsigned char,80>{};std::memcpy(extra.data(),actual.data(),actual.size());assert(!accepts(extra.data(),extra.size()));++checks;
 assert(!accepts(nullptr,actual.size()));++checks;
 assert(!hb::timer(actual.data(),79,0,target,put,should,ammo,burst));++checks;
 assert(!hb::timer(actual.data(),79,down,down,put,should,ammo,burst));++checks;
 assert(!hb::timer(actual.data(),79,down,target,{23997,1},should,ammo,burst));++checks;
 assert(!hb::timer(actual.data(),79,down,target,put,{0,0},ammo,burst));++checks;
 assert(!hb::timer(actual.data(),79,down,target,put,should,ammo,{4581,1}));++checks;
 assert(!hb::timer(actual.data(),79,target,down,put,should,ammo,burst));++checks;
 // Independent serialized retail83-byte ShouldRefire fixture. Object operands
 // are relocated only at the seven independently decoded offsets; all jumps,
 // conversions, context, omitted argument and return tokens stay untouched.
 const auto should_package=hex("072a00811901c5f6ffff0a00041c82f9ffff1616041cfd8b000016066400074500811b7d3b0000000000000193feffff501616042807620096383a0135940000383a01369400001604270428043a5d94000053");
 const hb::ShouldBindings bindings{0x11001000,0x22002000,0x33003000,0x44004000,0x55005000,0x66006000,0x77007000,{15229,0}};
 const std::array<std::size_t,7> offsets{{6,14,22,44,60,67,78}};
 const std::array<std::uint64_t,7> relocated{{bindings.instigator,bindings.human_function,bindings.super_should,
     bindings.mode,bindings.count,bindings.limit,bindings.returned}};
 std::array<unsigned char,111> actual_should{};std::size_t out{},ref{};
 for(std::size_t i=0;i<should_package.size();){
     if(ref<offsets.size()&&i==offsets[ref]){std::memcpy(actual_should.data()+out,&relocated[ref],8);out+=8;i+=4;++ref;}
     else actual_should[out++]=should_package[i++];
 }
 assert(out==111&&ref==7);++checks;
 const auto accepts_should=[&](unsigned char const* code,std::size_t n){return hb::should_refire(code,n,bindings);};
 assert(accepts_should(actual_should.data(),111));++checks;
 for(std::size_t i=0;i<111;++i){auto changed=actual_should;changed[i]^=1;assert(!accepts_should(changed.data(),111));++checks;}
 for(std::size_t i=0;i<111;++i){assert(!accepts_should(actual_should.data(),i));++checks;}
 assert(!accepts_should(nullptr,111));++checks;
 auto changed_bindings=bindings;changed_bindings.count=changed_bindings.limit;
 assert(!hb::should_refire(actual_should.data(),111,changed_bindings));++checks;
 changed_bindings=bindings;changed_bindings.has_ammo.number=1;
 assert(!hb::should_refire(actual_should.data(),111,changed_bindings));++checks;
 for(unsigned i=0;i<7;++i){auto altered=relocated;altered[i]=0;auto code=actual_should;
     // Independently rebuild with a missing required operand.
     std::size_t index=offsets[i]+4*i;std::memcpy(code.data()+index,&altered[i],8);
     assert(!accepts_should(code.data(),111));++checks;
 }
 const auto ammo_package=hex("8b0135940000161c508d000016040b53");
 const std::uint64_t count_property=0x12345678,super_ammo=0xabcdef00;
 std::array<unsigned char,24> actual_ammo{};out=0;
 for(std::size_t i=0;i<ammo_package.size();){
     if(i==2||i==8){const auto value=i==2?count_property:super_ammo;
         std::memcpy(actual_ammo.data()+out,&value,8);out+=8;i+=4;}
     else actual_ammo[out++]=ammo_package[i++];
 }
 assert(out==24);++checks;
 const auto accepts_ammo=[&](unsigned char const* code,std::size_t n){return hb::ammunition(code,n,count_property,super_ammo);};
 assert(accepts_ammo(actual_ammo.data(),24));++checks;
 for(std::size_t i=0;i<24;++i){auto changed=actual_ammo;changed[i]^=1;assert(!accepts_ammo(changed.data(),24));++checks;}
 for(std::size_t i=0;i<24;++i){assert(!accepts_ammo(actual_ammo.data(),i));++checks;}
 assert(!accepts_ammo(nullptr,24));++checks;
 assert(!hb::ammunition(actual_ammo.data(),24,0,super_ammo));++checks;
 assert(!hb::ammunition(actual_ammo.data(),24,count_property,count_property));++checks;
 std::printf("HB exact timer/finite-decision/ammunition matchers: %u recorded-fixture/mutation/identity checks passed; CPU only\n",checks);
}
