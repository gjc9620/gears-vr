// Observation-only path. Actual selection/stow/switch retains vrequip::snapshot.
// Shipped WarInventoryManager.GetWeaponInSlot iterates InventoryActors(WarWeapon)
// and returns the matching CharacterSlot. Certify the observed chain mapping
// against ALL FOUR actual original calls before using a structural profile;
// any object/link/class/slot/property/function change requires recertification.
#include "../vr-common/weapon_inventory_chain.h"
namespace idle_inventory {
namespace chain=gears_weapon_inventory_chain;
#include "weapon_equipment_inventory_fields.inl"
struct Node {abi::ObjectIdentity identity{};Field next{},slot{};};
struct Capture {abi::ObjectIdentity manager{};Field head{};uptr first{};chain::View view{};std::array<Node,32> nodes{};};
struct Certificate {Capture inventory{};abi::ObjectIdentity function{};Array script{};std::uint64_t hash{};bool valid{};};
Certificate certificate;
std::uint64_t hits{},qualifications{},rejected{};
bool same(Field const& a,Field const& b) noexcept {
    return abi::same(a.identity,b.identity)&&a.offset==b.offset&&a.width==b.width;
}
bool same(Capture const& a,Capture const& b) noexcept {
    if(!abi::same(a.manager,b.manager)||!same(a.head,b.head)||a.first!=b.first||!chain::same(a.view,b.view))return false;
    for(unsigned i=0;i<a.view.count;++i)if(!abi::same(a.nodes[i].identity,b.nodes[i].identity)||
       !same(a.nodes[i].next,b.nodes[i].next)||!same(a.nodes[i].slot,b.nodes[i].slot))return false;
    return true;
}
bool capture(abi::Frame const& frame,Capture& out) noexcept {
    Capture c{};const auto manager=playground::pointer(frame.pawn.address,L"InvManager");
    if(!playground::instance(manager,L"WarInventoryManager")||playground::deleted(manager)||
       !identify(nullptr,manager,c.manager)||!typed(manager,L"InventoryChain",L"ObjectProperty",8,c.head)||
       !copy(&c.first,manager+c.head.offset,8))return false;
    unsigned count{};
    const auto read=[&](std::uint64_t address,chain::Entry& entry)noexcept {
        if(count>=c.nodes.size())return false;auto& node=c.nodes[count++];
        if(!identify(nullptr,address,node.identity)||!playground::instance(address,L"Inventory")||playground::deleted(address))return false;
        entry.weapon=playground::instance(address,L"WarWeapon");
        if(entry.weapon){if(!weapon_fields(address,node.next,node.slot))return false;}
        else if(!typed(address,L"Inventory",L"ObjectProperty",8,node.next))return false;
        entry.address=address;if(!copy(&entry.next,address+node.next.offset,8))return false;
        if(entry.weapon){unsigned char slot{};
            if(!copy(&slot,address+node.slot.offset,1))return false;
            entry.slot=slot;
        }
        return true;
    };
    if(!chain::capture(c.first,read,c.view))return false;
    out=c;return true;
}
bool function_current(Certificate const& c) noexcept {
    abi::ObjectIdentity id{};Array script{};std::uint64_t hash{};
    return c.valid&&playground::field(c.inventory.manager.klass,L"GetWeaponInSlot")==c.function.address&&
        identify(nullptr,c.function.address,id)&&abi::same(id,c.function)&&
        copy(&script,c.function.address+0x8c,sizeof script)&&script.data==c.script.data&&
        script.count==c.script.count&&script.capacity==c.script.capacity&&
        weaponshot::hash_script(script.data,unsigned(script.count),hash)&&hash==c.hash;
}
bool read(abi::Frame const& frame,Snapshot& out) noexcept {
    out={};Capture before{},after{};
    if(!weaponhand::current_gt(nullptr)||!domain_ready()||
       playground::pointer(frame.controller.address,L"Pawn")!=frame.pawn.address||
       playground::pointer(frame.pawn.address,L"Controller")!=frame.controller.address||
       playground::pointer(frame.pawn.address,L"WorldInfo")!=frame.world.address||
       playground::pointer(frame.pawn.address,L"Weapon")!=frame.weapon.address||!capture(frame,before))return false;
    const bool cached=certificate.valid&&same(before,certificate.inventory)&&function_current(certificate);
    if(!cached){
        certificate={};playground::Args slot_query(before.manager.address,L"GetWeaponInSlot");Certificate candidate{};
        const auto fn=slot_query.r.function;
        if(!registered(fn)||!playground::named(get<uptr>(fn+0x50),L"Function")||
           !playground::named(fn,L"GetWeaponInSlot")||get<unsigned>(fn+0xf4)&0x400||
           !identify(nullptr,fn,candidate.function)||!copy(&candidate.script,fn+0x8c,sizeof(Array))||
           !candidate.script.data||candidate.script.count<16||candidate.script.count>candidate.script.capacity||
           candidate.script.capacity>131072||!weaponshot::hash_script(candidate.script.data,unsigned(candidate.script.count),candidate.hash))return false;
        std::array<std::uint64_t,4> actual{};
        for(unsigned slot=0;slot<4;++slot){uptr item{};
            if(!vrequip::read_slot(before.manager.address,std::uint8_t(slot),item))return false;actual[slot]=item;}
        if(actual!=before.view.slots||!capture(frame,after)||!same(before,after)){++rejected;return false;}
        candidate.inventory=after;candidate.valid=true;
        if(!function_current(candidate))return false;
        // Metadata correspondence only, not a source permit. Its qualification
        // may consume this Frame; the caller STILL rejects an expired Frame.
        certificate=candidate;++qualifications;
        if(log_file&&qualifications<=8){fwprintf(log_file,L"native_equipment_idle_inventory qualified=%llu entries=%u original_queries=4 action_permission=0\n",
            qualifications,after.view.count);fflush(log_file);}
    }else{
        if(!capture(frame,after)||!same(before,after)||!function_current(certificate)){++rejected;return false;}
        ++hits;
    }
    // This is the reduced OBSERVATION snapshot only. It cannot authorize a
    // switch, sound, new hold or ammo/throwable availability decision.
    out.world=frame.world.address;out.pawn=frame.pawn.address;out.current_weapon=frame.weapon.address;
    out.weapon_in_slot=after.view.slots;
    return playground::pointer(frame.pawn.address,L"InvManager")==after.manager.address&&
        playground::pointer(frame.pawn.address,L"Weapon")==frame.weapon.address&&
        playground::pointer(frame.controller.address,L"Pawn")==frame.pawn.address&&
        playground::pointer(frame.pawn.address,L"Controller")==frame.controller.address&&
        playground::pointer(frame.pawn.address,L"WorldInfo")==frame.world.address;
}
} // namespace idle_inventory
