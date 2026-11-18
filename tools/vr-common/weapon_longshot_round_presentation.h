#pragma once
#include "weapon_hand_participant.h"
#include "weapon_longshot_chamber_snap.h"

// One current-frame geometric result for the held round, its hand target and
// raw insertion measurement. The caller still proves live source, real ammo
// lease and open breech. Nothing here grants a lease or commits ammunition.
namespace gears_weapon_longshot_round_presentation {
namespace hand=gears_weapon_hand;
namespace geometry=gears_weapon_longshot_chamber_snap;
using P=gears_vr::Pose;using V=gears_vr::Vector;
struct Sample {
    hand::Frame source{};hand::Lease lease{};
    std::uint64_t item{},resource{},cycle{};
    P raw{},display{},grip{},root{},mount{};V port{};double scale{},units{},weight{};
    geometry::Alignment measurement{};
    bool inside{},valid{};
};
inline bool same_context(hand::Frame const& a,hand::Frame const& b) noexcept {
    auto ak=a.key,bk=b.key;ak.frame=bk.frame=0;
    return hand::same(ak,bk)&&hand::same(a.world,b.world)&&hand::same(a.controller,b.controller)&&
        hand::same(a.pawn,b.pawn)&&hand::same(a.mesh,b.mesh)&&hand::same(a.weapon,b.weapon)&&
        a.reference_generation==b.reference_generation&&a.stereo.session==b.stereo.session&&
        a.stereo.space_epoch==b.stereo.space_epoch&&a.controllers.input_epoch==b.controllers.input_epoch&&
        a.controllers.profile_generation==b.controllers.profile_generation;
}
inline bool same_pose(P const& a,P const& b) noexcept {
    const auto d=geometry::sub(a.position,b.position);
    const double dot=a.orientation.x*b.orientation.x+a.orientation.y*b.orientation.y+
        a.orientation.z*b.orientation.z+a.orientation.w*b.orientation.w;
    return std::hypot(std::hypot(d.x,d.y),d.z)<=1e-9&&std::abs(std::abs(dot)-1)<=1e-12;
}
class State {
    Sample sample_{};geometry::Correction correction_{};
    std::int64_t edge_ns_{};double edge_weight_{};bool latched_{};
    double weight(std::int64_t now) const noexcept {
        const double t=edge_ns_>0&&now>=edge_ns_?std::clamp(double(now-edge_ns_)/120000000.,0.,1.):0;
        return latched_?edge_weight_+(1-edge_weight_)*t:edge_weight_*(1-t);
    }
public:
    void reset() noexcept {sample_={};correction_={};edge_ns_=0;edge_weight_=0;latched_=false;}
    Sample const& sample() const noexcept {return sample_;}
    bool matches(hand::Frame const& f,hand::Lease const& lease,std::uint64_t item,
                 std::uint64_t resource,std::uint64_t cycle) const noexcept {
        return sample_.valid&&hand::same(sample_.source.key,f.key)&&same_context(sample_.source,f)&&
            sample_.source.controllers.sequence==f.controllers.sequence&&
            sample_.source.controllers.sync_ns==f.controllers.sync_ns&&
            sample_.source.controllers.received_ns==f.controllers.received_ns&&
            sample_.source.stereo.received_ns==f.stereo.received_ns&&sample_.source.now_ns==f.now_ns&&
            f.press_token==item&&f.left_tracked&&f.left_grip_known&&f.left_grip_down&&sample_.item==item&&
            sample_.resource==resource&&sample_.cycle==cycle&&hand::same(sample_.lease,lease);
    }
    bool prepare(hand::Frame const& f,hand::Lease const& lease,std::uint64_t item,
        std::uint64_t resource,std::uint64_t cycle,P const& raw,P const& mount,P const& root,
        V port,double scale,double units) noexcept {
        if(!item||!resource||!cycle||!f.key.frame||!f.controllers.sequence||f.controllers.sync_ns<=0||
           !f.left_tracked||!f.left_grip_known||!f.left_grip_down||
           lease.kind!=hand::LeaseKind::ammo||lease.id!=item||f.press_token!=item||!hand::lease_valid(lease,f.key.lifecycle)||
           !geometry::pose_valid(raw)||!geometry::pose_valid(mount)||!geometry::pose_valid(root)||
           !gears_vr::finite(port)||!std::isfinite(scale)||scale<=.1||scale>=10||
           !std::isfinite(units)||units<10||units>500){reset();return false;}
        if(matches(f,lease,item,resource,cycle)){
            // Repeated calls cannot advance the blend or replace this Frame's
            // raw measurement with a differently fitted display endpoint.
            if(!same_pose(sample_.raw,raw)||!same_pose(sample_.root,root)||!same_pose(sample_.mount,mount)||sample_.scale!=scale||
               sample_.units!=units||geometry::sub(sample_.port,port).x!=0||
               geometry::sub(sample_.port,port).y!=0||geometry::sub(sample_.port,port).z!=0){reset();return false;}
            return true;
        }
        const auto now=f.controllers.sync_ns;
        if(sample_.valid&&hand::same(sample_.source.key,f.key)&&
           sample_.source.controllers.sequence==f.controllers.sequence&&sample_.source.controllers.sync_ns==now){
            reset();return false; // Mixed witnesses cannot replace one already computed sample.
        }
        const bool owner=sample_.valid&&same_context(sample_.source,f)&&sample_.item==item&&
            sample_.resource==resource&&sample_.cycle==cycle&&hand::same(sample_.lease,lease);
        if(owner&&!same_pose(sample_.mount,mount)){reset();return false;}
        if(owner&&(f.key.frame<sample_.source.key.frame||f.controllers.sequence<sample_.source.controllers.sequence||now<sample_.source.controllers.sync_ns)){
            reset();return false;
        }
        if(!owner||now-sample_.source.controllers.sync_ns>100000000)reset();
        const double previous=weight(now);
        geometry::Correction next{};
        const bool zone_contact=geometry::make_correction(raw,root,port,scale,units,latched_,next);
        if(zone_contact!=latched_){edge_weight_=previous;edge_ns_=now;latched_=zone_contact;}
        if(zone_contact)correction_=next;
        const double display_weight=weight(now);
        geometry::Alignment measured{};
        // Outside the volume this is only a decaying visual correction. The
        // independent inside flag remains false, so fading cannot seat a round.
        if(!geometry::apply_correction(raw,root,port,scale,units,correction_,display_weight,measured)){
            reset();return false;
        }
        P grip{};if(!geometry::grip_for_item(measured.item,mount,grip)){reset();return false;}
        sample_={f,lease,item,resource,cycle,raw,measured.item,grip,root,mount,port,scale,units,display_weight,measured,zone_contact,true};
        return true;
    }
};
}
