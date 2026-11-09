#include "cover_point_overlap.h"
#include <cstdio>
#include <cstring>
#include <xmmintrin.h>
namespace p=gears_cover_point_overlap;namespace w=p::world;
using Vec=p::Vec;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
w::Primitive sphere(Vec center,float radius){w::Primitive s{};s.shape_world={center,{0,0,0,1}};
    s.dimensions.kind=w::metrics::Kind::sphere;s.dimensions.radius=radius;return s;}
w::Primitive capsule(Vec center,Vec axis,float half,float radius){auto s=sphere(center,radius);
    s.dimensions.kind=w::metrics::Kind::sphyl;s.dimensions.half_segment=half;s.segment_axis_world=axis;
    s.segment_start={center.x-axis.x*half,center.y-axis.y*half,center.z-axis.z*half};
    s.segment_end={center.x+axis.x*half,center.y+axis.y*half,center.z+axis.z*half};return s;}
void expect(w::Primitive const& shape,p::Box const& box,p::Relation relation,double distance=-1){
    const auto before=shape;const auto old_box=box;p::Result out{};
    C(p::classify(shape,box,out)==p::Status::evaluated);
    C(out.relation==relation);
    if(distance>=0)C(out.lower_squared<=distance&&out.upper_squared>=distance);
    C(!std::memcmp(&shape,&before,sizeof shape)&&!std::memcmp(&box,&old_box,sizeof box));
}
void boundary(w::Primitive const& shape,p::Box const& box){p::Result out{};
    C(p::classify(shape,box,out)==p::Status::evaluated);C(out.relation!=p::Relation::separated);}
std::uint32_t seed=0x1626a91u;
int random_int(int lower,int span){seed=seed*1664525u+1013904223u;return lower+int(seed%unsigned(span));}
long long gap(int lo,int hi,int other_lo,int other_hi){if(hi<other_lo)return other_lo-hi;if(lo>other_hi)return lo-other_hi;return 0;}
int main(){
    const p::Box unit{{-1,-1,-1},{1,1,1}};
    expect(sphere({0,0,0},1),unit,p::Relation::overlapping,0);
    expect(sphere({4,0,0},1),unit,p::Relation::separated,9);
    boundary(sphere({2,0,0},1),unit);
    expect(capsule({0,0,0},{1,0,0},4,1),unit,p::Relation::overlapping,0);
    expect(capsule({0,4,0},{1,0,0},4,1),unit,p::Relation::separated,9);
    boundary(capsule({0,2,0},{1,0,0},4,1),unit);
    // Bounding boxes overlap, while this diagonal capsule is separated from
    // the opposite box corner. A broad-phase-only answer would miss this gap.
    const double q=std::sqrt(.5);const p::Box corner{{1,-2,-.5},{2,-1,.5}};
    expect(capsule({0,0,0},{q,q,0},3,1),corner,p::Relation::separated,2);
    expect(capsule({0,0,0},{q,q,0},3,1.5f),corner,p::Relation::overlapping,2);
    const p::Box point{{5,0,0},{5,0,0}};
    expect(capsule({0,0,0},{1,0,0},3,1),point,p::Relation::separated,4);
    boundary(capsule({0,0,0},{1,0,0},4,1),point);
    expect(capsule({0,0,0},{0,0,1},0,1),unit,p::Relation::overlapping,0);
    expect(capsule({4,0,0},{0,0,1},0,1),unit,p::Relation::separated,9);
    // Independent integer oracles: exact squared distance to an AABB for a
    // point/sphere and an axis-aligned segment. These do not enumerate the
    // classifier's regions, evaluate quadratics, or share interval arithmetic.
    for(unsigned trial=0;trial<1200;++trial){
        const int center[]={random_int(-30,61),random_int(-30,61),random_int(-30,61)};
        const int low[]={random_int(-20,41),random_int(-20,41),random_int(-20,41)};
        const int high[]={low[0]+random_int(0,9),low[1]+random_int(0,9),low[2]+random_int(0,9)};
        const int radius=random_int(1,6),half=random_int(0,9),axis=random_int(0,3),sign=random_int(0,2)?1:-1;
        const p::Box box{{double(low[0]),double(low[1]),double(low[2])},{double(high[0]),double(high[1]),double(high[2])}};
        for(unsigned kind=0;kind<2;++kind){long long squared=0;
            for(unsigned k=0;k<3;++k){const int extent=kind&&k==unsigned(axis)?half:0;
                const auto d=gap(center[k]-extent,center[k]+extent,low[k],high[k]);squared+=d*d;}
            Vec direction{};if(axis==0)direction.x=sign;else if(axis==1)direction.y=sign;else direction.z=sign;
            const Vec c{double(center[0]),double(center[1]),double(center[2])};
            const auto shape=kind?capsule(c,direction,float(half),float(radius)):sphere(c,float(radius));p::Result out{};
            C(p::classify(shape,box,out)==p::Status::evaluated);
            C(out.lower_squared<=double(squared)&&out.upper_squared>=double(squared));
            if(squared>radius*radius)C(out.relation==p::Relation::separated);
            else if(squared<radius*radius)C(out.relation==p::Relation::overlapping);
            else C(out.relation!=p::Relation::separated);
        }
    }
    // Translate/permute the diagonal case without changing its true distance.
    expect(capsule({100,-200,300},{0,q,q},3,1),{{99.5,-199,298},{100.5,-198,299}},p::Relation::separated,2);
    for(unsigned defect=0;defect<12;++defect){auto shape=capsule({0,0,0},{1,0,0},3,1);auto box=unit;
        p::Result out{p::Relation::overlapping,12,13,7};const auto before=out;
        p::Status wanted=p::Status::invalid;
        switch(defect){
        case 0:shape.dimensions.radius=0;break;
        case 1:shape.dimensions.half_segment=-1;break;
        case 2:shape.segment_axis_world={2,0,0};break;
        case 3:shape.segment_end.x+=1;break;
        case 4:shape.shape_world.orientation={0,0,0,0};break;
        case 5:box.minimum.x=2;break;
        case 6:shape.shape_world.position.x=std::numeric_limits<double>::quiet_NaN();break;
        case 7:box.maximum.y=std::numeric_limits<double>::infinity();break;
        case 8:shape.dimensions.kind=w::metrics::Kind::box;wanted=p::Status::unsupported;break;
        case 9:shape.dimensions.kind=w::metrics::Kind::convex;wanted=p::Status::unsupported;break;
        case 10:shape=sphere({0,0,0},1);shape.segment_axis_world.x=1;break;
        default:shape=sphere({-1e308,0,0},1);box={{1e308,0,0},{1e308,0,0}};wanted=p::Status::numeric;break;
        }
        C(p::classify(shape,box,out)==wanted);C(!std::memcmp(&out,&before,sizeof out));
    }
    auto shape=sphere({0,0,0},1);const auto before_shape=shape;
    C(p::classify(shape,unit,*reinterpret_cast<p::Result*>(&shape))==p::Status::alias);
    C(!std::memcmp(&shape,&before_shape,sizeof shape));
    // Real FP-environment check on this isolated host test thread only. The
    // product function neither changes nor depends on the caller's FTZ/DAZ.
    const unsigned mxcsr=_mm_getcsr();
    for(unsigned mode=0;mode<2;++mode){_mm_setcsr(mode?mxcsr|0x8040u:mxcsr&~0x8040u);
        expect(sphere({4,0,0},1),unit,p::Relation::separated,9);
        p::Result out{p::Relation::overlapping,12,13,7};const auto before=out;
        C(p::classify(sphere({1e-200,0,0},1),{{0,0,0},{0,0,0}},out)==p::Status::numeric);
        C(!std::memcmp(&out,&before,sizeof out));
    }
    _mm_setcsr(mxcsr);
    std::printf("CoverPointOverlapTest checks=%u failures=%u synthetic_geometry=true scene_occupancy_verified=false\n",checks,failures);
    return failures?1:0;
}
