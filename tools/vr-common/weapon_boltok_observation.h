#pragma once
#include "weapon_boltok_original.h"
// Immutable observed geometry only. Current permission remains broker-owned.
namespace gears_boltok_observation {
namespace api=gears_boltok_original;namespace hand=gears_weapon_hand;
inline bool same_domain(hand::Frame const& a,hand::Frame const& b) noexcept {
    const auto& x=a.key;const auto& y=b.key;
    return x.game_pid==y.game_pid&&x.game_start==y.game_start&&x.native_started_qpc==y.native_started_qpc&&
        x.registration_cookie==y.registration_cookie&&x.lifecycle==y.lifecycle&&
        x.world_generation==y.world_generation&&x.thread_id==y.thread_id&&
        hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&hand::same(a.pawn,b.pawn)&&
        hand::same(a.mesh,b.mesh)&&hand::same(a.weapon,b.weapon)&&a.reference_generation==b.reference_generation&&
        a.stereo.session==b.stereo.session&&a.stereo.space_epoch==b.stereo.space_epoch&&
        a.controllers.input_epoch==b.controllers.input_epoch&&a.controllers.profile_generation==b.controllers.profile_generation;
}
inline bool request_shape(api::ObservationRequest const& q) noexcept {
    return q.size==sizeof q&&q.version==1&&q.handle&&q.cookie&&q.source.key.frame&&
        q.cookie==q.source.key.registration_cookie&&hand::identity_valid(q.source.world)&&
        hand::identity_valid(q.source.pawn)&&hand::identity_valid(q.source.mesh)&&
        hand::identity_valid(q.source.weapon)&&hand::identity_valid(q.component)&&hand::identity_valid(q.asset);
}
inline bool matrices_valid(api::Observation const& o) noexcept {
    if(!api::math::proper(o.root)||!api::math::proper(o.support))return false;
    if(!o.hidden)return api::math::proper(o.cylinder);
    for(auto v:o.cylinder)if(!std::isfinite(v))return false;
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)if(std::abs(o.cylinder[r*4+c])>1e-6)return false;
    return std::abs(o.cylinder[15]-1)<=1e-5;
}
inline bool metadata_same(api::Source const& a,api::Source const& b) noexcept {
    return api::part::identity(a,b)&&a.epoch==b.epoch&&api::part::same(a.local,b.local)&&api::part::same(a.space,b.space);
}
inline bool accepted(api::ObservationRequest const& q,hand::Frame const& bound_source,
    hand::ObjectIdentity const& component,hand::ObjectIdentity const& asset,
    api::Source const& saved,api::Source const& current,api::Observation const& o,std::int64_t now) noexcept {
    return request_shape(q)&&o.size==sizeof o&&o.version==1&&o.verified&&o.published&&!o.restoring&&
        o.handle==q.handle&&o.publish_serial&&o.epoch&&o.source_epoch==o.epoch&&current.epoch==o.epoch&&
        current.identity_generation==o.handle&&o.observed_ns>0&&now>=o.observed_ns&&now-o.observed_ns<=100000000&&
        same_domain(q.source,bound_source)&&same_domain(o.source,bound_source)&&q.source.key.frame>=o.source.key.frame&&
        hand::same(q.component,component)&&hand::same(q.asset,asset)&&hand::same(o.component,component)&&hand::same(o.asset,asset)&&
        hand::identity_valid(o.node)&&o.node.address==current.node&&metadata_same(saved,current)&&matrices_valid(o);
}
}
