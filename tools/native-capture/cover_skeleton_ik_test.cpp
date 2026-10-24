#include "cover_skeleton_ik.h"
#include "../vr-common/hand_two_bone_pose.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

namespace ik=gears_cover_skeleton_ik;
namespace arm=gears_hand_two_bone;
namespace {
unsigned checks{};
void check(bool okay,char const* expression,int line){++checks;if(!okay){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}}
#define CHECK(x) check((x),#x,__LINE__)
constexpr double pi=3.14159265358979323846;
ik::Workspace work; // Caller storage, not a large automatic kernel allocation.
std::array<ik::Atom,ik::max_bones> source{},before{},solved{},saved_atoms{};
std::array<std::int32_t,ik::max_bones> parents{};
std::array<ik::Matrix,ik::max_bones> space{},world{},saved_space{},saved_world{};
ik::Matrix component{};
std::size_t count=7;
ik::Chain chain{1,2,3};
ik::Vec plus(ik::Vec a,ik::Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
ik::Vec minus(ik::Vec a,ik::Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
ik::Vec times(ik::Vec a,double s){return {a.x*s,a.y*s,a.z*s};}
double norm(ik::Vec a){return std::hypot(a.x,a.y,a.z);}
ik::Vec point(ik::Matrix const& m){return {m.m[12],m.m[13],m.m[14]};}
bool near(double a,double b,double tolerance=1e-5){return std::abs(a-b)<tolerance;}
bool near(ik::Vec a,ik::Vec b,double tolerance=1e-5){return norm(minus(a,b))<tolerance;}
// Independent vector/quaternion geometry; no kernel matrix implementation used.
ik::Vec rotate(ik::Quat q,ik::Vec v){
    const ik::Vec u{q.x,q.y,q.z};const double d=u.x*v.x+u.y*v.y+u.z*v.z,uu=u.x*u.x+u.y*u.y+u.z*u.z;
    const ik::Vec c{u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x};
    return plus(plus(times(u,2*d),times(v,q.w*q.w-uu)),times(c,2*q.w));
}
ik::Quat yaw(double angle){return {0,0,std::sin(angle/2),std::cos(angle/2)};}
void set_q(ik::Atom& a,ik::Quat q){a.q[0]=static_cast<float>(q.x);a.q[1]=static_cast<float>(q.y);a.q[2]=static_cast<float>(q.z);a.q[3]=static_cast<float>(q.w);}
ik::Matrix identity(){ik::Matrix m{};m.m[0]=m.m[5]=m.m[10]=m.m[15]=1;return m;}
void reset(){
    count=7;component=identity();source={};parents={};
    for(auto& a:source){a.q[3]=1;a.scale=1;}
    parents[1]=0;parents[2]=1;parents[3]=2;parents[4]=3;parents[5]=1;parents[6]=0;
    source[2].translation[0]=3;source[3].translation[0]=2;source[4].translation[1]=1;
    source[5].translation[2]=2;source[6].translation[0]=-5;
}
ik::Status fk(){return ik::forward(source.data(),parents.data(),count,component,work,space.data(),world.data());}
void unchanged(bool adaptive=false){
    CHECK(!std::memcmp(source.data(),before.data(),count*sizeof(ik::Atom)));
    for(std::size_t i=0;i<count;++i){
        CHECK(!std::memcmp(&source[i].scale,&solved[i].scale,sizeof(float)));
        if(!adaptive||(i!=chain.lower&&i!=chain.end))CHECK(!std::memcmp(source[i].translation,solved[i].translation,3*sizeof(float)));
        if(i!=chain.upper&&i!=chain.lower&&i!=chain.end)CHECK(!std::memcmp(&source[i],&solved[i],sizeof(ik::Atom)));
    }
}
void stash(){saved_atoms=solved;saved_space=space;saved_world=world;before=source;}
void rejected_outputs(){
    CHECK(!std::memcmp(solved.data(),saved_atoms.data(),sizeof(solved)));
    CHECK(!std::memcmp(space.data(),saved_space.data(),sizeof(space)));
    CHECK(!std::memcmp(world.data(),saved_world.data(),sizeof(world)));
    CHECK(!std::memcmp(source.data(),before.data(),sizeof(source)));
}
ik::Status adapt(ik::Pose const* poses,ik::LengthPolicy policy,ik::AdaptResult& result){
    return ik::world_poses_to_local(source.data(),parents.data(),count,component,chain,poses,policy,work,solved.data(),space.data(),world.data(),result);
}
ik::Pose pose(ik::Matrix const& matrix){ik::Pose p{};CHECK(ik::world_pose(matrix,p)==ik::Status::okay);return p;}
arm::Input input(){return {pose(world[chain.upper]),pose(world[chain.lower]),pose(world[chain.end]),{},{},{},{}};}
void orientation(ik::Matrix const& matrix,ik::Quat q,double tolerance=1e-5){
    for(unsigned axis=0;axis<3;++axis){const ik::Vec basis=axis==0?ik::Vec{1,0,0}:axis==1?ik::Vec{0,1,0}:ik::Vec{0,0,1};
        const ik::Vec row{matrix.m[4*axis],matrix.m[4*axis+1],matrix.m[4*axis+2]};CHECK(near(times(row,1/norm(row)),rotate(q,basis),tolerance));}
}
ik::AdaptResult compose(arm::Solution const& solution,bool adaptive){
    const ik::Pose poses[]={solution.shoulder,solution.elbow,solution.wrist};ik::AdaptResult result{};
    CHECK(adapt(poses,adaptive?ik::LengthPolicy::adapt_translations:ik::LengthPolicy::fixed,result)==ik::Status::okay);
    unchanged(adaptive);CHECK(near(point(world[chain.end]),solution.wrist.position,1e-4));orientation(world[chain.end],solution.wrist.orientation);
    CHECK(result.max_position_error<1e-4&&result.translations_allowed==adaptive);return result;
}
void fk_geometry(){
    reset();source[0].translation[0]=2;source[0].translation[1]=-1;source[0].translation[2]=4;
    source[1].translation[0]=1;source[1].translation[1]=2;source[1].scale=2;set_q(source[1],yaw(pi/2));
    // World component: positive scale 3 and +90 degrees about Y.
    component={{0,0,-3,0,0,3,0,0,3,0,0,0,100,50,-20,1}};before=source;
    CHECK(fk()==ik::Status::okay);
    CHECK(near(point(space[1]),{3,1,4}));CHECK(near(point(space[2]),{3,7,4}));CHECK(near(point(space[3]),{3,11,4}));
    CHECK(near(point(world[1]),{112,53,-29}));CHECK(near(point(world[2]),{112,71,-29}));CHECK(near(point(world[3]),{112,83,-29}));
    CHECK(!std::memcmp(source.data(),before.data(),sizeof(source)));
}
void adapter_geometry(){
    reset();before=source;ik::AdaptResult result{};
    // Elbow at (0,3,0), wrist (0,3,2); independently specified world poses.
    const ik::Quat lower_goal{0,-std::sqrt(.5),0,std::sqrt(.5)};
    ik::Pose goals[]={{{0,0,0},yaw(pi/2)},{{0,3,0},lower_goal},{{0,3,2},yaw(-pi/3)}};
    CHECK(adapt(goals,ik::LengthPolicy::fixed,result)==ik::Status::okay);unchanged();
    CHECK(result.max_position_error<1e-5&&!result.translations_allowed);
    CHECK(near(point(world[2]),{0,3,0}));CHECK(near(point(world[3]),{0,3,2}));
    CHECK(near(point(world[4]),plus(goals[2].position,rotate(goals[2].orientation,{0,1,0}))));
    CHECK(near(point(world[6]),{-5,0,0}));
    for(unsigned j=0;j<3;++j)for(unsigned axis=0;axis<3;++axis){const ik::Vec basis=axis==0?ik::Vec{1,0,0}:axis==1?ik::Vec{0,1,0}:ik::Vec{0,0,1};
        const auto r=rotate(goals[j].orientation,basis);const auto& m=world[j+1];
        CHECK(near(ik::Vec{m.m[4*axis],m.m[4*axis+1],m.m[4*axis+2]},r));}
    // Exact adaptive world goal, far beyond the native 5-unit chain reach.
    goals[1].position={0,6,0};goals[2].position={0,6,9};
    stash();CHECK(adapt(goals,ik::LengthPolicy::fixed,result)==ik::Status::inconsistent_pose);rejected_outputs();
    CHECK(adapt(goals,ik::LengthPolicy::adapt_translations,result)==ik::Status::okay);unchanged(true);
    CHECK(result.translations_allowed&&result.max_position_error<1e-5);CHECK(near(point(world[3]),goals[2].position));
    CHECK(near(norm({solved[2].translation[0],solved[2].translation[1],solved[2].translation[2]}),6));
    CHECK(near(norm({solved[3].translation[0],solved[3].translation[1],solved[3].translation[2]}),9));
    // A component yaw/scale is part of the inverse conversion, exactly once.
    reset();component={{0,2,0,0,-2,0,0,0,0,0,2,0,10,20,30,1}};before=source;
    goals[0]={{10,20,30},yaw(pi)};goals[1]={{4,20,30},yaw(pi/2)};goals[2]={{4,24,30},yaw(0)};
    CHECK(adapt(goals,ik::LengthPolicy::fixed,result)==ik::Status::okay);unchanged();CHECK(near(point(world[3]),goals[2].position));
}
void rejection_cases(){
    reset();CHECK(fk()==ik::Status::okay);ik::AdaptResult result{123,true};
    ik::Pose goals[]={{{0,0,0},{}},{{3,0,0},{}},{{5,0,0},{}}};
    const auto good=source;const auto original_parents=parents;const auto original_component=component;
    for(unsigned variant=0;variant<12;++variant){
        source=good;parents=original_parents;component=original_component;auto bad=goals[2];
        if(variant==0)source[1].q[0]=(std::numeric_limits<float>::quiet_NaN)();
        if(variant==1)source[1].q[3]=0;
        if(variant==2)source[1].scale=0;
        if(variant==3)source[1].scale=-1;
        if(variant==4)source[1].translation[2]=(std::numeric_limits<float>::infinity)();
        if(variant==5)parents[0]=-1;
        if(variant==6)parents[2]=3;
        if(variant==7)component.m[0]=2;
        if(variant==8)component.m[0]=-1;
        if(variant==9)component.m[3]=1;
        if(variant==10)goals[2].position.x=(std::numeric_limits<double>::quiet_NaN)();
        if(variant==11)goals[2].orientation.w=0;
        stash();CHECK(adapt(goals,ik::LengthPolicy::fixed,result)!=ik::Status::okay);rejected_outputs();CHECK(result.max_position_error==123&&result.translations_allowed);goals[2]=bad;
    }
    reset();stash();CHECK(ik::forward(source.data(),parents.data(),0,component,work,space.data(),world.data())==ik::Status::count);rejected_outputs();
    CHECK(ik::forward(source.data(),parents.data(),513,component,work,space.data(),world.data())==ik::Status::count);
    CHECK(ik::forward(source.data(),parents.data(),count,component,work,space.data(),space.data()+1)==ik::Status::alias);rejected_outputs();
    CHECK(ik::forward(source.data(),parents.data(),count,component,work,work.space,world.data())==ik::Status::alias);rejected_outputs();
    CHECK(ik::world_poses_to_local(source.data(),parents.data(),count,component,chain,goals,ik::LengthPolicy::fixed,work,source.data(),space.data(),world.data(),result)==ik::Status::alias);rejected_outputs();
    auto bad_chain=chain;bad_chain.upper=0;
    CHECK(ik::world_poses_to_local(source.data(),parents.data(),count,component,bad_chain,goals,ik::LengthPolicy::fixed,work,solved.data(),space.data(),world.data(),result)==ik::Status::chain);rejected_outputs();
    goals[0].position.x=.01;CHECK(adapt(goals,ik::LengthPolicy::adapt_translations,result)==ik::Status::inconsistent_pose);rejected_outputs();
    // Bounded max hierarchy; overflow is rejected without committing matrices.
    reset();count=ik::max_bones;for(std::size_t i=1;i<count;++i)parents[i]=static_cast<std::int32_t>(i-1);
    CHECK(fk()==ik::Status::okay);for(auto& a:source)a.scale=1e30f;stash();CHECK(fk()==ik::Status::nonfinite);rejected_outputs();
}
void shared_backend(){
    reset();before=source;CHECK(fk()==ik::Status::okay);auto request=input();request.authored={true,{0,0,10}};
    for(const auto p:{ik::Vec{3,2,0},ik::Vec{-5,0,0},ik::Vec{4.9999999,0,0},ik::Vec{0,4,1}}){
        request.target={p,yaw(.7)};arm::Solution solution{};CHECK(arm::solve(request,solution)==arm::Status::solved);compose(solution,false);
        CHECK(near(norm(minus(point(world[2]),point(world[1]))),3));CHECK(near(norm(minus(point(world[3]),point(world[2]))),2));
    }
    for(const auto p:{ik::Vec{50,0,0},ik::Vec{.1,0,0},ik::Vec{0,0,0}}){
        request.target={p,yaw(-.9)};request.lengths.mode=arm::LengthMode::preserve;
        arm::Solution solution{},original{};std::memcpy(&original,&solution,sizeof(solution));stash();
        CHECK(arm::solve(request,solution)==arm::Status::unreachable);CHECK(!std::memcmp(&solution,&original,sizeof(solution)));rejected_outputs();
        request.lengths.mode=arm::LengthMode::adaptive;CHECK(arm::solve(request,solution)==arm::Status::solved);CHECK(solution.length_adjusted);compose(solution,true);
        CHECK(near(solution.wrist.position,p,1e-12)); // No projected target accepted as actual goal.
    }
    reset();source[3].translation[0]=3;before=source;CHECK(fk()==ik::Status::okay);request=input();request.target={{},yaw(.2)};
    arm::Solution solution{};CHECK(arm::solve(request,solution)==arm::Status::solved);compose(solution,false);
    source[2].translation[0]=0;CHECK(fk()==ik::Status::okay);request=input();stash();
    CHECK(arm::solve(request,solution)==arm::Status::degenerate_bone);rejected_outputs();
    reset();component={{0,2,0,0,-2,0,0,0,0,0,2,0,10,20,30,1}};before=source;CHECK(fk()==ik::Status::okay);
    request=input();request.target={{12,26,32},yaw(1.2)};CHECK(arm::solve(request,solution)==arm::Status::solved);compose(solution,false);
    // Matrix-to-pose conversion rejects invalid transforms without publication.
    auto matrix=identity();ik::Pose output{{123,456,789},yaw(.6)},original=output;matrix.m[0]=-1;
    CHECK(ik::world_pose(matrix,output)==ik::Status::component);CHECK(!std::memcmp(&output,&original,sizeof(output)));
    matrix=identity();matrix.m[12]=(std::numeric_limits<double>::infinity)();
    CHECK(ik::world_pose(matrix,output)==ik::Status::component);CHECK(!std::memcmp(&output,&original,sizeof(output)));
}
template<class T> void read(std::ifstream& f,T* out,std::size_t n){f.read(reinterpret_cast<char*>(out),static_cast<std::streamsize>(n*sizeof(T)));CHECK(static_cast<bool>(f));}
void fixture(char const* path){
    std::ifstream f(path,std::ios::binary);CHECK(static_cast<bool>(f));char magic[8]{};read(f,magic,8);CHECK(!std::memcmp(magic,"CSIK0001",8));
    std::uint32_t n{};read(f,&n,1);CHECK(n>0&&n<=ik::max_bones);count=n;read(f,parents.data(),count);read(f,source.data(),count);
    std::array<float,16> native_component{};read(f,native_component.data(),16);for(unsigned j=0;j<16;++j)component.m[j]=native_component[j];
    std::array<std::array<float,16>,ik::max_bones> expected{};for(std::size_t i=0;i<count;++i)read(f,expected[i].data(),16);
    CHECK(f.peek()==std::ifstream::traits_type::eof());CHECK(count==115&&parents[10]==8&&parents[11]==10);before=source;
    CHECK(fk()==ik::Status::okay);double translation_error{},rotation_error{};
    for(std::size_t i=0;i<count;++i)for(unsigned j=0;j<16;++j){const double e=std::abs(space[i].m[j]-expected[i][j]);
        if(j>=12&&j<15)translation_error=(std::max)(translation_error,e);
        else if(j%4<3&&j<12)rotation_error=(std::max)(rotation_error,e);}
    CHECK(translation_error<2e-4&&rotation_error<1e-5);CHECK(!std::memcmp(source.data(),before.data(),sizeof(source)));
    chain={8,10,11};const auto native_local=source;const auto neutral=input();const auto a=neutral.shoulder.position,b=neutral.elbow.position,c=neutral.wrist.position;
    const double l1=norm(minus(b,a)),l2=norm(minus(c,b)),radius=norm(minus(c,a));CHECK(radius>4);
    const auto direction=times(minus(c,a),1/radius);auto request=neutral;
    request.target=neutral.wrist;request.target.position=minus(c,times(direction,4));arm::Solution solution{};
    CHECK(near(norm(minus(request.target.position,c)),4,1e-10));CHECK(arm::solve(request,solution)==arm::Status::solved);
    compose(solution,false);const double goal_error=norm(minus(point(world[11]),request.target.position));
    CHECK(near(norm(minus(point(world[10]),point(world[8]))),l1,.001));CHECK(near(norm(minus(point(world[11]),point(world[10]))),l2,.001));
    // Sequential return from the displaced private pose, then wrist roll.
    source=solved;before=source;CHECK(fk()==ik::Status::okay);request=input();request.target=neutral.wrist;
    CHECK(arm::solve(request,solution)==arm::Status::solved);compose(solution,false);const double return_error=norm(minus(point(world[11]),c));
    source=solved;before=source;CHECK(fk()==ik::Status::okay);request=input();request.target=neutral.wrist;
    request.target.orientation=gears_vr::multiply(neutral.wrist.orientation,{std::sin(.4),0,0,std::cos(.4)});
    CHECK(arm::solve(request,solution)==arm::Status::solved);compose(solution,false);const double roll_position_error=norm(minus(point(world[11]),c));
    // Explicit adaptive outside-reach goal, kept exact rather than projected.
    source=native_local;before=source;CHECK(fk()==ik::Status::okay);request=input();request.target=neutral.wrist;
    request.target.position=plus(a,times(direction,l1+l2+25));request.lengths.mode=arm::LengthMode::adaptive;
    CHECK(arm::solve(request,solution)==arm::Status::solved);CHECK(solution.length_adjusted);CHECK(near(solution.wrist.position,request.target.position,1e-12));
    const ik::Pose poses[]={solution.shoulder,solution.elbow,solution.wrist};ik::AdaptResult fixed_result{123,false};stash();
    CHECK(adapt(poses,ik::LengthPolicy::fixed,fixed_result)==ik::Status::inconsistent_pose);rejected_outputs();CHECK(fixed_result.max_position_error==123);
    const auto adapted=compose(solution,true);const double adaptive_error=norm(minus(point(world[11]),request.target.position));
    CHECK(norm(minus(point(world[11]),a))>l1+l2+24.99);
    std::printf("{\"fixture_bones\":%u,\"chain\":[8,10,11],\"translation_max_error\":%.12g,\"rotation_max_error\":%.12g,\"goal_4unit_error\":%.12g,\"return_error\":%.12g,\"roll_position_error\":%.12g,\"adaptive_outside_error\":%.12g,\"world_pose_adapter_error\":%.12g,\"shared_solver\":true,\"game_write_authorized\":false}\n",n,translation_error,rotation_error,goal_error,return_error,roll_position_error,adaptive_error,adapted.max_position_error);
}
} // namespace
int main(int argc,char** argv){CHECK(argc==1||argc==2);fk_geometry();adapter_geometry();rejection_cases();shared_backend();if(argc==2)fixture(argv[1]);std::printf("CoverSkeletonIkTest: %u checks passed\n",checks);return 0;}
