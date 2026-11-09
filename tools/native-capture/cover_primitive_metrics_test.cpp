#include "cover_primitive_metrics.h"
#include <cstdio>
#include <cstring>
#include <limits>
namespace m=gears_cover_primitive_metrics;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
unsigned bits(float value){unsigned result{};static_assert(sizeof result==sizeof value);std::memcpy(&result,&value,4);return result;}
int main(){m::Metrics out{};
    // Native game units are preserved. Geometry examples are synthetic;
    // there is deliberately no current-game acceptance or world pose here.
    C(m::compute({m::Kind::sphere,2,{3,0,0}},out)==m::Code::computed);C(out.radius==6&&out.half_segment==0);
    C(m::compute({m::Kind::box,2,{3,4,5}},out)==m::Code::computed);C((out.box_half_extents==std::array<float,3>{{3,4,5}}));
    C(out.radius==0&&out.half_segment==0);
    C(m::compute({m::Kind::sphyl,2,{3,4,0}},out)==m::Code::computed);C(out.radius==6&&out.half_segment==4);
    C(m::compute({m::Kind::sphyl,2,{3,0,0}},out)==m::Code::computed);C(out.radius==6&&out.half_segment==0);
    // Golden IEEE754 result of the original mulss(scalar,.5), mulss(...,dim)
    // ordering. A dimension-first implementation overflows for this input.
    const float maximum=(std::numeric_limits<float>::max)();
    C(m::compute({m::Kind::box,maximum,{1.5f,1.5f,1.5f}},out)==m::Code::computed);
    for(float x:out.box_half_extents)C(bits(x)==0x7f3fffffu);
    for(unsigned defect=0;defect<13;++defect){
        m::Input in{m::Kind::sphere,1,{1,0,0}};
        switch(defect){case 0:in.receiver_scalar=0;break;case 1:in.receiver_scalar=-1;break;
        case 2:in.receiver_scalar=std::numeric_limits<float>::infinity();break;
        case 3:in.raw[0]=std::numeric_limits<float>::quiet_NaN();break;case 4:in.raw[0]=0;break;
        case 5:in.raw[1]=1;break;case 6:in.raw[2]=1;break;
        case 7:in={m::Kind::sphyl,1,{1,-1,0}};break;case 8:in={m::Kind::box,1,{1,1,0}};break;
        case 9:in={m::Kind::sphere,maximum,{2,0,0}};break;
        case 10:in={m::Kind::box,std::numeric_limits<float>::denorm_min(),{2,2,2}};break;
        case 11:in.kind=m::Kind::convex;break;default:in.kind=static_cast<m::Kind>(42);break;}
        const auto before=out;C(m::compute(in,out)!=m::Code::computed);C(!std::memcmp(&before,&out,sizeof out));
    }
    std::printf("CoverPrimitiveMetricsTest checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
