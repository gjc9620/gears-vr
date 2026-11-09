#pragma once
#include "hand_object_grasp.h"
#include "hand_authored_mount_profile.h"
#include "rig_tracking_math.h"

// Historical, currently unconnected cosmetic fitting draft. Its assumed part
// and wrist frames need live/skin validation before use. Contacts and feed
// travel must continue using the unchanged tracked hand, never this output.
namespace gears_weapon_hammerburst_grasp {
namespace vr=gears_vr;
struct MagazineAssist {
    vr::Vector correction{};double weight{};std::int64_t last_ns{};
    void reset() noexcept {*this={};}
    bool apply(vr::Pose const& raw,vr::Pose const& port,vr::Vector outward,std::int64_t ns,vr::Pose& out) noexcept {
        if(!vr::stereo_pose_valid(raw)||!vr::stereo_pose_valid(port)||!vr::finite(outward)||ns<=0||
           (last_ns&&ns<=last_ns))return false;
        const auto norm=[](vr::Vector v){return std::hypot(std::hypot(v.x,v.y),v.z);};
        if(std::abs(norm(outward)-1)>.001)return false;
        const double dt=last_ns?std::min(double(ns-last_ns)/1e9,.05):0;last_ns=ns;
        const vr::Vector delta{raw.position.x-port.position.x,raw.position.y-port.position.y,raw.position.z-port.position.z};
        const double depth=delta.x*outward.x+delta.y*outward.y+delta.z*outward.z;
        const vr::Vector lateral{delta.x-depth*outward.x,delta.y-depth*outward.y,delta.z-depth*outward.z};
        const double distance=norm(lateral),qdot=std::abs(raw.orientation.x*port.orientation.x+
            raw.orientation.y*port.orientation.y+raw.orientation.z*port.orientation.z+raw.orientation.w*port.orientation.w);
        const bool within=depth>=-.025&&depth<=.12&&distance<=.10&&qdot>=std::cos(.85*.5);
        const double desired=within?std::clamp((.12-depth)/.09,0.,1.):0.;
        weight+=std::clamp(desired-weight,-4*dt,4*dt);
        const double factor=distance>1e-8?std::min(distance,.08)/distance*weight:0.;
        const vr::Vector wanted{-lateral.x*factor,-lateral.y*factor,-lateral.z*factor};
        const vr::Vector step{wanted.x-correction.x,wanted.y-correction.y,wanted.z-correction.z};
        const double length=norm(step),fraction=length>1e-8?std::min(1.,.45*dt/length):1.;
        correction={correction.x+step.x*fraction,correction.y+step.y*fraction,correction.z+step.z*fraction};
        out={{raw.position.x+correction.x,raw.position.y+correction.y,raw.position.z+correction.z},
            gears_hand_object_grasp::blend(raw.orientation,port.orientation,weight)};
        return vr::stereo_pose_valid(out); // Cosmetic only: no insertion receipt.
    }
};
struct PartFit {
    bool known{};vr::Quaternion rotation{};vr::Vector offset{};
    void reset() noexcept {*this={};}
    bool solve(bool fresh,vr::Pose const& part,vr::Vector contact,vr::Vector hinge_axis,
        vr::Pose const& physical_wrist,double scale,double units,vr::Pose& out) noexcept {
        if(fresh)reset();
        if(!vr::stereo_pose_valid(part)||!vr::stereo_pose_valid(physical_wrist)||!vr::finite(contact)||
           !vr::finite(hinge_axis)||!std::isfinite(scale)||scale<=0||scale>3||
           !std::isfinite(units)||units<10||units>500)return false;
        if(!known){
            const auto dot=[](vr::Vector a,vr::Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;};
            const auto cross=[](vr::Vector a,vr::Vector b){return vr::Vector{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
            const auto norm=[](vr::Vector& v){const double n=std::hypot(std::hypot(v.x,v.y),v.z);
                if(!std::isfinite(n)||n<1e-6)return false;v={v.x/n,v.y/n,v.z/n};return true;};
            auto z=hinge_axis;if(!norm(z))return false;
            const auto old_z=vr::transform(vr::rotation_matrix(physical_wrist.orientation),{0,0,1});
            if(dot(z,old_z)<0)z={-z.x,-z.y,-z.z};
            vr::Vector x{contact.x-physical_wrist.position.x,contact.y-physical_wrist.position.y,contact.z-physical_wrist.position.z};
            const auto axial=dot(x,z);x={x.x-z.x*axial,x.y-z.y*axial,x.z-z.z*axial};
            if(!norm(x))return false;
            const auto y=cross(z,x);vr::Matrix basis{{{x.x,y.x,z.x},{x.y,y.y,z.y},{x.z,y.z,z.z}}};
            vr::Quaternion wrist{};if(!vr::rig_tracking::detail::quaternion(basis,wrist))return false;
            // Preserve a natural approach: at most30deg cosmetic alignment at
            // acquisition, retained in the actual part frame through travel.
            auto delta=vr::normalized(vr::multiply(wrist,vr::inverse(physical_wrist.orientation)));
            if(delta.w<0)delta={-delta.x,-delta.y,-delta.z,-delta.w};
            const double sine=std::hypot(std::hypot(delta.x,delta.y),delta.z),maximum=.523598775598298873;
            if(2*std::atan2(sine,delta.w)>maximum&&sine>1e-12){const double k=std::sin(maximum*.5)/sine;
                delta={delta.x*k,delta.y*k,delta.z*k,std::cos(maximum*.5)};}
            wrist=vr::normalized(vr::multiply(delta,physical_wrist.orientation));
            gears_hand_authored_mount::Mount mount{};
            if(!gears_hand_authored_mount::marcus115(gears_hand_authored_mount::marcus115_signature,
                gears_hand_authored_mount::Side::left,scale,units,mount))return false;
            // Historical Marcus index/thumb pad estimate, not a weapon
            // coordinate or proof of a delivered HB-specific C0 finger pose.
            const auto pad=vr::transform(vr::rotation_matrix(wrist),{9.75*scale,-4.6*scale,-2.1*scale});
            const auto grip=vr::normalized(vr::multiply(wrist,vr::inverse(mount.calibration.grip_to_wrist_rotation)));
            const auto d=vr::transform(vr::rotation_matrix(grip),mount.calibration.grip_to_wrist_metres);
            const vr::Vector p{contact.x-pad.x-d.x*units,contact.y-pad.y-d.y*units,contact.z-pad.z-d.z*units};
            rotation=vr::multiply(vr::inverse(part.orientation),grip);
            offset=vr::transform(vr::transpose(vr::rotation_matrix(part.orientation)),{p.x-contact.x,p.y-contact.y,p.z-contact.z});
            known=true;
        }
        const auto d=vr::transform(vr::rotation_matrix(part.orientation),offset);
        const vr::Pose result{{contact.x+d.x,contact.y+d.y,contact.z+d.z},vr::normalized(vr::multiply(part.orientation,rotation))};
        if(!vr::stereo_pose_valid(result))return false;out=result;return true;
    }
};
}
