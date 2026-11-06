#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace gears_weapon {
// No shipped key data is embedded here. The adapter supplies this instance's
// decoded, axis-verified samples and independently established uniform timing.
class HammerburstLatchCurve {
    std::array<double,256> angles_{};
    std::size_t count_{},peak_{};double length_{};bool bound_{};
public:
    enum class Sampling {reference_keys,endpoint_intervals};
    void reset() noexcept {bound_=false;count_=peak_=0;length_=0;}
    bool bound() const noexcept {return bound_;}
    double open_angle() const noexcept {return bound_?angles_[peak_]:0;}
    bool bind(double const* angles,std::size_t count,double length,bool uniform_time_verified) noexcept {
        reset();
        if(!uniform_time_verified||!angles||count<3||count>angles_.size()||
           !std::isfinite(length)||length<=0||length>10)return false;
        std::size_t peak=0;
        for(std::size_t i=0;i<count;++i){
            if(!std::isfinite(angles[i])||angles[i]<-.001||angles[i]>3.14)return false;
            if(angles[i]>angles[peak])peak=i;
        }
        if(peak==0||peak==count-1||angles[peak]<.05||std::abs(angles[0])>.001||
           std::abs(angles[count-1])>.001)return false;
        for(std::size_t i=1;i<=peak;++i)if(angles[i]+1e-5<angles[i-1])return false;
        std::copy_n(angles,count,angles_.begin());count_=count;peak_=peak;length_=length;bound_=true;return true;
    }
    // Reference decoder: key i is at i*SequenceLength/NumKeys. A native
    // endpoint-interval law must be selected explicitly from live evidence;
    // static package samples alone cannot certify the engine's interpolation.
    bool time_for_angle(double angle,float& out,Sampling sampling=Sampling::reference_keys) const noexcept {
        if(!bound_||!std::isfinite(angle)||angle<0||angle>angles_[peak_]+1e-5)return false;
        if(angle<=0){out=0;return true;}
        for(std::size_t i=1;i<=peak_;++i){
            const double span=angles_[i]-angles_[i-1];
            if(span<=1e-8||angle>angles_[i]+1e-5)continue;
            const double fraction=std::clamp((angle-angles_[i-1])/span,0.,1.);
            const auto intervals=sampling==Sampling::endpoint_intervals?count_-1:count_;
            out=float((double(i-1)+fraction)*length_/double(intervals));return true;
        }
        return false;
    }
    bool angle_at_time(double time,double& out,Sampling sampling) const noexcept {
        if(!bound_||!std::isfinite(time)||time<0)return false;
        const auto intervals=sampling==Sampling::endpoint_intervals?count_-1:count_;
        const double position=time*double(intervals)/length_;
        if(position>double(peak_)+1e-5)return false;
        const auto first=std::min(std::size_t(position),peak_);
        const auto next=std::min(first+1,peak_);
        out=angles_[first]+(angles_[next]-angles_[first])*std::clamp(position-double(first),0.,1.);
        return true;
    }
};
} // namespace gears_weapon
