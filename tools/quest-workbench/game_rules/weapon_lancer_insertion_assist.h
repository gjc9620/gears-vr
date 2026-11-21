#pragma once
#include "weapon_lancer_insertion.h"
#include "rig_tracking_math.h"

namespace gears_weapon {
// Local guide for an already-held Lancer magazine. No controller, skeleton,
// lease, inventory or gesture ownership is written here. Callers still
// authenticate the actual hand pair. Preserve the unsnapped motion witness;
// a real approach can engage smooth final seating WITHOUT a button release.
class LancerMagazineInsertionAssist {
    LancerInsertionSample last_{};
    bool captured_{};
    std::uint64_t began_ms_{};
    bool approach_known_{};
    double approach_max_{};
    std::uint64_t snap_began_ms_{};
    gears_vr::Vector snap_offset_metres_{};
    gears_vr::Quaternion snap_rotation_{};
    static double ease(double x) noexcept {x=std::clamp(x,0.,1.);return x*x*(3.-2.*x);}
    static bool same(LancerInsertionSample const& a,LancerInsertionSample const& b) noexcept {
        return a.weapon==b.weapon&&a.component==b.component&&a.world==b.world&&a.session==b.session&&
            a.space_epoch==b.space_epoch&&a.input_epoch==b.input_epoch&&a.world_generation==b.world_generation&&
            a.units_per_metre==b.units_per_metre;
    }
public:
    void reset() noexcept {*this={};}
    bool captured() const noexcept {return captured_;}
    bool apply(LancerInsertionSample& s) noexcept {
        s.assisted=false;s.snap_active=false;s.snap_progress=0;s.unassisted_magazine=s.magazine;
        if(!s.tracked||!s.grip_down||!s.weapon||!s.component||!s.world||!s.session||
           !s.space_epoch||!s.input_epoch||!s.world_generation||!s.sequence||!s.created_ms||
           !std::isfinite(s.units_per_metre)||s.units_per_metre<=0){reset();return false;}
        if(last_.sequence&&(!same(s,last_)||s.sequence<last_.sequence||s.created_ms<last_.created_ms||
           s.created_ms-last_.created_ms>lancer_gesture_observation_interval_ms))reset();
        LancerInsertionGeometry g{};LancerFeedWorld feed{};
        if(!lancer_insertion_geometry(s,g)||!lancer_feed_world(s.gun_mesh,feed)){reset();return false;}
        // Ideal authored seated axes: magazine X=gun Y, Y=gun X, Z=-gun Z.
        gears_vr::Matrix desired{};
        for(unsigned i=0;i<3;++i){
            const auto norm=[&](unsigned row){return std::hypot(std::hypot(double(s.gun_mesh[row]),
                double(s.gun_mesh[row+1])),double(s.gun_mesh[row+2]));};
            desired[i][0]=s.gun_mesh[4+i]/norm(4);desired[i][1]=s.gun_mesh[i]/norm(0);
            desired[i][2]=-s.gun_mesh[8+i]/norm(8);
        }
        gears_vr::Quaternion from{},to{};
        if(!gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(s.magazine.rotation),from)||
           !gears_vr::rig_tracking::detail::quaternion(desired,to)){reset();return false;}
        double dot=from.x*to.x+from.y*to.y+from.z*to.z+from.w*to.w;
        if(dot<0){to={-to.x,-to.y,-to.z,-to.w};dot=-dot;}
        const double angle=2.*std::acos(std::clamp(dot,0.,1.)),u=s.units_per_metre;
        const double depth=g.depth_raw/u,lateral=g.lateral_raw/u;
        constexpr double radians=3.14159265358979323846/180.;
        const bool enter=depth>=lancer_contact::snap_near&&depth<=lancer_contact::snap_far&&
            lateral<=lancer_contact::snap_radius&&angle<=lancer_contact::snap_angle;
        const bool remain=depth>=lancer_contact::retain_near&&depth<=lancer_contact::retain_far&&
            lateral<=lancer_contact::retain_radius&&angle<=lancer_contact::retain_angle;
        if(!captured_&&enter){captured_=true;began_ms_=s.created_ms;approach_known_=false;}
        if(captured_&&!remain){reset();last_=s;return true;}
        last_=s;
        if(!captured_)return true;
        if(enter){
            // A normal shoulder-to-port reach can approach diagonally. Keep
            // its real distance-to-port witness instead of demanding a second
            // straight axial stroke after entering the acquisition volume.
            const double distance=std::hypot(depth,lateral);
            approach_max_=approach_known_?(std::max)(approach_max_,distance):distance;
            approach_known_=true;
        }else{approach_known_=false;snap_began_ms_=0;}
        // Align before the4.5cm approach witness, not only after passing it.
        // Otherwise an ordinary off-centre hand reaches the seat before the
        // unchanged insertion stroke ever observes an aligned approach.
        // Begin alignment before a normal quick hand crosses the existing
        //4.5cm approach gate. The previous10cm capture/120ms ramp was late.
        const double gain=ease((.20-depth)/.06)*ease((.18-lateral)/.06)*
            ease((110.*radians-angle)/(25.*radians))*ease(double(s.created_ms-began_ms_)/120.);
        const auto off=difference(s.magazine.position,feed.point_raw);
        const auto side=difference(off,scaled(feed.outward_axis,g.depth_raw));
        const double amount=gain*(std::min)(g.lateral_raw,lancer_insertion_assist_metres*u);
        if(g.lateral_raw>1e-8)s.magazine.position=difference(s.magazine.position,scaled(side,amount/g.lateral_raw));
        // Broad but bounded85degree alignment. The hand follows the actual
        // magazine through the existing one C0 writer, not a controller snap.
        const double fraction=angle>1e-8?gain*(std::min)(1.,lancer_insertion_assist_radians/angle):0.;
        const double half=angle*.5,denom=std::sin(half);
        if(fraction>0&&denom>1e-8){
            const double a=std::sin((1.-fraction)*half)/denom,b=std::sin(fraction*half)/denom;
            const gears_vr::Quaternion q{a*from.x+b*to.x,a*from.y+b*to.y,a*from.z+b*to.z,a*from.w+b*to.w};
            s.magazine.rotation=gears_vr::ue_angles(gears_vr::rotation_matrix(q));
        }
        s.assisted=gain>0;
        // Enter the final6cm only after at least2cm of real continuous approach
        // approach by THIS held item. A stationary overlap cannot start a seat.
        // Store the interpolation in the moving gun/port frame, so ordinary
        // gun motion cannot leave the hand/magazine aiming at an old world point.
        const bool snap=enter&&approach_known_&&depth<=lancer_magwell_snap_metres&&
            approach_max_-std::hypot(depth,lateral)>=lancer_magwell_approach_metres-.000001;
        if(!snap){snap_began_ms_=0;return true;}
        if(!snap_began_ms_){
            gears_vr::Quaternion start{};
            if(!gears_vr::rig_tracking::detail::quaternion(gears_vr::ue_matrix(s.magazine.rotation),start))return false;
            const auto delta=difference(s.magazine.position,feed.point_raw);
            snap_offset_metres_=scaled(gears_vr::transform(gears_vr::transpose(desired),delta),1/u);
            snap_rotation_=gears_vr::normalized(gears_vr::multiply(gears_vr::inverse(to),start));
            if(snap_rotation_.w<0)snap_rotation_={-snap_rotation_.x,-snap_rotation_.y,-snap_rotation_.z,-snap_rotation_.w};
            snap_began_ms_=s.created_ms;
        }
        const double seated=ease(double(s.created_ms-snap_began_ms_)/lancer_magwell_snap_ms);
        const auto offset=gears_vr::transform(desired,scaled(snap_offset_metres_,u*(1.-seated)));
        s.magazine.position={feed.point_raw.x+offset.x,feed.point_raw.y+offset.y,feed.point_raw.z+offset.z};
        const auto relative=gears_vr::normalized(gears_vr::Quaternion{
            snap_rotation_.x*(1.-seated),snap_rotation_.y*(1.-seated),snap_rotation_.z*(1.-seated),
            snap_rotation_.w*(1.-seated)+seated});
        s.magazine.rotation=gears_vr::ue_angles(gears_vr::rotation_matrix(gears_vr::normalized(gears_vr::multiply(to,relative))));
        s.assisted=s.snap_active=true;s.snap_progress=seated;return true;
    }
};
}
