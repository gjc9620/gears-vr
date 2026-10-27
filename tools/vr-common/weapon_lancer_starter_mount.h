#pragma once
#include "pose_math.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace gears_weapon {

// Provisional location next to the original Lancer's chainsaw housing. This
// must be aligned to the visible VR starter and calibrated in a headset before
// it can become a playable grip. The private synthetic trial uses it only to
// verify the gesture-to-original-input boundary.
inline constexpr gears_vr::Vector lancer_starter_local_raw{6,26,10.65};
inline constexpr gears_vr::Vector lancer_starter_axis_local{1,0,0};
struct LancerStarterWorld {
    gears_vr::Vector anchor_raw{},pull_axis{};
};

inline bool lancer_starter_world(std::array<float,16> const& gun,
                                LancerStarterWorld& out) noexcept {
    for(float value:gun)if(!std::isfinite(value))return false;
    if(std::abs(gun[3])>1e-4||std::abs(gun[7])>1e-4||
       std::abs(gun[11])>1e-4||std::abs(gun[15]-1)>1e-4)return false;
    double scale[3]{};
    for(int i=0;i<3;++i){
        scale[i]=std::hypot(std::hypot(double(gun[i*4]),double(gun[i*4+1])),
                             double(gun[i*4+2]));
        if(scale[i]<.1||scale[i]>10)return false;
    }
    for(int i=0;i<3;++i)for(int j=i+1;j<3;++j){
        double cosine=0;
        for(int k=0;k<3;++k)cosine+=double(gun[i*4+k])*gun[j*4+k];
        if(std::abs(cosine/(scale[i]*scale[j]))>1e-3||
           std::abs(scale[i]-scale[j])>(std::max)(scale[i],scale[j])*.02)
            return false;
    }
    const double det=double(gun[0])*(double(gun[5])*gun[10]-double(gun[6])*gun[9])-
        double(gun[1])*(double(gun[4])*gun[10]-double(gun[6])*gun[8])+
        double(gun[2])*(double(gun[4])*gun[9]-double(gun[5])*gun[8]);
    if(det<=0||!std::isfinite(det))return false;
    out.anchor_raw={
        gun[12]+lancer_starter_local_raw.x*gun[0]+
            lancer_starter_local_raw.y*gun[4]+lancer_starter_local_raw.z*gun[8],
        gun[13]+lancer_starter_local_raw.x*gun[1]+
            lancer_starter_local_raw.y*gun[5]+lancer_starter_local_raw.z*gun[9],
        gun[14]+lancer_starter_local_raw.x*gun[2]+
            lancer_starter_local_raw.y*gun[6]+lancer_starter_local_raw.z*gun[10]};
    out.pull_axis={gun[0]/scale[0],gun[1]/scale[0],gun[2]/scale[0]};
    return gears_vr::finite(out.anchor_raw)&&gears_vr::finite(out.pull_axis);
}

} // namespace gears_weapon
