#pragma once
#include "../vr-common/weapon_original_input_cleanup_context.h"

// Read-only classification for an already-journalled original input. This is
// deliberately independent of hand/input/pose/reference freshness and grants
// no Engine operation or re-admission. Actual ancestry is read in the adapter.
namespace gears_original_input_cleanup_policy {
namespace api=gears_weapon_original_input_cleanup_context;
namespace hand=gears_weapon_hand;
struct Runtime {
    std::uint32_t game_pid{},game_thread{},current_thread{},worker_thread{};
    std::uint64_t game_start{},native_started{},cookie{},world_generation{},revision{};
    bool available{},registered{},terminal{},unsafe{};
};
inline bool shape(api::Request const& request) noexcept {
    return request.size==sizeof(api::Request)&&request.version==1&&!request.reserved[0]&&!request.reserved[1]&&
        hand::equipment_identity_valid(request.parent)&&hand::identity_valid(request.parent.weapon);
}
inline api::Result classify(api::Request const& request,Runtime const& current) noexcept {
    if(!shape(request))return api::Result::rejected;
    if(!current.available||!current.registered||current.terminal||current.unsafe)return api::Result::unavailable;
    auto const& key=request.parent.key;
    if(!current.current_thread||!current.game_thread||current.current_thread!=current.game_thread||
       current.current_thread==current.worker_thread||current.game_thread!=key.thread_id||
       current.game_pid!=key.game_pid||current.game_start!=key.game_start||
       current.native_started!=key.native_started_qpc||current.cookie!=key.registration_cookie||
       !current.world_generation||current.world_generation!=key.world_generation||!current.revision)return api::Result::rejected;
    // Frame/lifecycle/reference are historical source fields. The live broker
    // revision is bracketed below, never compared to an old pose revocation.
    return api::Result::same_incarnation;
}
inline bool same(Runtime const& a,Runtime const& b) noexcept {
    return a.game_pid==b.game_pid&&a.game_thread==b.game_thread&&a.current_thread==b.current_thread&&
        a.worker_thread==b.worker_thread&&a.game_start==b.game_start&&a.native_started==b.native_started&&
        a.cookie==b.cookie&&a.world_generation==b.world_generation&&a.revision==b.revision&&
        a.available==b.available&&a.registered==b.registered&&a.terminal==b.terminal&&a.unsafe==b.unsafe;
}
}
