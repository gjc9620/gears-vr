#pragma once
#include "ui_projection.h"

namespace gears_vr {
struct UiPlanePoint {double x{},y{};};
struct UiPlaneEye {
    // Row vector [source_x,source_y,1] * H = [pixel_x*w,pixel_y*w,w].
    std::array<double,9> homography{};
    std::array<UiEyeProjection,4> points{};
    bool affine{},viewport_scale_translation{};
};
struct UiPlaneProjection {
    std::array<Vector,4> world_points{};
    std::array<UiPlaneEye,2> eyes{};
};
namespace ui_plane_detail {
inline bool inverse_rotation(StereoMatrix const& view,double (&out)[3][3]){
    const double a=view[0],b=view[1],c=view[2],d=view[4],e=view[5],f=view[6],g=view[8],h=view[9],i=view[10];
    const double det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    if(!std::isfinite(det)||det==0)return false;
    // Actual quantized rigid matrix inverse, not an approximate transpose.
    const double values[3][3]{{(e*i-f*h)/det,(c*h-b*i)/det,(b*f-c*e)/det},
        {(f*g-d*i)/det,(a*i-c*g)/det,(c*d-a*f)/det},
        {(d*h-e*g)/det,(b*g-a*h)/det,(a*e-b*d)/det}};
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col){out[row][col]=values[row][col];if(!std::isfinite(out[row][col]))return false;}
    return true;
}
}
// Source-camera plane at caller-chosen +Z DISTANCE in native game units, not
// radial distance. Original source view orientation/position and asymmetric
// projection are preserved. No latest pose, world aim point, HUD depth policy,
// physical scale, occlusion, ticket or mutation authority is supplied here.
// Fixed four-point mapping: commit only if every world point and both-eye
// projections are finite/front. Outside XY succeeds with its existing flag.
inline bool project_ui_plane(StereoMatrix const& source_view,StereoMatrix const& source_projection,
    StereoMatrices const& frozen,unsigned width,unsigned height,double distance,
    std::array<UiPlanePoint,4> const& source_points,UiPlaneProjection& out){
    if(!width||!height||!std::isfinite(distance)||distance<=0||
       !stereo_rigid(source_view)||!stereo_projection_supported(source_projection))return false;
    for(unsigned eye=0;eye<2;++eye)if(!stereo_rigid(frozen.view[eye])||!stereo_projection_supported(frozen.projection[eye]))return false;
    double inverse[3][3]{};if(!ui_plane_detail::inverse_rotation(source_view,inverse))return false;
    // Source top-left pixels unproject to camera coordinates on z=distance.
    const double camera[3][3]{{2*distance/(double(width)*source_projection[0]),0,0},
        {0,-2*distance/(double(height)*source_projection[5]),0},
        {distance*(-1-double(source_projection[8]))/source_projection[0],
         distance*(1-double(source_projection[9]))/source_projection[5],distance}};
    double world[3][4]{};
    for(unsigned row=0;row<3;++row){world[row][3]=row==2?1:0;
        for(unsigned col=0;col<3;++col){for(unsigned k=0;k<3;++k)
            world[row][col]+=(camera[row][k]-(row==2?source_view[12+k]:0))*inverse[k][col];
            if(!std::isfinite(world[row][col]))return false;}}
    UiPlaneProjection candidate{};
    for(unsigned n=0;n<4;++n){auto const point=source_points[n];if(!std::isfinite(point.x)||!std::isfinite(point.y))return false;
        auto& p=candidate.world_points[n];p={point.x*world[0][0]+point.y*world[1][0]+world[2][0],
            point.x*world[0][1]+point.y*world[1][1]+world[2][1],point.x*world[0][2]+point.y*world[1][2]+world[2][2]};
        UiStereoProjection projected{};if(!project_ui_anchor(frozen,p,width,height,projected))return false;
        for(unsigned eye=0;eye<2;++eye)candidate.eyes[eye].points[n]=projected.eyes[eye];
    }
    for(unsigned eye=0;eye<2;++eye){double view[3][4]{},clip[3][4]{};
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col){
            for(unsigned k=0;k<4;++k)view[row][col]+=world[row][k]*frozen.view[eye][4*k+col];
        }
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col){clip[row][col]=0;
            for(unsigned k=0;k<4;++k)clip[row][col]+=view[row][k]*frozen.projection[eye][4*k+col];}
        auto& result=candidate.eyes[eye];auto& h=result.homography;
        for(unsigned row=0;row<3;++row){h[3*row]=(clip[row][0]+clip[row][3])*.5*width;
            h[3*row+1]=(clip[row][3]-clip[row][1])*.5*height;h[3*row+2]=clip[row][3];}
        for(auto value:h)if(!std::isfinite(value))return false;
        // Strict coefficient tests, not a tolerance that calls a small tilt exact.
        result.affine=h[2]==0&&h[5]==0;
        result.viewport_scale_translation=result.affine&&h[1]==0&&h[3]==0&&h[8]>0&&h[0]/h[8]>0&&h[4]/h[8]>0;
    }
    out=candidate;return true;
}
// Full source-canvas corners, in the existing Canvas Tile vertex order.
inline bool project_ui_plane(StereoMatrix const& source_view,StereoMatrix const& source_projection,
    StereoMatrices const& frozen,unsigned width,unsigned height,double distance,UiPlaneProjection& out){
    return project_ui_plane(source_view,source_projection,frozen,width,height,distance,
        {{{0,0},{double(width),0},{0,double(height)},{double(width),double(height)}}},out);
}
// Relative yaw/pitch generally makes H's denominator vary with source XY;
// relative roll can be affine with cross terms. Neither is, in general, an
// exact viewport scale/translation. Homography and per-point W are supplied;
// texture interpolation, raster clipping and the actual GPU route are separate.
}
