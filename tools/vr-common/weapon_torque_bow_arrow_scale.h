#pragma once
#include <array>
#include <cmath>
#include <algorithm>
namespace gears_torque_bow_arrow_scale {
// Exact original component attributes; zero defaults are deliberately invalid.
// Parent scale comes from the authenticated current avatar/socket frame. This
// helper grants no permission to adopt an arbitrary parent or component.
struct Local {double scalar{};std::array<double,3> xyz{};bool absolute{};};
inline bool world(Local const& source,double parent_model_scale,double& out) noexcept {
    out=0;
    if(!std::isfinite(source.scalar)||source.scalar<=0||!std::isfinite(parent_model_scale)||
       parent_model_scale<=0||parent_model_scale>=10)return false;
    std::array<double,3> scales{};
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(source.xyz[i])||source.xyz[i]<=0)return false;
        scales[i]=source.scalar*source.xyz[i]*(source.absolute?1.:parent_model_scale);
        if(!std::isfinite(scales[i])||scales[i]<=.1||scales[i]>=10)return false;
    }
    const double result=(scales[0]+scales[1]+scales[2])/3;
    for(double value:scales)if(std::abs(value-result)>1e-6*std::max(1.,result))return false;
    out=result;return true;
}
template<class T>inline bool world(Local const& source,double model_scale,std::array<T,3> const& socket,double& out) noexcept {
    auto scaled=source;
    for(unsigned i=0;i<3;++i){
        const double value=double(socket[i]);
        if(!std::isfinite(value)||value<=0){out=0;return false;}
        if(!scaled.absolute)scaled.xyz[i]*=value;
    }
    return world(scaled,model_scale,out);
}
}
