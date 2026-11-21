#include "weapon_gnasher_live_basis.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace w=gears_weapon;namespace v=gears_vr;namespace b=w::gnasher_live_basis;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %u: %s\n",unsigned(__LINE__),#x);std::exit(1);}}while(false)
bool near(double a,double c){return std::abs(a-c)<1e-10;}
bool vector(v::Vector a,v::Vector c){return near(a.x,c.x)&&near(a.y,c.y)&&near(a.z,c.z);}
bool rotation(v::Quaternion a,v::Quaternion c){return std::abs(a.x*c.x+a.y*c.y+a.z*c.z+a.w*c.w)>1-1e-10;}
int main(){
 const double h=std::sqrt(.5);
 // Independently written 90-degree component rotation and noncommuting
 // Main rotation. Actual world offsets include scale2 before this function.
 const v::Pose component{{100,200,300},{0,0,h,h}},main{{92,206,310},{.5,.5,.5,.5}};
 for(double units:{10.,100.,500.}){
  v::Pose relative{},target{};CHECK(b::relation(component,2,main,2,units,relative));
  CHECK(vector(relative.position,{6/units,8/units,10/units}));CHECK(rotation(relative.orientation,{h,0,0,h}));
  CHECK(b::component_for_main({{1,2,3},{}},relative,target));
  CHECK(vector(target.position,{1-6/units,2-10/units,3+8/units}));CHECK(rotation(target.orientation,{-h,0,0,h}));
 }
 // Static mesh reference and stored SG Main keys have opposite Z signs and
 // different Y records. Only the authenticated live pair supplies this basis.
 v::Pose relative{};const v::Pose actual{{17.54884147644043,0,9.907415390014648},{0,0,-h,h}};
 CHECK(b::relation({},1,actual,1,100,relative));CHECK(rotation(relative.orientation,actual.orientation));
 CHECK(!rotation(relative.orientation,{0,0,h,h}));CHECK(near(relative.position.y,0));
 for(unsigned i=0;i<9;++i){auto parent=component,child=main;double ps=2,cs=2,units=100;
  if(i==0)ps=0;if(i==1)cs=2.001;if(i==2)units=0;if(i==3)units=501;
  if(i==4)parent.orientation={0,0,0,0};if(i==5)child.orientation={0,0,0,0};
  if(i==6)child.position.x=std::numeric_limits<double>::infinity();
  if(i==7)ps=std::numeric_limits<double>::quiet_NaN();if(i==8)cs=std::numeric_limits<double>::quiet_NaN();
  CHECK(!b::relation(parent,ps,child,cs,units,relative));
 }
 CHECK(!b::component_for_main({{},{0,0,0,0}},{},relative));
 std::puts("Gnasher live component/Main basis: noncommuting actual rotations, scale applied once at10/100/500units, stored-key convention independence and malformed matrix-pose inputs rejected; numeric fixture only");
}
