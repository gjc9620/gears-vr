#pragma once
#include "weapon_longshot_animation_tracks.h"
#include <limits>
namespace gears_weapon::longshot_tracks {
// Geometry only. The caller separately authenticates a live hand lease and
// continuous input, then verifies native bone/seek agreement before completion.
class BoltPath {
    static constexpr unsigned subdivisions=8;
    std::array<gears_vr::Vector,2048> points_{};
    std::array<double,2048> arc_{};
    unsigned peak_{},count_{};double length_{},total_{};
    static gears_vr::Vector sub(gears_vr::Vector a,gears_vr::Vector b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
    static double dot(gears_vr::Vector a,gears_vr::Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;}
    static bool valid(gears_vr::Pose const& p){
        const auto q=p.orientation;const double norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
        return gears_vr::finite(p.position)&&std::isfinite(norm)&&std::abs(norm-1)<.001;
    }
public:
    bool bind(Samples const& samples,gears_vr::Vector handle_in_bone_raw,double units) noexcept {
        *this={};
        if(samples.count<3||samples.count>256||!std::isfinite(samples.length)||samples.length<=0||
           !gears_vr::finite(handle_in_bone_raw)||!std::isfinite(units)||units<10||units>500)return false;
        double farthest{};
        for(unsigned i=0;i<samples.count;++i){
            for(unsigned bone=0;bone<3;++bone){
                const auto& a=samples.bones[bone][0];const auto& b=samples.bones[bone][i];
                if(!valid(a)||!valid(b))return false;
                const auto delta=sub(a.position,b.position);
                const auto q=a.orientation,r=b.orientation;
                if(!gears_vr::finite(delta)||dot(delta,delta)>1e-8||
                   std::abs(q.x*r.x+q.y*r.y+q.z*r.z+q.w*r.w)<.999999)return false;
            }
            if(!valid(samples.bones[3][i]))return false;
        }
        // Sample the rotation arc between original keys, so a sparse rotating
        // handle cannot be mistaken for the straight chord between endpoints.
        const unsigned dense_count=(samples.count-1)*subdivisions+1;
        for(unsigned i=0;i<dense_count;++i){
            gears_vr::Pose pose{};
            if(!sample(samples,3,double(i)*samples.length/(samples.count*subdivisions),pose))return false;
            const auto rotated=gears_vr::transform(gears_vr::rotation_matrix(pose.orientation),handle_in_bone_raw);
            points_[i]={(pose.position.x+rotated.x)/units,(pose.position.y+rotated.y)/units,(pose.position.z+rotated.z)/units};
            const auto delta=sub(points_[i],points_[0]);const auto distance=dot(delta,delta);
            if(!std::isfinite(distance))return false;
            if(distance>farthest){farthest=distance;peak_=i;}
        }
        const auto closing=sub(points_[dense_count-1],points_[0]);
        if(peak_==0||peak_==dense_count-1||farthest<.000025||dot(closing,closing)>.000025)return false;
        const auto a=samples.bones[3][0].orientation,b=samples.bones[3][samples.count-1].orientation;
        if(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)<std::cos(2.5*3.14159265358979323846/180))return false;
        for(unsigned i=1;i<=peak_;++i){const auto delta=sub(points_[i],points_[i-1]);
            arc_[i]=arc_[i-1]+std::sqrt(dot(delta,delta));}
        total_=arc_[peak_];if(!std::isfinite(total_)||total_<.005||total_>.5)return false;
        count_=samples.count;length_=samples.length;return true;
    }
    bool bound() const noexcept {return count_!=0;}
    double travel_m() const noexcept {return bound()?total_:0.;}
    bool time_for_progress(double progress,float& out) const noexcept {
        if(!bound()||!std::isfinite(progress)||progress<0||progress>1)return false;
        if(progress==0){out=0;return true;}
        const double target=progress*total_;
        for(unsigned i=1;i<=peak_;++i){
            const double span=arc_[i]-arc_[i-1];
            if(span<=1e-10||target>arc_[i])continue;
            out=float((i-1+(target-arc_[i-1])/span)*length_/(count_*subdivisions));return true;
        }
        return false;
    }
    bool fit(gears_vr::Vector hand,double previous,double maximum_change,double tolerance,double& out) const noexcept {
        if(!bound()||!gears_vr::finite(hand)||!std::isfinite(previous)||previous<0||previous>1||
           !std::isfinite(maximum_change)||maximum_change<=0||maximum_change>1||
           !std::isfinite(tolerance)||tolerance<=0||tolerance>.1)return false;
        double best=std::numeric_limits<double>::infinity(),progress{};
        const double low=(std::max)(0.,previous-maximum_change),high=(std::min)(1.,previous+maximum_change);
        for(unsigned i=1;i<=peak_;++i){
            const auto axis=sub(points_[i],points_[i-1]);const double squared=dot(axis,axis);
            if(squared<=1e-12)continue;
            const double span=arc_[i]-arc_[i-1];
            const double t0=(std::max)(0.,(low*total_-arc_[i-1])/span);
            const double t1=(std::min)(1.,(high*total_-arc_[i-1])/span);
            if(t0>t1)continue;
            const double t=(std::clamp)(dot(sub(hand,points_[i-1]),axis)/squared,t0,t1);
            const gears_vr::Vector point{points_[i-1].x+t*axis.x,points_[i-1].y+t*axis.y,points_[i-1].z+t*axis.z};
            const auto delta=sub(hand,point);const double distance=dot(delta,delta);
            if(distance<best){best=distance;progress=(arc_[i-1]+t*span)/total_;}
        }
        if(best>tolerance*tolerance)return false;
        out=progress;return true;
    }
};
}
