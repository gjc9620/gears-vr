#include "weapon_troika_mechanics.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
namespace m=gears_weapon::troika_mechanics;
using V=gears_vr::Vector;
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::printf("failed %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
V add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V mul(V a,double b){return {a.x*b,a.y*b,a.z*b};}
V rx(V p,double a){return {p.x,std::cos(a)*p.y-std::sin(a)*p.z,std::sin(a)*p.y+std::cos(a)*p.z};}
V rz(V p,double a){return {std::cos(a)*p.x-std::sin(a)*p.y,std::sin(a)*p.x+std::cos(a)*p.y,p.z};}
double distance(V a,V b){const auto p=sub(a,b);return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
m::Input input(){
    m::Input in{};in.binding=1;in.calibration=2;in.units_per_metre=100;in.maximum_error_metres=.002;
    in.original_control_binding_and_reference_closed=true;in.control_bone={1,2};in.rotation_mask={4,1};
    in.space={m::Space::world,m::Space::parent_bone};in.component_world=m::identity();
    for(auto& bone:in.bone_world){bone=m::identity();bone[12]=100000;bone[13]=-250000;bone[14]=3100;}
    in.bone_world[2][13]+=.54840087890625f;in.bone_world[2][14]+=24.724105834960938f;
    in.anchors={V{99987,-250040,3135},V{100013,-250040,3135}};return in;
}
int main(){
    for(bool follows:{true,false}){
        auto in=input();if(!follows)in.space[1]=m::Space::world;m::Geometry g{};CHECK(m::geometry(in,g));
        CHECK(g.parent_control==0&&g.child_control==1&&g.serial.child_axis_follows_parent==follows);
        const V child=m::vector(g.serial.child.centre);
        for(double a:{-.7,-.3,0.,.3,.7})for(double b:{-.6,-.2,0.,.2,.6}){
            std::array<V,2> target{};
            for(unsigned side=0;side<2;++side){const auto local=mul(sub(in.anchors[side],g.world_origin),.01);
                const auto fitted=follows?rz(add(child,rx(sub(local,child),b)),a):add(rz(child,a),rx(sub(rz(local,a),rz(child,a)),b));
                target[side]=add(g.world_origin,mul(fitted,100));}
            gears_weapon::TroikaSerialCandidate out{};CHECK(m::propose(g,3,target,true,true,out));
            CHECK(std::abs(out.fit.parent_angle-a)<1e-4&&std::abs(out.fit.child_angle-b)<1e-4);
            V direction{};CHECK(m::direction(g,out,{0,1,0},direction));const auto expected=follows?rz(rx({0,1,0},b),a):rx(rz({0,1,0},a),b);
            CHECK(distance(direction,expected)<1e-5);
        }
        auto flipped=in;flipped.control_bone={2,1};flipped.rotation_mask={1,4};flipped.space={in.space[1],in.space[0]};
        m::Geometry other{};CHECK(m::geometry(flipped,other));
        CHECK(other.parent_control==1&&other.child_control==0&&other.serial.child_axis_follows_parent==follows);
        CHECK(other.serial.parent.axis.z==g.serial.parent.axis.z&&other.serial.child.axis.x==g.serial.child.axis.x);
    }
    {
        auto in=input();m::Geometry g{};CHECK(m::geometry(in,g));
        gears_weapon::TroikaSerialCalibration strict{g.serial.binding,g.serial.calibration,g.serial.parent,g.serial.child,
            g.serial.left,g.serial.right,g.serial.max_hand_error_m,false,g.serial.child_axis_follows_parent};
        gears_weapon::TroikaSolverSample sample{1,2,3,g.serial.left,g.serial.right,true,true,true,true,true};
        gears_weapon::TroikaSerialResult result{};CHECK(!gears_weapon::solve_troika_serial_grips(strict,sample,result)); // Old API proof is not weakened.
        gears_weapon::TroikaSerialCandidate candidate{};CHECK(m::propose(g,3,in.anchors,true,true,candidate));
        CHECK(candidate.binding==1&&candidate.calibration==2);auto wrong=g;wrong.serial.calibration++;
        V unchanged{7,8,9};CHECK(!m::direction(wrong,candidate,{0,1,0},unchanged));CHECK(unchanged.x==7&&unchanged.y==8&&unchanged.z==9);
    }
    for(auto space:{m::Space::world,m::Space::actor,m::Space::component,m::Space::parent_bone,m::Space::bone,m::Space::other_bone}){
        auto in=input();in.space[1]=space;in.other_bone[1]=0;m::Geometry g{};CHECK(m::geometry(in,g));
        CHECK(g.serial.child_axis_follows_parent==(space==m::Space::bone||space==m::Space::parent_bone));
    }
    {
        auto in=input();in.actor_rotation={0,16384,0};in.space[0]=m::Space::actor;in.rotation_mask[0]=1;
        in.rotation_mask[1]=4;in.space[1]=m::Space::world;m::Geometry g{};CHECK(m::geometry(in,g));
        CHECK(std::abs(g.serial.parent.axis.x)<1e-6&&std::abs(g.serial.parent.axis.y-1)<1e-6);
        in.space[0]=m::Space::component;in.component_world=m::actor_basis({0,16384,0});CHECK(m::geometry(in,g));
        CHECK(std::abs(g.serial.parent.axis.y-1)<1e-6);
    }
    m::Geometry sentinel{};std::memset(&sentinel,0x54,sizeof sentinel);
    for(unsigned fault=0;fault<14;++fault){auto in=input();auto g=sentinel;
        if(fault==0)in.original_control_binding_and_reference_closed=false;
        if(fault==1)in.rotation_mask[0]=3;
        if(fault==2)in.rotation_mask[0]=0;
        if(fault==3)in.control_bone[0]=2;
        if(fault==4)in.space[1]=m::Space::unknown;
        if(fault==5){in.space[0]=m::Space::other_bone;in.other_bone[0]=2;}
        if(fault==6){in.space[1]=m::Space::other_bone;in.other_bone[1]=9;}
        if(fault==7)in.bone_world[1][0]=-1;
        if(fault==8)in.bone_world[1][1]=.1f;
        if(fault==9)in.bone_world[1][5]=2;
        if(fault==10)in.units_per_metre=0;
        if(fault==11)in.anchors[0].x=std::numeric_limits<double>::quiet_NaN();
        if(fault==12)in.rotation_mask[1]=4;
        if(fault==13)in.maximum_error_metres=.26;
        CHECK(!m::geometry(in,g));CHECK(!std::memcmp(&g,&sentinel,sizeof g));
    }
    {
        auto in=input();m::Geometry g{};CHECK(m::geometry(in,g));gears_weapon::TroikaSerialCandidate sentinel_candidate{};
        std::memset(&sentinel_candidate,0x56,sizeof sentinel_candidate);
        for(unsigned fault=0;fault<4;++fault){auto out=sentinel_candidate;auto target=in.anchors;
            if(fault==3)target[0].x+=50;
            CHECK(!m::propose(g,fault==0?0:3,target,fault!=1,fault!=2,out));CHECK(!std::memcmp(&out,&sentinel_candidate,sizeof out));}
    }
    std::printf("Troika mechanical frame/candidate: %u checks passed; synthetic row matrices/axes and original-limit proof remains absent.\n",checks);
}
