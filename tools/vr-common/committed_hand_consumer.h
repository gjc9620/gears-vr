#pragma once
#include "committed_hand_endpoint.h"

// Notification from the actual Native C0 boundary to the EXISTING weapon
// component writer. It is not an input frame or an equipment/fire permission.
namespace gears_committed_hand_consumer {
inline constexpr unsigned version=3;
enum class Kind:unsigned {revoke,committed};
enum class Result:unsigned {unchanged,applied,fault};
struct Notice {
    unsigned size{sizeof(Notice)},abi_version{version};
    Kind kind{Kind::revoke};unsigned reserved{};
    gears_committed_hand::Frame frame{};
};
#if defined(_WIN32)
#define GEARS_HAND_RESULT_CALL __cdecl
#else
#define GEARS_HAND_RESULT_CALL
#endif
using Notify=Result(GEARS_HAND_RESULT_CALL*)(void*,Notice const*) noexcept;
struct Registration {
    unsigned size{sizeof(Registration)},abi_version{version},notice_bytes{sizeof(Notice)},reserved{};
    std::uint64_t hand_cookie{},native_started_qpc{};
    void* context{};Notify notify{};
};
inline bool shape(Registration const& r) noexcept {
    return r.size==sizeof r&&r.abi_version==version&&r.notice_bytes==sizeof(Notice)&&!r.reserved&&
        r.hand_cookie&&r.native_started_qpc&&r.context&&r.notify;
}
inline bool same(Registration const& a,Registration const& b) noexcept {
    return shape(a)&&shape(b)&&a.hand_cookie==b.hand_cookie&&a.native_started_qpc==b.native_started_qpc&&
        a.context==b.context&&a.notify==b.notify;
}
using Attach=gears_weapon_hand::Reply(GEARS_HAND_RESULT_CALL*)(Registration const*,gears_weapon_hand::RegistrationResult*) noexcept;
inline constexpr char attach_export[]="GearsVrAttachCommittedHandConsumerV3";
#undef GEARS_HAND_RESULT_CALL
static_assert(std::is_trivially_copyable_v<Notice>&&std::is_standard_layout_v<Notice>);
static_assert(std::is_trivially_copyable_v<Registration>&&sizeof(Registration)==48);
}
