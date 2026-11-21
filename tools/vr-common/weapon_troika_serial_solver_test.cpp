#include "weapon_troika_grip_solver.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace gears_weapon;
namespace d=troika_serial_detail;namespace old=troika_solver_detail;
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL %u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
// Independently specified X-pitch then Z-yaw affine matrices. Only the pivot
// separation/reference positions come from original-bones.json. Axis choices,
// angular limits and metres here are synthetic, not a retail certificate.
TroikaSolverPoint target(TroikaSolverPoint p,TroikaSolverPoint child,double a,double b){
    const double x=p.x,y=child.y+std::cos(b)*(p.y-child.y)-std::sin(b)*(p.z-child.z),
        z=child.z+std::sin(b)*(p.y-child.y)+std::cos(b)*(p.z-child.z);
    return {std::cos(a)*x-std::sin(a)*y,std::sin(a)*x+std::cos(a)*y,z};
}
TroikaSolverPoint basis(TroikaSolverPoint p,bool translate){
    const double c=std::cos(.7),s=std::sin(.7);
    return {c*p.x-s*p.y+(translate?.3:0),s*p.x+c*p.y+(translate?-.4:0),p.z+(translate?.2:0)};
}
int main(){
    TroikaSerialCalibration c{1,2,{{0,0,0},{0,0,1},-.8,.8},
        {{-2.39714630367871e-10,.0054840087890625,.24724105834960938},{1,0,0},-.6,.5},
        {-.13,-.41432830810546875,.35330780029296875},{.13,-.41432830810546875,.35330780029296875},.002,true};
    for(double a:{-.8,-.4,0.,.3,.8})for(double b:{-.6,-.25,0.,.2,.5}){
        TroikaSolverSample s{1,2,3,target(c.left,c.child.centre,a,b),target(c.right,c.child.centre,a,b),true,true,true,true,true};
        TroikaSerialResult r{};CHECK(solve_troika_serial_grips(c,s,r));
        CHECK(std::abs(r.parent_angle-a)<1e-5&&std::abs(r.child_angle-b)<1e-5&&r.frame_ticket==3);
        CHECK(old::distance(r.left,r.right)>old::distance(c.left,c.right)-1e-8&&old::distance(r.left,r.right)<old::distance(c.left,c.right)+1e-8);
        CHECK(old::distance(r.child_centre,c.parent.centre)>old::distance(c.child.centre,c.parent.centre)-1e-8);
        // World basis/origin changes must preserve the SAME relative angles.
        auto transformed=c;transformed.parent.centre=basis(c.parent.centre,true);transformed.child.centre=basis(c.child.centre,true);
        transformed.parent.axis=basis(c.parent.axis,false);transformed.child.axis=basis(c.child.axis,false);
        transformed.left=basis(c.left,true);transformed.right=basis(c.right,true);
        auto sample=s;sample.left=basis(s.left,true);sample.right=basis(s.right,true);TroikaSerialResult changed{};
        CHECK(solve_troika_serial_grips(transformed,sample,changed));
        CHECK(std::abs(changed.parent_angle-a)<1e-5&&std::abs(changed.child_angle-b)<1e-5);
    }
    const auto original=target(c.left,c.child.centre,.3,-.6),coincident=target(c.left,{},.3,-.6);
    CHECK(old::distance(original,coincident)>.01); // >10 cm error from collapsing the original centres.
    TroikaSolverSample s{1,2,4,target(c.left,c.child.centre,.3,.2),target(c.right,c.child.centre,.3,.2),true,true,true,true,true};
    TroikaSerialResult sentinel{9,8,7,6,{1,2,3},{4,5,6},{7,8,9},{1,0,0}};
    for(unsigned fault=0;fault<13;++fault){auto bad=c;auto sample=s;auto r=sentinel;
        switch(fault){case 0:bad.original_hierarchy_axes_limits_verified=false;break;
        case 1:bad.child.axis=bad.parent.axis;break;case 2:bad.child.axis.x=2;break;
        case 3:bad.child.centre.x=std::nan("");break;case 4:bad.parent.maximum=bad.parent.minimum;break;
        case 5:sample.left_grip=false;break;case 6:sample.right_tracked=false;break;
        case 7:sample.binding=2;break;case 8:sample.pivot_basis_verified=false;break;
        case 9:sample.left.x+=.3;break;case 10:sample.right.z=std::nan("");break;
        case 11:bad.max_hand_error_m=.3;break;default:bad.child.minimum= -3.2;break;}
        CHECK(!solve_troika_serial_grips(bad,sample,r));
        CHECK(r.parent_angle==sentinel.parent_angle&&r.child_angle==sentinel.child_angle&&r.frame_ticket==sentinel.frame_ticket&&r.left.x==sentinel.left.x);
    }
    auto limited=c;limited.parent.minimum= -.1;limited.parent.maximum=.1;
    auto r=sentinel;CHECK(!solve_troika_serial_grips(limited,s,r));CHECK(r.frame_ticket==sentinel.frame_ticket);
    auto singular=c;singular.left=singular.child.centre;singular.right=d::add(singular.child.centre,{.2,0,0});
    auto sample=s;sample.left=singular.left;sample.right=singular.right;CHECK(!solve_troika_serial_grips(singular,sample,r));
    std::printf("Troika serial original-centre geometry: %u checks passed; synthetic axes/limits only, no game permit.\n",checks);
}
