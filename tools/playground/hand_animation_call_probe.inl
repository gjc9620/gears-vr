// Private arena observer for the existing four-pointer CallFunction boundary.
// Do not install alongside another owner: reserve requires original bytes.
// The caller frame is not a parameter buffer; its bytecode and result are
// forwarded unchanged. Only synchronous pre/post metadata is retained.
namespace handanimcall {
using Exec=uptr(__fastcall*)(uptr,uptr,uptr,uptr);
Exec original{};
bool reserved{};
uptr target{};
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
// POD-only SEH helper: close the new observer on unwind without swallowing it.
// The original outer Active/handanim exception behavior is not changed.
uptr invoke_observed(uptr self,uptr caller,uptr result,uptr callee){
    const auto token=gripdamage::enter(self,caller,result,callee);
    uptr value{};bool returned{};
    __try{value=original(self,caller,result,callee);returned=true;}
    __finally{gripdamage::leave(token,returned);}
    return value;
}
#endif
uptr __fastcall hook(uptr self,uptr caller,uptr result,uptr callee){
    struct Active {Active(){++callbacks;}~Active(){--callbacks;}} active;
#if defined(GEARS_PLAYGROUND_HAND_IK_STREAM_TRIAL)
    handstream::before_action(self,callee);
#endif
#if defined(GEARS_PLAYGROUND_EQUIPMENT_STOW_TEST)
    vrequip::stow::before_action(self,callee);
#endif
    const auto token=handanim::enter(self,callee,nullptr,handanim::Source::call_function,caller);
#if defined(GEARS_PLAYGROUND_WEAPON_GRIP_DAMAGE_RESULT_PROBE)
    const auto value=invoke_observed(self,caller,result,callee);
#else
    const auto value=original(self,caller,result,callee);
#endif
    handanim::leave(token,self,callee,nullptr,caller);
    return value; // RAX bits have no inferred semantic meaning here.
}
bool reserve(){
    if(reserved||base>UINTPTR_MAX-gears_ui_call_boundary::rva)return false;
    target=base+gears_ui_call_boundary::rva;
    std::array<unsigned char,32> before{},after{};
    if(!copy(before.data(),target,before.size())||before!=gears_ui_call_boundary::entry_bytes||
       MH_CreateHook(reinterpret_cast<void*>(target),reinterpret_cast<void*>(hook),
                     reinterpret_cast<void**>(&original))!=MH_OK)return false;
    reserved=true;
    const auto delta=static_cast<std::int64_t>(reinterpret_cast<uptr>(original))-
        static_cast<std::int64_t>(target+5);
    if(!copy(after.data(),target,after.size())||after!=before||
       delta<INT32_MIN||delta>INT32_MAX-64)return false;
    if(log_file){fwprintf(log_file,L"hand_animation_call_hook_reserved rva=%llx bytes=32 matched=1 relay_delta=%lld\n",
        static_cast<unsigned long long>(gears_ui_call_boundary::rva),delta);fflush(log_file);}
    return true;
}
bool remove(){
    if(!reserved)return true;
    if(MH_RemoveHook(reinterpret_cast<void*>(target))!=MH_OK)return false;
    reserved=false;
    std::array<unsigned char,32> bytes{};
    const bool matched=copy(bytes.data(),target,bytes.size())&&bytes==gears_ui_call_boundary::entry_bytes;
    if(log_file){fwprintf(log_file,L"hand_animation_call_hook_restored bytes=32 matched=%u\n",unsigned(matched));fflush(log_file);}
    return matched;
}
}
