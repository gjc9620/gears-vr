#pragma once
#include "gpu_viewport_stats.h"
#include "../vr-common/stereo_frame.h"
#include <cwchar>
#include <cerrno>
#include <cstdlib>

namespace gears_ui_hud {
struct Parameters {double horizontal_fov_degrees=65,depth_metres=2,source_aspect=0;};
enum class Reject : unsigned {not_attempted,none,overlay_state,stereo_geometry,parameters,tangents,
    eye_rotation,eye_position,viewport_nonfinite,viewport_bounds,corner_geometry,corner_error,float_viewport,empty_clip};
struct Diagnostic {Reject reject{Reject::not_attempted};unsigned eye{2};
    double rotation_difference{},corner_error_x{},corner_error_y{};};
inline const char* reject_name(Reject reason){
    switch(reason){
    case Reject::not_attempted:return "not_attempted";case Reject::none:return "none";
    case Reject::overlay_state:return "overlay_state";case Reject::stereo_geometry:return "stereo_geometry";
    case Reject::parameters:return "parameters";case Reject::tangents:return "tangents";
    case Reject::eye_rotation:return "eye_rotation";case Reject::eye_position:return "eye_position";
    case Reject::viewport_nonfinite:return "viewport_nonfinite";case Reject::viewport_bounds:return "viewport_bounds";
    case Reject::corner_geometry:return "corner_geometry";case Reject::corner_error:return "corner_error";
    case Reject::float_viewport:return "float_viewport";case Reject::empty_clip:return "empty_clip";
    }return "unknown";
}
inline Parameters for_source(Parameters parameters,double aspect){parameters.source_aspect=aspect;return parameters;}
// Strict bounded decimal parsing for the optional native INI value. Preserve
// the destination on error; no whitespace, hex, special values or suffixes.
inline bool parse_bounded_decimal(wchar_t const* text,double minimum,double maximum,double& out){
    if(!text)return false;
    unsigned length=0;while(length<63&&text[length])++length;if(!length||length==63)return false;
    auto p=text;if(*p==L'+'||*p==L'-')++p;
    unsigned digits=0;while(*p>=L'0'&&*p<=L'9'){++digits;++p;}
    if(*p==L'.'){++p;while(*p>=L'0'&&*p<=L'9'){++digits;++p;}}
    if(!digits)return false;
    if(*p==L'e'||*p==L'E'){
        ++p;if(*p==L'+'||*p==L'-')++p;unsigned exponent=0;
        while(*p>=L'0'&&*p<=L'9'){++exponent;++p;}if(!exponent)return false;
    }
    if(*p)return false;
    errno=0;wchar_t* end{};const double value=std::wcstod(text,&end);
    if(errno||*end||!std::isfinite(value)||value<minimum||value>maximum)return false;
    out=value;return true;
}
inline bool parse_horizontal_fov(wchar_t const* text,double& out){return parse_bounded_decimal(text,45,100,out);}
// Deliberate head-locked panel, independent of game camera/ADS projection.
// Admission of pure ordinary HUD/weapon/subtitle draws belongs to the caller.
// This geometry neither places a reticle target nor proves sample freshness.
inline bool plan(gears_viewport::DrawState const& saved,unsigned full_width,unsigned full_height,
    gears_vr::StereoSample const& frozen,gears_viewport::OverlayPlan& out,Parameters parameters={},Diagnostic* diagnostic=nullptr) {
    if(diagnostic)*diagnostic={Reject::none,2};
    auto fail=[&](Reject reason,unsigned eye=2){if(diagnostic){diagnostic->reject=reason;diagnostic->eye=eye;}return false;};
    if(!gears_viewport::overlay_plan(saved,full_width,full_height).valid)return fail(Reject::overlay_state);
    if(!gears_vr::stereo_geometry_valid(frozen))return fail(Reject::stereo_geometry);
    if(!std::isfinite(parameters.horizontal_fov_degrees)||parameters.horizontal_fov_degrees<=0||
       parameters.horizontal_fov_degrees>=179||!std::isfinite(parameters.depth_metres)||parameters.depth_metres<=0||
       !std::isfinite(parameters.source_aspect)||parameters.source_aspect<0)return fail(Reject::parameters);
    const double depth=parameters.depth_metres;
    const double tx=std::tan(parameters.horizontal_fov_degrees*3.14159265358979323846/360);
    const double ty=parameters.source_aspect?tx/parameters.source_aspect:tx*double(full_height)/full_width;
    if(!std::isfinite(tx)||!std::isfinite(ty)||tx<=0||ty<=0)return fail(Reject::tangents);
    // The panel belongs to the binocular view frozen by xrLocateViews. A
    // separate VIEW-space query can differ from that view even at the same
    // predicted time. Derive the panel frame from these exact eye poses rather
    // than requiring the independent head query to be bit-identical to them.
    // Parallel-eye admission and the corner reprojection bound remain below.
    const auto head_q=gears_vr::normalized(frozen.eyes[0].pose.orientation);
    const gears_vr::Vector head_position{
        (frozen.eyes[0].pose.position.x+frozen.eyes[1].pose.position.x)/2,
        (frozen.eyes[0].pose.position.y+frozen.eyes[1].pose.position.y)/2,
        (frozen.eyes[0].pose.position.z+frozen.eyes[1].pose.position.z)/2};
    const auto head_rotation=gears_vr::rotation_matrix(head_q);
    const auto head_inverse=gears_vr::transpose(head_rotation);
    gears_viewport::OverlayPlan result{};
    const double eye_width=double(full_width)/2,eye_height=full_height;
    const double source_left=(std::max)(0.,double(saved.scissor.x));
    const double source_right=(std::min)(double(full_width),double(saved.scissor.x)+saved.scissor.width);
    const double source_top=(std::max)(0.,double(saved.scissor.y));
    const double source_bottom=(std::min)(double(full_height),double(saved.scissor.y)+saved.scissor.height);
    for(unsigned eye=0;eye<2;++eye) {
        auto const& e=frozen.eyes[eye];
        auto q=gears_vr::normalized(e.pose.orientation);
        const double dot=q.x*head_q.x+q.y*head_q.y+q.z*head_q.z+q.w*head_q.w;
        if(dot<0)q={-q.x,-q.y,-q.z,-q.w};
        const double difference=std::hypot(std::hypot(q.x-head_q.x,q.y-head_q.y),std::hypot(q.z-head_q.z,q.w-head_q.w));
        // Only normalization/rotation roundoff, never a general canted-eye approximation.
        // Corner projection below also bounds this tolerance to 1e-4 pixel.
        if(!std::isfinite(difference)||difference>1e-12){
            if(diagnostic)diagnostic->rotation_difference=difference;
            return fail(Reject::eye_rotation,eye);
        }
        const auto delta=gears_vr::transform(head_inverse,{e.pose.position.x-head_position.x,
            e.pose.position.y-head_position.y,e.pose.position.z-head_position.z});
        // XR -Z forward -> native +Z forward, retaining X-right and Y-up.
        const double z=depth+delta.z;
        if(!gears_vr::finite(delta)||!std::isfinite(z)||z<=0)return fail(Reject::eye_position,eye);
        const double l=std::tan(e.fov.left),r=std::tan(e.fov.right),u=std::tan(e.fov.up),b=std::tan(e.fov.down);
        const double px=2/(r-l),py=2/(u-b),ox=-(r+l)/(r-l),oy=-(u+b)/(u-b);
        const double ax=px*depth*tx/z,ay=py*depth*ty/z;
        const double bx=ox-px*delta.x/z,by=oy-py*delta.y/z;
        const double origin=eye*eye_width;
        const double vx=origin+eye_width*(1-ax+bx)/2,vy=eye_height*(1-ay-by)/2;
        const double vw=eye_width*ax,vh=eye_height*ay;
        for(double value:{vx,vy,vw,vh,vx+vw,vy+vh})if(!std::isfinite(value))return fail(Reject::viewport_nonfinite,eye);
        // Do not clamp viewport: doing so would alter the panel's affine map.
        if(vw<=0||vh<=0||vw>32767||vh>32767||vx<-32768||vy<-32768||vx+vw>32767||vy+vh>32767)return fail(Reject::viewport_bounds,eye);
        const auto relative=gears_vr::multiply(gears_vr::transpose(gears_vr::rotation_matrix(q)),head_rotation);
        for(double sx:{-1.,1.})for(double sy:{-1.,1.}) {
            const auto actual=gears_vr::transform(relative,{sx*depth*tx-delta.x,sy*depth*ty-delta.y,-depth-delta.z});
            if(!gears_vr::finite(actual)||actual.z>=0)return fail(Reject::corner_geometry,eye);
            const double actual_x=origin+eye_width*(1+px*actual.x/(-actual.z)+ox)/2;
            const double actual_y=eye_height*(1-py*actual.y/(-actual.z)-oy)/2;
            const double affine_x=vx+vw*(sx+1)/2,affine_y=vy+vh*(1-sy)/2;
            if(!std::isfinite(actual_x)||!std::isfinite(actual_y)||std::abs(actual_x-affine_x)>1e-4||std::abs(actual_y-affine_y)>1e-4){
                if(diagnostic){diagnostic->corner_error_x=actual_x-affine_x;diagnostic->corner_error_y=actual_y-affine_y;}
                return fail(Reject::corner_error,eye);
            }
        }
        result.viewports[eye]={float(vx),float(vy),float(vw),float(vh)};
        auto const& viewport=result.viewports[eye];
        if(!std::isfinite(viewport.x)||!std::isfinite(viewport.y)||!std::isfinite(viewport.width)||!std::isfinite(viewport.height)||
           viewport.width<=0||viewport.height<=0||viewport.width>32767||viewport.height>32767||viewport.x<-32768||viewport.y<-32768||
           double(viewport.x)+viewport.width>32767||double(viewport.y)+viewport.height>32767)return fail(Reject::float_viewport,eye);
        // Clip with the actual float viewport sent to D3D, so quantization cannot
        // shrink the conservative integer scissor at an exact pixel boundary.
        const double clip_l=(std::max)(origin,double(viewport.x)+viewport.width*source_left/full_width);
        const double clip_r=(std::min)(origin+eye_width,double(viewport.x)+viewport.width*source_right/full_width);
        const double clip_t=(std::max)(0.,double(viewport.y)+viewport.height*source_top/full_height);
        const double clip_b=(std::min)(eye_height,double(viewport.y)+viewport.height*source_bottom/full_height);
        if(!(clip_r>clip_l&&clip_b>clip_t))return fail(Reject::empty_clip,eye);
        const double sl=std::floor(clip_l),st=std::floor(clip_t),sr=std::ceil(clip_r),sb=std::ceil(clip_b);
        result.scissors[eye]={float(sl),float(st),float(sr-sl),float(sb-st)};
    }
    result.valid=true;out=result;return true;
}
} // namespace gears_ui_hud
