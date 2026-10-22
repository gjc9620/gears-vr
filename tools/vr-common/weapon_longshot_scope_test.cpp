#include "weapon_longshot_scope.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

using namespace gears_weapon::longshot_scope;
using gears_vr::Quaternion;
namespace {
unsigned checks{};
void check(bool ok,char const* message) {
    ++checks;
    if(!ok){std::fprintf(stderr,"Longshot scope check failed: %s\n",message);std::abort();}
}
bool close(double a,double b,double eps=1e-9){return std::abs(a-b)<=eps;}
bool close(Vector a,Vector b,double eps=1e-9){return close(a.x,b.x,eps)&&close(a.y,b.y,eps)&&close(a.z,b.z,eps);}
bool close(Point2 a,Point2 b,double eps=1e-9){return close(a.x,b.x,eps)&&close(a.y,b.y,eps);}
bool close(Matrix4 const& a,Matrix4 const& b,double eps=1e-9){
    for(unsigned i=0;i<16;++i)if(!close(a[i],b[i],eps))return false;
    return true;
}
Vector add(Vector a,Vector b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vector sub(Vector a,Vector b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vector mul(Vector a,double s){return {a.x*s,a.y*s,a.z*s};}
double length(Vector a){return std::hypot(std::hypot(a.x,a.y),a.z);}
Vector rotated(Quaternion q,Vector v){return gears_vr::transform(gears_vr::rotation_matrix(q),v);}
Vector moved(Vector p,Quaternion q,Vector t){return add(rotated(q,p),t);}
Quaternion axis_quat(Vector axis,double angle){
    const double s=std::sin(angle/2)/length(axis);
    return {axis.x*s,axis.y*s,axis.z*s,std::cos(angle/2)};
}
Optics synthetic_optics(){
    // Synthetic optics only. These are NOT extracted Longshot dimensions.
    Optics o{};
    o.ocular_radius_m=.020;o.objective_radius_m=.040;o.objective_distance_m=.250;
    o.rear_baffle_distance_m=.008;o.rear_baffle_radius_m=.020;
    o.eye_relief_min_m=.060;o.eye_relief_max_m=.120;
    o.exit_pupil_radius_m=.004;o.eye_pupil_radius_m=.002;
    o.magnification=4;o.pupil_transfer=4;
    o.parallax_focus_distance_m=100;
    o.virtual_image_distance_m=std::numeric_limits<double>::infinity();
    o.near_m=.1;o.far_m=10000;o.render_width=1024;o.render_height=1024;
    return o;
}
struct Fixture {
    Optics optics{synthetic_optics()};
    WeaponFrame weapon{};
    EyeFrame eyes{};
    std::int64_t now{1000000000};
    Fixture(){
        weapon.key={3,7,2,456};weapon.received_ns=now;weapon.weapon=0x1234;
        weapon.world_generation=5;weapon.longshot_active=true;
        eyes.key=weapon.key;eyes.received_ns=now;eyes.world_generation=5;eyes.tracked_mask=3;
        for(auto& v:eyes.visibility)v={Visibility::clear,Visibility::clear};
        set_eye(0,{-.064,0,.09});set_eye(1,{0,0,.09});
    }
    Pose ocular() const {
        return {add(weapon.world_pose_m.position,
                    rotated(weapon.world_pose_m.orientation,optics.ocular_in_weapon.position)),
            gears_vr::normalized(gears_vr::multiply(weapon.world_pose_m.orientation,
                    optics.ocular_in_weapon.orientation))};
    }
    Vector world(Vector p) const {auto a=ocular();return moved(p,a.orientation,a.position);}
    Vector target(double distance,Vector xy={}) const {
        return world({xy.x,xy.y,-optics.objective_distance_m-distance});
    }
    void set_eye(unsigned i,Vector p){eyes.world_pose_m[i]={world(p),ocular().orientation};}
    Plan build() const {
        Plan p{};check(build_plan(optics,weapon,eyes,now,p),"valid synthetic frozen frame");return p;
    }
};
TargetProjection project(EyePlan const& eye,Vector world){
    TargetProjection p{};check(project_world_point(eye,world,p),"project front world point");return p;
}
std::array<double,4> transform4(Matrix4 const& m,std::array<double,4> a){
    std::array<double,4> r{};
    for(unsigned i=0;i<4;++i)for(unsigned k=0;k<4;++k)r[i]+=m[4*i+k]*a[k];
    return r;
}
void test_independent_camera_and_other_eye(){
    Fixture f;const auto p=f.build();
    check(p.eyes[0].status==EyeStatus::outside_pupil&&!p.eyes[0].render_scene,
        "non-aiming eye keeps normal world view outside physical exit pupil");
    const auto& e=p.eyes[1];
    check(e.camera_valid&&e.optically_eligible&&e.render_scene,"aiming eye requests independent scene pass");
    check(close(e.camera.position_m,{0,0,-.25}),"camera starts at objective, not HMD");
    check(close(e.camera.forward,{0,0,-1}),"camera follows gun optical axis");
    check(close(e.camera.frustum.right_tan,1./18),"narrow scene field from physical lens and 4x optics");
    check(e.camera.render_width==1024&&e.camera.render_height==1024,"dedicated scene target dimensions");
    check(close(e.pupil_overlap_fraction,1),"centred eye pupil fully illuminated");
    const auto focus=project(e,f.target(100));
    check(close(focus.uv,{.5,.5})&&focus.inside_aperture,"optical axis projects through lens centre");
    f.eyes.world_pose_m[1].orientation=axis_quat({1,1,0},1.1);
    const auto turned_head=f.build();
    check(close(turned_head.optical_axis.direction,p.optical_axis.direction)&&
          close(turned_head.eyes[1].camera.view,e.camera.view)&&
          close(turned_head.eyes[1].camera.projection,e.camera.projection),
        "head rotation alone cannot steer gun or scope scene camera");
    f.optics.render_width=2048;f.optics.render_height=2048;
    const auto high=f.build().eyes[1].camera;
    check(close(high.view,e.camera.view)&&close(high.projection,e.camera.projection),
        "higher independent render resolution retains optical geometry");
    check(close(high.tangent_per_texel.x*2,e.camera.tangent_per_texel.x),
        "doubling independent scene resolution halves angular texel step");
}
void test_eye_motion_focus_and_parallax(){
    Fixture f;f.set_eye(1,{.001,-.0005,.09});auto p=f.build();auto const& e=p.eyes[1];
    check(close(e.camera.position_m,{.004,-.002,-.25}),"eye movement transfers to entrance-pupil camera");
    check(close(e.camera.forward,p.optical_axis.direction),"lateral eye movement never turns gun axis");
    const auto focus=project(e,f.target(100));
    check(close(focus.uv,e.optical_center_uv)&&close(focus.uv,{.525,.5125}),
        "infinity-focus image moves on lens to retain parallel outgoing rays");
    Vector lens{};check(ocular_point(e.aperture,focus.uv,lens),"focus image lies in physical lens aperture");
    const auto eye=f.eyes.world_pose_m[1].position;
    const auto outgoing=sub(lens,eye);
    check(close(outgoing.x,0)&&close(outgoing.y,0)&&outgoing.z<0,
        "collimated virtual image has no ocular-plane stereo convergence");
    const auto near=project(e,f.target(50)),far=project(e,f.target(200));
    check(near.uv.x<focus.uv.x&&far.uv.x>focus.uv.x,
        "near/far targets show opposite eye-box parallax around scene focus");
    const double half=e.camera.frustum.right_tan-e.camera.frustum.left_tan;
    check(close(near.uv.x-focus.uv.x,.004*(1./100-1./50)/half),
        "quantitative parallax derives from independent scene baseline");
    f.optics.virtual_image_distance_m=12;
    auto finite=f.build();const auto target=project(finite.eyes[1],f.target(100));
    check(ocular_point(finite.eyes[1].aperture,target.uv,lens),"finite virtual image lens point");
    const double t=(-12-eye.z)/(lens.z-eye.z);
    check(close(add(eye,mul(sub(lens,eye),t)),{0,0,-12},1e-8),
        "physical eye/lens ray converges at configured virtual focal plane");
    f.set_eye(1,{0,0,.11});const auto farther_eye=f.build();
    check(farther_eye.eyes[1].camera.frustum.right_tan<1./18,
        "axial eye movement changes visible field through fixed aperture");
}
void test_binocular_views_are_distinct(){
    Fixture f;
    // Deliberately enlarged synthetic exit/ocular stops allow both eye centres.
    // This tests stereo math and is not a claim that Longshot admits both eyes.
    f.optics.exit_pupil_radius_m=.04;f.optics.objective_radius_m=.2;
    f.optics.ocular_radius_m=.05;f.optics.rear_baffle_radius_m=.05;
    f.optics.virtual_image_distance_m=8;
    f.set_eye(0,{-.032,0,.09});f.set_eye(1,{.032,0,.09});
    const auto p=f.build();
    check(p.eyes[0].render_scene&&p.eyes[1].render_scene,"both synthetic eye boxes admitted independently");
    check(!close(p.eyes[0].camera.view,p.eyes[1].camera.view)&&
          !close(p.eyes[0].camera.projection,p.eyes[1].camera.projection),
        "two eyes never share a copied centred scene texture");
    check(close(length(sub(p.eyes[1].camera.position_m,p.eyes[0].camera.position_m)),.256),
        "virtual entrance-pupil baseline follows calibrated pupil transfer");
    auto a=project(p.eyes[0],f.target(100)),b=project(p.eyes[1],f.target(100));
    Vector la{},lb{};
    check(ocular_point(p.eyes[0].aperture,a.uv,la)&&ocular_point(p.eyes[1].aperture,b.uv,lb),
        "both focal target projections reach their own lens positions");
    for(unsigned i=0;i<2;++i){
        const auto eye=f.eyes.world_pose_m[i].position,lens=i?lb:la;
        const double t=(-8-eye.z)/(lens.z-eye.z);
        check(close(add(eye,mul(sub(lens,eye),t)),{0,0,-8},1e-8),
            "both eye/lens rays converge at same configured virtual depth");
    }
    check(!close(a.uv,b.uv),"stereo target image has separate per-eye lens coordinates");
}
void test_world_rigid_transform_and_matrices(){
    Fixture f;
    f.optics.ocular_in_weapon={{.02,.1,-.3},axis_quat({0,0,1},.23)};
    f.set_eye(0,{-.064,0,.09});f.set_eye(1,{.001,.001,.09});
    const auto base=f.build();const Vector target=f.target(80,{.4,-.2,0});
    const auto before=project(base.eyes[1],target);
    const auto q=axis_quat({1,2,-3},1.2);const Vector translation{12,3,9};
    f.weapon.world_pose_m={translation,q};
    for(auto& eye:f.eyes.world_pose_m){
        eye.position=moved(eye.position,q,translation);
        eye.orientation=gears_vr::normalized(gears_vr::multiply(q,eye.orientation));
    }
    const auto transformed=f.build();const auto point=moved(target,q,translation);
    const auto after=project(transformed.eyes[1],point);
    check(close(before.uv,after.uv,1e-8)&&close(before.depth_m,after.depth_m),
        "full weapon/eyes/world rigid movement preserves optical projection");
    check(close(transformed.eyes[1].camera.position_m,
        moved(base.eyes[1].camera.position_m,q,translation),1e-8),"mounted camera respects weapon 6DoF pose");
    const auto& camera=transformed.eyes[1].camera;
    const auto clip=transform4(camera.projection,transform4(camera.view,{point.x,point.y,point.z,1}));
    check(close(Point2{(clip[0]/clip[3]+1)/2,(1-clip[1]/clip[3])/2},after.uv,1e-8),
        "explicit view/projection matrices agree with physical world projection");
    for(double depth:{camera.frustum.near_m,camera.frustum.far_m}){
        const auto p=add(camera.position_m,mul(camera.forward,depth));
        const auto c=transform4(camera.projection,transform4(camera.view,{p.x,p.y,p.z,1}));
        check(close(c[2]/c[3],depth==camera.frustum.near_m?0.:1.,1e-8),
            "normal D3D depth is zero at near and one at far");
    }
    Ray ray{};check(scene_ray(transformed.eyes[1],after.uv,ray),"unproject eligible lens texel to independent scene ray");
    const auto direction=mul(sub(point,ray.origin),1/length(sub(point,ray.origin)));
    check(close(ray.direction,direction,1e-8),"scene reconstruction ray intersects original world point");
}
void test_aperture_eye_box_and_occlusion(){
    Fixture f;auto p=f.build();
    check(aperture_contains(p.eyes[1].aperture,{.5,.5}),"central circular aperture open");
    check(!aperture_contains(p.eyes[1].aperture,{0,0}),"square render target corner is opaque outside lens circle");
    f.optics.rear_baffle_radius_m=.005;p=f.build();
    check(!aperture_contains(p.eyes[1].aperture,{.9,.5}),"rear housing blocks off-axis lens fragment");
    f=Fixture{};f.set_eye(1,{.004,0,.09});p=f.build();
    check(p.eyes[1].pupil_overlap_fraction>0&&p.eyes[1].pupil_overlap_fraction<1,
        "partial pupil overlap yields bounded per-eye vignette weight");
    f.set_eye(1,{.0061,0,.09});p=f.build();
    check(p.eyes[1].status==EyeStatus::outside_pupil&&!p.eyes[1].render_scene,
        "eye beyond exit-pupil/eye-pupil overlap loses scope image");
    f.set_eye(1,{0,0,.03});check(f.build().eyes[1].status==EyeStatus::outside_eye_relief,"eye too close rejected");
    f.set_eye(1,{0,0,.15});check(f.build().eyes[1].status==EyeStatus::outside_eye_relief,"eye too far rejected");
    f.set_eye(1,{0,0,-.1});check(f.build().eyes[1].status==EyeStatus::wrong_side,"eye in front of ocular rejected");
    f.set_eye(1,{.003,0,.09});f.optics.objective_radius_m=.01;
    check(f.build().eyes[1].status==EyeStatus::objective_stop,"entrance pupil outside objective rejected");
    f=Fixture{};f.eyes.visibility[1]={};p=f.build();
    check(p.eyes[1].camera_valid&&p.eyes[1].optically_eligible&&!p.eyes[1].render_scene&&
        p.eyes[1].status==EyeStatus::occlusion_unresolved,"unknown world occlusion never authorizes compositing");
    f.eyes.visibility[1]={Visibility::blocked,Visibility::clear};
    check(f.build().eyes[1].status==EyeStatus::occluded,"eye-to-lens wall blocks independent scene");
    f.eyes.visibility[1]={Visibility::clear,Visibility::blocked};
    check(f.build().eyes[1].status==EyeStatus::occluded,"blocked lens-to-objective path cannot see through walls");
    f=Fixture{};f.eyes.tracked_mask=2;f.eyes.world_pose_m[0].orientation={0,0,0,0};p=f.build();
    check(p.eyes[0].status==EyeStatus::untracked&&p.eyes[1].render_scene,
        "untracked non-aiming eye does not borrow the other eye image");
}
void test_authoritative_target_is_separate(){
    Fixture f;auto p=f.build();
    const Vector authoritative_hit=f.target(50,{.3,.1,0});
    const auto target=project(p.eyes[1],authoritative_hit);
    check(!close(target.uv,p.eyes[1].optical_center_uv)&&target.inside_aperture,
        "explicit firing-line scene intersection is projected separately from optical axis");
    const auto prior=target;
    f.eyes.world_pose_m[1].orientation=axis_quat({1,0,0},.7);p=f.build();
    check(close(project(p.eyes[1],authoritative_hit).uv,prior.uv),
        "looking up cannot alter weapon-line target supplied by adapter");
    TargetProjection unchanged{{.2,.3},42,true,true};
    check(!project_world_point(p.eyes[1],{0,0,1},unchanged)&&close(unchanged.uv,{.2,.3}),
        "target behind independent camera fails without changing output");
    const auto distant=project(p.eyes[1],f.target(f.optics.far_m*2));
    check(!distant.in_frustum&&!distant.inside_aperture,"target beyond scene far clip cannot draw reticle");
}
void test_invalid_frames_and_optics(){
    Fixture base;const auto sentinel=base.build();
    auto reject=[&](Fixture f,char const* reason){
        auto out=sentinel;
        check(!build_plan(f.optics,f.weapon,f.eyes,f.now,out),reason);
        check(out.weapon==sentinel.weapon&&same_key(out.key,sentinel.key)&&
              close(out.eyes[1].camera.view,sentinel.eyes[1].camera.view),"failed build leaves output untouched");
    };
    auto f=base;f.weapon.longshot_active=false;reject(f,"other weapon is not a Longshot view");
    f=base;f.weapon.weapon=0;reject(f,"missing weapon identity");
    f=base;f.weapon.world_generation=0;reject(f,"missing world generation");
    f=base;++f.eyes.world_generation;reject(f,"eye conversion from a different world generation");
    f=base;++f.eyes.key.sequence;reject(f,"mismatched source frame sequence");
    f=base;++f.eyes.key.space_epoch;reject(f,"mismatched recenter epoch");
    f=base;++f.eyes.key.session;reject(f,"mismatched XR session");
    f=base;++f.eyes.key.predicted_time;reject(f,"mismatched predicted display time");
    f=base;f.weapon.key.sequence=f.eyes.key.sequence=0;reject(f,"zero source sequence");
    f=base;f.weapon.received_ns-=100000001;reject(f,"stale weapon pose");
    f=base;f.eyes.received_ns-=100000001;reject(f,"stale eye poses");
    f=base;++f.eyes.received_ns;reject(f,"future monotonic receipt");
    f=base;f.eyes.tracked_mask=4;reject(f,"unknown eye tracking bits");
    f=base;f.eyes.world_pose_m[1].orientation={0,0,0,0};reject(f,"tracked invalid eye pose");
    f=base;f.weapon.world_pose_m.position.x=std::numeric_limits<double>::quiet_NaN();reject(f,"nonfinite weapon pose");
    f=base;f.eyes.visibility[1].eye_to_ocular=static_cast<Visibility>(5);reject(f,"unknown collision proof state");
    f=base;f.optics={};reject(f,"there is no implicit unverified retail optics preset");
    f=base;f.optics.magnification=1;reject(f,"main-view magnification is not a scope configuration");
    f=base;f.optics.pupil_transfer=0;reject(f,"uncalibrated entrance pupil mapping");
    f=base;f.optics.parallax_focus_distance_m=f.optics.near_m;reject(f,"parallax plane must lie inside scene clip range");
    f=base;f.optics.virtual_image_distance_m=std::numeric_limits<double>::quiet_NaN();reject(f,"invalid virtual image depth");
    f=base;f.optics.render_width=0;reject(f,"missing independent scene target");
    f=base;f.optics.rear_baffle_distance_m=f.optics.eye_relief_min_m;reject(f,"rear housing cannot cross eye box");
    f=base;f.optics.exit_pupil_radius_m=1e308;f.optics.eye_pupil_radius_m=1e308;
    reject(f,"overflowing pupil geometry must fail closed");
}
} // namespace
int main(){
    test_independent_camera_and_other_eye();test_eye_motion_focus_and_parallax();
    test_binocular_views_are_distinct();test_world_rigid_transform_and_matrices();
    test_aperture_eye_box_and_occlusion();test_authoritative_target_is_separate();
    test_invalid_frames_and_optics();
    std::printf("Longshot scope independent-camera, 6DoF, stereo, focus/parallax, mask/occlusion and frame gates: %u checks passed\n",checks);
}
