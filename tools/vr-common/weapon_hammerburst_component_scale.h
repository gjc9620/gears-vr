#pragma once
#include <array>
#include <cmath>
// Prospective component scale only. This never creates an actual world pose,
// source frame, attachment or permission to move a component.
namespace gears_hammerburst_component_scale {
inline bool uninitialized(std::array<float,16> const& matrix) noexcept {
    for(float value:matrix)if(!std::isfinite(value)||value!=0.f)return false;
    return true;
}
inline bool prospective(float scalar,std::array<float,3> const& xyz,bool absolute,
    bool attached,double parent,double& out) noexcept {
    if(attached||!std::isfinite(scalar)||scalar<=0||!std::isfinite(parent)||parent<=0)return false;
    for(float value:xyz)if(!std::isfinite(value)||value<=0)return false;
    if(std::abs(double(xyz[0])-xyz[1])>1e-5||std::abs(double(xyz[0])-xyz[2])>1e-5)return false;
    const double scale=double(scalar)*xyz[0]*(absolute?1.:parent);
    if(!std::isfinite(scale)||scale<=.01||scale>3.)return false;
    out=scale;return true;
}
}
