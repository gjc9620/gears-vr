#pragma once
#include "gpu_viewport_stats.h"
#include "../vr-common/stereo_frame.h"

namespace gears_ui_hud {
struct Parameters {double horizontal_fov_degrees=55,depth_metres=2,source_aspect=0;};
// Deliberate head-locked panel, independent of game camera/ADS projection.
// Admission of pure ordinary HUD/weapon/subtitle draws belongs to the caller.
// This geometry neither places a reticle target nor proves sample freshness.
inline bool plan(gears_viewport::DrawState const& saved,unsigned full_width,unsigned full_height,
    gears_vr::StereoSample const& frozen,gears_viewport::OverlayPlan& out,Parameters parameters={}) {
    if(!gears_viewport::overlay_plan(saved,full_width,full_height).valid||
       !gears_vr::stereo_geometry_valid(frozen)||
       !std::isfinite(parameters.horizontal_fov_degrees)||parameters.horizontal_fov_degrees<=0||
       parameters.horizontal_fov_degrees>=179||!std::isfinite(parameters.depth_metres)||parameters.depth_metres<=0||
       !std::isfinite(parameters.source_aspect)||parameters.source_aspect<0)return false;
    const double depth=parameters.depth_metres;
    const double tx=std::tan(parameters.horizontal_fov_degrees*3.14159265358979323846/360);
    const double ty=parameters.source_aspect?tx/parameters.source_aspect:tx*double(full_height)/full_width;
    if(!std::isfinite(tx)||!std::isfinite(ty)||tx<=0||ty<=0)return false;
    const auto head_q=gears_vr::normalized(frozen.head.orientation);
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
        if(!std::isfinite(difference)||difference>1e-12)return false;
        const auto delta=gears_vr::transform(head_inverse,{e.pose.position.x-frozen.head.position.x,
            e.pose.position.y-frozen.head.position.y,e.pose.position.z-frozen.head.position.z});
        // XR -Z forward -> native +Z forward, retaining X-right and Y-up.
        const double z=depth+delta.z;
        if(!gears_vr::finite(delta)||!std::isfinite(z)||z<=0)return false;
        const double l=std::tan(e.fov.left),r=std::tan(e.fov.right),u=std::tan(e.fov.up),b=std::tan(e.fov.down);
        const double px=2/(r-l),py=2/(u-b),ox=-(r+l)/(r-l),oy=-(u+b)/(u-b);
        const double ax=px*depth*tx/z,ay=py*depth*ty/z;
        const double bx=ox-px*delta.x/z,by=oy-py*delta.y/z;
        const double origin=eye*eye_width;
        const double vx=origin+eye_width*(1-ax+bx)/2,vy=eye_height*(1-ay-by)/2;
        const double vw=eye_width*ax,vh=eye_height*ay;
        for(double value:{vx,vy,vw,vh,vx+vw,vy+vh})if(!std::isfinite(value))return false;
        // Do not clamp viewport: doing so would alter the panel's affine map.
        if(vw<=0||vh<=0||vw>32767||vh>32767||vx<-32768||vy<-32768||vx+vw>32767||vy+vh>32767)return false;
        const auto relative=gears_vr::multiply(gears_vr::transpose(gears_vr::rotation_matrix(q)),head_rotation);
        for(double sx:{-1.,1.})for(double sy:{-1.,1.}) {
            const auto actual=gears_vr::transform(relative,{sx*depth*tx-delta.x,sy*depth*ty-delta.y,-depth-delta.z});
            if(!gears_vr::finite(actual)||actual.z>=0)return false;
            const double actual_x=origin+eye_width*(1+px*actual.x/(-actual.z)+ox)/2;
            const double actual_y=eye_height*(1-py*actual.y/(-actual.z)-oy)/2;
            const double affine_x=vx+vw*(sx+1)/2,affine_y=vy+vh*(1-sy)/2;
            if(!std::isfinite(actual_x)||!std::isfinite(actual_y)||std::abs(actual_x-affine_x)>1e-4||std::abs(actual_y-affine_y)>1e-4)return false;
        }
        result.viewports[eye]={float(vx),float(vy),float(vw),float(vh)};
        auto const& viewport=result.viewports[eye];
        if(!std::isfinite(viewport.x)||!std::isfinite(viewport.y)||!std::isfinite(viewport.width)||!std::isfinite(viewport.height)||
           viewport.width<=0||viewport.height<=0||viewport.width>32767||viewport.height>32767||viewport.x<-32768||viewport.y<-32768||
           double(viewport.x)+viewport.width>32767||double(viewport.y)+viewport.height>32767)return false;
        // Clip with the actual float viewport sent to D3D, so quantization cannot
        // shrink the conservative integer scissor at an exact pixel boundary.
        const double clip_l=(std::max)(origin,double(viewport.x)+viewport.width*source_left/full_width);
        const double clip_r=(std::min)(origin+eye_width,double(viewport.x)+viewport.width*source_right/full_width);
        const double clip_t=(std::max)(0.,double(viewport.y)+viewport.height*source_top/full_height);
        const double clip_b=(std::min)(eye_height,double(viewport.y)+viewport.height*source_bottom/full_height);
        if(!(clip_r>clip_l&&clip_b>clip_t))return false;
        const double sl=std::floor(clip_l),st=std::floor(clip_t),sr=std::ceil(clip_r),sb=std::ceil(clip_b);
        result.scissors[eye]={float(sl),float(st),float(sr-sl),float(sb-st)};
    }
    result.valid=true;out=result;return true;
}
} // namespace gears_ui_hud
