#include "weapon_boltok_aim_grip.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

namespace a=gears_boltok_aim_grip;namespace v=gears_vr;
unsigned checks{};
#define C(x) do{++checks;assert(x);}while(false)

constexpr double pi=3.14159265358979323846;
v::Quaternion qx(double a){return {std::sin(a/2),0,0,std::cos(a/2)};}
v::Quaternion qy(double a){return {0,std::sin(a/2),0,std::cos(a/2)};}
v::Quaternion qz(double a){return {0,0,std::sin(a/2),std::cos(a/2)};}
v::Vector add(v::Vector x,v::Vector y){return {x.x+y.x,x.y+y.y,x.z+y.z};}
v::Vector sub(v::Vector x,v::Vector y){return {x.x-y.x,x.y-y.y,x.z-y.z};}
double near(v::Vector x,v::Vector y){const auto d=sub(x,y);return std::hypot(std::hypot(d.x,d.y),d.z);}

struct Fixture {
    v::ControllerHand hand{};
    v::StereoMatrix base{};
    v::Pose reference{};
    v::Pose wrist_in_mesh{};
    v::Quaternion muzzle_in_mesh{};
    v::Vector marker{};
    double units_per_metre{100};
    Fixture(){
        hand.grip_active=hand.aim_active=1;
        hand.grip_flags=hand.aim_flags=15;
        hand.grip.position={.2,1.4,-.4};
        hand.grip.orientation={0,0,0,1};
        hand.aim.position={.25,1.45,-.35};
        hand.aim.orientation={0,0,0,1};
        base[0]=base[5]=base[10]=base[15]=1;
        reference={ {.03,-.02,.05},qx(.2) };
        wrist_in_mesh={{.03,-.01,.02},qy(-.35)};
        muzzle_in_mesh=qx(pi/2);
        marker={.02,-.03,.01};
    }
    a::Input input(){return {&hand,&base,&reference,units_per_metre,&wrist_in_mesh,&muzzle_in_mesh,&marker};}
};

void require_same(a::Result const& x,a::Result const& y){
    C(near(x.wrist_world.position,y.wrist_world.position)==0);
    C(near(x.mesh_world.position,y.mesh_world.position)==0);
    C(x.wrist_world.orientation.x==y.wrist_world.orientation.x&&x.wrist_world.orientation.y==y.wrist_world.orientation.y&&
      x.wrist_world.orientation.z==y.wrist_world.orientation.z&&x.wrist_world.orientation.w==y.wrist_world.orientation.w);
    C(x.mesh_world.orientation.x==y.mesh_world.orientation.x&&x.mesh_world.orientation.y==y.mesh_world.orientation.y&&
      x.mesh_world.orientation.z==y.mesh_world.orientation.z&&x.mesh_world.orientation.w==y.mesh_world.orientation.w);
}

int main(){
    {
        const auto pitch=qy(55*pi/180);
        const v::Vector local{-.054,.004,.002};
        gears_hand_wrist::Calibration authored{true,v::transform(v::rotation_matrix(pitch),local),pitch},neutral{};
        C(a::neutral_calibration(authored,neutral));
        C(near(neutral.grip_to_wrist_metres,local)<1e-10);
        C(near(v::transform(v::rotation_matrix(neutral.grip_to_wrist_rotation),{1,0,0}),{1,0,0})<1e-10);
    }
    // Independent neutral-world expectation: XR -Z and this view map to UE
    // +X. A local +Y bore must remain horizontal, with no inherited55deg fit.
    {
        Fixture neutral;
        neutral.base={0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,1};
        neutral.reference={};
        neutral.wrist_in_mesh.orientation=qz(pi/2);
        neutral.muzzle_in_mesh=qz(pi/2);
        a::Result result{};
        C(a::derive(neutral.input(),result));
        C(near(v::transform(v::rotation_matrix(result.mesh_world.orientation),{0,1,0}),{1,0,0})<1e-8);
        C(near(v::transform(v::rotation_matrix(result.wrist_world.orientation),{1,0,0}),{1,0,0})<1e-8);
        neutral.hand.aim.orientation=qx(pi/6);
        C(a::derive(neutral.input(),result));
        C(near(v::transform(v::rotation_matrix(result.mesh_world.orientation),{0,1,0}),{std::cos(pi/6),0,std::sin(pi/6)})<1e-8);
    }
    Fixture f;
    v::Pose grip{},aim{};
    C(v::rig_tracking::pose(f.base,f.reference,f.hand.grip,100,grip));
    C(v::rig_tracking::pose(f.base,f.reference,f.hand.aim,100,aim));
    a::Result r{};
    C(a::derive(f.input(),r));
    const auto world_marker=add(r.wrist_world.position,v::transform(v::rotation_matrix(r.wrist_world.orientation),
        {f.marker.x*100,f.marker.y*100,f.marker.z*100}));
    C(near(world_marker,grip.position)<1e-8);
    const auto mesh_wrist_marker=add(r.mesh_world.position,v::transform(v::rotation_matrix(r.mesh_world.orientation),
        add({f.wrist_in_mesh.position.x*100,f.wrist_in_mesh.position.y*100,f.wrist_in_mesh.position.z*100},
            v::transform(v::rotation_matrix(f.wrist_in_mesh.orientation),{f.marker.x*100,f.marker.y*100,f.marker.z*100}))));
    C(near(mesh_wrist_marker,grip.position)<1e-8);
    // Any local muzzle direction has the same world direction under mesh and
    // under Aim, because Rmesh*M == Aim. This is the actual axis invariant.
    for(v::Vector local_muzzle: {v::Vector{1,0,0},v::Vector{.2,-.7,.55}}){
        const auto from_mesh=v::transform(v::rotation_matrix(r.mesh_world.orientation),
            v::transform(v::rotation_matrix(f.muzzle_in_mesh),local_muzzle));
        const auto from_aim=v::transform(v::rotation_matrix(aim.orientation),local_muzzle);
        C(near(from_mesh,from_aim)<1e-8);
    }
    const auto expected_mesh=v::multiply(v::rotation_matrix(aim.orientation),
        v::transpose(v::rotation_matrix(f.muzzle_in_mesh)));
    const auto actual_mesh=v::rotation_matrix(r.mesh_world.orientation);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)C(std::abs(expected_mesh[i][j]-actual_mesh[i][j])<1e-8);
    const auto previous=r;
    auto moved=f.input();f.hand.aim.position={.251,1.451,-.399};C(a::derive(moved,r));C(near(r.wrist_world.position,previous.wrist_world.position)<1e-8);
    C(near(r.mesh_world.position,previous.mesh_world.position)<1e-8);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)C(std::abs(v::rotation_matrix(r.mesh_world.orientation)[i][j]-actual_mesh[i][j])<1e-8);

    // Grip is horizontal while Aim pitches the mesh. Position remains Grip and
    // the mesh forward axis follows Aim, not Grip.
    f.hand.aim.orientation=qx(pi/3);C(a::derive(f.input(),r));
    v::Pose g{},q{};C(v::rig_tracking::pose(f.base,f.reference,f.hand.grip,100,g));
    C(v::rig_tracking::pose(f.base,f.reference,f.hand.aim,100,q));
    const auto aim_forward=v::transform(v::rotation_matrix(q.orientation),{1,0,0});
    const auto produced_aim_forward=v::transform(v::rotation_matrix(r.mesh_world.orientation),
        v::transform(v::rotation_matrix(f.muzzle_in_mesh),{1,0,0}));
    C(near(produced_aim_forward,aim_forward)<1e-8);
    C(std::abs(aim_forward.z-v::transform(v::rotation_matrix(g.orientation),{1,0,0}).z)>1e-4);

    // Base/reference and scale change twice: first co-batch, then units.
    auto scaled=f;scaled.units_per_metre=200;const auto before_scale=r;
    C(a::derive(scaled.input(),r));
    v::Pose scaled_grip{};
    C(v::rig_tracking::pose(scaled.base,scaled.reference,scaled.hand.grip,200,scaled_grip));
    const auto scaled_wrist_r=v::multiply(v::rotation_matrix(r.mesh_world.orientation),
        v::rotation_matrix(scaled.wrist_in_mesh.orientation));
    const auto scaled_marker=v::transform(scaled_wrist_r,
        {scaled.marker.x*200,scaled.marker.y*200,scaled.marker.z*200});
    C(near(add(r.wrist_world.position,scaled_marker),scaled_grip.position)<1e-8);
    C(near(r.wrist_world.position,before_scale.wrist_world.position)>1e-4);
    auto moved_base=scaled;moved_base.base[12]=25;moved_base.base[13]=-8;
    const auto old_scaled=r;C(a::derive(moved_base.input(),r));
    C(near(r.wrist_world.position,old_scaled.wrist_world.position)>1e-4);
    v::Pose moved_grip{};C(v::rig_tracking::pose(moved_base.base,moved_base.reference,moved_base.hand.grip,200,moved_grip));
    const auto moved_wrist_r=v::multiply(v::rotation_matrix(r.mesh_world.orientation),
        v::rotation_matrix(moved_base.wrist_in_mesh.orientation));
    const auto moved_marker=v::transform(moved_wrist_r,
        {moved_base.marker.x*200,moved_base.marker.y*200,moved_base.marker.z*200});
    C(near(add(r.wrist_world.position,moved_marker),moved_grip.position)<1e-8);

    // Failure atomicity.
    const auto stable=r;auto bad=f.input();
    auto h=f.hand;h.aim_active=0;bad.hand=&h;C(!a::derive(bad,r));require_same(stable,r);
    h=f.hand;h.aim_flags=0;bad=f.input();bad.hand=&h;C(!a::derive(bad,r));require_same(stable,r);
    h=f.hand;h.grip_active=0;bad=f.input();bad.hand=&h;C(!a::derive(bad,r));require_same(stable,r);
    bad=f.input();bad.units_per_metre=0;C(!a::derive(bad,r));require_same(stable,r);
    auto wrist=f.wrist_in_mesh;bad=f.input();bad.wrist_in_mesh=&wrist;wrist.orientation={0,0,0,0};C(!a::derive(bad,r));require_same(stable,r);
    auto muzzle=f.muzzle_in_mesh;bad=f.input();bad.muzzle_in_mesh=&muzzle;muzzle.w=2;C(!a::derive(bad,r));require_same(stable,r);
    bad=f.input();bad.base_view=nullptr;C(!a::derive(bad,r));require_same(stable,r);
    std::printf("Boltok Aim/Grip geometry: %u checks; same-frame Aim drives mesh orientation, Grip anchors position; no engine/data acceptance implied.\n",checks);
}
