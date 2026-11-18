#pragma once
#include "weapon_boomshot_types.h"
#include "pose_math.h"
#include <algorithm>
#include <cmath>

namespace gears_weapon::boomshot_insertion {
// No Lancer anchor, drum orientation or insertion depth is supplied here.
// The original game seats by hiding its separate component and showing the
// embedded mesh. The live owner must authenticate the original socket/assets
// and bind a separately reviewed authored carrier fit. An offline best-fit
// rotation alone cannot set verified=true; this is not headset acceptance.
struct Geometry {
    std::uint64_t id{};gears_vr::Pose seated_in_root_m{};gears_vr::Vector outward{};
    double travel_m{},approach_margin_m{},capture_radius_m{},retain_radius_m{},
        capture_angle_degrees{},retain_angle_degrees{},seat_tolerance_m{};
    bool verified{},magnetic_snap{};
};
struct Sample {
    BoomshotStamp stamp{};gears_vr::Pose raw_carrier_in_root_m{};
    std::uint64_t frame{},lease{},press{},item{},operation{},geometry_id{};
    bool grip_held{},new_grant{},left_tracked{},right_tracked{},lease_current{};
};
enum class Result {rejected,approaching,moved,waiting,complete,interrupted};
// Only the actual stroke may mint this receipt. Begin/end remain the ORIGINAL
// raw sample identity/time, independently mapped to the owner's typed events.
class Completion {
    friend class Stroke;Sample begin_{},end_{};bool known_{};
public:
    bool valid()const noexcept{return known_;}
    Sample const& began()const noexcept{return begin_;}
    Sample const& ended()const noexcept{return end_;}
};
inline bool unit(gears_vr::Quaternion q) noexcept {
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;return std::isfinite(n)&&std::abs(n-1)<1e-5;
}
inline bool valid(Geometry const& g) noexcept {
    const double n=g.outward.x*g.outward.x+g.outward.y*g.outward.y+g.outward.z*g.outward.z;
    return g.verified&&g.id&&gears_vr::finite(g.seated_in_root_m.position)&&unit(g.seated_in_root_m.orientation)&&
        std::isfinite(n)&&std::abs(n-1)<1e-5&&
        std::isfinite(g.travel_m)&&g.travel_m>.005&&g.travel_m<.5&&
        std::isfinite(g.approach_margin_m)&&g.approach_margin_m>0&&g.approach_margin_m<=g.travel_m*.2&&
        std::isfinite(g.capture_radius_m)&&g.capture_radius_m>0&&g.capture_radius_m<.05&&
        std::isfinite(g.retain_radius_m)&&g.retain_radius_m>=g.capture_radius_m&&g.retain_radius_m<.08&&
        std::isfinite(g.capture_angle_degrees)&&g.capture_angle_degrees>0&&g.capture_angle_degrees<45&&
        std::isfinite(g.retain_angle_degrees)&&g.retain_angle_degrees>=g.capture_angle_degrees&&g.retain_angle_degrees<60&&
        std::isfinite(g.seat_tolerance_m)&&g.seat_tolerance_m>0&&g.seat_tolerance_m<=g.travel_m*(g.magnetic_snap?.2:.05);
}
class Stroke {
    Geometry geometry_{};Sample source_{},begin_{};bool active_{},outside_{},entered_{};
    Completion completion_{};
    double entry_depth_{},last_depth_{};std::uint64_t reached_frame_{};
    BoomshotIdentity last_identity_{};std::uint64_t last_press_{};
    static bool usable(Sample const& s,std::int64_t now) noexcept {
        return s.stamp.valid()&&now>=s.stamp.monotonic_ns&&now-s.stamp.monotonic_ns<=100000000&&
            s.frame&&s.lease&&s.press&&s.item&&s.operation&&s.geometry_id&&s.grip_held&&s.left_tracked&&s.right_tracked&&s.lease_current&&
            gears_vr::finite(s.raw_carrier_in_root_m.position)&&unit(s.raw_carrier_in_root_m.orientation);
    }
    static bool same(Sample const& a,Sample const& b) noexcept {
        return a.stamp.identity==b.stamp.identity&&a.stamp.binding==b.stamp.binding&&a.stamp.motion_epoch==b.stamp.motion_epoch&&
            a.lease==b.lease&&a.press==b.press&&a.item==b.item&&a.operation==b.operation;
    }
    void geometry(Sample const& s,double& depth,double& lateral,double& angle) const noexcept {
        const auto p=s.raw_carrier_in_root_m.position,t=geometry_.seated_in_root_m.position,n=geometry_.outward;
        const gears_vr::Vector d{p.x-t.x,p.y-t.y,p.z-t.z};depth=d.x*n.x+d.y*n.y+d.z*n.z;
        lateral=std::hypot(std::hypot(d.x-depth*n.x,d.y-depth*n.y),d.z-depth*n.z);
        const auto a=s.raw_carrier_in_root_m.orientation,b=geometry_.seated_in_root_m.orientation;
        angle=2*std::acos((std::clamp)(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w),0.,1.))*180/3.14159265358979323846;
    }
public:
    bool begin(Geometry const& g,Sample const& s,std::int64_t now) noexcept {
        if(active_||!valid(g)||!usable(s,now)||!s.new_grant||s.geometry_id!=g.id||
           (s.stamp.identity==last_identity_&&s.press<=last_press_))return false;
        geometry_=g;source_=begin_=s;active_=true;entered_=false;reached_frame_=0;completion_={};
        last_identity_=s.stamp.identity;last_press_=s.press;
        double lateral{},angle{};geometry(s,last_depth_,lateral,angle);
        outside_=last_depth_>=g.travel_m+g.approach_margin_m;return true;
    }
    bool resume(Geometry const& g,Sample const& s,std::int64_t now) noexcept {
        if(active_||!g.magnetic_snap||!valid(g)||!usable(s,now)||s.new_grant||g.id!=geometry_.id||
           !same(source_,s)||s.frame<=source_.frame||s.stamp.sequence<=source_.stamp.sequence||
           s.stamp.monotonic_ns<=source_.stamp.monotonic_ns)return false;
        double depth{},lateral{},angle{};geometry(s,depth,lateral,angle);
        // A held item may retry alignment, but must start outside again. No
        // completion, press or inventory receipt survives the interrupted path.
        if(depth<g.travel_m+g.approach_margin_m)return false;
        source_=begin_=s;active_=outside_=true;entered_=false;reached_frame_=0;completion_={};last_depth_=depth;return true;
    }
    Result observe(Sample const& s,std::int64_t now) noexcept {
        if(!active_)return Result::rejected;
        if(!usable(s,now)||!same(source_,s)||s.geometry_id!=geometry_.id||s.stamp.monotonic_ns<source_.stamp.monotonic_ns||
           s.stamp.monotonic_ns-source_.stamp.monotonic_ns>(geometry_.magnetic_snap?250000000:100000000)||s.stamp.game_tick<source_.stamp.game_tick){
            release();return Result::interrupted;}
        if(s.stamp.sequence<=source_.stamp.sequence||s.frame<=source_.frame)return Result::waiting;
        const auto a=s.raw_carrier_in_root_m.position,b=source_.raw_carrier_in_root_m.position;
        const double dt=double(s.stamp.monotonic_ns-source_.stamp.monotonic_ns)/1e9;
        if(std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z)>2*dt+.002){release();return Result::interrupted;}
        double depth{},lateral{},angle{};geometry(s,depth,lateral,angle);source_=s;
        if(!entered_){
            outside_=outside_||depth>=geometry_.travel_m+geometry_.approach_margin_m;
            if(!outside_||depth<geometry_.travel_m-geometry_.approach_margin_m||
               depth>geometry_.travel_m+geometry_.approach_margin_m||lateral>geometry_.capture_radius_m||angle>geometry_.capture_angle_degrees){
                last_depth_=depth;return Result::approaching;}
            entered_=true;entry_depth_=last_depth_=depth;begin_=s;return Result::moved;
        }
        if(lateral>geometry_.retain_radius_m||angle>geometry_.retain_angle_degrees||
           depth>geometry_.travel_m+geometry_.approach_margin_m||depth< -geometry_.seat_tolerance_m){
            release();return Result::interrupted;}
        const double required=geometry_.travel_m-geometry_.approach_margin_m-geometry_.seat_tolerance_m;
        const bool seated=std::abs(depth)<=geometry_.seat_tolerance_m&&entry_depth_-depth+1e-9>=required&&
            lateral<=geometry_.capture_radius_m&&angle<=geometry_.capture_angle_degrees;
        if(seated){
            if(reached_frame_&&s.frame>reached_frame_){active_=false;completion_.begin_=begin_;completion_.end_=s;completion_.known_=true;return Result::complete;}
            reached_frame_=s.frame;
        }else reached_frame_=0;
        const bool moved=std::abs(depth-last_depth_)>1e-5;last_depth_=depth;
        return moved?Result::moved:Result::waiting;
    }
    void release() noexcept {active_=outside_=entered_=false;reached_frame_=0;completion_={};}
    bool take_completion(Completion& out) noexcept {out=completion_;completion_={};return out.valid();}
    bool active() const noexcept{return active_;}
    Sample const& began() const noexcept{return begin_;}
};
}
