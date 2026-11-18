#pragma once
#include "pose_math.h"
#include "authored_contact_volume.h"
#include <array>
#include <cmath>

namespace gears_weapon {
// Optional adapter-local acquisition geometry. Never part of a Native wire
// request, never a continuing-grip boundary. Convert the current mount-space
// offset from the support socket into the authored object's metre coordinates.
struct SupportRegion {
    bool enabled{};
    gears_authored_contact_volume::Volume volume{};
    gears_vr::Matrix object_from_mount{{{1,0,0},{0,1,0},{0,0,1}}};
    gears_vr::Vector origin_in_object{};
    double scale{1};
    bool snap_limits_enabled{};
    gears_vr::Pose target_grip_in_mount{};
    gears_vr::Vector wrist_offset_in_grip{}; // physical calibration, metres
    bool operator==(SupportRegion const& b) const noexcept {
        return enabled==b.enabled&&volume==b.volume&&object_from_mount==b.object_from_mount&&
            origin_in_object.x==b.origin_in_object.x&&origin_in_object.y==b.origin_in_object.y&&
            origin_in_object.z==b.origin_in_object.z&&scale==b.scale&&
            snap_limits_enabled==b.snap_limits_enabled&&
            target_grip_in_mount.position.x==b.target_grip_in_mount.position.x&&
            target_grip_in_mount.position.y==b.target_grip_in_mount.position.y&&
            target_grip_in_mount.position.z==b.target_grip_in_mount.position.z&&
            target_grip_in_mount.orientation.x==b.target_grip_in_mount.orientation.x&&
            target_grip_in_mount.orientation.y==b.target_grip_in_mount.orientation.y&&
            target_grip_in_mount.orientation.z==b.target_grip_in_mount.orientation.z&&
            target_grip_in_mount.orientation.w==b.target_grip_in_mount.orientation.w&&
            wrist_offset_in_grip.x==b.wrist_offset_in_grip.x&&wrist_offset_in_grip.y==b.wrist_offset_in_grip.y&&
            wrist_offset_in_grip.z==b.wrist_offset_in_grip.z;
    }
    bool valid() const noexcept {
        if(!enabled||!gears_authored_contact_volume::valid(volume)||!gears_vr::finite(origin_in_object)||
           !std::isfinite(scale)||scale<=0||!gears_vr::finite(wrist_offset_in_grip)||
           !gears_authored_contact_volume::pose_valid(target_grip_in_mount))return false;
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
            double dot{};
            for(unsigned k=0;k<3;++k){if(!std::isfinite(object_from_mount[k][i]))return false;
                dot+=object_from_mount[k][i]*object_from_mount[k][j];}
            if(std::abs(dot-(i==j?1.:0.))>1e-6)return false;
        }
        return true;
    }
    bool contains(gears_vr::Vector offset) const noexcept {
        if(!valid()||!gears_vr::finite(offset))return false;
        const auto p=gears_vr::transform(object_from_mount,offset);
        return gears_authored_contact_volume::contains(volume,{origin_in_object.x+p.x*scale,
            origin_in_object.y+p.y*scale,origin_in_object.z+p.z*scale});
    }
    bool snap(gears_vr::Pose const& physical_grip_in_mount,bool retained) const noexcept {
        if(!valid()||!gears_authored_contact_volume::pose_valid(physical_grip_in_mount))return false;
        if(!snap_limits_enabled)return true;
        const auto& a=physical_grip_in_mount;const auto& b=target_grip_in_mount;
        const auto x=gears_vr::transform(gears_vr::rotation_matrix(a.orientation),wrist_offset_in_grip);
        const auto y=gears_vr::transform(gears_vr::rotation_matrix(b.orientation),wrist_offset_in_grip);
        const double dx=a.position.x+x.x-b.position.x-y.x,dy=a.position.y+x.y-b.position.y-y.y,
            dz=a.position.z+x.z-b.position.z-y.z;
        const double limit=retained?(std::max)(volume.release_distance,volume.snap_distance):volume.snap_distance;
        if(dx*dx+dy*dy+dz*dz>limit*limit)return false;
        const auto p=gears_vr::normalized(a.orientation),q=gears_vr::normalized(b.orientation);
        // A common right-side calibration rotation cancels from quaternion
        // geodesic angle; wrist translation above does NOT cancel, so use it.
        const double angle=2*std::acos(std::clamp(std::abs(p.x*q.x+p.y*q.y+p.z*q.z+p.w*q.w),0.,1.));
        return angle<=(retained?(std::min)(3.14159265358979323846,volume.snap_angle+.35):volume.snap_angle);
    }
};
// Optional contact shape, centred at the authored controller-grip socket.
// Columns are the box's local axes in the weapon mount's XR basis; metres.
// This only evaluates geometry. It never grants a lease or moves a hand/gun.
struct SupportBox {
    bool enabled{};
    gears_vr::Matrix axes{{{1,0,0},{0,1,0},{0,0,1}}};
    gears_vr::Vector acquire_half{},retain_half{};
    bool operator==(SupportBox const& b) const noexcept {
        return enabled==b.enabled&&axes==b.axes&&
            acquire_half.x==b.acquire_half.x&&acquire_half.y==b.acquire_half.y&&acquire_half.z==b.acquire_half.z&&
            retain_half.x==b.retain_half.x&&retain_half.y==b.retain_half.y&&retain_half.z==b.retain_half.z;
    }
    bool valid() const noexcept {
        if(!enabled)return false;
        const std::array<double,3> a{acquire_half.x,acquire_half.y,acquire_half.z};
        const std::array<double,3> r{retain_half.x,retain_half.y,retain_half.z};
        for(unsigned i=0;i<3;++i){
            if(!std::isfinite(a[i])||!std::isfinite(r[i])||a[i]<=0||r[i]<a[i]||r[i]>.25)return false;
            for(unsigned j=0;j<3;++j){
                double product{};
                for(unsigned k=0;k<3;++k){if(!std::isfinite(axes[k][i]))return false;product+=axes[k][i]*axes[k][j];}
                if(std::abs(product-(i==j?1.:0.))>1e-6)return false;
            }
        }
        const double determinant=axes[0][0]*(axes[1][1]*axes[2][2]-axes[1][2]*axes[2][1])-
            axes[0][1]*(axes[1][0]*axes[2][2]-axes[1][2]*axes[2][0])+
            axes[0][2]*(axes[1][0]*axes[2][1]-axes[1][1]*axes[2][0]);
        // UE and XR vector bases have opposite handedness. A reflected
        // orthonormal box is valid; it is never treated as a pose quaternion.
        return std::abs(std::abs(determinant)-1.)<=1e-6;
    }
    bool contains(gears_vr::Vector offset_in_weapon,bool retained) const noexcept {
        if(!valid()||!gears_vr::finite(offset_in_weapon))return false;
        const auto p=gears_vr::transform(gears_vr::transpose(axes),offset_in_weapon);
        const auto h=retained?retain_half:acquire_half;
        return std::abs(p.x)<=h.x&&std::abs(p.y)<=h.y&&std::abs(p.z)<=h.z;
    }
};
}
