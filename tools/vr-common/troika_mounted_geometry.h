#pragma once
#include "troika_mounted_source.h"
#include <cstring>

// Supplemental geometry only. Uses the existing hand participant's context
// and module pin. No normal hand Frame, lease, pose writer or action grant.
namespace gears_troika_geometry {
namespace hand=gears_weapon_hand;
constexpr std::uint32_t version=3;
struct Source {gears_troika_source::Frame frame{};std::uint64_t entry_binding{},entry_token{};};
struct Request {
    std::uint32_t size{sizeof(Request)},abi_version{version};
    std::uint64_t query_id{};Source source{};
};
struct Response {
    std::uint32_t size{sizeof(Response)},abi_version{version};
    std::uint64_t query_id{},publication_sequence{},source_ticket{},entry_binding{},entry_token{};
    hand::ObjectIdentity component{},asset{};
    // Bit 0 = original handle_L shaft (+mesh X); bit 1 = handle_R.
    // Physical hand assignment comes from the ACTUAL turret instance fields,
    // never from an assumed mesh-X meaning of left/right.
    // Each raw physical palm is measured against BOTH shafts. No snapped skin.
    std::uint32_t tracked_mask{},contacts[2]{};std::uint32_t reserved{};
    double distances[2][2]{{10,10},{10,10}};
    std::uint32_t press_tracked_mask{},press_contacts[2]{},press_reserved{};
    double press_distances[2][2]{{10,10},{10,10}};
    std::uint32_t hand_handle[2]{0,1};
};
enum class Status:std::uint32_t {ready,unavailable,source_mismatch,geometry_unavailable};
using Prepare=std::uint32_t(__cdecl*)(void*,Request const*,Response*) noexcept;
struct Registration {
    std::uint32_t size{sizeof(Registration)},abi_version{version};
    std::uint32_t request_bytes{sizeof(Request)},response_bytes{sizeof(Response)};
    std::uint64_t hand_cookie{},native_started_qpc{};void* context{};Prepare prepare{};
};
inline bool shape(Registration const& r) noexcept {
    return r.size==sizeof(r)&&r.abi_version==version&&r.request_bytes==sizeof(Request)&&r.response_bytes==sizeof(Response)&&
        r.hand_cookie&&r.native_started_qpc&&r.context&&r.prepare;
}
inline bool same(Registration const& a,Registration const& b) noexcept {
    return a.hand_cookie==b.hand_cookie&&a.native_started_qpc==b.native_started_qpc&&a.context==b.context&&a.prepare==b.prepare;
}
inline bool source_valid(Source const& s,std::int64_t now,bool sim) noexcept {
    return s.entry_binding&&s.entry_token&&gears_troika_source::frame_valid(s.frame,now,100000000,sim);
}
inline bool valid(Response const& r,Request const& q) noexcept {
    if(r.size!=sizeof(r)||r.abi_version!=version||r.reserved||!q.query_id||r.query_id!=q.query_id||
       r.publication_sequence!=q.source.frame.publication_sequence||r.source_ticket!=q.source.frame.source_ticket||
       r.entry_binding!=q.source.entry_binding||r.entry_token!=q.source.entry_token||r.tracked_mask>3||
       !hand::same(r.component,q.source.frame.turret_mesh)||!hand::identity_valid(r.asset)||
       r.hand_handle[0]>1||r.hand_handle[1]>1||r.hand_handle[0]==r.hand_handle[1])return false;
    unsigned expected_tracking{};
    for(unsigned i=0;i<2;++i){auto const& h=q.source.frame.controllers.hands[i];
        if(gears_vr::controller_pose_usable(h.grip,h.grip_flags,h.grip_active))expected_tracking|=1u<<i;}
    unsigned expected_press{};
    for(unsigned i=0;i<2;++i){auto const& h=q.source.frame.controllers.hands[i];
        if(h.last_squeeze_press_sequence&&h.last_squeeze_press_sequence<=q.source.frame.controllers.sequence&&
           gears_vr::controller_pose_usable(h.last_squeeze_press_grip,h.last_squeeze_press_flags,1))expected_press|=1u<<i;}
    if(r.tracked_mask!=expected_tracking||r.press_tracked_mask!=expected_press||r.press_reserved)return false;
    for(unsigned side=0;side<2;++side){
        if(r.contacts[side]>3||(!(r.tracked_mask&(1u<<side))&&r.contacts[side]))return false;
        for(unsigned handle=0;handle<2;++handle){const auto d=r.distances[side][handle];
            if(!std::isfinite(d)||d<0||d>10||bool(r.contacts[side]&(1u<<handle))!=bool((r.tracked_mask&(1u<<side))&&d<=.012))return false;}
        if(r.press_contacts[side]>3||(!(r.press_tracked_mask&(1u<<side))&&r.press_contacts[side]))return false;
        for(unsigned handle=0;handle<2;++handle){const auto d=r.press_distances[side][handle];
            if(!std::isfinite(d)||d<0||d>10||bool(r.press_contacts[side]&(1u<<handle))!=bool((r.press_tracked_mask&(1u<<side))&&d<=.012))return false;}
    }
    return true;
}
inline bool empty(Response const& r) noexcept {
    if(r.size!=sizeof(r)||r.abi_version!=version||r.query_id||r.publication_sequence||r.source_ticket||r.entry_binding||r.entry_token||
       !hand::same(r.component,hand::ObjectIdentity{})||!hand::same(r.asset,hand::ObjectIdentity{})||r.tracked_mask||r.contacts[0]||r.contacts[1]||r.reserved||
       r.press_tracked_mask||r.press_contacts[0]||r.press_contacts[1]||r.press_reserved||r.hand_handle[0]||r.hand_handle[1]!=1)return false;
    for(auto const& side:r.distances)for(double d:side)if(d!=10)return false;
    for(auto const& side:r.press_distances)for(double d:side)if(d!=10)return false;
    return true;
}
using Attach=hand::Reply(__cdecl*)(Registration const*,hand::RegistrationResult*) noexcept;
constexpr char attach_export[]="GearsVrAttachTroikaMountedGeometryV3";
static_assert(std::is_standard_layout_v<Request>&&std::is_trivially_copyable_v<Request>);
static_assert(std::is_standard_layout_v<Response>&&std::is_trivially_copyable_v<Response>);
} // namespace gears_troika_geometry
