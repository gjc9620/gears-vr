#include "cover_control_output_pose.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace control=gears_cover_control_output_pose;
namespace ik=gears_cover_skeleton_ik;
namespace {
unsigned checks{},cases{};
void check(bool value,char const* expression,int line){++checks;if(!value){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}}
#define CHECK(x) check((x),#x,__LINE__)
using V=control::Vector;using Q=control::Quaternion;using M=control::Matrix64;
control::Workspace scratch;std::array<M,3> output{},saved{};
V add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V scale(V a,double k){return {a.x*k,a.y*k,a.z*k};}
double length(V a){return std::hypot(a.x,a.y,a.z);}
Q qmul(Q a,Q b){return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
Q angles(double x,double y,double z){return qmul(qmul({0,0,std::sin(z/2),std::cos(z/2)},{0,std::sin(y/2),0,std::cos(y/2)}),{std::sin(x/2),0,0,std::cos(x/2)});}
// Independent vector geometry, not the encoder's Atom->matrix formula.
V rotate(Q q,V v){const V u{q.x,q.y,q.z};const double d=u.x*v.x+u.y*v.y+u.z*v.z,uu=u.x*u.x+u.y*u.y+u.z*u.z;
    return add(add(scale(u,2*d),scale(v,q.w*q.w-uu)),scale({u.y*v.z-u.z*v.y,u.z*v.x-u.x*v.z,u.x*v.y-u.y*v.x},2*q.w));}
M matrix(V p,Q q={}){M m{};for(unsigned i=0;i<3;++i){const auto r=rotate(q,i==0?V{1,0,0}:i==1?V{0,1,0}:V{0,0,1});
    m.values[4*i]=static_cast<float>(r.x);m.values[4*i+1]=static_cast<float>(r.y);m.values[4*i+2]=static_cast<float>(r.z);}
    m.values[12]=static_cast<float>(p.x);m.values[13]=static_cast<float>(p.y);m.values[14]=static_cast<float>(p.z);m.values[15]=1;return m;}
V position(M const& m){return {m.values[12],m.values[13],m.values[14]};}
V vector(M const& c,V v){return {v.x*c.values[0]+v.y*c.values[4]+v.z*c.values[8],v.x*c.values[1]+v.y*c.values[5]+v.z*c.values[9],v.x*c.values[2]+v.y*c.values[6]+v.z*c.values[10]};}
V point(M const& c,V p){return add(vector(c,p),position(c));}
bool near(V a,V b,double tolerance=1e-5){return length(sub(a,b))<tolerance;}
M compose(M const& a,M const& b){M out{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j){double value{};
    for(unsigned k=0;k<4;++k)value+=double(a.values[4*i+k])*b.values[4*k+j];out.values[4*i+j]=static_cast<float>(value);}return out;}
void orientation(M const& component,M const& c,Q wanted,double tolerance=2e-6){
    for(unsigned i=0;i<3;++i){const auto world=vector(c,{component.values[4*i],component.values[4*i+1],component.values[4*i+2]});
        CHECK(near(scale(world,1/length(world)),rotate(wanted,i==0?V{1,0,0}:i==1?V{0,1,0}:V{0,0,1}),tolerance));}
}
void rigid(M const& m){
    CHECK(m.values[3]==0&&m.values[7]==0&&m.values[11]==0&&m.values[15]==1);
    V rows[3]{};for(unsigned i=0;i<3;++i){rows[i]={m.values[4*i],m.values[4*i+1],m.values[4*i+2]};CHECK(std::abs(length(rows[i])-1)<1e-6);}
    const V cross{rows[0].y*rows[1].z-rows[0].z*rows[1].y,rows[0].z*rows[1].x-rows[0].x*rows[1].z,rows[0].x*rows[1].y-rows[0].y*rows[1].x};CHECK(near(cross,rows[2],1e-6));
}
control::LimbInput input(){control::LimbInput in{};
    in.component[0]=matrix({0,0,0},angles(.2,.3,-.1));in.component[1]=matrix({2.8,.6,0},angles(-.4,.1,.3));in.component[2]=matrix({4.8,0,0},angles(.1,-.2,.4));
    in.component_to_world=matrix({100,-200,50},angles(.4,-.3,.7));
    in.wrist_world={point(in.component_to_world,{3.4,1.8,.5}),angles(1.1,-.8,2)};
    in.authored={true,point(in.component_to_world,{0,0,5})};return in;}
void solve_check(control::LimbInput const& in){
    control::LimbInput before{};std::memcpy(&before,&in,sizeof(in));CHECK(control::solve(in,scratch,output.data())==control::Status::solved);
    CHECK(!std::memcmp(&before,&in,sizeof(in)));for(auto const& m:output)rigid(m);
    CHECK(near(point(in.component_to_world,position(output[2])),in.wrist_world.position));orientation(output[2],in.component_to_world,in.wrist_world.orientation);
    CHECK(!std::memcmp(output[0].values+12,in.component[0].values+12,12));
}
void geometry(){
    ++cases;auto in=input();CHECK(in.mode==gears_hand_two_bone::LengthMode::preserve);solve_check(in);const auto first=output;
    const auto old_upper=length(sub(point(in.component_to_world,position(in.component[1])),point(in.component_to_world,position(in.component[0]))));
    const auto old_lower=length(sub(point(in.component_to_world,position(in.component[2])),point(in.component_to_world,position(in.component[1]))));
    CHECK(std::abs(length(sub(position(output[1]),position(output[0])))-old_upper)<1e-5);CHECK(std::abs(length(sub(position(output[2]),position(output[1])))-old_lower)<1e-5);
    CHECK(position(output[1]).z>0);
    ++cases;in.previous=in.authored;in.authored.known=false;solve_check(in);for(unsigned i=0;i<3;++i)for(unsigned j=0;j<16;++j)CHECK(std::abs(output[i].values[j]-first[i].values[j])<1e-5);
    ++cases;in=input();in.authored={true,point(in.component_to_world,position(in.component[1]))};
    in.wrist_world={point(in.component_to_world,position(in.component[2])),qmul(angles(.4,-.3,.7),angles(.1,-.2,.4))};solve_check(in);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<16;++j)CHECK(std::abs(output[i].values[j]-in.component[i].values[j])<2e-6);
    ++cases;in=input();solve_check(in);const auto base_output=output;const Q q=angles(-.8,.6,-1.2);const M change=matrix({-30,10,25},q);
    in.component_to_world=compose(in.component_to_world,change);in.wrist_world.position=point(change,in.wrist_world.position);in.wrist_world.orientation=qmul(q,in.wrist_world.orientation);
    in.authored.point=point(change,in.authored.point);solve_check(in);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<16;++j)CHECK(std::abs(output[i].values[j]-base_output[i].values[j])<3e-5);
    ++cases;auto left=input();solve_check(left);const auto left_output=output;std::array<M,3> right_output{};auto right=input();
    for(auto& m:right.component)m.values[12]=-m.values[12];right.wrist_world.position=point(right.component_to_world,{-3.2,1.5,-.3});right.wrist_world.orientation=angles(-.9,.2,-1.1);
    CHECK(control::solve(right,scratch,right_output.data())==control::Status::solved);CHECK(!std::memcmp(left_output.data(),output.data(),sizeof(output)));
    CHECK(near(point(right.component_to_world,position(right_output[2])),right.wrist_world.position));orientation(right_output[2],right.component_to_world,right.wrist_world.orientation);
}
void adaptive(){
    ++cases;auto in=input();in.wrist_world.position=point(in.component_to_world,{25,10,3});saved=output;
    CHECK(control::solve(in,scratch,output.data())==control::Status::unreachable);CHECK(!std::memcmp(saved.data(),output.data(),sizeof(output)));
    in.mode=gears_hand_two_bone::LengthMode::adaptive;solve_check(in);CHECK(scratch.solution.length_adjusted);CHECK(length(sub(position(output[2]),position(output[0])))>25);
    ++cases;in=input();in.wrist_world.position=point(in.component_to_world,position(in.component[0]));saved=output;
    CHECK(control::solve(in,scratch,output.data())==control::Status::unreachable);CHECK(!std::memcmp(saved.data(),output.data(),sizeof(output)));
    in.mode=gears_hand_two_bone::LengthMode::adaptive;solve_check(in);CHECK(scratch.solution.length_adjusted);
}
void invalid(){
    for(unsigned i=0;i<13;++i){++cases;auto in=input();
        if(i==0)in.component[0].values[0]=(std::numeric_limits<float>::quiet_NaN)();
        if(i==1)for(unsigned k:{0u,5u,10u})in.component[0].values[k]*=2;
        if(i==2)in.component_to_world=matrix({},{}),in.component_to_world.values[0]=-1;
        if(i==3)in.component_to_world=matrix({},{}),in.component_to_world.values[1]=.01f;
        if(i==4)in.component[1].values[3]=.1f;
        if(i==5)in.component[2].values[15]=0;
        if(i==6)in.wrist_world.position.x=(std::numeric_limits<double>::infinity)();
        if(i==7)in.wrist_world.orientation={0,0,0,0};
        if(i==8)in.wrist_world.orientation={0,0,0,2};
        if(i==9)in.authored.point.y=(std::numeric_limits<double>::quiet_NaN)();
        if(i==10)in.mode=static_cast<gears_hand_two_bone::LengthMode>(99);
        if(i==11)std::memcpy(in.component[1].values+12,in.component[0].values+12,12);
        if(i==12)in.component_to_world=matrix({},{}),in.component_to_world.values[0]=in.component_to_world.values[5]=in.component_to_world.values[10]=1.0001f;
        saved=output;control::LimbInput before{};std::memcpy(&before,&in,sizeof(in));CHECK(control::solve(in,scratch,output.data())!=control::Status::solved);
        CHECK(!std::memcmp(saved.data(),output.data(),sizeof(output)));CHECK(!std::memcmp(&before,&in,sizeof(in)));}
    ++cases;auto in=input();control::LimbInput before{};std::memcpy(&before,&in,sizeof(in));saved=output;
    CHECK(control::solve(in,scratch,in.component)==control::Status::alias);CHECK(!std::memcmp(&before,&in,sizeof(in)));
    CHECK(control::solve(in,scratch,in.component+1)==control::Status::alias);CHECK(!std::memcmp(&before,&in,sizeof(in)));
    CHECK(control::solve(in,scratch,scratch.candidate)==control::Status::alias);CHECK(control::solve(in,scratch,nullptr)==control::Status::alias);
    CHECK(!std::memcmp(saved.data(),output.data(),sizeof(output)));
}
void single(){
    ++cases;const auto limb=input();control::SingleInput in{limb.component[2],limb.component_to_world,angles(1.7,-1.1,2.6)};
    in.component.values[12]=-0.0f;in.component.values[13]=2.25f;in.component.values[14]=-10;in.component.values[3]=-0.0f;
    const auto original=in;M result{};CHECK(control::replace_wrist_rotation(in,scratch,&result)==control::Status::solved);rigid(result);
    CHECK(!std::memcmp(result.values+12,in.component.values+12,12));for(unsigned i:{3u,7u,11u,15u})CHECK(!std::memcmp(&result.values[i],&in.component.values[i],4));
    orientation(result,in.component_to_world,in.world_orientation);CHECK(near(point(in.component_to_world,position(result)),point(in.component_to_world,position(in.component)),1e-12));
    CHECK(!std::memcmp(&in,&original,sizeof(in)));
    ++cases;in.world_orientation=qmul(angles(.4,-.3,.7),angles(.1,-.2,.4));CHECK(control::replace_wrist_rotation(in,scratch,&result)==control::Status::solved);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)CHECK(std::abs(result.values[4*i+j]-in.component.values[4*i+j])<2e-6);
    ++cases;const auto good=in;const auto saved_result=result;in.world_orientation={0,0,0,0};CHECK(control::replace_wrist_rotation(in,scratch,&result)==control::Status::invalid_target);CHECK(!std::memcmp(&result,&saved_result,64));
    in=good;in.component.values[0]*=3;CHECK(control::replace_wrist_rotation(in,scratch,&result)==control::Status::invalid_matrix);CHECK(!std::memcmp(&result,&saved_result,64));
    in=good;CHECK(control::replace_wrist_rotation(in,scratch,&in.component)==control::Status::alias);CHECK(control::replace_wrist_rotation(in,scratch,scratch.candidate)==control::Status::alias);
    CHECK(!std::memcmp(&in,&good,sizeof(in)));
}
template<class T> void read(std::ifstream& f,T* out,std::size_t n){f.read(reinterpret_cast<char*>(out),static_cast<std::streamsize>(n*sizeof(T)));CHECK(static_cast<bool>(f));}
void fixture(char const* path){
    ++cases;std::ifstream f(path,std::ios::binary);CHECK(static_cast<bool>(f));char magic[8]{};read(f,magic,8);CHECK(!std::memcmp(magic,"CCOP0001",8));
    control::LimbInput in{};read(f,in.component,3);read(f,&in.component_to_world,1);CHECK(f.peek()==std::ifstream::traits_type::eof());
    ik::Matrix world[3]{};ik::Pose original[3]{};const auto c=control::detail::unpack(in.component_to_world);
    for(unsigned i=0;i<3;++i){world[i]=ik::detail::multiply(control::detail::unpack(in.component[i]),c);CHECK(ik::world_pose(world[i],original[i])==ik::Status::okay);}
    const auto d=sub(original[2].position,original[0].position);const auto direction=scale(d,1/length(d));
    in.wrist_world={sub(original[2].position,scale(direction,4)),angles(-.2,.5,.8)};in.authored={true,original[1].position};solve_check(in);
    const double preserve_error=length(sub(point(in.component_to_world,position(output[2])),in.wrist_world.position));
    ++cases;in.wrist_world.position=add(original[0].position,scale(direction,2*(length(sub(original[1].position,original[0].position))+length(sub(original[2].position,original[1].position)))));
    in.mode=gears_hand_two_bone::LengthMode::adaptive;solve_check(in);const double adaptive_error=length(sub(point(in.component_to_world,position(output[2])),in.wrist_world.position));
    ++cases;control::SingleInput one{in.component[2],in.component_to_world,angles(.9,-1,.4)};M rotated{};
    CHECK(control::replace_wrist_rotation(one,scratch,&rotated)==control::Status::solved);CHECK(!std::memcmp(rotated.values+12,one.component.values+12,12));orientation(rotated,one.component_to_world,one.world_orientation);
    std::printf("RO2 C1 Space encoding sample only (not v238 temporary output): preserve_error=%.12g adaptive_error=%.12g\n",preserve_error,adaptive_error);
}
} // namespace
int main(int argc,char** argv){CHECK(argc==1||argc==2);geometry();adaptive();invalid();single();if(argc==2)fixture(argv[1]);std::printf("CoverControlOutputPoseTest: %u checks in %u cases passed; native_permission=0 live_ready=0\n",checks,cases);return 0;}
