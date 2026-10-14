#include "ui_reticle_direction_plan.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_vr;
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static StereoMatrix projection(double l=-.7853981633974483,double r=.7853981633974483,
                               double u=.7853981633974483,double b=-.7853981633974483){
    const double left=std::tan(l),right=std::tan(r),up=std::tan(u),bottom=std::tan(b);
    return {float(2/(right-left)),0,0,0,0,float(2/(up-bottom)),0,0,
        float(-(right+left)/(right-left)),float(-(up+bottom)/(up-bottom)),1,1,0,0,-1,0};
}
static StereoTicketGeometry geometry(){StereoTicketGeometry g{};g.source_view=identity();g.source_projection=projection();
    for(unsigned i=0;i<2;++i){g.eyes.view[i]=identity();g.eyes.projection[i]=projection();}return g;}
static gears_viewport::DrawState state(){return {true,{0,0,3840,2160},{0,0,3840,2160},1,1,true,0,1};}
static void near(double a,double b,double epsilon=.0004){assert(std::abs(a-b)<=epsilon);}
static bool same(gears_viewport::OverlayPlan const& a,gears_viewport::OverlayPlan const& b){return a.valid==b.valid&&a.viewports==b.viewports&&a.scissors==b.scissors;}
static void reject(StereoTicketGeometry const& g,gears_viewport::DrawState s=state()){
    gears_viewport::OverlayPlan out{true,{{{1,2,3,4},{5,6,7,8}}},{{{9,10,11,12},{13,14,15,16}}}};
    const auto old=out;assert(!gears_ui_reticle::plan(s,3840,2160,g,out)&&same(out,old));
}
struct ExtentFixture {
    gears_ui_receipts::Claim claim{};gears_ui_indices::Snapshot snapshot{};
    explicit ExtentFixture(unsigned quads=1){gears_ui_receipts::Store store;
        for(unsigned i=0;i<quads;++i){gears_ui_batch::Receipt r{};r.canvas=1;r.fcanvas=2;r.item=3;r.payload=4;r.begin=i*4;r.end=r.begin+4;
            const float x=quads==1?1910.f:(i%2?1920.f:1900.f),y=quads==1?1070.f:(i/2?1080.f:1060.f);
            const float size=20;const float points[4][2]={{x,y},{x+size,y},{x,y+size},{x+size,y+size}};
            for(unsigned j=0;j<4;++j){const float p[4]={points[j][0],points[j][1],0,1};std::memcpy(r.vertices[j].data(),p,16);}
            assert(store.publish(r,1,77));}
        assert(store.claim(4,claim));snapshot.payload=4;snapshot.generation=claim.generation;snapshot.vertex_count=quads*4;snapshot.segment_count=1;snapshot.total_indices=quads*6;
        auto& segment=snapshot.segments[0];segment.maximum=int(quads*4-1);segment.index_count=quads*6;segment.kind_mask=1;segment.entry_mask=(1u<<quads)-1;segment.known[0]=segment.index_count;segment.expected={segment.index_count,1,0,0,0};
        const unsigned indices[6]={0,1,2,1,3,2};for(unsigned i=0;i<quads;++i)for(unsigned j=0;j<6;++j)snapshot.indices[i*6+j]=snapshot.global_vertices[i*6+j]=i*4+indices[j];
    }
    void position(unsigned vertex,unsigned component,float value){auto copy=std::make_shared<gears_ui_receipts::VertexBuffer>(*claim.vertices);std::memcpy(copy->data[vertex].data()+component*4,&value,4);claim.vertices=copy;}
};
static void extent_tests(){using namespace gears_ui_reticle;Extent e{};ExtentFixture f;
    assert(capture_extent(f.claim,f.snapshot,0,e)&&centered(e,3840,2160));assert(e.left==1910&&e.right==1930&&e.root_invocation==77);
    ExtentFixture expanded(4);assert(capture_extent(expanded.claim,expanded.snapshot,0,e)&&centered(e,3840,2160));assert(e.left==1900&&e.right==1940&&e.top==1060&&e.bottom==1100);
    const auto unchanged=e;auto failed=[&](ExtentFixture const& fixture){assert(!capture_extent(fixture.claim,fixture.snapshot,0,e)&&e.valid==unchanged.valid&&e.left==unchanged.left&&e.root_invocation==unchanged.root_invocation);};
    ExtentFixture off;for(unsigned i=0;i<4;++i)off.position(i,0,2000.f+float(i%2)*20);assert(capture_extent(off.claim,off.snapshot,0,e)&&!centered(e,3840,2160));e=unchanged;
    ExtentFixture mixed;mixed.claim.entries[0].kind=2;mixed.claim.kind_mask=2;failed(mixed);
    ExtentFixture source;source.claim.entries[0].source=gears_ui_receipts::Source::CanvasText;failed(source);
    ExtentFixture roots(4);roots.claim.entries[2].root_invocation=78;failed(roots);
    ExtentFixture index;index.snapshot.indices[0]=index.snapshot.global_vertices[0]=999;failed(index);
    ExtentFixture metadata;metadata.snapshot.segments[0].entry_mask=2;failed(metadata);
    ExtentFixture z;z.position(0,2,1);failed(z);ExtentFixture w;w.position(0,3,0);failed(w);
    ExtentFixture nan;nan.position(0,0,std::numeric_limits<float>::infinity());failed(nan);
    ExtentFixture bounds;bounds.claim.entries[0].vertex_offset=1024;failed(bounds);
    Extent too_wide{true,0,1070,3840,1090,77};assert(!centered(too_wide,3840,2160));Extent too_tall{true,1910,0,1930,2160,77};assert(!centered(too_tall,3840,2160));
    Extent edge{true,1910.5,1070.5,1930.5,1090.5,77};assert(centered(edge,3840,2160));edge.left+=1;edge.right+=1;assert(!centered(edge,3840,2160));
    assert(!centered(Extent{},3840,2160));
}
int main(){auto g=geometry();auto s=state();gears_viewport::OverlayPlan p{};
    extent_tests();
    assert(gears_ui_reticle::plan(s,3840,2160,g,p));assert(same(p,gears_viewport::overlay_plan(s,3840,2160)));
    auto asym=g;asym.eyes.projection[0]=projection(-.942477763,.69813168,.767595768,-.947190166);
    asym.eyes.projection[1]=projection(-.69813168,.942477763,.767595768,-.947190166);
    assert(gears_ui_reticle::plan(s,3840,2160,asym,p));near(p.viewports[0].x+960,1192.8121261);near(p.viewports[1].x+960,2647.1878739);near(p.viewports[0].y+1080,885.0617101);
    for(unsigned i=0;i<2;++i){assert(p.viewports[i].width==1920&&p.viewports[i].height==2160);auto c=p.scissors[i];assert(c.x>=i*1920&&c.x+c.width<=(i+1)*1920&&c.y>=0&&c.y+c.height<=2160);}
    assert(p.scissors[0].x==232&&p.scissors[0].width==1688&&p.scissors[0].y==0&&p.scissors[0].height==1966);
    const auto unchanged=p;auto translated=asym;translated.source_view[12]=120;translated.source_view[13]=-8;translated.source_view[14]=30;
    for(unsigned i=0;i<2;++i){translated.eyes.view[i][12]=i?1000.f:-1000.f;translated.eyes.view[i][13]=17;translated.eyes.view[i][14]=-400;}
    assert(gears_ui_reticle::plan(s,3840,2160,translated,p)&&same(p,unchanged));
    // Rotate source center toward world +X; independent tangent oracle.
    auto rotated=g;const float c=float(std::cos(.2)),sn=float(std::sin(.2));
    rotated.source_view={c,0,sn,0,0,1,0,0,-sn,0,c,0,0,0,0,1};
    assert(gears_ui_reticle::plan(s,3840,2160,rotated,p));near(p.viewports[0].x,960*std::tan(.2));
    for(unsigned i=0;i<2;++i)rotated.eyes.view[i]=rotated.source_view;
    assert(gears_ui_reticle::plan(s,3840,2160,rotated,p));near(p.viewports[0].x,0);near(p.viewports[1].x,1920);
    // Source asymmetric projection center is not camera +Z.
    auto source=asym;source.source_projection=asym.eyes.projection[0];assert(gears_ui_reticle::plan(s,3840,2160,source,p));near(p.viewports[0].x,0);near(p.viewports[0].y,0);
    auto partial=s;partial.scissor={100,200,1000,600};assert(gears_ui_reticle::plan(partial,3840,2160,asym,p));
    assert(p.scissors[0].x==282&&p.scissors[0].width==501&&p.scissors[0].y==5&&p.scissors[0].height==601);
    auto bad=g;bad.source_view[0]=0;reject(bad);bad=g;bad.source_projection[0]=0;reject(bad);
    bad=g;bad.eyes.view[1][0]=-1;bad.eyes.view[1][10]=-1;reject(bad); // Behind eye.
    bad=g;bad.source_view[12]=std::numeric_limits<float>::infinity();reject(bad);
    bad=g;bad.eyes.projection[0][8]=std::numeric_limits<float>::quiet_NaN();reject(bad);
    bad=g;bad.eyes.projection[1][8]=2;reject(bad); // Direction outside eye bounds.
    bad=g;bad.source_projection[3]=1;reject(bad);bad=g;bad.eyes.view[0][3]=1;reject(bad);
    auto invalid=s;invalid.viewport_count=2;reject(g,invalid);invalid=s;invalid.scissor.width=0;reject(g,invalid);
    invalid=s;invalid.scissor={0,0,10,10};reject(asym,invalid); // Shifted clip empty.
    std::puts("ui_reticle_direction_plan tests passed");
}
