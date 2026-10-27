#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include "../native-capture/ui_call_boundary.h"
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
#include <windows.h>
#include "../vr-common/weapon_shot_scope.h"
#endif

using uptr=std::uintptr_t;
namespace {
uptr base{};
std::atomic<unsigned> callbacks{};
FILE* log_file{};
std::array<unsigned char,32> entry{};
unsigned created{},removed{};
bool fail_create{},fail_remove{};
constexpr int MH_OK=0;
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
namespace weaponshot {
gears_weapon_shot::Stack stack;unsigned entered{},left{};bool fault{};unsigned originals{};
gears_weapon_shot::Token enter(uptr self,uptr caller,uptr callee) noexcept {
    assert(callbacks.load()==1&&self==11&&caller==22&&callee==44);++entered;return stack.mask();
}
void leave(gears_weapon_shot::Token const& token) noexcept {++left;stack.leave(token);}
}
#endif
namespace handanim {
enum class Source {call_function};
struct Token {unsigned id{};};
std::array<uptr,3> before{},after{};
unsigned entered{},left{};
Token enter(uptr self,uptr callee,void* params,Source,uptr caller){
    assert(!params);before={self,callee,caller};++entered;return {17};
}
void leave(Token const& token,uptr self,uptr callee,void* params,uptr caller){
    assert(token.id==17&&!params);after={self,callee,caller};++left;
}
}
uptr __fastcall original_call(uptr self,uptr caller,uptr result,uptr callee){
    assert(callbacks.load()==1&&self==11&&caller==22&&result==33&&callee==44);
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    ++weaponshot::originals;assert(weaponshot::stack.depth()==1);
    if(weaponshot::fault)RaiseException(0xe1234567ul,0,0,nullptr);
#endif
    return 0xfedcba9876543210ull;
}
bool copy(void* output,uptr address,std::size_t bytes){
    if(address!=base+gears_ui_call_boundary::rva||bytes!=entry.size())return false;
    std::memcpy(output,entry.data(),bytes);return true;
}
int MH_CreateHook(void* address,void*,void** original){
    assert(reinterpret_cast<uptr>(address)==base+gears_ui_call_boundary::rva);
    ++created;if(fail_create)return 1;
    *original=reinterpret_cast<void*>(original_call);return MH_OK;
}
int MH_RemoveHook(void* address){
    assert(reinterpret_cast<uptr>(address)==base+gears_ui_call_boundary::rva);
    ++removed;return fail_remove?1:MH_OK;
}
#include "hand_animation_call_probe.inl"
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
bool original_exception_propagates(){
    __try{handanimcall::invoke_scoped(11,22,33,44);}
    __except(GetExceptionCode()==0xe1234567ul?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
    return false;
}
#endif

void reset(){
    base=reinterpret_cast<uptr>(original_call)-gears_ui_call_boundary::rva-0x1000;
    entry=gears_ui_call_boundary::entry_bytes;created=removed=0;
    fail_create=fail_remove=false;handanimcall::reserved=false;
    handanimcall::target=0;handanimcall::original=nullptr;
}
}
int main(){
    reset();entry[0]^=1;
    assert(!handanimcall::reserve()&&!created&&!handanimcall::reserved);
    reset();fail_create=true;
    assert(!handanimcall::reserve()&&created==1&&!handanimcall::reserved);
    reset();assert(handanimcall::reserve()&&created==1&&handanimcall::reserved);
    assert(!handanimcall::reserve()&&created==1);
    const auto unchanged=entry;
    assert(handanimcall::hook(11,22,33,44)==0xfedcba9876543210ull);
    assert(!callbacks.load()&&entry==unchanged&&handanim::entered==1&&handanim::left==1);
    assert(handanim::before==handanim::after);
#if defined(GEARS_PLAYGROUND_WEAPON_SHOT_SCOPE_TRIAL)
    assert(weaponshot::entered==1&&weaponshot::left==1&&weaponshot::originals==1&&weaponshot::stack.drained());
    weaponshot::fault=true;++callbacks;
    assert(original_exception_propagates());--callbacks;
    assert(weaponshot::entered==2&&weaponshot::left==2&&weaponshot::originals==2&&weaponshot::stack.drained());
    weaponshot::fault=false;
#endif
    fail_remove=true;assert(!handanimcall::remove()&&handanimcall::reserved);
    fail_remove=false;assert(handanimcall::remove()&&!handanimcall::reserved);
    assert(handanimcall::remove()&&removed==2);
    reset();assert(handanimcall::reserve());entry[31]^=1;
    assert(!handanimcall::remove()&&!handanimcall::reserved);
    std::puts("HandAnimationCallProbeOfflineTest passed: occupied entry, reservation, forwarding, callback drain and restore proof");
}
