// Veto original automatic refill for authenticated manual-reload inventory
// actors, including the same gun while holstered or being equipped again.
// Uses Native's existing CallFunction owner; original still executes once.
// This is an invocation-local non-action veto, never a tracking/fire permit.
namespace lancerreload_manual {
namespace hand=gears_weapon_hand;namespace abi=gears_weapon_call;
struct Binding {
    hand::ObjectIdentity world{},controller{},pawn{},weapon{},function{},property{};
    std::uint64_t cookie{},native{},hold{},script_hash{},exec{};
    Array script{};unsigned mask{},offset{},extent{};bool valid{},starter{};
};
Binding binding;
Binding starter_binding;
// Retain the non-action auto-reload veto for actual owned inventory actors
// across stow/equip. A holstered empty weapon must not refill while there is
// no current hand hold. These entries never retain a pose or authorize input.
std::array<Binding,32> inventory_bindings{};
std::uint64_t blocked{},reported{};
std::uint64_t starter_blocked{},starter_reported{};
struct Permission {uptr weapon{};std::uint64_t cookie{},native{};DWORD thread{};};
Permission manual_start;
Permission starter_start;
bool requires_permission(hand::Frame const& source) noexcept {
    // CanReload is currently vetoed only for the exact Lancer adapter.
    // Shared HB acceptance must not require a nonexistent Lancer binding.
    return playground::instance(source.weapon.address,L"Weap_AssaultRifle")||
        (hammerburst_runtime::manual_enabled()&&playground::instance(source.weapon.address,L"Weap_LocustAssaultRifle"))||
        (snub_runtime::manual_enabled()&&playground::instance(source.weapon.address,L"Weap_COGPistol"));
}
bool enter(hand::Frame const& source) noexcept {
    if(manual_start.weapon||!binding.valid||!weaponhand::current_gt(nullptr)||
       !unified_weapon_fire::manual_profile()||!hand::same(binding.weapon,source.weapon)||
       binding.cookie!=source.key.registration_cookie||binding.native!=source.key.native_started_qpc||
       !nativeequipment::current(nullptr,source))return false;
    manual_start={source.weapon.address,source.key.registration_cookie,source.key.native_started_qpc,GetCurrentThreadId()};
    return true;
}
void leave() noexcept {manual_start={};}
bool permitted(abi::Invocation const& call) noexcept {
    return manual_start.weapon==call.self&&manual_start.cookie==call.hand_cookie&&
        manual_start.native==call.native_started_qpc&&manual_start.thread==call.thread_id;
}
bool layout(Binding const& b) noexcept {
    const auto fn=b.function.address,p=b.property.address;
    Array script{};std::uint64_t hash{};
    if(!lancerreload_ui::object(b.function)||!lancerreload_ui::object(b.property)||
       !lancerreload_ui::original_function(fn,b.starter?L"CanDoSpecialMeleeAttack":L"CanReload",b.starter?L"Weap_AssaultRifle":L"WarWeapon",false)||
       (get<unsigned>(fn+0xf4)&0x400)||get<std::uint16_t>(fn+0xf8)||
       b.exec!=base+0x1d6ca0||get<uptr>(fn+0x114)!=b.exec||get<std::uint16_t>(fn+0x106)!=4||
       get<std::uint16_t>(fn+0x108)!=b.offset||get<unsigned>(fn+0x88)!=b.extent||
       get<uptr>(p+0x40)!=fn||!playground::named(p,L"ReturnValue")||
       !playground::named(get<uptr>(p+0x50),L"BoolProperty")||get<unsigned>(p+0x70)!=1||
       get<unsigned>(p+0x74)!=4||(get<unsigned long long>(p+0x78)&0x480)!=0x480||
       get<unsigned>(p+0x94)!=b.offset||get<unsigned>(p+0xd0)!=b.mask||
       !copy(&script,fn+0x8c,sizeof script)||script.data!=b.script.data||
       script.count!=b.script.count||script.capacity!=b.script.capacity||
       !weaponshot::hash_script(script.data,unsigned(script.count),hash)||hash!=b.script_hash)return false;
    return true;
}
void prepare_starter(hand::Frame const& source) noexcept {
    if(starter_blocked!=starter_reported){
        static unsigned rows{};
        if(log_file&&rows++<8){fwprintf(log_file,L"lancer_starter_raw_button_veto blocked=%llu original_called_once=1 menu_button_changed=0\n",starter_blocked);fflush(log_file);}
        starter_reported=starter_blocked;
    }
    if(!unified_weapon_fire::manual_profile()||!weaponhand::current_gt(nullptr)||
       !nativeequipment::current(nullptr,source)||!playground::instance(source.weapon.address,L"Weap_AssaultRifle"))return;
    auto const& old=starter_binding;
    if(old.valid&&old.cookie==source.key.registration_cookie&&old.native==source.key.native_started_qpc&&
       hand::same(old.world,source.world)&&hand::same(old.controller,source.controller)&&
       hand::same(old.pawn,source.pawn)&&hand::same(old.weapon,source.weapon))return;
    starter_binding={};playground::Args query(source.weapon.address,L"CanDoSpecialMeleeAttack");
    if(query.r.size!=4||!vrequip::result_property(query,L"BoolProperty",4))return;
    Binding b{};b.starter=true;b.world=source.world;b.controller=source.controller;b.pawn=source.pawn;b.weapon=source.weapon;
    b.cookie=source.key.registration_cookie;b.native=source.key.native_started_qpc;
    const auto prop=query.prop(L"ReturnValue");
    if(!nativeequipment::identify(nullptr,query.r.function,b.function)||!nativeequipment::identify(nullptr,prop,b.property))return;
    b.mask=get<unsigned>(prop+0xd0);b.offset=get<unsigned>(prop+0x94);b.extent=get<unsigned>(query.r.function+0x88);
    b.exec=get<uptr>(query.r.function+0x114);
    if(!b.mask||(b.mask&(b.mask-1))||b.offset||b.extent<4||b.extent>4096||
       !copy(&b.script,query.r.function+0x8c,sizeof b.script)||!b.script.data||b.script.count<=0||
       b.script.count>b.script.capacity||b.script.capacity>131072||
       !weaponshot::hash_script(b.script.data,unsigned(b.script.count),b.script_hash)||!layout(b)||
       !nativeequipment::current(nullptr,source))return;
    b.valid=true;starter_binding=b;
}
bool enter_starter(hand::Frame const& source) noexcept {
    auto const& b=starter_binding;
    if(starter_start.weapon||!b.valid||!b.starter||!layout(b)||!nativeequipment::current(nullptr,source)||
       b.cookie!=source.key.registration_cookie||b.native!=source.key.native_started_qpc||
       !hand::same(b.weapon,source.weapon)||!hand::same(b.pawn,source.pawn)||!hand::same(b.world,source.world))return false;
    starter_start={source.weapon.address,source.key.registration_cookie,source.key.native_started_qpc,GetCurrentThreadId()};return true;
}
void leave_starter() noexcept {starter_start={};}
void prepare(hand::Frame const& source) noexcept {
    if(blocked!=reported){
        static unsigned rows{};
        if(log_file&&rows++<8){fwprintf(log_file,L"lancer_manual_auto_reload blocked=%llu original_called_once=1 inventory_write=0\n",blocked);
            fflush(log_file);}
        reported=blocked;
    }
    if(!unified_weapon_fire::manual_profile()||!weaponhand::current_gt(nullptr)||
       !vrequip::model.held_for(source.world.address,source.pawn.address,source.weapon.address)){
        binding={};return;
    }
    const auto held=vrequip::model.hold_generation();
    if(binding.valid&&binding.cookie==source.key.registration_cookie&&binding.native==source.key.native_started_qpc&&
       binding.hold==held&&hand::same(binding.world,source.world)&&hand::same(binding.controller,source.controller)&&
       hand::same(binding.pawn,source.pawn)&&hand::same(binding.weapon,source.weapon))return;
    binding={};lancermain_asset::Evidence asset{};
    if(!requires_permission(source)||!nativeequipment::current(nullptr,source)||
       lancermain_asset::read(source,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready)return;
    // Active.BeginFire(0) calls ForceReload directly on an empty gun, bypassing
    // ShouldAutoReload. Its CanReload query is the shared entry decision.
    // Only the already-authenticated physical begin scope may accept it.
    playground::Args query(source.weapon.address,L"CanReload");
    if(!vrequip::result_property(query,L"BoolProperty",4)||query.r.size!=4)return;
    Binding b{};b.world=source.world;b.controller=source.controller;b.pawn=source.pawn;b.weapon=source.weapon;
    b.cookie=source.key.registration_cookie;b.native=source.key.native_started_qpc;b.hold=held;
    const auto prop=query.prop(L"ReturnValue");
    if(!nativeequipment::identify(nullptr,query.r.function,b.function)||!nativeequipment::identify(nullptr,prop,b.property))return;
    b.mask=get<unsigned>(prop+0xd0);b.offset=get<unsigned>(prop+0x94);b.extent=get<unsigned>(query.r.function+0x88);
    b.exec=get<uptr>(query.r.function+0x114);
    if(!b.mask||(b.mask&(b.mask-1))||b.offset>0||b.extent<4||b.extent>4096||
       !copy(&b.script,query.r.function+0x8c,sizeof b.script)||!b.script.data||b.script.count<=0||
       b.script.count>b.script.capacity||b.script.capacity>131072||
       !weaponshot::hash_script(b.script.data,unsigned(b.script.count),b.script_hash)||!layout(b)||
       !nativeequipment::current(nullptr,source))return;
    b.valid=true;binding=b;
    Binding* retained=nullptr;
    for(auto& old:inventory_bindings){
        if(old.valid&&hand::same(old.weapon,b.weapon)&&old.cookie==b.cookie&&old.native==b.native){retained=&old;break;}
        if(!retained&&(!old.valid||old.cookie!=b.cookie||old.native!=b.native||
            !hand::same(old.world,b.world)||!lancerreload_ui::object(old.weapon)))retained=&old;
    }
    if(retained)*retained=b;
    if(log_file){fwprintf(log_file,L"lancer_manual_auto_reload_bound weapon=%llx hold=%llu mask=%u flags=%llx exec=%llx script_hash=%llx\n",
        b.weapon.address,b.hold,b.mask,get<unsigned long long>(prop+0x78),b.exec,b.script_hash);fflush(log_file);}
}
bool current(abi::Invocation const& call,Binding const& b) noexcept {
    // No pose/age refresh here. Preventing automatic ammunition is not a hand
    // action, and a brief tracking interruption must not refill the gun.
    return b.valid&&enabled.load(std::memory_order_acquire)&&unified_weapon_fire::manual_profile()&&
        weaponhand::current_gt(nullptr)&&call.self==b.weapon.address&&call.callee==b.function.address&&
        (!b.starter||playground::pointer(b.pawn.address,L"Weapon")==b.weapon.address)&&
        call.hand_cookie==b.cookie&&call.native_started_qpc==b.native&&
        weaponhand::client.registration_alive(b.cookie,b.native)&&
        lancerreload_ui::object(b.world)&&lancerreload_ui::object(b.controller)&&
        lancerreload_ui::object(b.pawn)&&lancerreload_ui::object(b.weapon)&&
        playground::pointer(b.controller.address,L"Pawn")==b.pawn.address&&
        playground::pointer(b.controller.address,L"WorldInfo")==b.world.address&&
        playground::pointer(b.pawn.address,L"Controller")==b.controller.address&&
        playground::pointer(b.pawn.address,L"WorldInfo")==b.world.address&&
        playground::pointer(b.weapon.address,L"Instigator")==b.pawn.address&&layout(b);
}
struct Slot {abi::Invocation call{};Binding owner{};bool active{};};
std::array<Slot,4> slots{};
void begin(abi::Invocation const& call) noexcept {
    if(!call.result||(call.result&3)||call.result>UINTPTR_MAX-4)return;
    const bool starter_permitted=(starter_start.weapon==call.self&&starter_start.cookie==call.hand_cookie&&
        starter_start.native==call.native_started_qpc&&starter_start.thread==call.thread_id)||
        lancersaw_original::starter_continuation(call);
    if(!starter_permitted&&current(call,starter_binding)){
        for(auto& slot:slots)if(!slot.active){slot={call,starter_binding,true};return;}
        return;
    }
    if(permitted(call))return;
    for(auto const& b:inventory_bindings)if(current(call,b)){
        for(auto& slot:slots)if(!slot.active){slot={call,b,true};return;}
        return;
    }
}
bool compare_result(uptr address,unsigned before,unsigned after,bool& exchanged) noexcept {
    exchanged=false;
    __try{exchanged=unsigned(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(address),LONG(after),LONG(before)))==before;return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?
        EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool returned(abi::Invocation const& call) noexcept {
    Slot* found=nullptr;for(auto& slot:slots)if(slot.active&&slot.call.call_id==call.call_id){found=&slot;break;}
    if(!found)return true;const auto s=*found;*found={};
    if(!abi::same(s.call,call)||!current(call,s.owner))return true;
    unsigned word{};if(!copy(&word,call.result,4)||!(word&s.owner.mask))return true;
    bool exchanged{};
    if(!compare_result(call.result,word,word&~s.owner.mask,exchanged))return false;
    if(exchanged){if(s.owner.starter)++starter_blocked;else ++blocked;}
    return true;
}
void cleanup(abi::Invocation const& call) noexcept {
    for(auto& slot:slots)if(slot.active&&slot.call.call_id==call.call_id)slot={};
}
} // namespace lancerreload_manual
