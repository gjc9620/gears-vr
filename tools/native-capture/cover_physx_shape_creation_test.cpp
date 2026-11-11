#include "cover_physx_shape_creation.h"
#include <cstdio>
#include <limits>
namespace c=gears_cover_physx_shape_creation;
static unsigned checks{},failed{};
#define CHECK(x) do{++checks;if(!(x)){++failed;std::printf("FAIL %d %s\n",__LINE__,#x);}}while(false)
int main(){c::Input in{};c::Descriptor out{},sentinel{};sentinel.radius=123;
 in.tm={1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1};in.scale={2,3,4};in.dimensions={5,6,7};
 in.kind=c::Kind::sphere;CHECK(c::convert(in,out)==c::Code::converted);
 CHECK(out.radius==(.02f*2.f)*5.f+.025f);CHECK(out.local.values[9]==10.f*(.02f*2.f)&&out.local.values[10]==20.f*(.02f*3.f));
 in.kind=c::Kind::box;CHECK(c::convert(in,out)==c::Code::converted);
 for(unsigned k=0;k<3;++k)CHECK(out.half[k]==((.02f*2.f)*in.dimensions[k])*.5f+.025f);
 in.kind=c::Kind::capsule;CHECK(c::convert(in,out)==c::Code::converted);
 CHECK(out.full_segment_length==(.02f*2.f)*6.f);
 CHECK(out.local.values[0]==-1&&out.local.values[4]==0&&out.local.values[7]==1&&out.local.values[5]==1);
 // Rotate each UE basis about XYZ. The corrected PhysX Y column must equal
 // the original authored Z row for every rotation, without sign reversal.
 for(unsigned axis=0;axis<3;++axis)for(unsigned n=0;n<128;++n){
  const double a=n*.037;const unsigned x=(axis+1)%3,y=(axis+2)%3;
  for(unsigned r=0;r<3;++r)for(unsigned k=0;k<3;++k)in.tm[4*r+k]=r==k?1.f:0.f;
  in.tm[4*x+x]=float(std::cos(a));in.tm[4*x+y]=float(std::sin(a));in.tm[4*y+x]=-float(std::sin(a));in.tm[4*y+y]=float(std::cos(a));
  CHECK(c::convert(in,out)==c::Code::converted);
  for(unsigned k=0;k<3;++k)CHECK(out.local.values[3*k+1]==in.tm[8+k]);
 }
 in.no_rb_collision=1;out=sentinel;CHECK(c::convert(in,out)==c::Code::disabled&&out.radius==123);in.no_rb_collision=0;
 for(float bad:{-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
  in.dimensions[0]=bad;CHECK(c::convert(in,out)==c::Code::value&&out.radius==123);
 }
 in.dimensions[0]=5;in.scale[0]=0;CHECK(c::convert(in,out)==c::Code::value&&out.radius==123);in.scale[0]=2;
 in.dimensions[1]=0;CHECK(c::convert(in,out)==c::Code::converted&&out.full_segment_length==0);
 in.kind=c::Kind::convex;out=sentinel;CHECK(c::convert(in,out)==c::Code::unsupported&&out.radius==123);
 CHECK(c::convert(in,*reinterpret_cast<c::Descriptor*>(&in))==c::Code::alias);
 std::printf("ShapeCreation checks=%u failures=%u original descriptor math only; no runtime correspondence\n",checks,failed);return failed?1:0;
}
