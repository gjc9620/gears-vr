#pragma once
#include "cover_primitive_query.h"
#include "cover_physx_transform.h"
// Original game-unit geometry -> native PhysX query parameters, exactly once.
// The caller's budget is in GAME units; output expansion is in PhysX units.
// Conservative parameter enclosure only, not active/native OBB or world proof.
namespace gears_cover_query_units {
namespace p=gears_cover_primitive_query;namespace w=p::w;using Code=p::Code;
inline constexpr double factor=double(gears_cover_physx_transform::game_to_physics);
namespace detail {
inline bool coordinate(double input,double& output,double& error)noexcept{
    if(!std::isfinite(input)||(input!=0&&!std::isnormal(input)))return false;
    const double value=input*factor;
    if(!std::isfinite(value)||std::abs(value)>(std::numeric_limits<float>::max)()||
       (value!=0&&std::abs(value)<(std::numeric_limits<float>::min)())||(value==0&&input!=0))return false;
    // factor has24 significant bits. In this float-range domain the product
    // residual has at most24 bits and is a normal double (or exact zero).
    const double residual=std::fma(input,factor,-value);
    if(!std::isfinite(residual)||(residual!=0&&!std::isnormal(residual)))return false;
    output=value;error=p::detail::up(std::abs(residual));return true;
}
inline bool coordinate(w::Vec input,w::Vec& output,double& error)noexcept{
    double x{},y{},z{};if(!coordinate(input.x,output.x,x)||!coordinate(input.y,output.y,y)||!coordinate(input.z,output.z,z))return false;
    error=p::detail::up(p::detail::up(x+y)+z);return std::isfinite(error);
}
inline bool dimension(float input,double error,float& output,double& exact)noexcept{
    if(!std::isnormal(input)||input<=0||!std::isfinite(error)||error<0)return false;
    // float * float fits in double exactly throughout this domain.
    exact=double(input)*factor;const double upper=error==0?exact:p::detail::up(exact+error);
    if(!std::isfinite(upper)||upper>(std::numeric_limits<float>::max)()||upper<(std::numeric_limits<float>::min)())return false;
    float value=float(upper);if(double(value)<upper)value=std::nextafter(value,(std::numeric_limits<float>::infinity)());
    if(!std::isnormal(value)||value<=0)return false;output=value;return true;
}
}
inline Code convert(w::Primitive const& input,double maximum_game_expansion,p::Shape& output)noexcept{
    if(p::q::source::aliases(&input,sizeof input,&output,sizeof output))return Code::alias;
    if(!std::isfinite(maximum_game_expansion)||maximum_game_expansion<0)return Code::invalid;
    unsigned mode{};if(_controlfp_s(&mode,0,0)||(mode&_MCW_RC)!=_RC_NEAR)return Code::precision;
    if(unsigned(input.dimensions.kind)>unsigned(w::metrics::Kind::sphyl))return Code::unsupported;
    if(input.dimensions.kind==w::metrics::Kind::box){for(float value:input.dimensions.box_half_extents)
      if(!p::q::detail::scalar(value)||value<=0)return Code::invalid;
    }else if(!p::q::detail::scalar(input.dimensions.radius)||input.dimensions.radius<=0)return Code::invalid;
    double native_limit=maximum_game_expansion*factor;
    if(std::fma(maximum_game_expansion,factor,-native_limit)<0)native_limit=std::nextafter(native_limit,-(std::numeric_limits<double>::infinity)());
    auto scaled=input;double error{},radius{},halves[3]{};
    if(input.dimensions.kind==w::metrics::Kind::sphyl){double first{},second{};
      if(!detail::coordinate(input.segment_start,scaled.segment_start,first)||!detail::coordinate(input.segment_end,scaled.segment_end,second))return Code::precision;
      error=(std::max)(first,second);
    }else if(!detail::coordinate(input.shape_world.position,scaled.shape_world.position,error))return Code::precision;
    if(input.dimensions.kind==w::metrics::Kind::box){
      for(unsigned i=0;i<3;++i)if(!detail::dimension(input.dimensions.box_half_extents[i],error,scaled.dimensions.box_half_extents[i],halves[i]))return Code::precision;
    }else if(!detail::dimension(input.dimensions.radius,error,scaled.dimensions.radius,radius))return Code::precision;
    p::Shape next{};const auto code=p::convert_bounded(scaled,native_limit,next);if(code!=Code::converted)return code;
    if(input.dimensions.kind==w::metrics::Kind::box){const float values[]={next.box.half.x,next.box.half.y,next.box.half.z};
      for(unsigned i=0;i<3;++i){next.box_half_expansion[i]=double(values[i])-halves[i];
        if(next.box_half_expansion[i]<0||next.box_half_expansion[i]>native_limit)return Code::precision;}
    }else{next.radius_expansion=double(input.dimensions.kind==w::metrics::Kind::sphere?next.sphere.radius:next.capsule.radius)-radius;
      if(next.radius_expansion<0||next.radius_expansion>native_limit)return Code::precision;}
    next.units=p::Units::physx;output=next;return Code::converted;
}
}
