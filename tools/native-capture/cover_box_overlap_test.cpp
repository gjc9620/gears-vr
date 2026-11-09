#include "cover_box_overlap.h"
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>
namespace p=gears_cover_box_overlap;namespace w=p::world;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
w::Primitive shape(w::Vec center,w::Quat q={0,0,0,1},std::array<float,3> extent={1,1,1}){
    w::Primitive s{};s.shape_world={center,q};s.dimensions.kind=w::metrics::Kind::box;
    s.dimensions.box_half_extents=extent;return s;
}
void expect(w::Primitive const& s,p::Box const& b,p::Relation wanted){
    const auto before=s;const auto box_before=b;p::Result out{};
    C(p::classify(s,b,out)==p::Status::evaluated);C(out.relation==wanted);
    C(out.tested_axes>=6&&out.tested_axes<=15);
    C(!std::memcmp(&before,&s,sizeof s)&&!std::memcmp(&box_before,&b,sizeof b));
}
std::uint32_t seed=0x1265a99u;
int random_int(int start,int count){seed=seed*1664525u+1013904223u;return start+int(seed%unsigned(count));}
int main(){
    const p::Box unit{{-1,-1,-1},{1,1,1}};
    expect(shape({0,0,0}),unit,p::Relation::overlapping);
    expect(shape({3,0,0}),unit,p::Relation::separated);
    expect(shape({0,3,0}),unit,p::Relation::separated);
    expect(shape({0,0,3}),unit,p::Relation::separated);
    p::Result touch{};C(p::classify(shape({2,0,0}),unit,touch)==p::Status::evaluated);
    C(touch.relation!=p::Relation::separated);
    expect(shape({0,0,0},{0,0,0,1},{3,4,5}),{{0,0,0},{0,0,0}},p::Relation::overlapping);
    // Exactly representable rational rotations; these are true 3D orientations.
    expect(shape({0,0,0},{.5,.5,.5,.5},{3,1,2}),unit,p::Relation::overlapping);
    expect(shape({10,0,0},{.5,.5,.5,.5},{3,1,2}),unit,p::Relation::separated);
    expect(shape({0,0,0},{.5,.5,.5,.5},{.1f,.2f,.3f}),unit,p::Relation::overlapping);
    // Exact rational edge clipping proves these boxes disjoint although all
    // SIX face-axis projections overlap. The omitted nine cross axes matter.
    p::Result cross{};
    C(p::classify(shape({-128,24,80},{.5163977794943223,.6024640760767093,.6024640760767093,-.08606629658238704},
        {64,48,88}),{{-48,-88,32},{80,-40,144}},cross)==p::Status::evaluated);
    C(cross.relation==p::Relation::separated&&cross.separating_axis>=7);
    // Quaternion sign and near-unit storage normalization preserve orientation.
    expect(shape({0,0,0},{-.5,-.5,-.5,-.5},{3,1,2}),unit,p::Relation::overlapping);
    expect(shape({10,0,0},{.50001,.50001,.50001,.50001},{3,1,2}),unit,p::Relation::separated);
    // Independent exact integer overlap oracle for aligned boxes. Covers all
    // signs, zero-thickness query volumes, disjoint axes and true contacts.
    for(unsigned trial=0;trial<1000;++trial){
        const int center[]={random_int(-20,41),random_int(-20,41),random_int(-20,41)};
        const int extent[]={random_int(1,8),random_int(1,8),random_int(1,8)};
        const int low[]={random_int(-20,41),random_int(-20,41),random_int(-20,41)};
        const int high[]={low[0]+random_int(0,12),low[1]+random_int(0,12),low[2]+random_int(0,12)};
        bool separate=false,tangent=false;
        for(unsigned k=0;k<3;++k){
            separate|=center[k]+extent[k]<low[k]||center[k]-extent[k]>high[k];
            tangent|=center[k]+extent[k]==low[k]||center[k]-extent[k]==high[k];
        }
        const auto s=shape({double(center[0]),double(center[1]),double(center[2])},{0,0,0,1},
            {float(extent[0]),float(extent[1]),float(extent[2])});
        const p::Box b{{double(low[0]),double(low[1]),double(low[2])},{double(high[0]),double(high[1]),double(high[2])}};
        p::Result out{};C(p::classify(s,b,out)==p::Status::evaluated);
        if(separate)C(out.relation==p::Relation::separated);
        else if(tangent)C(out.relation!=p::Relation::separated);
        else C(out.relation==p::Relation::overlapping);
    }
    for(unsigned defect=0;defect<16;++defect){auto s=shape({0,0,0});auto box=unit;
        p::Status wanted=p::Status::invalid;
        switch(defect){
        case 0:s.dimensions.box_half_extents[1]=0;break;
        case 1:s.dimensions.box_half_extents[1]=-1;break;
        case 2:s.shape_world.orientation={0,0,0,0};break;
        case 3:s.shape_world.position.x=std::numeric_limits<double>::quiet_NaN();break;
        case 4:box.minimum.z=2;break;
        case 5:s.segment_start.x=1;break;
        case 6:s.dimensions.radius=1;break;
        case 7:s.dimensions.half_segment=1;break;
        case 8:s.dimensions.kind=w::metrics::Kind::convex;wanted=p::Status::unsupported;break;
        case 9:s.dimensions.kind=w::metrics::Kind::sphere;wanted=p::Status::unsupported;break;
        case 10:s.dimensions.kind=w::metrics::Kind::sphyl;wanted=p::Status::unsupported;break;
        case 11:s.shape_world.position.x=-1e308;box={{1e308,0,0},{1e308,0,0}};wanted=p::Status::numeric;break;
        case 12:s.shape_world.position.x=std::numeric_limits<double>::denorm_min();wanted=p::Status::numeric;break;
        case 13:s.shape_world.orientation.w=std::numeric_limits<double>::quiet_NaN();break;
        case 14:s.shape_world.orientation={2,0,0,0};break;
        default:s.shape_world.orientation={0,0,1e-200,1};wanted=p::Status::numeric;break;
        }
        p::Result out{p::Relation::overlapping,12,13,7};const auto before=out;
        C(p::classify(s,box,out)==wanted);C(!std::memcmp(&out,&before,sizeof out));
    }
    auto s=shape({0,0,0});const auto before=s;
    C(p::classify(s,unit,*reinterpret_cast<p::Result*>(&s))==p::Status::alias);
    C(!std::memcmp(&s,&before,sizeof s));
    auto box_alias=unit;const auto box_before=box_alias;
    C(p::classify(s,box_alias,*reinterpret_cast<p::Result*>(&box_alias))==p::Status::alias);
    C(!std::memcmp(&box_alias,&box_before,sizeof box_alias));
    const unsigned original=_mm_getcsr();
    for(unsigned mode=0;mode<8;++mode){const unsigned state=(original&~0xe040u)|((mode%4)<<13)|(mode>=4?0x8040u:0);
        _mm_setcsr(state);p::Result out{};const auto status=p::classify(shape({4,0,0}),unit,out);
        const auto after=_mm_getcsr();_mm_setcsr(original);
        C(status==p::Status::evaluated&&out.relation==p::Relation::separated);C(!((state^after)&0xe040u));
    }
    std::printf("CoverBoxOverlapTest checks=%u failures=%u synthetic_geometry=true scene_occupancy_verified=false\n",checks,failures);
    return failures?1:0;
}
