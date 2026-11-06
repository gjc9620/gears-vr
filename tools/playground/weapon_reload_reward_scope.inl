// One synchronous original reward invocation, on the existing Native owner.
// Preserve its opening eligibility after the manual inventory has committed.
// No ammo/default/function patch, new hook, engine call in a notice, or input
// permission. Only the exact invocation-local Bool return can be replaced.
#include "../native-capture/weapon_call_trace.h"
namespace lancerreload_reward {
namespace hand=gears_weapon_hand;namespace abi=gears_weapon_call;
namespace inv=lancerreload_inventory;
struct Function {
    hand::ObjectIdentity identity{};Array script{};std::uint64_t hash{};
};
struct Context {
    inv::Identity owner{};Function getter{},caller{};hand::ObjectIdentity property{},controller{};
    unsigned mask{},extent{},queries{};DWORD thread{};bool eligible{},active{},failed{};
};
thread_local Context context;
bool function(uptr address,const wchar_t* label,Function& out) noexcept {
    Function f{};
    if(!lancerreload_ui::original_function(address,label,L"WarWeapon",false)||
       get<unsigned>(address+0xf4)&0x400||get<std::uint16_t>(address+0xf8)||
       get<uptr>(address+0x114)!=base+0x1d6ca0||
       !nativeequipment::identify(nullptr,address,f.identity)||
       !copy(&f.script,address+0x8c,sizeof f.script)||!f.script.data||f.script.count<=0||
       f.script.count>f.script.capacity||f.script.capacity>131072||
       !weaponshot::hash_script(f.script.data,unsigned(f.script.count),f.hash))return false;
    out=f;return true;
}
bool same_function(Function const& expected,const wchar_t* label) noexcept {
    Function now{};return function(expected.identity.address,label,now)&&
        hand::same(now.identity,expected.identity)&&now.script.data==expected.script.data&&
        now.script.count==expected.script.count&&now.script.capacity==expected.script.capacity&&now.hash==expected.hash;
}
bool current(Context const& c) noexcept {
    const auto& id=c.owner;const auto fn=c.getter.identity.address,p=c.property.address;
    return c.active&&c.thread==GetCurrentThreadId()&&weaponhand::current_gt(nullptr)&&
        enabled.load(std::memory_order_acquire)&&unified_weapon_fire::manual_profile()&&
        weaponhand::client.registration_alive(id.registration,id.native_start)&&
        lancerreload_ui::object(id.world)&&lancerreload_ui::object(id.pawn)&&lancerreload_ui::object(id.weapon)&&
        lancerreload_ui::object(c.controller)&&lancerreload_ui::object(c.property)&&
        playground::pointer(c.controller.address,L"Pawn")==id.pawn.address&&
        playground::pointer(c.controller.address,L"WorldInfo")==id.world.address&&
        playground::pointer(id.pawn.address,L"Controller")==c.controller.address&&
        playground::pointer(id.pawn.address,L"Weapon")==id.weapon.address&&
        playground::pointer(id.weapon.address,L"Instigator")==id.pawn.address&&
        same_function(c.getter,L"IsApplicableForARDamageBonus")&&
        same_function(c.caller,L"SetActiveReloadBonusActive")&&
        get<std::uint16_t>(fn+0x106)==4&&get<std::uint16_t>(fn+0x108)==0&&
        get<unsigned>(fn+0x88)==c.extent&&get<uptr>(p+0x40)==fn&&
        playground::named(get<uptr>(p+0x50),L"BoolProperty")&&
        get<unsigned>(p+0x70)==1&&get<unsigned>(p+0x74)==4&&get<unsigned>(p+0x94)==0&&
        // Original ReturnValue is Parm|OutParm|ReturnParm (0x580). Live
        // four-function read-only metadata receipt: reload-reward-metadata-50104.
        (get<unsigned long long>(p+0x78)&0x580)==0x580&&get<unsigned>(p+0xd0)==c.mask;
}
bool returned(abi::Invocation const& call) noexcept {
    auto& c=context;
    if(!c.active||call.self!=c.owner.weapon.address||call.callee!=c.getter.identity.address)return true;
    gears_ui_call_boundary::Frame frame{};
    if(!abi::invocation_valid(call)||call.thread_id!=c.thread||call.hand_cookie!=c.owner.registration||
       call.native_started_qpc!=c.owner.native_start||!call.result||(call.result&3)||call.result>UINTPTR_MAX-4||
       !current(c)||!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,frame)||
       frame.node!=c.caller.identity.address||frame.object!=c.owner.weapon.address||
       !gears_weapon_call_trace::script_pc(frame.code,c.caller.script.data,unsigned(c.caller.script.count))||
       c.queries>=4){c.failed=true;return true;}
    unsigned before{};if(!copy(&before,call.result,4)){c.failed=true;return true;}
    const auto wanted=c.eligible?before|c.mask:before&~c.mask;
    bool exchanged=true;
    if(wanted!=before&&!lancerreload_manual::compare_result(call.result,before,wanted,exchanged)){
        c.failed=true;return false;
    }
    if(!exchanged){c.failed=true;return true;}
    ++c.queries;return true;
}
bool invoke(inv::Identity const& owner,bool eligible,playground::Args& success,bool& qualification_applied,bool& dispatched) noexcept {
    qualification_applied=false;dispatched=false;
    if(context.active||!owner.valid()||!weaponhand::current_gt(nullptr))return false;
    playground::Args getter(owner.weapon.address,L"IsApplicableForARDamageBonus");
    playground::Args caller(owner.weapon.address,L"SetActiveReloadBonusActive");
    Context c{};c.owner=owner;c.thread=GetCurrentThreadId();c.eligible=eligible;
    const auto p=getter.prop(L"ReturnValue");
    if(!vrequip::result_property(getter,L"BoolProperty",4)||getter.r.size!=4||
       !function(getter.r.function,L"IsApplicableForARDamageBonus",c.getter)||
       !function(caller.r.function,L"SetActiveReloadBonusActive",c.caller)||
       !nativeequipment::identify(nullptr,p,c.property)||
       !nativeequipment::identify(nullptr,playground::pointer(owner.pawn.address,L"Controller"),c.controller))return false;
    c.mask=get<unsigned>(p+0xd0);c.extent=get<unsigned>(getter.r.function+0x88);c.active=true;
    if(!c.mask||(c.mask&(c.mask-1))||c.extent<4||c.extent>4096||!current(c))return false;
    struct Guard {~Guard(){context={};}} guard;
    context=c;
    success.flag(L"bDidSuperSweetReload",true);
    if(!success.valid)return false;
    // Preflight rejection issued no original event. Once an invocation is
    // attempted, retain this bit even on an uncertain/failed engine return so
    // score, feedback and reward can never be dispatched twice.
    dispatched=true;
    const bool ran=success.run();
    qualification_applied=ran&&!context.failed&&context.queries>0;
    if(log_file){fwprintf(log_file,L"reload_original_reward_scope eligible=%u ran=%u queries=%u applied=%u failed=%u\n",
        unsigned(eligible),unsigned(ran),context.queries,unsigned(qualification_applied),unsigned(context.failed));fflush(log_file);}
    return ran;
}
}
