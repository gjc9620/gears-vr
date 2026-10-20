#include "ui_reticle_direction_plan.h"
#include "../vr-common/ui_projection.h"
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
    explicit ExtentFixture(unsigned quads=1){auto store=std::make_unique<gears_ui_receipts::Store>();
        for(unsigned i=0;i<quads;++i){gears_ui_batch::Receipt r{};r.canvas=1;r.fcanvas=2;r.item=3;r.payload=4;r.begin=i*4;r.end=r.begin+4;
            const float x=quads==1?1910.f:(i%2?1920.f:1900.f),y=quads==1?1070.f:(i/2?1080.f:1060.f);
            const float size=20;const float points[4][2]={{x,y},{x+size,y},{x,y+size},{x+size,y+size}};
            for(unsigned j=0;j<4;++j){const float p[4]={points[j][0],points[j][1],0,1};std::memcpy(r.vertices[j].data(),p,16);}
            assert(store->publish(r,1,77));}
        assert(store->claim(4,claim));snapshot.payload=4;snapshot.generation=claim.generation;snapshot.vertex_count=quads*4;snapshot.segment_count=1;snapshot.total_indices=quads*6;
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
    // Four expanding-ring quadrants in separate native draw calls must use
    // the same correction as the already centered icon, retaining recoil gaps.
    std::array<Extent,gears_ui_indices::segment_capacity> split{};
    for(unsigned i=0;i<4;++i){auto snap=expanded.snapshot;snap.segment_count=4;
        for(unsigned j=0;j<4;++j){auto& s=snap.segments[j];s=expanded.snapshot.segments[0];
            s.index_offset=j*6;s.index_count=6;s.known[0]=6;s.entry_mask=1u<<j;
            s.minimum=j*4;s.maximum=j*4+3;s.expected={6,1,0,0,0};
            for(unsigned k=0;k<6;++k)snap.indices[j*6+k]-=j*4;}
        assert(capture_extent(expanded.claim,snap,i,split[i]));
        assert(!centered(split[i],3840,2160));
    }
    split[4]={true,1915,1075,1925,1085,78}; // A separate icon root.
    split[5]={true,100,100,120,120,79}; // Unrelated off-center helper.
    const auto original=split;group_extents(split,6);
    for(unsigned i=0;i<4;++i){assert(centered(split[i],3840,2160));
        assert(split[i].left==1900&&split[i].right==1940&&split[i].root_invocation==77);}
    assert(split[4].left==original[4].left&&centered(split[4],3840,2160));
    assert(split[5].left==original[5].left&&!centered(split[5],3840,2160));
    // No synthesized missing quadrant or cross-root borrowing. A subset can
    // still have a centered union, but an isolated quadrant stays off-center.
    auto partial=original;partial[3]={};group_extents(partial,4);
    for(unsigned i=0;i<3;++i)assert(centered(partial[i],3840,2160));
    auto foreign=original;foreign[1].root_invocation=80;foreign[3].root_invocation=80;
    group_extents(foreign,4);for(unsigned i=0;i<4;++i)assert(!centered(foreign[i],3840,2160));
    std::array<Extent,gears_ui_indices::segment_capacity> isolated{};isolated[0]=original[0];group_extents(isolated,1);
    assert(!centered(isolated[0],3840,2160));
}
static void finite_tests(){
    auto g=geometry();auto s=state();
    // Parallel eyes separated by .25 world units. Projection scale is 1;
    // x_ndc=(point.x-eye.x)/point.z, giving +/- .0625 at depth 2.
    g.eyes.view[0][12]=.125f;g.eyes.view[1][12]=-.125f;
    gears_viewport::OverlayPlan infinite{},finite{};
    assert(gears_ui_reticle::plan(s,3840,2160,g,infinite));
    near(infinite.viewports[0].x,0);near(infinite.viewports[1].x,1920);
    g.finite_aim={true,{0,0,2},1,2};
    assert(gears_ui_reticle::plan(s,3840,2160,g,finite));
    near(finite.viewports[0].x,60);near(finite.viewports[1].x,1860);
    near(finite.viewports[0].y,0);near(finite.viewports[1].y,0);
    for(unsigned eye=0;eye<2;++eye){near(finite.viewports[eye].width,infinite.viewports[eye].width);
        near(finite.viewports[eye].height,infinite.viewports[eye].height);}
    // A draw's clip cannot choose a different projection for the same aim.
    // These nonempty source clips leave only one eye visible after translation.
    for(unsigned side=0;side<2;++side){auto clipped=s;
        clipped.scissor={side?0.f:3800.f,0,40,2160};
        gears_viewport::OverlayPlan narrow{};
        assert(gears_ui_reticle::plan(clipped,3840,2160,g,narrow));
        assert(narrow.viewports==finite.viewports);
        const unsigned empty_eye=side,visible_eye=1-side;
        assert((narrow.scissors[empty_eye]==gears_viewport::Rect{float(empty_eye*1920),0,0,0}));
        assert((narrow.scissors[visible_eye]==gears_viewport::Rect{side?60.f:3760.f,0,20,2160}));
        // The production replay forwards empty clips, completes both draws,
        // and restores the original rectangles. D3D conversion keeps zero area.
        unsigned viewports=0,scissors=0,draws=0;
        const auto mask=gears_viewport::replay_overlay(clipped,narrow,
            [&](gears_viewport::Rect r,float minimum,float maximum){
                assert(r==(viewports<2?narrow.viewports[viewports]:clipped.viewport));
                assert(minimum==clipped.min_depth&&maximum==clipped.max_depth);++viewports;},
            [&](gears_viewport::Rect r){
                assert(r==(scissors<2?narrow.scissors[scissors]:clipped.scissor));
                if(scissors==empty_eye){const long left=static_cast<long>(r.x),top=static_cast<long>(r.y);
                    assert(left==static_cast<long>(r.x+r.width)&&top==static_cast<long>(r.y+r.height));}
                ++scissors;},
            [&](unsigned eye){assert(eye==draws&&viewports==eye+1&&scissors==eye+1);++draws;});
        assert(mask==3&&viewports==3&&scissors==3&&draws==2);
        // An empty first eye must not bypass invalid geometry in the second.
        if(!side){auto bad=g;bad.eyes.view[1][14]=-3;reject(bad,clipped);}
    }
    // Top/bottom clipping may empty both eyes without invalidating the aim.
    for(unsigned side=0;side<2;++side){auto vertical=g;vertical.finite_aim.point[1]=side?-.125f:.125f;
        auto clipped=s;clipped.scissor={0,side?2120.f:0.f,3840,40};
        gears_viewport::OverlayPlan full{},narrow{};
        assert(gears_ui_reticle::plan(s,3840,2160,vertical,full));
        assert(gears_ui_reticle::plan(clipped,3840,2160,vertical,narrow));
        assert(narrow.viewports==full.viewports);
        for(unsigned eye=0;eye<2;++eye)
            assert((narrow.scissors[eye]==gears_viewport::Rect{float(eye*1920),0,0,0}));
    }
    // A common world-origin change moves the point and both camera positions;
    // it must preserve the finite image, including clipped scissors.
    auto shifted=g;const float translation[3]{16,-8,4};
    for(unsigned axis=0;axis<3;++axis){shifted.finite_aim.point[axis]+=translation[axis];
        shifted.source_view[12+axis]-=translation[axis];
        for(unsigned eye=0;eye<2;++eye)shifted.eyes.view[eye][12+axis]-=translation[axis];}
    gears_viewport::OverlayPlan copy{};
    assert(gears_ui_reticle::plan(s,3840,2160,shifted,copy)&&same(copy,finite));
    // Finite input already lives in world coordinates: source translation
    // cannot silently move that point or reconstruct it from screen center.
    shifted=g;shifted.source_view[12]=100;shifted.source_view[14]=-200;
    assert(gears_ui_reticle::plan(s,3840,2160,shifted,copy)&&same(copy,finite));
    auto far=g;far.finite_aim.point[2]=1000000;
    assert(gears_ui_reticle::plan(s,3840,2160,far,copy));
    for(unsigned eye=0;eye<2;++eye){near(copy.viewports[eye].x,infinite.viewports[eye].x);
        near(copy.viewports[eye].y,infinite.viewports[eye].y);}
    for(unsigned fault=0;fault<10;++fault){auto bad=g;
        if(fault<3)bad.finite_aim.point[fault]=std::numeric_limits<float>::quiet_NaN();
        if(fault==3)bad.finite_aim.point[0]=std::numeric_limits<float>::infinity();
        if(fault==4)bad.finite_aim.world=0;if(fault==5)bad.finite_aim.tick=0;
        if(fault==6)bad.finite_aim.point[2]=-2;
        if(fault==7)bad.finite_aim.point[0]=100000; // Exceeds legal D3D viewport range.
        if(fault==8)bad.eyes.view[1][14]=-3; // Behind only the second eye.
        if(fault==9)bad.finite_aim.point[2]=0;
        reject(bad); // Every failed plan leaves output unchanged.
    }
}
static void seated_projection_tests(){
    // Independent world-point projection oracle: the rendered source center
    // must land on the same point in both eyes, including asymmetric frusta,
    // a shifted seat, head roll, nearby targets and camera distance adjustment.
    unsigned checked=0,outside=0,behind=0;
    auto s=state();
    for(double x:{-.4,0.,.4})for(double y:{-.15,0.,.15})for(double z:{-.25,0.,.25})
    for(double yaw:{-.5,0.,.5})for(double roll:{-.35,0.,.35})for(float depth:{60.f,200.f,800.f}){
        auto g=geometry();StereoSample sample{};
        sample.head={{x,1.7+y,z},multiply(Quaternion{0,std::sin(yaw/2),0,std::cos(yaw/2)},
            Quaternion{0,0,std::sin(roll/2),std::cos(roll/2)})};
        for(unsigned eye=0;eye<2;++eye){
            const auto offset=transform(rotation_matrix(sample.head.orientation),{eye?.032:-.032,0,0});
            sample.eyes[eye].pose={{x+offset.x,1.7+y+offset.y,z+offset.z},sample.head.orientation};
            sample.eyes[eye].fov=eye?StereoFov{-.70,.94,.77,-.95}:StereoFov{-.94,.70,.77,-.95};
        }
        auto closer=g.source_view;closer[14]=-20;
        assert(stereo_matrices(closer,g.source_projection,Pose{{0,1.7,0},{}},sample,100,g.eyes));
        g.finite_aim={true,{0,0,depth},1,2};
        UiStereoProjection expected{};
        const bool in_front=project_ui_anchor(g.eyes,{0,0,depth},1920,2160,expected);
        gears_viewport::OverlayPlan actual{};bool suppressed=true;
        assert(gears_ui_reticle::display_plan(s,3840,2160,g,actual,0,&suppressed));
        ++checked;
        if(!in_front){
            bool behind_eye=false;for(unsigned eye=0;eye<2;++eye)
                behind_eye|=double(depth)*g.eyes.view[eye][10]+g.eyes.view[eye][14]<=0;
            assert(behind_eye&&suppressed);
            for(unsigned eye=0;eye<2;++eye)assert(actual.scissors[eye].width==0&&actual.scissors[eye].height==0);
            ++behind;continue;
        }
        assert(!suppressed);
        for(unsigned eye=0;eye<2;++eye){
            near(actual.viewports[eye].x+960,eye*1920+expected.eyes[eye].x,.002);
            near(actual.viewports[eye].y+1080,expected.eyes[eye].y,.002);
            if(!expected.eyes[eye].in_viewport_xy)++outside;
            auto clip=actual.scissors[eye];assert(clip.x>=eye*1920&&clip.x+clip.width<=(eye+1)*1920);
            assert(clip.y>=0&&clip.y+clip.height<=2160);
        }
    }
    assert(checked==729&&outside>0);
    // The old live caller converted these failed finite plans to infinity.
    // Behind-eye/invalid finite geometry now draws nothing and retains the
    // finite target; it cannot masquerade as the valid infinite direction.
    auto g=geometry();g.finite_aim={true,{0,0,2},1,2};g.eyes.view[1][14]=-3;
    gears_viewport::OverlayPlan hidden{};bool suppressed=false;
    assert(gears_ui_reticle::display_plan(s,3840,2160,g,hidden,0,&suppressed)&&suppressed);
    for(unsigned eye=0;eye<2;++eye)assert((hidden.scissors[eye]==gears_viewport::Rect{float(eye*1920),0,0,0}));
    auto invalid=s;invalid.viewport_count=2;const auto old=hidden;
    assert(!gears_ui_reticle::display_plan(invalid,3840,2160,g,hidden)&&same(hidden,old));
    g.finite_aim={};assert(gears_ui_reticle::display_plan(s,3840,2160,g,hidden,0,&suppressed)&&!suppressed);
    near(hidden.viewports[0].x,0);near(hidden.viewports[1].x,1920);
    g.eyes.view[1][0]=0;const auto unchanged=hidden;
    assert(!gears_ui_reticle::display_plan(s,3840,2160,g,hidden)&&same(hidden,unchanged));
    // Only one eye crosses the edge: preserve both projections, rather than
    // shifting the still-visible eye to the unrelated infinite target.
    g=geometry();g.finite_aim={true,{1.9f,0,2},1,2};g.eyes.view[0][12]=.125f;g.eyes.view[1][12]=-.125f;
    assert(gears_ui_reticle::display_plan(s,3840,2160,g,hidden,0,&suppressed)&&!suppressed);
    near(hidden.viewports[0].x+960,1932,.001);near(hidden.viewports[1].x+960,3732,.001);
    std::printf("seated reticle projection: %u poses, %u outside-eye projections, %u behind-eye cases\n",checked,outside,behind);
}
int main(){auto g=geometry();auto s=state();gears_viewport::OverlayPlan p{};
    finite_tests();
    seated_projection_tests();
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
    bad=g;bad.eyes.projection[1][8]=2;assert(gears_ui_reticle::plan(s,3840,2160,bad,p)); // Outside is clipping.
    bad=g;bad.source_projection[3]=1;reject(bad);bad=g;bad.eyes.view[0][3]=1;reject(bad);
    auto invalid=s;invalid.viewport_count=2;reject(g,invalid);invalid=s;invalid.scissor.width=0;reject(g,invalid);
    for(auto clip:std::array<gears_viewport::Rect,6>{{{0,0,40,0},{0,0,-1,40},{0,0,40,-1},
            {3840,0,40,40},{0,2160,40,40},{.5f,0,40,40}}}){
        invalid=s;invalid.scissor=clip;reject(g,invalid);}
    invalid=s;invalid.scissor={0,0,10,10};
    assert(gears_ui_reticle::plan(invalid,3840,2160,asym,p)); // Shifted clip empty.
    assert(p.viewports==unchanged.viewports);
    for(unsigned eye=0;eye<2;++eye)
        assert((p.scissors[eye]==gears_viewport::Rect{float(eye*1920),0,0,0}));
    auto owned=s;owned.viewport={0,0,4096,2048};owned.scissor=owned.viewport;
    const double source_aspect=3840./2160;
    assert(gears_ui_reticle::plan(owned,4096,2048,g,p,source_aspect));
    for(unsigned eye=0;eye<2;++eye){auto v=p.viewports[eye];auto clip=p.scissors[eye];
        near(v.x,eye*2048.);near(v.y,448);near(v.width,2048);near(v.height,1152);
        near(v.y+v.height/2,1024);near(v.width*20/3840,v.height*20/2160);
        near(clip.y,448);near(clip.height,1152);}
    auto clip_owned=owned;clip_owned.scissor={100,201,1000,601};
    assert(gears_ui_reticle::plan(clip_owned,4096,2048,asym,p,source_aspect));
    for(unsigned eye=0;eye<2;++eye){auto v=p.viewports[eye];auto clip=p.scissors[eye];
        const double top=(std::max)(0.,double(v.y)+double(v.height)*201/2048);
        const double bottom=(std::min)(2048.,double(v.y)+double(v.height)*802/2048);
        near(clip.y,std::floor(top),0);near(clip.y+clip.height,std::ceil(bottom),0);
        assert(clip.x>=eye*2048&&clip.x+clip.width<=(eye+1)*2048&&clip.y>=0&&clip.y+clip.height<=2048);}
    // A tall source panel clips to the target without changing its affine map.
    assert(gears_ui_reticle::plan(owned,4096,2048,g,p,.5));near(p.viewports[0].y,-1024);near(p.viewports[0].height,4096);
    near(p.scissors[0].y,0);near(p.scissors[0].height,2048);
    for(double invalid_aspect:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),1e-300}){
        const auto old=p;assert(!gears_ui_reticle::plan(owned,4096,2048,g,p,invalid_aspect));assert(same(p,old));}
    std::puts("ui_reticle_direction_plan tests passed");
}
