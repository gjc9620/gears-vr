#pragma once
#include "troika_mounted_geometry.h"

// Mounted grip ownership and R2 intent only. The ordinary Frame/binder and ordinary
// authorization remain separate. This receipt grants no aim/fire/bone writer.
namespace gears_troika_control {
namespace geometry=gears_troika_geometry;namespace hand=gears_weapon_hand;
constexpr std::uint32_t version=2;
struct Decision {
    std::uint32_t size{sizeof(Decision)},abi_version{version};
    std::uint64_t query_id{},publication_sequence{},source_ticket{},lifecycle{},press_token{};
    hand::Lease lease{};hand::DecisionStatus status{hand::DecisionStatus::needs_release};
    std::uint32_t new_grant{},release_required{1},left_down{},left_known{},left_tracked{},reserved{};
    std::int64_t decided_ns{};
    std::uint32_t right_known{},right_down{},right_owned{},trigger_known{},trigger_down{},both_owned{},fire_requested{},right_reserved{};
    std::uint64_t right_press_sequence{},fire_edge_sequence{};
    std::int64_t right_window_begin_ns{},fire_window_begin_ns{};
};
inline bool valid(Decision const& d,geometry::Request const& q,std::int64_t now) noexcept {
    if(d.size!=sizeof(d)||d.abi_version!=version||d.query_id!=q.query_id||!d.query_id||
       d.publication_sequence!=q.source.frame.publication_sequence||d.source_ticket!=q.source.frame.source_ticket||!d.lifecycle||
       d.reserved||d.new_grant>1||d.release_required>1||d.left_down>1||d.left_known>1||d.left_tracked>1||
       d.decided_ns<q.source.frame.published_ns||now<d.decided_ns||
       !geometry::source_valid(q.source,now,q.source.frame.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)||
       unsigned(d.status)>unsigned(hand::DecisionStatus::lease_lost)||!hand::lease_valid(d.lease,d.lifecycle)||
       (d.lease.kind!=hand::LeaseKind::none&&d.lease.kind!=hand::LeaseKind::support))return false;
    const bool held=d.lease.kind==hand::LeaseKind::support;
    if(d.right_reserved||d.right_known>1||d.right_down>1||d.right_owned>1||d.trigger_known>1||d.trigger_down>1||d.both_owned>1||d.fire_requested>1)return false;
    const auto edge_valid=[&](std::uint64_t sequence,std::int64_t begin){return sequence&&sequence<=q.source.frame.controllers.sequence&&
        begin>0&&begin<=q.source.frame.controllers.sync_ns;};
    if(d.right_down&&!d.right_known)return false;
    if(d.trigger_down&&!d.trigger_known)return false;
    if(d.right_owned&&(!d.right_known||!d.right_down||!edge_valid(d.right_press_sequence,d.right_window_begin_ns)))return false;
    auto const& right=q.source.frame.controllers.hands[1];
    if((d.right_known&&right.squeeze_active!=1)||(d.trigger_known&&right.trigger_active!=1))return false;
    if(d.right_owned&&(d.right_press_sequence!=right.last_squeeze_press_sequence||
       !gears_vr::controller_pose_usable(right.grip,right.grip_flags,right.grip_active)))return false;
    if(!d.right_owned&&(d.right_press_sequence||d.right_window_begin_ns))return false;
    if(bool(d.both_owned)!=(held&&bool(d.right_owned)))return false;
    if(d.fire_requested&&(!d.both_owned||!d.trigger_known||!d.trigger_down||!edge_valid(d.fire_edge_sequence,d.fire_window_begin_ns)))return false;
    if(!d.fire_requested&&(d.fire_edge_sequence||d.fire_window_begin_ns))return false;
    if(d.left_down&&(!d.left_known||!d.left_tracked))return false;
    if(held&&(!d.press_token||!d.left_down||!d.left_known||!d.left_tracked||d.release_required))return false;
    if(held!=(d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained))return false;
    return bool(d.new_grant)==(d.status==hand::DecisionStatus::granted);
}
enum class CommitReply:std::uint32_t {accepted,rejected};
using Commit=std::uint32_t(__cdecl*)(void*,geometry::Request const*,geometry::Response const*,Decision const*) noexcept;
struct Registration {
    std::uint32_t size{sizeof(Registration)},abi_version{version},decision_bytes{sizeof(Decision)},reserved{};
    geometry::Registration geometry{};Commit commit{};
};
inline bool shape(Registration const& r) noexcept {
    return r.size==sizeof(r)&&r.abi_version==version&&r.decision_bytes==sizeof(Decision)&&!r.reserved&&geometry::shape(r.geometry)&&r.commit;
}
inline bool same(Registration const& a,Registration const& b) noexcept {return geometry::same(a.geometry,b.geometry)&&a.commit==b.commit;}
using Attach=hand::Reply(__cdecl*)(Registration const*,hand::RegistrationResult*) noexcept;
constexpr char attach_export[]="GearsVrAttachTroikaMountedControlV2";
using Read=gears_rig_tracking::Reply(__cdecl*)(geometry::Source const*,Decision*) noexcept;
constexpr char read_export[]="GearsVrReadTroikaMountedControlV2";
static_assert(std::is_standard_layout_v<Decision>&&std::is_trivially_copyable_v<Decision>);
} // namespace gears_troika_control
