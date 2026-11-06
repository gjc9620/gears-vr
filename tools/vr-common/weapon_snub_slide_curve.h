#pragma once
#include "rig_tracking_math.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace gears_weapon {
// Live decoded original keys only. The reference mesh supplies the CLOSED
// parent-local position: key0 of AR_Reload is already partly pulled back.
// Use the final pull/close branch in reverse; never replay magazine notifies.
class SnubSlideCurve {
public:
    enum class Sampling {reference_keys,endpoint_intervals};
private:
    std::array<double,256> distance_{};
    std::size_t count_{},open_{},closed_{};double duration_{};bool bound_{};
public:
    void reset() noexcept {*this={};}
    bool bound() const noexcept {return bound_;}
    double travel() const noexcept {return bound_?distance_[open_]:0;}
    std::size_t open_key() const noexcept {return open_;}
    std::size_t closed_key() const noexcept {return closed_;}
    std::size_t key_count() const noexcept {return count_;}
    bool bind(gears_vr::Vector const* positions,std::size_t count,double duration,
        gears_vr::Vector closed,gears_vr::Vector axis,bool uniform_time_verified) noexcept {
        reset();
        const auto dot=[](gears_vr::Vector a,gears_vr::Vector b){return a.x*b.x+a.y*b.y+a.z*b.z;};
        if(!positions||!uniform_time_verified||count<4||count>distance_.size()||
           !std::isfinite(duration)||duration<=0||duration>10||!gears_vr::finite(closed)||
           !gears_vr::finite(axis)||std::abs(dot(axis,axis)-1)>1e-5)return false;
        std::size_t open=0;
        for(std::size_t i=0;i<count;++i){
            if(!gears_vr::finite(positions[i]))return false;
            const gears_vr::Vector d{positions[i].x-closed.x,positions[i].y-closed.y,positions[i].z-closed.z};
            const double along=dot(d,axis),lateral=std::max(0.,dot(d,d)-along*along);
            if(along<-.00005||along>.15||lateral>.0001*.0001)return false;
            distance_[i]=std::max(0.,along);if(distance_[i]>distance_[open])open=i;
        }
        if(!open||open>=count-2||distance_[open]<.01)return false;
        // Require a final stable closed plateau, and a strictly monotone
        // original closing branch from maximum pull to that plateau.
        std::size_t closed_key=count-1;
        if(distance_[closed_key]>.00005)return false;
        while(closed_key>open&&distance_[closed_key-1]<=.00005)--closed_key;
        if(closed_key<=open||count-closed_key<2)return false;
        for(auto i=open+1;i<=closed_key;++i)if(distance_[i]>=distance_[i-1]-.000001)return false;
        count_=count;open_=open;closed_=closed_key;duration_=duration;bound_=true;return true;
    }
    bool time_for_distance(double distance,float& out,Sampling law) const noexcept {
        if(!bound_||!std::isfinite(distance)||distance<0||distance>travel()+1e-6)return false;
        // The actual end pose is closed under both supported sampling laws.
        if(distance<=.000001){out=float(duration_);return true;}
        for(auto i=open_;i<closed_;++i){
            if(distance>distance_[i]+1e-6||distance<distance_[i+1]-1e-6)continue;
            const double fraction=std::clamp((distance_[i]-distance)/(distance_[i]-distance_[i+1]),0.,1.);
            const auto intervals=law==Sampling::endpoint_intervals?count_-1:count_;
            out=float((double(i)+fraction)*duration_/double(intervals));return true;
        }
        return false;
    }
    bool distance_at_time(double time,double& out,Sampling law) const noexcept {
        if(!bound_||!std::isfinite(time)||time<0||time>duration_)return false;
        const auto intervals=law==Sampling::endpoint_intervals?count_-1:count_;
        const double key=time*double(intervals)/duration_;
        if(key<double(open_)-1e-5)return false;
        if(key>=double(closed_)){out=0;return true;}
        const auto i=std::size_t(key);const double t=std::clamp(key-double(i),0.,1.);
        out=distance_[i]+(distance_[i+1]-distance_[i])*t;return true;
    }
};
} // namespace gears_weapon
