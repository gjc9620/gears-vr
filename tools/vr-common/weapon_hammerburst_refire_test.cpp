#include "weapon_hammerburst_refire.h"
#ifdef NDEBUG
#error Assertions required
#endif
#include <cassert>
#include <cstdio>
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
 std::printf("HB exact timer matcher: %u recorded-fixture/mutation/identity checks passed; CPU only\n",checks);
}
