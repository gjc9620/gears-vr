#pragma once
#include <array>
#include <cstdint>

namespace gears_weapon_inventory_chain {
struct Entry {std::uint64_t address{},next{};unsigned slot{255};bool weapon{};};
inline bool same(Entry const& a,Entry const& b) noexcept {
    return a.address==b.address&&a.next==b.next&&a.slot==b.slot&&a.weapon==b.weapon;
}
struct View {std::array<Entry,32> entries{};std::array<std::uint64_t,4> slots{};unsigned count{};};
// One bounded live InventoryChain walk. No game call, selection, ammo result,
// held publication or tracking stamp is supplied by this observation.
template<class Read> bool capture(std::uint64_t head,Read read,View& out) noexcept {
    View v{};
    for(auto address=head;address;){
        if(v.count==v.entries.size())return false;
        for(unsigned i=0;i<v.count;++i)if(v.entries[i].address==address)return false;
        Entry e{};if(!read(address,e)||e.address!=address)return false;
        // GetWeaponInSlot returns the FIRST matching actor. A replacement
        // actor can temporarily share a slot with another chain member.
        if(e.weapon&&e.slot<4&&!v.slots[e.slot])v.slots[e.slot]=address;
        v.entries[v.count++]=e;address=e.next;
    }
    out=v;return true;
}
inline bool same(View const& a,View const& b) noexcept {
    if(a.count!=b.count||a.slots!=b.slots)return false;
    for(unsigned i=0;i<a.count;++i)if(!same(a.entries[i],b.entries[i]))return false;
    return true;
}
}
