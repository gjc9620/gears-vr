#include "cover_body_query_catalog.h"
#include <cstdio>
int main(){namespace c=gears_cover_body_query_catalog;namespace g=c::g;unsigned checks{},failed{};
#define C(x) do{++checks;if(!(x)){++failed;printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
 g::names::Mapping mapping{};mapping.count=1;mapping.geometry=g::body::rig::marcus115_signature;mapping.root_bone_index=2;
 mapping.rows[0].bone_index=2;mapping.rows[0].name={10,0};mapping.reference_names[2]={10,0};g::Inventory inventory{};inventory.count={1,0,0,0};
 g::Geometry geometry{};geometry.body_count=geometry.count=1;auto& shape=geometry.rows[0].primitive;
 shape.bone_index=2;shape.bone={10,0};shape.dimensions.kind=g::metrics::Kind::sphere;shape.dimensions.radius=2;shape.shape_world.position={10,20,30};
 c::Workspace work{};c::Catalog output{};C(c::prepare(mapping,&inventory,1,geometry,work,output,.001)==c::Code::prepared);
 const float factor=.02f;const auto& sphere=output.rows[0].shape.sphere;
 C(sphere.centre.x==10.f*factor&&sphere.centre.y==20.f*factor&&sphere.centre.z==30.f*factor);
 C(sphere.radius>=2.f*factor&&sphere.radius<.041f);
 namespace units=gears_cover_query_units;namespace p=c::p;using Vec=g::body::Vec;
 C(output.units==p::Units::physx&&output.rows[0].shape.units==p::Units::physx);
 const auto saved=output;
 C(c::prepare(mapping,&inventory,1,geometry,work,output,0)==c::Code::precision&&!memcmp(&saved,&output,sizeof output));
 g::world::Primitive input{};input.bone_index=2;input.bone={10,0};input.dimensions.kind=g::metrics::Kind::sphere;input.dimensions.radius=1;
 p::Shape native{};input.shape_world.position={1,0,0};
 C(units::convert(input,0,native)==p::Code::converted&&native.sphere.centre.x==factor&&native.sphere.radius==factor&&native.radius_expansion==0);
 p::Shape generic{};C(p::convert(input,generic)==p::Code::converted&&generic.units==p::Units::unspecified&&generic.sphere.centre.x==1);
 for(unsigned i=1;i<=128;++i){input={};input.bone_index=2;input.bone={10,0};input.dimensions.kind=g::metrics::Kind::sphere;
  input.dimensions.radius=1.5f;input.shape_world.position={double(i)/7.,-double(i)/11.,double(i)/13.};
  C(units::convert(input,.001,native)==p::Code::converted&&native.units==p::Units::physx);
  const double error=std::hypot(input.shape_world.position.x*units::factor-native.sphere.centre.x,
    input.shape_world.position.y*units::factor-native.sphere.centre.y,input.shape_world.position.z*units::factor-native.sphere.centre.z);
  C(double(native.sphere.radius)>=1.5*units::factor+error&&native.radius_expansion<=.001*units::factor);
  input.dimensions.kind=g::metrics::Kind::sphyl;input.segment_start={double(i)/3.,2./7.,3};input.segment_end={double(i)/5.,2./9.,7};
  C(units::convert(input,.001,native)==p::Code::converted);
  for(unsigned n=0;n<=8;++n){const double t=double(n)/8.;const auto target=g::body::d::mul(g::body::d::add(g::body::d::mul(input.segment_start,1-t),g::body::d::mul(input.segment_end,t)),units::factor);
   const Vec a{native.capsule.first.x,native.capsule.first.y,native.capsule.first.z},b{native.capsule.second.x,native.capsule.second.y,native.capsule.second.z};
   const auto rounded=g::body::d::add(g::body::d::mul(a,1-t),g::body::d::mul(b,t));
   C(g::body::d::length(g::body::d::sub(target,rounded))+1.5*units::factor<=native.capsule.radius);}
  input={};input.bone_index=2;input.bone={10,0};input.dimensions.kind=g::metrics::Kind::box;input.dimensions.box_half_extents={1,2,3};
  const double t=double(i)/37.;g::body::Quat q{.3*std::sin(t),.4*std::cos(t),.5*std::sin(2*t),1};
  const double norm=std::hypot(std::hypot(q.x,q.y),std::hypot(q.z,q.w));q={q.x/norm,q.y/norm,q.z/norm,q.w/norm};input.shape_world.orientation=q;
  input.shape_world.position={double(i)/7.,-double(i)/11.,double(i)/13.};C(units::convert(input,.001,native)==p::Code::converted);
  const auto* m=native.box.rotation;const Vec x{m[0],m[3],m[6]},y{m[1],m[4],m[7]},z{m[2],m[5],m[8]};
  auto cross=[](Vec a,Vec b){return Vec{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
  auto dot=[](Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;};
  const Vec inverse[]={cross(y,z),cross(z,x),cross(x,y)},axes[]={x,y,z};const double det=dot(x,inverse[0]);C(det>0);
  const double halves[]={native.box.half.x,native.box.half.y,native.box.half.z};
  for(unsigned corner=0;corner<8;++corner){const Vec local{corner&1?1.:-1.,corner&2?2.:-2.,corner&4?3.:-3.};
   const auto point=g::body::d::mul(g::body::d::add(input.shape_world.position,g::body::d::rotate(q,local)),units::factor);
   const auto delta=g::body::d::sub(point,{native.box.centre.x,native.box.centre.y,native.box.centre.z});
   for(unsigned axis=0;axis<3;++axis){C(std::abs(dot(inverse[axis],delta)/det)<=halves[axis]+1e-12);C(std::abs(dot(axes[axis],delta))<=halves[axis]+1e-12);}}
  for(double expansion:native.box_half_expansion)C(expansion>=0&&expansion<=.001*units::factor);
 }
 const auto before=native;C(units::convert(input,-1,native)==p::Code::invalid&&!memcmp(&before,&native,sizeof native));
 C(units::convert(input,1e-15,native)==p::Code::precision&&!memcmp(&before,&native,sizeof native));
 unsigned mode{};C(_controlfp_s(&mode,0,0)==0);const auto original=mode;C(_controlfp_s(&mode,_RC_DOWN,_MCW_RC)==0);
 C(units::convert(input,.001,native)==p::Code::precision&&!memcmp(&before,&native,sizeof native));C(_controlfp_s(&mode,original&_MCW_RC,_MCW_RC)==0);
 printf("BodyQueryUnits checks=%u failures=%u original game-to-physics boundary only\n",checks,failed);return failed?1:0;
}
