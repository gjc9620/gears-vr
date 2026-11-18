// Non-action veto only. FinishedCockingShotgun still runs exactly once, but
// its original ShouldRefire result cannot spend an unpaid physical cycle.
// Bound on normal GT before the first R2; no engine calls in normal-return.
namespace gnasher_refire {
namespace hand=gears_weapon_hand;namespace abi=gears_weapon_call;
using Binding=lancerreload_manual::Binding;
std::array<Binding,16> bindings{};
std::uint64_t blocked{};
bool layout(Binding const& b) noexcept {
    const auto fn=b.function.address,p=b.property.address;
    Array script{};std::uint64_t hash{};
    return lancerreload_ui::object(b.function)&&lancerreload_ui::object(b.property)&&
        lancerreload_ui::original_function(fn,L"ShouldRefire",L"WarWeapon",false)&&
        !(get<unsigned>(fn+0xf4)&0x400)&&!get<std::uint16_t>(fn+0xf8)&&
        b.exec==base+0x1d6ca0&&get<uptr>(fn+0x114)==b.exec&&
        get<std::uint16_t>(fn+0x106)==4&&get<std::uint16_t>(fn+0x108)==0&&
        get<unsigned>(fn+0x88)==b.extent&&b.extent>=4&&b.extent<=4096&&
        get<uptr>(p+0x40)==fn&&playground::named(p,L"ReturnValue")&&
        playground::named(get<uptr>(p+0x50),L"BoolProperty")&&get<unsigned>(p+0x70)==1&&
        get<unsigned>(p+0x74)==4&&(get<unsigned long long>(p+0x78)&0x580)==0x580&&
        get<unsigned>(p+0x94)==0&&get<unsigned>(p+0xd0)==b.mask&&b.mask&&!(b.mask&(b.mask-1))&&
        copy(&script,fn+0x8c,sizeof script)&&script.data==b.script.data&&
        script.count==b.script.count&&script.capacity==b.script.capacity&&script.count>0&&
        script.count<=script.capacity&&script.capacity<=131072&&
        weaponshot::hash_script(script.data,unsigned(script.count),hash)&&hash==b.script_hash;
}
bool matches(Binding const& b,hand::Frame const& f) noexcept {
    return b.valid&&b.cookie==f.key.registration_cookie&&b.native==f.key.native_started_qpc&&
        hand::same(b.world,f.world)&&hand::same(b.controller,f.controller)&&
        hand::same(b.pawn,f.pawn)&&hand::same(b.weapon,f.weapon);
}
bool bound_for(hand::Frame const& f) noexcept {
    for(auto const& b:bindings)if(matches(b,f)&&gnasher_refire::layout(b)&&nativeequipment::current(nullptr,f))return true;
    return false;
}
unsigned prepare_stage{};
bool prepare(hand::Frame const& f) noexcept {
    prepare_stage=1;
    if(!unified_weapon_fire::manual_profile()||!weaponhand::current_gt(nullptr)||
       !playground::instance(f.weapon.address,L"Weap_Shotgun")||!nativeequipment::current(nullptr,f))return false;
    prepare_stage=2;
    if(bound_for(f)){prepare_stage=10;return true;}
    prepare_stage=3;
    auto* entry=gnasher_fire::entry(f,false);
    if(!entry||!entry->prepared||entry->cycle.blocked())return false;
    prepare_stage=4;playground::Args query(f.weapon.address,L"ShouldRefire");
    if(query.r.size!=4||!vrequip::result_property(query,L"BoolProperty",4))return false;
    Binding b{};b.world=f.world;b.controller=f.controller;b.pawn=f.pawn;b.weapon=f.weapon;
    b.cookie=f.key.registration_cookie;b.native=f.key.native_started_qpc;
    prepare_stage=5;const auto p=query.prop(L"ReturnValue");
    if(!nativeequipment::identify(nullptr,query.r.function,b.function)||!nativeequipment::identify(nullptr,p,b.property))return false;
    b.mask=get<unsigned>(p+0xd0);b.extent=get<unsigned>(query.r.function+0x88);b.exec=get<uptr>(query.r.function+0x114);
    prepare_stage=6;
    if(!copy(&b.script,query.r.function+0x8c,sizeof b.script)||!b.script.data||b.script.count<=0||
       b.script.count>b.script.capacity||b.script.capacity>131072)return false;
    prepare_stage=7;
    if(!weaponshot::hash_script(b.script.data,unsigned(b.script.count),b.script_hash))return false;
    prepare_stage=8;if(!gnasher_refire::layout(b))return false;
    prepare_stage=9;if(!nativeequipment::current(nullptr,f))return false;
    b.valid=true;
    for(auto& slot:bindings)if(matches(slot,f)||!slot.valid||!lancerreload_ui::object(slot.weapon)){
        slot=b;prepare_stage=10;return true;
    }
    return false;
}
bool current(abi::Invocation const& call,Binding const& b) noexcept {
    // Identity checks are live; a stale tracking pose is neither required nor
    // renewed to DENY an automatic shot. No result is ever changed to true.
    return abi::invocation_valid(call)&&call.thread_id==GetCurrentThreadId()&&b.valid&&
        enabled.load(std::memory_order_acquire)&&unified_weapon_fire::manual_profile()&&weaponhand::current_gt(nullptr)&&
        call.self==b.weapon.address&&call.callee==b.function.address&&call.hand_cookie==b.cookie&&
        call.native_started_qpc==b.native&&weaponhand::client.registration_alive(b.cookie,b.native)&&
        gnasher_fire::blocks_fire(b.weapon)&&lancerreload_ui::object(b.world)&&lancerreload_ui::object(b.controller)&&
        lancerreload_ui::object(b.pawn)&&lancerreload_ui::object(b.weapon)&&
        playground::pointer(b.controller.address,L"Pawn")==b.pawn.address&&
        playground::pointer(b.controller.address,L"WorldInfo")==b.world.address&&
        playground::pointer(b.pawn.address,L"Controller")==b.controller.address&&
        playground::pointer(b.pawn.address,L"WorldInfo")==b.world.address&&
        playground::pointer(b.weapon.address,L"Instigator")==b.pawn.address&&gnasher_refire::layout(b);
}
bool returned(abi::Invocation const& call) noexcept {
    if(!call.result||(call.result&3)||call.result>UINTPTR_MAX-4||!playground::named(call.callee,L"ShouldRefire"))return true;
    for(auto const& b:bindings)if(gnasher_refire::current(call,b)){
        unsigned word{};if(!copy(&word,call.result,4)||!(word&b.mask))return true;
        bool exchanged{};
        if(!lancerreload_manual::compare_result(call.result,word,word&~b.mask,exchanged))return false;
        if(exchanged)++blocked;
        return true;
    }
    return true;
}
}
