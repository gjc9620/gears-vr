#pragma once
#include "stereo_frame.h"
#include <limits>

namespace gears_vr {
// Native coordinate units; a numerical bound, not a physical scale calibration.
inline constexpr double view_origin_max_error=.02;
// Row-vector rigid view: world point P maps to P*R+t. Keep R/homogeneous
// elements unchanged and replace only t=-origin*R. Failure leaves out intact.
inline bool view_with_origin(StereoMatrix const& view,Vector origin,StereoMatrix& out) {
    if(!finite(origin)||!stereo_rigid(view))return false;
    StereoMatrix candidate=view;
    const double a=view[0],b=view[1],c=view[2],d=view[4],e=view[5],f=view[6],g=view[8],h=view[9],i=view[10];
    const double determinant=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    // Use the actual float rotation inverse, not an approximate transpose: at
    // large origins a tiny orthogonality error can exceed the numerical bound.
    const double inverse[3][3]={{(e*i-f*h)/determinant,(c*h-b*i)/determinant,(b*f-c*e)/determinant},
        {(f*g-d*i)/determinant,(a*i-c*g)/determinant,(c*d-a*f)/determinant},
        {(d*h-e*g)/determinant,(b*g-a*h)/determinant,(a*e-b*d)/determinant}};
    for(unsigned j=0;j<3;++j){
        double value=-(origin.x*view[j]+origin.y*view[4+j]+origin.z*view[8+j]);
        if(!std::isfinite(value)||std::abs(value)>(std::numeric_limits<float>::max)())return false;
        candidate[12+j]=static_cast<float>(value);
    }
    double recovered[3]{};
    for(unsigned j=0;j<3;++j){for(unsigned k=0;k<3;++k)recovered[j]-=double(candidate[12+k])*inverse[k][j];
        if(!std::isfinite(recovered[j]))return false;}
    double error=std::hypot(std::hypot(recovered[0]-origin.x,recovered[1]-origin.y),recovered[2]-origin.z);
    if(!std::isfinite(error)||error>view_origin_max_error)return false;
    out=candidate;return true;
}
}
