#include "weapon_snub_magazine_mount.h"
#include <cstdio>
#include <cstdlib>
namespace {
unsigned checks{};void check(bool v,char const* label){++checks;if(!v){std::fprintf(stderr,"FAIL %s\n",label);std::exit(1);}}
using P=gears_vr::Pose;using V=gears_vr::Vector;
bool near(V a,V b){return gears_weapon::length(gears_weapon::difference(a,b))<1e-8;}
}
int main(){
    gears_weapon::SnubMagazineMount m;P wrist{{12000,7000,130},{0,0,0,1}},item{{12007,6998,134},{0,0,0,1}},out{};
    check(m.capture(wrist,item,wrist.position,8),"actual first contact binds fixed wrist-relative mount");
    check(m.apply(wrist,out)&&near(out.position,item.position),"first display has zero position jump");
    check(std::abs(out.orientation.w-1)<1e-8,"first display has zero orientation flip");
    check(!m.capture(wrist,item,wrist.position,8),"held mount cannot silently rebase");
    auto moved=wrist;moved.position.x+=20;check(m.apply(moved,out)&&near(out.position,{12027,6998,134}),"hand translation moves item by same displacement");
    moved=wrist;moved.orientation={0,0,std::sqrt(.5),std::sqrt(.5)};
    check(m.apply(moved,out)&&near(out.position,{12002,7007,134}),"quarter wrist turn rotates fixed offset about the hand");
    check(std::abs(out.orientation.z-std::sqrt(.5))<1e-8,"item rotates fully with wrist");
    moved.orientation={0,0,0,0};check(!m.apply(moved,out),"invalid wrist never generates a floating item pose");
    m.reset();check(!m.capture(wrist,item,{0,0,0},8),"remote/floor contact rejected");
    check(gears_vr::to_rotator_units(3.14159265358979323846/2)==16384,"radian quarter turn maps to engine quarter turn");
    std::array<V,16> mesh{};unsigned index=0;
    for(double z:{0.,10.})for(double x:{-1.,1.})for(double y:{-1.,1.}){mesh[index++]={x,y,z};mesh[index++]={x,y,z};}
    gears_weapon::SnubMagazineGeometry geometry{};
    check(gears_weapon::snub_magazine_geometry(mesh.size(),[&](auto i){return mesh[i];},{0,0,10},{0,0,-1},true,geometry)&&geometry.valid,"bounded original-mesh shape fitting");
    check(near(geometry.feed,{0,0,10})&&near(geometry.outward,{0,0,-1}),"feed at top and extraction away from gun");
    std::printf("Snub magazine mount: %u checks passed\n",checks);
}
