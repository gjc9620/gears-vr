#include "weapon_inventory_chain.h"
#include <cassert>
#include <cstdio>
#include <vector>
namespace c=gears_weapon_inventory_chain;
int main(){
    std::vector<c::Entry> memory{{1,2,3,true},{2,3,255,false},{3,0,2,true}};
    unsigned reads{};bool unavailable{};
    auto read=[&](std::uint64_t a,c::Entry& e){++reads;if(unavailable)return false;
        for(auto const& item:memory)if(item.address==a){e=item;return true;}return false;};
    c::View out{};assert(c::capture(1,read,out)&&reads==3&&out.count==3&&out.slots[3]==1&&out.slots[2]==3);
    auto certified=out;assert(c::same(out,certified));
    // The same head is insufficient: changed linkage, class/slot or address
    // invalidates correspondence and requires original-query qualification.
    memory[2].slot=0;assert(c::capture(1,read,out)&&!c::same(out,certified));
    memory[2].slot=3;assert(c::capture(1,read,out)&&out.slots[3]==1&&!out.slots[2]); // original first-match order
    memory[2].slot=2;memory[2].next=1;assert(!c::capture(1,read,out)); // cycle
    memory[2].next=99;assert(!c::capture(1,read,out)); // unknown chain member
    memory[2].next=0;unavailable=true;assert(!c::capture(1,read,out));unavailable=false;
    memory[0].weapon=false;assert(c::capture(1,read,out)&&!out.slots[3]&&!c::same(out,certified));
    memory[0].weapon=true;memory[1].slot=2; // non-weapon slot is irrelevant
    assert(c::capture(1,read,out)&&out.slots[2]==3);
    assert((c::capture(0,read,out)&&out.count==0&&out.slots==std::array<std::uint64_t,4>{}));
    memory.clear();for(unsigned i=1;i<=33;++i)memory.push_back({i,i==33?0:i+1,255,false});
    assert(!c::capture(1,read,out)); // never accept a truncated32-entry prefix
    memory[31].next=0;assert(c::capture(1,read,out)&&out.count==32);
    std::puts("Inventory chain: live mapping, topology changes, non-weapons, first-match duplicates, cycles and unavailable/truncated chains passed (CPU only)");
}
