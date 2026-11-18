// Sole Bow cancellation recipient on the EXISTING Call subscriber. The shared
// R2 owner supplies this journal before its non-firing cancellation and
// retains its exact already-owned Charge serial through cleanup. No hook or original call is made
// by this observer, and an idle journal cannot adopt a stock/external charge.
namespace torquebow_charge {
namespace hand=gears_weapon_hand;namespace call=gears_weapon_call;
torquebow_cancel_live::Owner cancel_owner;
void returned(call::Invocation const& invocation) noexcept {
    if(!cancel_owner.pending()||!weaponhand::current_gt(nullptr)||
       !call::invocation_valid(invocation)||invocation.thread_id!=GetCurrentThreadId()||
       invocation.hand_cookie!=weaponhand::client.cookie()||
       invocation.native_started_qpc!=weaponhand::client.native_started_qpc()||
       !weaponhand::client.registration_alive(invocation.hand_cookie,invocation.native_started_qpc))return;
    cancel_owner.end_state_return(invocation.self,invocation.callee,invocation.call_id);
}
}
