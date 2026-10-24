// Lifecycle for the external boundary. Entry restoration/PE bridging is not
// connected yet. The transport below performs only export/lifetime accounting.
namespace weaponboundary {
namespace boundaryabi=gears_weapon_boundary;
boundaryabi::Admission admission;
boundaryabi::StatePool<128> states; // Never move State512 after Open can observe it.
void seal() noexcept {admission.seal();}
bool local_quiescent() noexcept {return admission.quiescent()&&!states.retained();}
bool detached() noexcept {return local_quiescent()&&admission.detach();}
// A normally accepted Native scope is rollback-only authority for its already
// admitted entry. Worker stop may be draining by the time its stage starts;
// the retained local Entry keeps this exact tuple/client state alive. This
// does not permit a new Open, frame grant, fire packet or forward write.
bool grant_matches(boundaryabi::Admission::Entry const& entry,boundaryabi::Request const& request,
                   boundaryabi::Receipt const& receipt,gears_weapon_hand::Reply open_reply) noexcept {
    const auto gt=weaponhand::current_gt(nullptr);
    if(!gt||!enabled.load(std::memory_order_acquire)||!boundaryabi::request_valid(request)||
       (request.flags&boundaryabi::unresolved)||request.thread_id!=gt||open_reply!=gears_weapon_hand::Reply::accepted||
       receipt.size!=sizeof(receipt)||receipt.abi_version!=boundaryabi::version||receipt.hand_cookie!=request.hand_cookie||
       receipt.native_started_qpc!=request.native_started_qpc||receipt.thread_id!=gt||!receipt.scope_id||
       !receipt.revocation_generation||receipt.flags!=(boundaryabi::scope_active|boundaryabi::engine_calls_allowed))return false;
    const auto binding=admission.view(entry);
    const auto phase=weaponhand::client.phase();
    return binding&&binding->cookie==request.hand_cookie&&binding->native_started_qpc==request.native_started_qpc&&
        reinterpret_cast<uptr>(binding->context)==request.context&&binding->context==&weaponhand::client&&
        weaponhand::client.cookie()==request.hand_cookie&&weaponhand::client.native_started_qpc()==request.native_started_qpc&&
        (phase==gears_weapon_hand::ClientPhase::active||phase==gears_weapon_hand::ClientPhase::draining);
}
gears_weapon_hand::Reply attach(void* image) noexcept {
    using Reply=gears_weapon_hand::Reply;
    const auto native=static_cast<HMODULE>(image);
    if(!native||native!=weaponhand::native_module||!weaponaction::ready()||!weaponcall::ready()||states.retained())return Reply::unavailable;
    const auto revoke_address=GetProcAddress(native,boundaryabi::revoke_export),open_address=GetProcAddress(native,boundaryabi::open_export),
        close_address=GetProcAddress(native,boundaryabi::close_export);
    if(!weaponhand::executable(revoke_address,native)||!weaponhand::executable(open_address,native)||!weaponhand::executable(close_address,native))
        return Reply::unavailable;
    boundaryabi::Binding binding{weaponhand::client.cookie(),weaponhand::client.native_started_qpc(),&weaponhand::client,
        reinterpret_cast<boundaryabi::Revoke>(revoke_address),reinterpret_cast<boundaryabi::Open>(open_address),reinterpret_cast<boundaryabi::Close>(close_address)};
    return admission.connect(binding)?Reply::accepted:Reply::busy;
}
} // namespace weaponboundary
#include "weapon_boundary_transport.inl"
