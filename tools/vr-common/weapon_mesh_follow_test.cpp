#include "weapon_mesh_follow.h"
#include "weapon_lancer_held_contact.h"
#include "weapon_frag_grasp.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace gears_vr;
using namespace gears_weapon;

static bool near(double a,double b){return std::abs(a-b)<1e-4;}
static std::array<float,16> mesh_matrix(Rotator angle,Vector origin,double scale=1){
    const auto b=ue_matrix(angle);
    return {float(b[0][0]*scale),float(b[1][0]*scale),float(b[2][0]*scale),0,
            float(b[0][1]*scale),float(b[1][1]*scale),float(b[2][1]*scale),0,
            float(b[0][2]*scale),float(b[1][2]*scale),float(b[2][2]*scale),0,
            float(origin.x),float(origin.y),float(origin.z),1};
}
int main(){
    WeaponMeshFollow follow;
    const Viewpoint first{{100,200,300},{.12,.3,-.1}};
    const Vector mesh_origin{140,225,287};
    const Rotator mesh_rotation{-.09,.48,.16};
    auto matrix=mesh_matrix(mesh_rotation,mesh_origin,1.25);
    assert(follow.bind(first,matrix));
    Viewpoint actual{};
    assert(follow.solve(first,actual));
    assert(near(actual.position.x,mesh_origin.x)&&near(actual.position.y,mesh_origin.y)&&
           near(actual.position.z,mesh_origin.z));
    const auto expected=ue_matrix(mesh_rotation),initial=ue_matrix(actual.rotation);
    for(int row=0;row<3;++row)for(int col=0;col<3;++col)
        assert(near(initial[row][col],expected[row][col]));
    const Viewpoint moved{{110,210,320},{.12,.3+1.5707963267948966,-.1}};
    assert(follow.solve(moved,actual));
    assert(std::hypot(std::hypot(actual.position.x-mesh_origin.x,
        actual.position.y-mesh_origin.y),actual.position.z-mesh_origin.z)>10);
    const auto saved=actual;
    Viewpoint impossible=moved;impossible.position.x=INFINITY;
    assert(!follow.solve(impossible,actual)&&near(actual.position.x,saved.position.x));
    follow.reset();assert(!follow.solve(first,actual));
    matrix=mesh_matrix(mesh_rotation,mesh_origin);
    matrix[0]*=-1;matrix[1]*=-1;matrix[2]*=-1; // Mirror/reflection is not a safe rigid mount.
    assert(!follow.bind(first,matrix));
    matrix=mesh_matrix(mesh_rotation,mesh_origin);matrix[4]*=2;
    assert(!follow.bind(first,matrix));
    matrix=mesh_matrix(mesh_rotation,{1000,2000,3000});
    assert(!follow.bind(first,matrix));
    WeaponMeshFollow support;
    const Viewpoint right{{0,0,0},{0,0,0}};
    const Vector contact{8.55624,43.52518,.16516};
    assert(support.bind(right,mesh_matrix({0,0,0},{0,0,0})));
    bool used{};
    const Viewpoint near_left{contact,{0,0,0}};
    assert(support.solve_with_support(right,near_left,true,false,contact,actual,used));
    assert(!used&&!support.support_latched());
    const Viewpoint far_left{{40,43.5,0},{0,0,0}};
    assert(support.solve_with_support(right,far_left,true,true,contact,actual,used));
    assert(!used&&!support.support_latched());
    assert(support.solve_with_support(right,near_left,true,true,contact,actual,used));
    assert(!used); // Moving close with LGrip already held is not a new grab.
    assert(support.solve_with_support(right,near_left,true,false,contact,actual,used));
    assert(support.solve_with_support(right,near_left,true,true,contact,actual,used));
    assert(used&&support.support_latched());
    const Viewpoint moved_left{{contact.x+5,contact.y,contact.z},{0,0,0}};
    assert(support.solve_with_support(right,moved_left,true,true,contact,actual,used));
    assert(used&&support.support_latched()&&std::abs(actual.rotation.yaw)>.01);
    assert(support.solve_with_support(right,near_left,false,true,contact,actual,used));
    assert(!used&&!support.support_latched()); // Tracking loss releases support.
    assert(support.solve_with_support(right,near_left,true,true,contact,actual,used));
    assert(!used); // Tracking recovery cannot replay the held press.
    assert(support.solve_with_support(right,near_left,true,false,contact,actual,used));
    assert(support.solve_with_support(right,near_left,true,true,contact,actual,used));
    assert(used);
    assert(support.solve_with_support(right,far_left,true,true,contact,actual,used));
    assert(!used&&!support.support_latched());
    WeaponMeshFollow held_on_bind;
    assert(held_on_bind.bind(right,mesh_matrix({0,0,0},{0,0,0}),true));
    assert(held_on_bind.solve_with_support(right,near_left,true,true,contact,actual,used));
    assert(!used&&!held_on_bind.support_latched());
    assert(held_on_bind.solve_with_support(right,near_left,true,false,contact,actual,used));
    assert(held_on_bind.solve_with_support(right,near_left,true,true,contact,actual,used));
    assert(used&&held_on_bind.support_latched());
    WeaponMeshFollow scaled;
    assert(scaled.bind(right,mesh_matrix({0,0,0},{0,0,0},1.25)));
    const Viewpoint scaled_left{{contact.x*1.25,contact.y*1.25,contact.z*1.25},{0,0,0}};
    assert(scaled.solve_with_support(right,scaled_left,true,true,contact,actual,used));
    assert(used&&scaled.support_latched());
    // Retail Lancer's settled right-hand bone is about +90 degrees yaw in
    // the gun frame, with its palm center 2.3226 units behind the gun origin.
    // This checks mount algebra only; a Quest controller-to-palm calibration
    // is still needed before enabling it for physical tracking.
    constexpr double quarter_turn=1.5707963267948966;
    const Vector lancer_palm{-2.322615648,0,0};
    const auto original_hand=ue_matrix({0,quarter_turn,0});
    const auto controller_to_palm=ue_matrix({0,0,0});
    const Rotator gun_angle{.08,.42,-.04};
    const Vector gun_origin{110,220,330};
    const auto gun_basis=ue_matrix(gun_angle);
    const auto grip_shift=transform(gun_basis,lancer_palm);
    const Viewpoint grip{{gun_origin.x+grip_shift.x,gun_origin.y+grip_shift.y,
        gun_origin.z+grip_shift.z},
        ue_angles(multiply(gun_basis,original_hand))};
    WeaponMeshFollow mounted;
    assert(mounted.bind_mount(grip,mesh_matrix(gun_angle,gun_origin),lancer_palm,
        original_hand,controller_to_palm));
    assert(mounted.solve(grip,actual));
    assert(near(actual.position.x,gun_origin.x)&&near(actual.position.y,gun_origin.y)&&
           near(actual.position.z,gun_origin.z));
    const auto mounted_basis=ue_matrix(actual.rotation);
    for(int row=0;row<3;++row)for(int col=0;col<3;++col)
        assert(near(mounted_basis[row][col],gun_basis[row][col]));
    auto translated_grip=grip;translated_grip.position.x+=12;
    assert(mounted.solve(translated_grip,actual));
    assert(near(actual.position.x,gun_origin.x+12));
    WeaponMeshFollow invalid_mount;
    auto reflected=controller_to_palm;reflected[0][0]=-1;
    assert(!invalid_mount.bind_mount(grip,mesh_matrix(gun_angle,gun_origin),
        lancer_palm,original_hand,reflected));
    assert(!invalid_mount.bound());
    // Different pickup animation rotations must not determine held aim.
    const auto authored=ue_matrix({0,-quarter_turn,0});
    for(auto animation : {Rotator{0,0,0},Rotator{.9,.4,-.7}}){
        WeaponMeshFollow oriented;
        const auto pickup_basis=ue_matrix(animation);
        const Vector mount_contact{2,3,-4};
        const auto contact_world=transform(pickup_basis,mount_contact);
        const Vector origin{first.position.x-contact_world.x,
            first.position.y-contact_world.y,first.position.z-contact_world.z};
        assert(oriented.bind_oriented(first,mesh_matrix(animation,origin),authored));
        assert(oriented.solve(moved,actual));
        const auto actual_basis=ue_matrix(actual.rotation);
        const auto expected_basis=multiply(ue_matrix(moved.rotation),authored);
        for(int i=0;i<3;++i)for(int j=0;j<3;++j)
            assert(near(actual_basis[i][j],expected_basis[i][j]));
        const auto offset=transform(actual_basis,mount_contact);
        assert(near(actual.position.x+offset.x,moved.position.x));
        assert(near(actual.position.y+offset.y,moved.position.y));
        assert(near(actual.position.z+offset.z,moved.position.z));
        std::array<float,16> contact_matrix{};
        assert(oriented.model_matrix(moved,contact_matrix));
        for(int axis=0;axis<3;++axis)for(int world=0;world<3;++world)
            assert(std::abs(contact_matrix[axis*4+world]-actual_basis[world][axis])<1e-5);
        assert(std::abs(contact_matrix[12]-actual.position.x)<1e-4);
        assert(std::abs(contact_matrix[13]-actual.position.y)<1e-4);
        assert(std::abs(contact_matrix[14]-actual.position.z)<1e-4);
        assert(contact_matrix[3]==0&&contact_matrix[7]==0&&contact_matrix[11]==0&&contact_matrix[15]==1);
        oriented.reset();
        assert(!oriented.model_matrix(moved,contact_matrix));
    }
    // Regression: pickup animation position/orientation must not become a
    // permanent Lancer hand offset. The authored palm, not the wrist origin,
    // must coincide with the controller after any accepted pickup pose.
    Vector authored_contact{};Matrix authored_wrist{};
    assert(gears_weapon_lancer_contact::authored(authored_contact,authored_wrist));
    for(auto pickup : {Rotator{0,0,0},Rotator{.9,.4,-.7}})
    for(auto pickup_position : {Vector{110,220,330},Vector{70,180,295}}){
        WeaponMeshFollow fixed;
        assert(fixed.bind_mount(first,mesh_matrix(pickup,pickup_position),
            authored_contact,authored_wrist,ue_matrix({0,0,0})));
        for(auto target : {first,moved}){
            Viewpoint solved{};assert(fixed.solve(target,solved));
            const auto shift=transform(ue_matrix(solved.rotation),authored_contact);
            assert(near(solved.position.x+shift.x,target.position.x));
            assert(near(solved.position.y+shift.y,target.position.y));
            assert(near(solved.position.z+shift.z,target.position.z));
        }
    }
    // Frag mounts on the original handle instead of inheriting an equip
    // animation offset. The two object scales preserve the same palm offset;
    // no held shot/muzzle calibration is created by this visual data.
    for(double scale:{.75,1.,1.25}){
        gears_weapon_frag_grasp::Profile profile{};
        assert(gears_weapon_frag_grasp::marcus115(gears_hand_authored_mount::marcus115_signature,1,100,profile,scale));
        WeaponMeshFollow frag;const auto before=mesh_matrix({.3,.7,-.2},{80,170,260},scale);
        assert(frag.bind_mount(first,before,profile.palm_in_mesh,profile.wrist_in_mesh,
            rotation_matrix(profile.hand.calibration.grip_to_wrist_rotation)));
        for(auto hand:{first,moved}){std::array<float,16> frag_matrix{};assert(frag.model_matrix(hand,frag_matrix));
            const auto p=profile.palm_in_mesh;
            assert(near(frag_matrix[12]+frag_matrix[0]*p.x+frag_matrix[4]*p.y+frag_matrix[8]*p.z,hand.position.x));
            assert(near(frag_matrix[13]+frag_matrix[1]*p.x+frag_matrix[5]*p.y+frag_matrix[9]*p.z,hand.position.y));
            assert(near(frag_matrix[14]+frag_matrix[2]*p.x+frag_matrix[6]*p.y+frag_matrix[10]*p.z,hand.position.z));
            const auto r=hand.rotation;
            const auto hand_q=multiply(multiply(Quaternion{0,0,std::sin(r.yaw/2),std::cos(r.yaw/2)},
                Quaternion{0,-std::sin(r.pitch/2),0,std::cos(r.pitch/2)}),Quaternion{-std::sin(r.roll/2),0,0,std::cos(r.roll/2)});
            Pose endpoint{hand.position,multiply(hand_q,profile.hand.calibration.grip_to_wrist_rotation)};
            Pose recovered{};assert(gears_weapon_frag_grasp::grip_from_palm_endpoint(endpoint,recovered));
            const auto q=rotation_matrix(recovered.orientation),frag_expected=ue_matrix(hand.rotation);
            for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)assert(near(q[row][col],frag_expected[row][col]));
        }
    }
    std::puts("weapon mesh follow calibration and invalid geometry gates passed; Frag authored handle mount CPU-only");
}
