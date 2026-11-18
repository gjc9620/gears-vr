#pragma once
#include "weapon_reload_geometry.h"
#include "weapon_torque_bow_arrow_mount.h"
#include <array>
namespace gears_weapon {
// Explicit instance/asset calibration. A caller must supply the original
// arrow and slot transforms; there is no guessed default Bow feed point.
struct TorqueBowFeedCalibration {
    std::uint64_t instance_serial{};
    std::array<std::uint8_t,32> bow_asset{},arrow_asset{};
    FeedPortMount mount{};
    bool original_arrow_verified{},original_slot_verified{};
    bool valid() const noexcept {
        bool bow=false,arrow=false;for(auto x:bow_asset)bow=bow||x;for(auto x:arrow_asset)arrow=arrow||x;
        return instance_serial&&bow&&arrow&&original_arrow_verified&&original_slot_verified&&mount.valid();
    }
};
struct TorqueBowFeedSnap {
    Pose item{};double raw_depth{},raw_lateral{},raw_angle{};
};
// Original front loading: the forked +Y tail enters the front bore first;
// the -Y arrow head keeps pointing +BowRootX while the arrow moves -X.
// Original mesh RotOrigin yaw-16384 cancels the root's reference yaw+16384.
// These are Root coordinates, never raw mesh vertices or a C0/snapped hand.
inline constexpr Pose torque_bow_seated_source_raw{{57.69151886065814,-.00934543869430371,8.773020371814567},
    {.7077178535459494,-.7064950140087274,-.0004810838713174405,.00005925732776247398}};
inline constexpr Vector torque_bow_muzzle_root_raw{58.00706100463867,.10888981819152832,8.750000953674316};
inline constexpr Vector torque_bow_tail_raw{0,18.724075317382812,-.5764920115470886};
constexpr std::array<std::uint8_t,32> torque_bow_digest(char const* text) noexcept {
    std::array<std::uint8_t,32> out{};
    for(unsigned i=0;i<32;++i){const auto digit=[](char c)constexpr{return unsigned(c>='0'&&c<='9'?c-'0':c-'a'+10);};
        out[i]=std::uint8_t(digit(text[i*2])*16+digit(text[i*2+1]));}return out;
}
inline bool author_torque_bow_feed(std::uint64_t serial,double bow_scale,
    gears_torque_bow_arrow_mount::Mount const& arrow,TorqueBowFeedCalibration& out) noexcept {
    namespace a=gears_torque_bow_arrow_mount;out={};
    const auto units=arrow.units_per_metre;
    if(!serial||!a::pose_valid(arrow.tip_in_grip_units)||!a::pose_valid(arrow.source_in_grip_units)||
       !std::isfinite(bow_scale)||bow_scale<=.1||bow_scale>=10||!std::isfinite(units)||units<10||units>500||
       !std::isfinite(arrow.component_scale)||arrow.component_scale<=.1||arrow.component_scale>=10||
       std::abs(bow_scale-arrow.component_scale)>1e-5*bow_scale)return false;
    const auto expected_tip=sum(arrow.source_in_grip_units.position,
        rotate(arrow.source_in_grip_units.orientation,scaled(a::tip_raw,arrow.component_scale)));
    const auto& q=arrow.source_in_grip_units.orientation;const auto& t=arrow.tip_in_grip_units.orientation;
    if(length(difference(expected_tip,arrow.tip_in_grip_units.position))>1e-8||
       std::abs(std::abs(q.x*t.x+q.y*t.y+q.z*t.z+q.w*t.w)-1)>1e-10||
       !gears_vr::finite(arrow.raw_tip_in_grip_m)||
       length(difference(scaled(arrow.tip_in_grip_units.position,1/units),arrow.raw_tip_in_grip_m))>1e-10)return false;
    TorqueBowFeedCalibration c{};c.instance_serial=serial;
    c.bow_asset=torque_bow_digest("4769f3da359b895b8ea5d47947958da707bbfeef1801ed93ccd9da2d0de702db");
    c.arrow_asset=torque_bow_digest("c06788de423217fd5b2a89fe64606271699e7aaa29e7534cd407e682f0c98a16");
    const auto& seat=torque_bow_seated_source_raw;
    const auto head=sum(scaled(seat.position,bow_scale/units),rotate(seat.orientation,scaled(a::tip_raw,arrow.component_scale/units)));
    const auto tail=sum(scaled(seat.position,bow_scale/units),rotate(seat.orientation,scaled(torque_bow_tail_raw,arrow.component_scale/units)));
    const double travel=torque_bow_muzzle_root_raw.x*bow_scale/units-tail.x;
    if(!std::isfinite(travel)||travel<=.02||travel>=.196)return false; // never shorten an unsupported original stroke
    c.mount.port_in_weapon={head,seat.orientation};
    c.mount.item_in_left_grip={scaled(arrow.tip_in_grip_units.position,1/units),arrow.tip_in_grip_units.orientation};
    c.mount.outward_axis_in_weapon={1,0,0};c.mount.seated_depth_metres=.003;
    c.mount.approach_depth_metres=travel+c.mount.seated_depth_metres; // minimum admitted travel remains the whole original stroke
    c.mount.lateral_tolerance_metres=.006;c.mount.maximum_orientation_error_radians=.13962634015954636;
    c.original_arrow_verified=c.original_slot_verified=true;
    if(!c.valid())return false;out=c;return true;
}
// Cosmetic assistance removes only small lateral/angular error. It does not
// advance the arrow along the groove and cannot submit an insertion receipt.
inline bool torque_bow_feed_snap(FeedSample const& raw,TorqueBowFeedCalibration const& c,
    bool keeping,bool another_object_owns_left,TorqueBowFeedSnap& out) noexcept {
    if(!c.valid()||another_object_owns_left||!raw.tracked||!raw.item_held||
       !gears_vr::stereo_pose_valid(raw.weapon_pose)||!gears_vr::stereo_pose_valid(raw.left_grip))return false;
    const auto& m=c.mount;
    const auto port=sum(raw.weapon_pose.position,rotate(raw.weapon_pose.orientation,m.port_in_weapon.position));
    const auto axis=rotate(raw.weapon_pose.orientation,m.outward_axis_in_weapon);
    const auto item=sum(raw.left_grip.position,rotate(raw.left_grip.orientation,m.item_in_left_grip.position));
    const auto q=gears_vr::normalized(gears_vr::multiply(raw.left_grip.orientation,m.item_in_left_grip.orientation));
    const auto desired=gears_vr::normalized(gears_vr::multiply(raw.weapon_pose.orientation,m.port_in_weapon.orientation));
    const auto delta=difference(item,port);const double depth=dot(delta,axis);
    const double lateral=length(difference(delta,scaled(axis,depth)));
    const double cosine=std::clamp(std::abs(q.x*desired.x+q.y*desired.y+q.z*desired.z+q.w*desired.w),0.,1.);
    const double angle=2*std::acos(cosine);
    const double radius=keeping?.035:.025,maximum_angle=keeping?.4363323129985824:.3141592653589793;
    if(!std::isfinite(depth)||!std::isfinite(lateral)||!std::isfinite(angle)||lateral>radius||
       angle>maximum_angle||depth<-.01||depth>m.approach_depth_metres+.03)return false;
    out={{sum(port,scaled(axis,depth)),desired},depth,lateral,angle};return true;
}
class TorqueBowArrowFeed {
    FeedInsertionStroke raw_stroke_;TorqueBowFeedCalibration calibration_{};
public:
    bool arm(std::uint64_t weapon,std::uint64_t item,std::uint64_t world,std::uint64_t space,
        TorqueBowFeedCalibration const& c) noexcept {
        calibration_={};if(!c.valid())return false;
        if(!raw_stroke_.arm(weapon,item,world,space))return false;calibration_=c;return true;
    }
    // FeedSample MUST be built from the authenticated unsnapped controller
    // chain. No cosmetic palm, C0 bone or FeedSnap output enters this method.
    FeedResult observe_raw(FeedSample const& raw,TorqueBowFeedCalibration const& c) noexcept {
        if(!c.valid()||!calibration_.instance_serial||calibration_.instance_serial!=c.instance_serial||
           calibration_.bow_asset!=c.bow_asset||calibration_.arrow_asset!=c.arrow_asset){reset();return FeedResult::rejected;}
        // An in-flight arm keeps its original calibration. A caller cannot
        // widen the port/tolerance or shorten travel by mutating a live table.
        return raw_stroke_.observe(raw,calibration_.mount);
    }
    void reset() noexcept {calibration_={};raw_stroke_.reset();}
};
}
