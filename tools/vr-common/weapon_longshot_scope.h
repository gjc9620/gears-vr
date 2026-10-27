#pragma once
#include "pose_math.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>

namespace gears_weapon::longshot_scope {
using gears_vr::Pose;
using gears_vr::Vector;

// An independent scene-rendering plan, not a main-eye image crop or GPU pass.
// All world positions and lengths are metres in one frozen world frame. Optics
// local axes are +X right, +Y up, -Z forward; the ocular is the z=0 plane.
// The live adapter must convert the actual weapon/eye poses into that frame.
//
// Existing evidence only establishes b_LS_Scope/b_LS_LensCap declarations:
// docs/weapon-6dof-interaction-design-20261022.md records neither as active in
// LongShot LOD0. It provides no measured lens aperture, eye relief, pupil
// transfer, magnification or lens transform. Consequently there is no retail
// Longshot preset here. Every optical value below needs separate calibration.
struct Optics {
    Pose ocular_in_weapon{};
    double ocular_radius_m{};
    double objective_radius_m{};
    double objective_distance_m{}; // Ocular to entrance-pupil plane, forward.
    double rear_baffle_distance_m{},rear_baffle_radius_m{}; // Toward the eye.
    double eye_relief_min_m{},eye_relief_max_m{};
    double exit_pupil_radius_m{},eye_pupil_radius_m{};
    double magnification{}; // First-order angular magnification, >1.
    // Lateral entrance-pupil offset / lateral eye offset; explicit rather
    // than assuming the original game's telescope has a known optical design.
    double pupil_transfer{};
    double parallax_focus_distance_m{}; // From entrance plane, along its axis.
    // Apparent focal plane forward of ocular; +infinity means collimated.
    // This controls binocular vergence, independently of scene focus above.
    double virtual_image_distance_m{};
    double near_m{},far_m{}; // Independent camera depth range, normal Z [0,1].
    unsigned render_width{},render_height{}; // Dedicated scene render target.
};

struct FrameKey {
    std::uint64_t session{},sequence{},space_epoch{};
    std::int64_t predicted_time{}; // Opaque XrTime; only equality is used.
};
inline bool same_key(FrameKey a,FrameKey b) noexcept {
    return a.session==b.session&&a.sequence==b.sequence&&
        a.space_epoch==b.space_epoch&&a.predicted_time==b.predicted_time;
}
inline bool key_valid(FrameKey k) noexcept {
    return k.session&&k.sequence&&k.space_epoch&&k.predicted_time!=0;
}

enum class Visibility : unsigned {unknown,clear,blocked};
struct VisibilityProof {
    // Adapter-supplied coarse collision results in this same frozen frame.
    // These are NOT a substitute for per-pixel lens depth/mask testing.
    Visibility eye_to_ocular{Visibility::unknown};
    Visibility ocular_to_objective{Visibility::unknown};
};
struct WeaponFrame {
    FrameKey key{};
    std::int64_t received_ns{}; // Monotonic host clock, not predicted_time.
    std::uint64_t weapon{},world_generation{};
    Pose world_pose_m{};
    bool longshot_active{};
};
struct EyeFrame {
    FrameKey key{};
    std::int64_t received_ns{};
    std::uint64_t world_generation{}; // World conversion used for these eyes.
    std::array<Pose,2> world_pose_m{}; // Same indexing as the normal XR eyes.
    unsigned tracked_mask{};
    std::array<VisibilityProof,2> visibility{};
};

struct Point2 {double x{},y{};};
struct Ray {Vector origin{},direction{};};
struct Segment {Vector start{},end{};};
// Row-major mathematical matrix acting on a COLUMN vector. This intentionally
// differs from the existing native row-vector StereoMatrix: the renderer must
// explicitly convert axes, metres, storage order and reversed-Z if required.
using Matrix4=std::array<double,16>;
struct Frustum {
    double left_tan{},right_tan{},bottom_tan{},top_tan{},near_m{},far_m{};
};
struct Camera {
    Vector position_m{},right{},up{},forward{};
    Frustum frustum{};
    Matrix4 view{},projection{}; // RH, -Z forward, D3D depth 0 near / 1 far.
    unsigned render_width{},render_height{};
    Point2 tangent_per_texel{};
};
struct Aperture {
    Vector ocular_center_m{},right{},up{},forward{};
    Vector eye_in_optics_m{}; // z>0 is behind ocular, in the eye box.
    double radius_m{},baffle_distance_m{},baffle_radius_m{};
};
enum class EyeStatus {
    untracked,wrong_side,outside_eye_relief,outside_pupil,objective_stop,
    occlusion_unresolved,occluded,render_candidate
};
struct EyePlan {
    EyeStatus status{EyeStatus::untracked};
    bool camera_valid{},optically_eligible{},render_scene{};
    double eye_relief_m{},pupil_overlap_fraction{};
    Camera camera{};
    Aperture aperture{};
    // Ocular UV of the optical-axis image at the configured scene focus.
    // This is an optical reference, NOT the weapon's authoritative aim point.
    Point2 optical_center_uv{};
    Segment eye_to_ocular{},ocular_to_objective{};
};
struct Plan {
    FrameKey key{};
    std::uint64_t weapon{},world_generation{};
    Pose ocular_world_m{};
    Ray optical_axis{}; // Never use this as proof of the actual firing ray.
    std::array<EyePlan,2> eyes{};
};

namespace detail {
inline Vector add(Vector a,Vector b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vector sub(Vector a,Vector b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vector scale(Vector a,double s) noexcept {return {a.x*s,a.y*s,a.z*s};}
inline double dot(Vector a,Vector b) noexcept {return a.x*b.x+a.y*b.y+a.z*b.z;}
inline bool positive(double x) noexcept {return std::isfinite(x)&&x>0;}
inline bool valid_pose(Pose const& p) noexcept {
    auto q=p.orientation;
    const double n=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
    return gears_vr::finite(p.position)&&std::isfinite(n)&&std::abs(n-1)<=.001;
}
inline bool known(Visibility v) noexcept {
    return v==Visibility::unknown||v==Visibility::clear||v==Visibility::blocked;
}
inline bool fresh(std::int64_t received,std::int64_t now) noexcept {
    return now>0&&received>0&&received<=now&&now-received<=100000000;
}
inline bool finite_matrix(Matrix4 const& m) noexcept {
    for(double x:m)if(!std::isfinite(x))return false;
    return true;
}
// Uniform circular eye pupil / exit pupil overlap at an axial eye-box slice.
// It is an analytic vignette weight, not measured optical transmission.
inline double pupil_overlap(double eye_radius,double exit_radius,double d) noexcept {
    if(d>=eye_radius+exit_radius)return 0;
    constexpr double pi=3.14159265358979323846;
    if(d<=std::abs(exit_radius-eye_radius)){
        if(exit_radius>=eye_radius)return 1;
        const double ratio=exit_radius/eye_radius;return ratio*ratio;
    }
    const double unit=(std::max)({eye_radius,exit_radius,d});
    const double er=eye_radius/unit,xr=exit_radius/unit,separation=d/unit;
    const double a=(separation*separation+er*er-xr*xr)/(2*separation*er);
    const double b=(separation*separation+xr*xr-er*er)/(2*separation*xr);
    const double k=(-separation+er+xr)*(separation+er-xr)*
        (separation-er+xr)*(separation+er+xr);
    const double area=er*er*std::acos((std::clamp)(a,-1.,1.))+
        xr*xr*std::acos((std::clamp)(b,-1.,1.))-
        .5*std::sqrt((std::max)(0.,k));
    return (std::clamp)(area/(pi*er*er),0.,1.);
}
inline Matrix4 make_view(Vector p,Vector r,Vector u,Vector f) noexcept {
    return {r.x,r.y,r.z,-dot(r,p),u.x,u.y,u.z,-dot(u,p),
        -f.x,-f.y,-f.z,dot(f,p),0,0,0,1};
}
inline Matrix4 make_projection(Frustum f) noexcept {
    const double w=f.right_tan-f.left_tan,h=f.top_tan-f.bottom_tan;
    const double z=f.far_m-f.near_m;
    return {2/w,0,(f.right_tan+f.left_tan)/w,0,
        0,2/h,(f.top_tan+f.bottom_tan)/h,0,
        0,0,-f.far_m/z,-f.far_m*f.near_m/z,0,0,-1,0};
}
} // namespace detail

inline bool optics_valid(Optics const& o) noexcept {
    using namespace detail;
    if(!valid_pose(o.ocular_in_weapon))return false;
    for(double x:{o.ocular_radius_m,o.objective_radius_m,o.objective_distance_m,
        o.rear_baffle_distance_m,o.rear_baffle_radius_m,o.eye_relief_min_m,
        o.eye_relief_max_m,o.exit_pupil_radius_m,o.eye_pupil_radius_m,
        o.magnification,o.pupil_transfer,o.parallax_focus_distance_m,o.near_m,o.far_m})
        if(!positive(x))return false;
    return positive(o.exit_pupil_radius_m+o.eye_pupil_radius_m)&&
        o.magnification>1&&o.eye_relief_max_m>o.eye_relief_min_m&&
        o.rear_baffle_distance_m<o.eye_relief_min_m&&
        o.near_m<o.parallax_focus_distance_m&&o.parallax_focus_distance_m<o.far_m&&
        o.near_m<o.far_m&&
        ((std::isfinite(o.virtual_image_distance_m)&&o.virtual_image_distance_m>0)||
         o.virtual_image_distance_m==std::numeric_limits<double>::infinity())&&
        o.render_width>=16&&o.render_width<=16384&&
        o.render_height>=16&&o.render_height<=16384;
}

// UV is top-left texture convention. The actual aperture is a circular world
// plane plus a rear housing stop, NOT a screen-space circle at HMD centre.
// Renderer contract: test this for each lens fragment, depth-test the physical
// ocular surface in the corresponding normal eye, retain the opaque housing,
// and composite only that eye's matching independent scene/depth result.
inline bool aperture_contains(Aperture const& a,Point2 uv) noexcept {
    if(!std::isfinite(uv.x)||!std::isfinite(uv.y)||
       !detail::positive(a.radius_m)||!detail::positive(a.baffle_radius_m)||
       !detail::positive(a.baffle_distance_m)||!gears_vr::finite(a.eye_in_optics_m)||
       a.eye_in_optics_m.z<=a.baffle_distance_m)return false;
    const double nx=2*uv.x-1,ny=1-2*uv.y;
    if(std::hypot(nx,ny)>1)return false;
    const double x=nx*a.radius_m,y=ny*a.radius_m;
    const double t=a.baffle_distance_m/a.eye_in_optics_m.z;
    const double bx=x+(a.eye_in_optics_m.x-x)*t;
    const double by=y+(a.eye_in_optics_m.y-y)*t;
    return std::hypot(bx,by)<=a.baffle_radius_m;
}
inline bool ocular_point(Aperture const& a,Point2 uv,Vector& out) noexcept {
    if(!aperture_contains(a,uv))return false;
    const auto p=detail::add(a.ocular_center_m,detail::add(
        detail::scale(a.right,(2*uv.x-1)*a.radius_m),
        detail::scale(a.up,(1-2*uv.y)*a.radius_m)));
    if(!gears_vr::finite(p))return false;
    out=p;return true;
}

// All-or-nothing frame construction. No previous result is reused on failure;
// the caller must discard any old scope texture when this returns false or
// when its renderer ticket no longer matches Plan::key/weapon/world_generation.
// A successful result only describes math; render_scene also requires BOTH
// coarse visibility proofs. It does not certify collision, GPU or headset QA.
inline bool build_plan(Optics const& o,WeaponFrame const& weapon,EyeFrame const& eyes,
        std::int64_t now_ns,Plan& out) {
    using namespace detail;
    if(!optics_valid(o)||!weapon.longshot_active||!weapon.weapon||!weapon.world_generation||
       !key_valid(weapon.key)||!same_key(weapon.key,eyes.key)||
       weapon.world_generation!=eyes.world_generation||
       !fresh(weapon.received_ns,now_ns)||!fresh(eyes.received_ns,now_ns)||
       !valid_pose(weapon.world_pose_m)||(eyes.tracked_mask&~3u))return false;
    for(unsigned i=0;i<2;++i) {
        auto v=eyes.visibility[i];
        if(!known(v.eye_to_ocular)||!known(v.ocular_to_objective)||
           ((eyes.tracked_mask&(1u<<i))&&!valid_pose(eyes.world_pose_m[i])))return false;
    }
    const auto weapon_rotation=gears_vr::rotation_matrix(weapon.world_pose_m.orientation);
    Plan result{};result.key=weapon.key;result.weapon=weapon.weapon;
    result.world_generation=weapon.world_generation;
    result.ocular_world_m.position=add(weapon.world_pose_m.position,
        gears_vr::transform(weapon_rotation,o.ocular_in_weapon.position));
    result.ocular_world_m.orientation=gears_vr::normalized(gears_vr::multiply(
        weapon.world_pose_m.orientation,o.ocular_in_weapon.orientation));
    const auto rotation=gears_vr::rotation_matrix(result.ocular_world_m.orientation);
    const auto inverse=gears_vr::transpose(rotation);
    const auto r=gears_vr::transform(rotation,{1,0,0});
    const auto u=gears_vr::transform(rotation,{0,1,0});
    const auto f=gears_vr::transform(rotation,{0,0,-1});
    const auto objective=add(result.ocular_world_m.position,scale(f,o.objective_distance_m));
    result.optical_axis={objective,f};
    if(!valid_pose(result.ocular_world_m)||!gears_vr::finite(objective))return false;
    for(unsigned i=0;i<2;++i) {
        if(!(eyes.tracked_mask&(1u<<i)))continue;
        auto& e=result.eyes[i];
        const auto eye=eyes.world_pose_m[i].position;
        const auto local=gears_vr::transform(inverse,sub(eye,result.ocular_world_m.position));
        e.aperture={result.ocular_world_m.position,r,u,f,local,o.ocular_radius_m,
            o.rear_baffle_distance_m,o.rear_baffle_radius_m};
        e.eye_relief_m=local.z;
        e.eye_to_ocular={eye,result.ocular_world_m.position};
        e.ocular_to_objective={result.ocular_world_m.position,objective};
        if(!gears_vr::finite(local))return false;
        if(local.z<=0){e.status=EyeStatus::wrong_side;continue;}
        if(local.z<o.eye_relief_min_m||local.z>o.eye_relief_max_m){
            e.status=EyeStatus::outside_eye_relief;continue;}
        const double pupil_distance=std::hypot(local.x,local.y);
        e.pupil_overlap_fraction=pupil_overlap(o.eye_pupil_radius_m,o.exit_pupil_radius_m,pupil_distance);
        if(!std::isfinite(e.pupil_overlap_fraction))return false;
        if(e.pupil_overlap_fraction<=0){e.status=EyeStatus::outside_pupil;continue;}
        const double px=local.x*o.pupil_transfer,py=local.y*o.pupil_transfer;
        // A centre ray outside the objective cannot become a valid scene view
        // merely because the exit pupil admits a small part of the eye pupil.
        if(std::hypot(px,py)>=o.objective_radius_m){e.status=EyeStatus::objective_stop;continue;}
        const double image_factor=std::isinf(o.virtual_image_distance_m)?1.:
            1/(1+local.z/o.virtual_image_distance_m);
        const double image_x=local.x*image_factor,image_y=local.y*image_factor;
        const double scale_to_scene=local.z*o.magnification;
        const double half=o.ocular_radius_m/scale_to_scene;
        // First-order optical mapping, for entrance-plane scene depth D:
        // lens_x = image_x + E*M*((X-px)/D + px/P), likewise for Y.
        // P is scene parallax focus. image_x gives the requested apparent
        // virtual-image depth, preventing a reticle at ocular-plane vergence.
        // Both terms are necessary: a shared centred texture loses eye motion.
        const double cx=-image_x/scale_to_scene-px/o.parallax_focus_distance_m;
        const double cy=-image_y/scale_to_scene-py/o.parallax_focus_distance_m;
        auto& c=e.camera;
        c.position_m=add(objective,add(scale(r,px),scale(u,py)));
        c.right=r;c.up=u;c.forward=f;
        c.frustum={cx-half,cx+half,cy-half,cy+half,o.near_m,o.far_m};
        c.view=make_view(c.position_m,r,u,f);c.projection=make_projection(c.frustum);
        c.render_width=o.render_width;c.render_height=o.render_height;
        c.tangent_per_texel={2*half/o.render_width,2*half/o.render_height};
        e.optical_center_uv={.5+image_x/(2*o.ocular_radius_m),.5-image_y/(2*o.ocular_radius_m)};
        if(!gears_vr::finite(c.position_m)||!positive(half)||
           !positive(c.frustum.right_tan-c.frustum.left_tan)||
           !positive(c.frustum.top_tan-c.frustum.bottom_tan)||
           !finite_matrix(c.view)||!finite_matrix(c.projection)||
           !std::isfinite(e.optical_center_uv.x)||!std::isfinite(e.optical_center_uv.y))return false;
        e.camera_valid=true;e.optically_eligible=true;
        const auto proof=eyes.visibility[i];
        if(proof.eye_to_ocular==Visibility::blocked||proof.ocular_to_objective==Visibility::blocked)
            e.status=EyeStatus::occluded;
        else if(proof.eye_to_ocular==Visibility::unknown||proof.ocular_to_objective==Visibility::unknown)
            e.status=EyeStatus::occlusion_unresolved;
        else {e.status=EyeStatus::render_candidate;e.render_scene=true;}
    }
    out=result;return true;
}

struct TargetProjection {
    Point2 uv{};double depth_m{};bool in_frustum{},inside_aperture{};
};
// Feed the authoritative aim-line SCENE INTERSECTION here for a live reticle.
// A muzzle animation, HMD forward or optical_axis is not such evidence. No
// collision or aim-zero correction is inferred. Both eyes project independently.
inline bool project_world_point(EyePlan const& eye,Vector point_m,TargetProjection& out) noexcept {
    using namespace detail;
    if(!eye.camera_valid||!gears_vr::finite(point_m))return false;
    auto const& c=eye.camera;const auto delta=sub(point_m,c.position_m);
    const double depth=dot(delta,c.forward);
    if(!positive(depth))return false;
    const auto& f=c.frustum;const double w=f.right_tan-f.left_tan,h=f.top_tan-f.bottom_tan;
    if(!positive(w)||!positive(h))return false;
    TargetProjection p{};p.depth_m=depth;
    p.uv={(dot(delta,c.right)/depth-f.left_tan)/w,
        (f.top_tan-dot(delta,c.up)/depth)/h};
    if(!std::isfinite(p.uv.x)||!std::isfinite(p.uv.y))return false;
    p.in_frustum=p.uv.x>=0&&p.uv.x<=1&&p.uv.y>=0&&p.uv.y<=1&&depth>=f.near_m&&depth<=f.far_m;
    p.inside_aperture=p.in_frustum&&aperture_contains(eye.aperture,p.uv);
    out=p;return true;
}
inline bool scene_ray(EyePlan const& eye,Point2 uv,Ray& out) noexcept {
    if(!eye.camera_valid||!aperture_contains(eye.aperture,uv))return false;
    const auto& c=eye.camera;const auto& f=c.frustum;
    auto d=detail::add(c.forward,detail::add(
        detail::scale(c.right,f.left_tan+uv.x*(f.right_tan-f.left_tan)),
        detail::scale(c.up,f.top_tan-uv.y*(f.top_tan-f.bottom_tan))));
    const double n=std::hypot(std::hypot(d.x,d.y),d.z);
    if(!detail::positive(n))return false;
    out={c.position_m,detail::scale(d,1/n)};return true;
}
} // namespace gears_weapon::longshot_scope
