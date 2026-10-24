// Include after the Broker types and runtime begin/finish declarations, before
// ui_call_boundary_live. Native owns the sole original CallFunction trampoline.
namespace weapon_call_live {
using Exec=uintptr_t(__fastcall*)(void*,void*,void*,void*);
std::atomic<bool> owned{};

uintptr_t invoke(Exec original,void* self,void* caller_frame,void* result,void* callee){
    gears_weapon_hand::CallScope call{};
    gears_weapon_hand::ActionScope gate{};
    static_assert(std::is_trivially_destructible_v<decltype(call)>&&std::is_trivially_copyable_v<decltype(call)>);
    static_assert(std::is_trivially_destructible_v<decltype(gate)>&&std::is_trivially_copyable_v<decltype(gate)>);
    uintptr_t value{};bool returned{};
    // A game SEH must propagate with its original code. Only PG notifications
    // have their own fault barriers; this finally closes the whole original gap.
    __try {
        weapon_hand_runtime::begin_action(reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(caller_frame),
            reinterpret_cast<uintptr_t>(result),reinterpret_cast<uintptr_t>(callee),gate);
        // Scene observes the atomic generation/barrier. Do not introduce an
        // owner/cache callback here: the caller can inherit Scene/stereo locks.
        __try {
            weapon_hand_runtime::begin_call(reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(caller_frame),
                reinterpret_cast<uintptr_t>(result),reinterpret_cast<uintptr_t>(callee),call,&gate);
            value=original(self,caller_frame,result,callee);returned=true;
        }__finally {weapon_hand_runtime::finish_call(call,returned,value);}
    }__finally {weapon_hand_runtime::finish_action(gate,returned,value);}
    return value;
}
} // namespace weapon_call_live
