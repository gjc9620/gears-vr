#include "hand_stream_grip_binding.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_vr;
using gears_hand_stream::GripHandBinding;
namespace {
constexpr double pi=3.14159265358979323846;
std::array<float,16> matrix_data(Matrix const& basis,Vector origin){
    return {{float(basis[0][0]),float(basis[1][0]),float(basis[2][0]),0,
        float(basis[0][1]),float(basis[1][1]),float(basis[2][1]),0,
        float(basis[0][2]),float(basis[1][2]),float(basis[2][2]),0,
        float(origin.x),float(origin.y),float(origin.z),1}};
}
std::array<float,16> matrix_data(Viewpoint const& pose){return matrix_data(ue_matrix(pose.rotation),pose.position);}
Vector sum(Vector a,Vector b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
double distance(Vector a,Vector b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}
double matrix_error(Matrix const& a,Matrix const& b){
    double error{};
    for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)
        error=(std::max)(error,std::abs(a[row][column]-b[row][column]));
    return error;
}
void near_pose(Viewpoint const& actual,Vector position,Matrix const& rotation,double tolerance=5e-5){
    assert(distance(actual.position,position)<tolerance);
    assert(matrix_error(ue_matrix(actual.rotation),rotation)<tolerance);
}
void unusable(Viewpoint const& out){
    assert(!std::isfinite(out.position.x)&&!std::isfinite(out.position.y)&&!std::isfinite(out.position.z));
    assert(!std::isfinite(out.rotation.pitch)&&!std::isfinite(out.rotation.yaw)&&!std::isfinite(out.rotation.roll));
}
void full_rigid_follow(){
    const Viewpoint first{{103,-72,360},{.37,-.62,.28}},moved{{-40,71,390},{-.26,1.17,-.53}};
    const Vector local_offset{12,-7,4};
    const auto local_rotation=ue_matrix({-.41,.33,-.24});
    const auto first_basis=ue_matrix(first.rotation),moved_basis=ue_matrix(moved.rotation);
    const auto bone_position=sum(first.position,transform(first_basis,local_offset));
    const auto bone_rotation=multiply(first_basis,local_rotation);
    const auto bone=matrix_data(bone_rotation,bone_position);
    GripHandBinding binding;Viewpoint actual{};
    assert(binding.bind(first,bone)&&binding.bound()&&binding.solve(first,actual));
    near_pose(actual,bone_position,bone_rotation);
    assert(binding.solve(moved,actual));
    near_pose(actual,sum(moved.position,transform(moved_basis,local_offset)),multiply(moved_basis,local_rotation));
    const Vector old_world_offset{bone_position.x-first.position.x,
        bone_position.y-first.position.y,bone_position.z-first.position.z};
    assert(distance(actual.position,sum(moved.position,old_world_offset))>1);
    const auto bone_angles=ue_angles(bone_rotation);
    const Rotator euler_difference{moved.rotation.pitch+bone_angles.pitch-first.rotation.pitch,
        moved.rotation.yaw+bone_angles.yaw-first.rotation.yaw,
        moved.rotation.roll+bone_angles.roll-first.rotation.roll};
    assert(matrix_error(ue_matrix(actual.rotation),ue_matrix(euler_difference))>.01);
    auto in_place=moved;assert(binding.solve(in_place,in_place));
    near_pose(in_place,actual.position,ue_matrix(actual.rotation));
}
void yaw_rotates_local_offset(){
    const Viewpoint first{{10,20,30},{}},bone{{15,22,33},{.2,.3,.4}};
    GripHandBinding binding;Viewpoint actual{};
    assert(binding.bind(first,matrix_data(bone)));
    auto moved=first;moved.rotation.yaw=pi/2;
    assert(binding.solve(moved,actual));
    const auto initial=ue_matrix(bone.rotation);
    Matrix expected{};
    for(unsigned column=0;column<3;++column){
        expected[0][column]=-initial[1][column];expected[1][column]=initial[0][column];
        expected[2][column]=initial[2][column];
    }
    near_pose(actual,{8,25,33},expected);
}
void independent_hands_and_reset(){
    const Viewpoint left_grip{{2,3,4},{.3,-.7,.2}},right_grip{{-5,7,11},{-.2,.4,-.3}};
    const Viewpoint left_bone{{9,5,7},{-.2,.5,.1}},right_bone{{-2,13,17},{.4,-.3,.7}};
    GripHandBinding left,right;Viewpoint unchanged{},moved{},again{};
    assert(left.bind(left_grip,matrix_data(left_bone))&&right.bind(right_grip,matrix_data(right_bone)));
    assert(left.solve(left_grip,unchanged));
    auto new_right=right_grip;new_right.position.y+=20;new_right.rotation.roll+=.6;
    assert(right.solve(new_right,moved)&&left.solve(left_grip,again));
    near_pose(again,unchanged.position,ue_matrix(unchanged.rotation));
    assert(distance(moved.position,right_bone.position)>5);
    right.reset();assert(!right.bound()&&!right.solve(new_right,moved));unusable(moved);
    assert(left.bound()&&left.solve(left_grip,again));
    near_pose(again,unchanged.position,ue_matrix(unchanged.rotation));
}
void reject_bad_bones_and_clear_old_binding(){
    const Viewpoint grip{{1,2,3},{.1,.2,.3}},bone{{4,8,16},{-.3,.6,-.4}};
    const auto good=matrix_data(bone);
    GripHandBinding binding;Viewpoint out{};
    auto reject=[&](std::array<float,16> const& bad){
        assert(binding.bind(grip,good)&&binding.bound());
        assert(!binding.bind(grip,bad)&&!binding.bound());
        assert(!binding.solve(grip,out));unusable(out);
    };
    for(unsigned i=0;i<16;++i){auto bad=good;bad[i]=std::numeric_limits<float>::quiet_NaN();reject(bad);}
    for(unsigned i:{3u,7u,11u,15u}){auto bad=good;bad[i]+=.01f;reject(bad);}
    auto bad=good;for(unsigned i=0;i<3;++i)bad[i]=-bad[i];reject(bad); // Mirror.
    bad=good;for(unsigned i=0;i<3;++i)bad[i]=0;reject(bad); // Degenerate.
    bad=good;for(unsigned axis=0;axis<3;++axis)for(unsigned i=0;i<3;++i)bad[axis*4+i]*=1.01f;reject(bad);
    bad=good;for(unsigned i=0;i<3;++i)bad[8+i]*=.98f;reject(bad);
    bad=good;for(unsigned i=0;i<3;++i)bad[4+i]+=.02f*good[i];reject(bad); // Shear.
    bad=good;for(unsigned i=0;i<3;++i)bad[4+i]=good[i];reject(bad); // Parallel axes.
    auto noisy=good;noisy[0]+=1e-6f;noisy[5]-=1e-6f;noisy[9]+=1e-6f;
    noisy[3]=5e-7f;noisy[7]=-1e-6f;noisy[11]=2e-7f;noisy[15]+=1e-6f;
    assert(binding.bind(grip,noisy)&&binding.solve(grip,out));
    near_pose(out,bone.position,ue_matrix(bone.rotation),1e-5);
}
void invalid_grips_and_failed_outputs(){
    const Viewpoint grip{{1,2,3},{.3,-.6,.2}},bone{{4,7,9},{-.1,.8,-.5}};
    GripHandBinding binding;Viewpoint out=bone;
    assert(!binding.bound()&&!binding.solve(grip,out));unusable(out);
    for(unsigned field=0;field<6;++field)for(double invalid:{std::numeric_limits<double>::quiet_NaN(),
                                                            std::numeric_limits<double>::infinity()}){
        auto bad=grip;
        switch(field){
            case 0:bad.position.x=invalid;break;case 1:bad.position.y=invalid;break;
            case 2:bad.position.z=invalid;break;case 3:bad.rotation.pitch=invalid;break;
            case 4:bad.rotation.yaw=invalid;break;case 5:bad.rotation.roll=invalid;break;
        }
        assert(binding.bind(grip,matrix_data(bone)));
        assert(!binding.solve(bad,out)&&binding.bound());unusable(out);
        assert(binding.solve(grip,out));near_pose(out,bone.position,ue_matrix(bone.rotation));
        assert(!binding.bind(bad,matrix_data(bone))&&!binding.bound());
        assert(!binding.solve(grip,out));unusable(out);
    }
    // Finite inputs can still overflow while composing; never publish infinity.
    const auto maximum=std::numeric_limits<double>::max();
    const Viewpoint far_grip{{-maximum,0,0},{}};
    assert(binding.bind(far_grip,matrix_data(Viewpoint{})));
    assert(!binding.solve({{maximum,0,0},{}},out));unusable(out);
    assert(!binding.bind({{-maximum,-maximum,0},{0,pi/4,0}},matrix_data(Viewpoint{}))&&!binding.bound());
}
} // namespace
int main(){
    full_rigid_follow();yaw_rotates_local_offset();independent_hands_and_reset();
    reject_bad_bones_and_clear_old_binding();invalid_grips_and_failed_outputs();
    std::puts("Per-hand full rigid grip binding, independent hands and fail-closed outputs passed");
}
