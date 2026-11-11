#include "cover_physx_observed_pose.h"
#include "cover_physx_shape_geometry.h"
#include <cstdio>
namespace o=gears_cover_physx_observed_pose;
unsigned checks{},failed{};
#define CHECK(x) do{++checks;if(!(x)){++failed;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
struct V {double x,y,z;};
static V add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
static V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static V scale(V a,double x){return {a.x*x,a.y*x,a.z*x};}
// Independent original (unnormalized) Rodrigues form I+2w[v]+2[v]^2.
static V original(std::array<float,4> q,V v){V xyz{q[0],q[1],q[2]};return add(v,scale(add(scale(cross(xyz,v),q[3]),cross(xyz,cross(xyz,v))),2));}
static V apply(o::Pose const& a,V v){auto m=a.matrix.values;return {m[0]*v.x+m[1]*v.y+m[2]*v.z+m[9],m[3]*v.x+m[4]*v.y+m[5]*v.z+m[10],m[6]*v.x+m[7]*v.y+m[8]*v.z+m[11]};}
static bool close(V a,V b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z)<3e-5;}
int main(){
 for(unsigned phase=0;phase<2;++phase)for(unsigned n=0;n<256;++n){double x=.2+std::sin(n*.11),y=-.3+std::cos(n*.07),z=.4+std::sin(n*.19),w=.8;
  const double norm=std::sqrt(x*x+y*y+z*z+w*w),f=std::sqrt(1+(phase?(n%2?.1:-.1):(n%2?2.9e-5:-2.9e-5)))/norm;
  std::array<float,4> q{float(x*f),float(y*f),float(z*f),float(w*f)};o::Pose a{},b{},joined{};
  CHECK(o::quaternion(q,{10,-20,30},a)&&o::qualified(a));CHECK(o::quaternion({0,.707106769f,.707106769f,0},{2,-3,4},b));
  CHECK(o::compose(a,b,joined)&&joined.gram_error>=o::gram_bound(joined.matrix));
  if(phase){o::m::Matrix strict{};CHECK(!o::m::quaternion(q,{10,-20,30},strict));}
  for(V v: {V{0,0,0},V{1,0,0},V{0,1,0},V{0,0,1},V{2.25,-3.5,4.75}}){
   const auto expected=add(original(q,add(original({0,.707106769f,.707106769f,0},v),{2,-3,4})),{10,-20,30});CHECK(close(apply(joined,v),expected));}
 }
 o::Pose p{},sentinel{};sentinel.matrix.values[0]=123;sentinel.gram_error=456;
 for(auto q:{std::array<float,4>{0,0,0,0},std::array<float,4>{0,0,0,std::numeric_limits<float>::infinity()},std::array<float,4>{std::numeric_limits<float>::denorm_min(),0,0,1}}){p=sentinel;CHECK(!o::quaternion(q,{0,0,0},p)&&p.matrix.values[0]==123&&p.gram_error==456);}
 const std::array<float,4> actual{.559741497f,-.394303977f,.607427657f,.402782083f};CHECK(o::quaternion(actual,{0,0,0},p)&&!o::m::rigid(p.matrix)&&p.gram_error>3e-5);
 gears_cover_physx_shape_world::Snapshot raw{};raw.parameters.owner.kind=gears_cover_physx_shape_owner::Kind::box;raw.parameters.box_dimensions={1,2,3};raw.shape_world=p.matrix;raw.shape_gram_error=p.gram_error;raw.parameters.stored_quaternion=actual;
 gears_cover_physx_shape_geometry::Geometry geometry{};CHECK(gears_cover_physx_shape_geometry::convert(raw,geometry)==gears_cover_physx_shape_geometry::Code::pose);
 o::Pose nonunit{};CHECK(o::quaternion({0,0,0,2},{0,0,0},nonunit)&&o::qualified(nonunit));
 raw.parameters.stored_quaternion={0,0,0,2};raw.shape_world=nonunit.matrix;raw.shape_gram_error=nonunit.gram_error;
 CHECK(o::m::rigid(nonunit.matrix)&&gears_cover_physx_shape_geometry::convert(raw,geometry)==gears_cover_physx_shape_geometry::Code::pose);
 auto rejected=sentinel;CHECK(!o::quaternion({.5f,.5f,0,0},{0,0,0},rejected)&&rejected.matrix.values[0]==123);
 auto reflected=p;reflected.matrix.values[0]*=-1;reflected.matrix.values[1]*=-1;reflected.matrix.values[2]*=-1;CHECK(!o::qualified(reflected));auto forged=p;forged.gram_error=0;CHECK(!o::qualified(forged));
 _clearfp();auto extreme=sentinel;CHECK(!o::quaternion({std::numeric_limits<float>::max()/4,0,0,1},{0,0,0},extreme)&&extreme.matrix.values[0]==123&&(_statusfp()&_SW_OVERFLOW)==0);
 o::Pose huge_a{},huge_b{},huge_out=sentinel;CHECK(o::quaternion({0,0,0,1},{std::numeric_limits<float>::max(),0,0},huge_a)&&o::quaternion({0,0,0,1},{std::numeric_limits<float>::max(),0,0},huge_b));
 _clearfp();CHECK(!o::compose(huge_a,huge_b,huge_out)&&huge_out.matrix.values[0]==123&&(_statusfp()&_SW_OVERFLOW)==0);
 unsigned saved{};CHECK(_controlfp_s(&saved,0,0)==0);unsigned ignored{};CHECK(_controlfp_s(&ignored,_RC_UP,_MCW_RC)==0);auto before=sentinel;const bool accepted=o::quaternion(actual,{0,0,0},before);CHECK(_controlfp_s(&ignored,saved,_MCW_RC)==0);CHECK(!accepted&&before.matrix.values[0]==123&&before.gram_error==456);
 std::printf("ObservedPoseMath checks=%u failed=%u unnormalized point oracle; no query permission\n",checks,failed);return failed?1:0;
}
