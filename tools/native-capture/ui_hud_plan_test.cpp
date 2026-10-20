#include "ui_hud_plan.h"
#include <cassert>
#include <iostream>
#include <limits>
using namespace gears_vr;
static constexpr double pi=3.14159265358979323846;
static gears_viewport::DrawState state(){return {true,{0,0,1920,1080},{0,0,1920,1080},1,1,true,.2f,.8f};}
static StereoSample sample(){StereoSample s{};s.head.orientation={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position={i?.032:-.032,0,0};
        s.eyes[i].fov={ (i?-40.:-54.)*pi/180,(i?54.:40.)*pi/180,43.98*pi/180,-54.27*pi/180};}return s;}
static void near(double a,double b,double tolerance=.0002){assert(std::abs(a-b)<=tolerance);}
// Independent frustum interpolation oracle: panel point in eye coordinates,
// then directly interpolate between frustum tangent boundaries.
static void oracle(StereoSample const& s,unsigned eye,double x,double y,
    gears_viewport::OverlayPlan const& p, double depth=2,double hfov=65) {
    const double half=depth*std::tan(hfov*pi/360);
    const double px=(2*x/1920-1)*half,py=(1-2*y/1080)*half*1080/1920;
    const Vector origin{(s.eyes[0].pose.position.x+s.eyes[1].pose.position.x)/2,
        (s.eyes[0].pose.position.y+s.eyes[1].pose.position.y)/2,
        (s.eyes[0].pose.position.z+s.eyes[1].pose.position.z)/2};
    const auto inv=transpose(rotation_matrix(s.eyes[0].pose.orientation));
    const auto delta=transform(inv,{s.eyes[eye].pose.position.x-origin.x,
        s.eyes[eye].pose.position.y-origin.y,s.eyes[eye].pose.position.z-origin.z});
    const auto f=s.eyes[eye].fov;
    const double tangent_x=(px-delta.x)/(depth+delta.z),tangent_y=(py-delta.y)/(depth+delta.z);
    const double expected_x=eye*960+960*(tangent_x-std::tan(f.left))/(std::tan(f.right)-std::tan(f.left));
    const double expected_y=1080*(std::tan(f.up)-tangent_y)/(std::tan(f.up)-std::tan(f.down));
    auto v=p.viewports[eye];near(v.x+v.width*x/1920,expected_x);near(v.y+v.height*y/1080,expected_y);
}
static bool same(gears_viewport::OverlayPlan const&a,gears_viewport::OverlayPlan const&b){return a.valid==b.valid&&a.viewports==b.viewports&&a.scissors==b.scissors;}
static void reject(gears_viewport::DrawState const& d,StereoSample const&s,gears_ui_hud::Parameters args={}) {
    gears_viewport::OverlayPlan out{true,{{{1,2,3,4},{5,6,7,8}}},{{{9,10,11,12},{13,14,15,16}}}};
    const auto before=out;assert(!gears_ui_hud::plan(d,1920,1080,s,out,args));assert(same(out,before));
}
int main(){auto s=sample();auto saved=state();gears_viewport::OverlayPlan p{};
    assert(gears_ui_hud::Parameters{}.horizontal_fov_degrees==65);
    double parsed=65;
    const gears_ui_hud::Parameters configured{58.5,2,0};
    const auto batch=gears_ui_hud::for_source(configured,16./9);
    const auto material=gears_ui_hud::for_source(configured,16./9);
    assert(batch.horizontal_fov_degrees==58.5&&material.horizontal_fov_degrees==batch.horizontal_fov_degrees);
    assert(batch.depth_metres==2&&material.depth_metres==2&&configured.source_aspect==0);
    gears_viewport::OverlayPlan batch_plan{},material_plan{};
    assert(gears_ui_hud::plan(saved,1920,1080,s,batch_plan,batch));
    assert(gears_ui_hud::plan(saved,1920,1080,s,material_plan,material));assert(same(batch_plan,material_plan));
    for(unsigned eye=0;eye<2;++eye)for(double x:{0.,960.,1920.})for(double y:{0.,540.,1080.}){
        oracle(s,eye,x,y,batch_plan,2,58.5);oracle(s,eye,x,y,material_plan,2,58.5);
    }
    for(auto text:{L"45",L"100",L"65.25",L"6.5e1"})assert(gears_ui_hud::parse_horizontal_fov(text,parsed));
    for(auto text:{L"",L"true",L"nan",L"inf",L"44.99",L"100.01",L"65junk",L" 65",L"65 ",L"0x41",L"6e",L"1e9999"}){
        parsed=65;assert(!gears_ui_hud::parse_horizontal_fov(text,parsed)&&parsed==65);
    }
    wchar_t too_long[64];for(auto& ch:too_long)ch=L'0';too_long[63]=0;
    assert(!gears_ui_hud::parse_horizontal_fov(nullptr,parsed));assert(!gears_ui_hud::parse_horizontal_fov(too_long,parsed));
    assert(gears_ui_hud::plan(saved,1920,1080,s,p));
    for(unsigned i=0;i<2;++i)for(double x:{0.,384.,960.,1920.})for(double y:{0.,216.,540.,1080.})oracle(s,i,x,y,p);
    // Center is no longer each-eye pixel center with this asymmetric FOV.
    assert(p.viewports[0].x+p.viewports[0].width/2>480);
    assert(p.viewports[1].x+p.viewports[1].width/2<1440);
    assert(p.viewports[0].y+p.viewports[0].height/2<540);
    // The smaller panel keeps its spatial center/depth while pulling every
    // edge inward. Check the asymmetric eye frusta, not pixel-center symmetry.
    gears_viewport::OverlayPlan previous{};
    assert(gears_ui_hud::plan(saved,1920,1080,s,previous,{70,2}));
    const double shrink=std::tan(65*pi/360)/std::tan(70*pi/360);
    for(unsigned eye=0;eye<2;++eye){auto v=p.viewports[eye];auto old=previous.viewports[eye];
        near(v.x+v.width/2,old.x+old.width/2);near(v.y+v.height/2,old.y+old.height/2);
        near(v.width,old.width*shrink);near(v.height,old.height*shrink);
        assert(v.x>old.x&&v.y>old.y&&v.x+v.width<old.x+old.width&&v.y+v.height<old.y+old.height);
    }
    auto symmetric=s;for(auto&e:symmetric.eyes)e.fov={-pi/4,pi/4,pi/4,-pi/4};
    gears_viewport::OverlayPlan two{},one{};assert(gears_ui_hud::plan(saved,1920,1080,symmetric,two));
    assert(gears_ui_hud::plan(saved,1920,1080,symmetric,one,{70,1}));
    const auto center=[](auto const&a,unsigned i){return a.viewports[i].x+a.viewports[i].width/2-i*960;};
    near(center(two,0)-center(two,1),15.36);near(center(one,0)-center(one,1),30.72);
    // Global head yaw/pitch/roll and translation cancel when offsets rotate with it.
    auto moved=s;auto yaw=Quaternion{0,std::sin(.4),0,std::cos(.4)};
    auto pitch=Quaternion{std::sin(-.15),0,0,std::cos(-.15)};
    auto roll=Quaternion{0,0,std::sin(.1),std::cos(.1)};
    moved.head={{12,-7,4},multiply(multiply(yaw,pitch),roll)};
    for(unsigned i=0;i<2;++i){auto v=transform(rotation_matrix(moved.head.orientation),s.eyes[i].pose.position);
        moved.eyes[i].pose={{12+v.x,-7+v.y,4+v.z},moved.head.orientation};}
    gears_viewport::OverlayPlan rotated{};assert(gears_ui_hud::plan(saved,1920,1080,moved,rotated));
    for(unsigned i=0;i<2;++i){near(rotated.viewports[i].x,p.viewports[i].x);near(rotated.viewports[i].y,p.viewports[i].y);near(rotated.viewports[i].width,p.viewports[i].width);near(rotated.viewports[i].height,p.viewports[i].height);}
    auto negative=moved;for(auto&e:negative.eyes){auto&q=e.pose.orientation;q={-q.x,-q.y,-q.z,-q.w};}
    gears_viewport::OverlayPlan signed_plan{};assert(gears_ui_hud::plan(saved,1920,1080,negative,signed_plan));assert(same(rotated,signed_plan));
    auto scaled=moved;for(auto&e:scaled.eyes){auto&q=e.pose.orientation;q={q.x*1.0001,q.y*1.0001,q.z*1.0001,q.w*1.0001};}
    assert(gears_ui_hud::plan(saved,1920,1080,scaled,signed_plan));assert(same(rotated,signed_plan));
    auto shifted=s;for(auto&e:shifted.eyes)e.pose.position.z=-.1;
    gears_viewport::OverlayPlan shifted_plan{};assert(gears_ui_hud::plan(saved,1920,1080,shifted,shifted_plan));
    for(unsigned i=0;i<2;++i)oracle(shifted,i,700,300,shifted_plan);assert(same(shifted_plan,p));
    auto staggered=s;staggered.eyes[0].pose.position.z=-.01;staggered.eyes[1].pose.position.z=.01;
    gears_ui_hud::Diagnostic diagnostic{};
    assert(gears_ui_hud::plan(saved,1920,1080,staggered,shifted_plan,{},&diagnostic));
    assert(diagnostic.reject==gears_ui_hud::Reject::none);
    for(unsigned i=0;i<2;++i)oracle(staggered,i,700,300,shifted_plan);
    auto one_sign=s;one_sign.eyes[1].pose.orientation={0,0,0,-1};
    gears_viewport::OverlayPlan one_sign_plan{};
    assert(gears_ui_hud::plan(saved,1920,1080,one_sign,one_sign_plan));assert(same(one_sign_plan,p));
    // Independent head/VIEW-space queries can disagree with the exact stereo
    // view. The HUD remains tied to the frozen binocular frame. Preserve the
    // sample and all world camera data; only this local panel frame changes.
    auto independent=moved;independent.head={{0,1.7,0},{0,0,0,1}};
    const auto unchanged=independent;
    gears_viewport::OverlayPlan coherent{};
    assert(gears_ui_hud::plan(saved,1920,1080,independent,coherent));
    for(unsigned i=0;i<2;++i){near(coherent.viewports[i].x,rotated.viewports[i].x);
        near(coherent.viewports[i].y,rotated.viewports[i].y);
        near(coherent.viewports[i].width,rotated.viewports[i].width);
        near(coherent.viewports[i].height,rotated.viewports[i].height);}
    assert(stereo_same(independent,unchanged));
    auto partial=saved;partial.scissor={101,203,701,401};gears_viewport::OverlayPlan clipped{};
    assert(gears_ui_hud::plan(partial,1920,1080,s,clipped));
    for(unsigned i=0;i<2;++i){auto v=clipped.viewports[i];auto c=clipped.scissors[i];
        const double left=(std::max)(i*960.,double(v.x)+double(v.width)*101/1920);
        const double top=(std::max)(0.,double(v.y)+double(v.height)*203/1080);
        const double right=(std::min)((i+1)*960.,double(v.x)+double(v.width)*802/1920);
        const double bottom=(std::min)(1080.,double(v.y)+double(v.height)*604/1080);
        near(c.x,std::floor(left),0);near(c.y,std::floor(top),0);near(c.x+c.width,std::ceil(right),0);near(c.y+c.height,std::ceil(bottom),0);}
    gears_viewport::OverlayPlan wide{};assert(gears_ui_hud::plan(saved,1920,1080,s,wide,{120,2}));
    for(unsigned i=0;i<2;++i){assert(wide.scissors[i].x==float(i*960)&&wide.scissors[i].width==960);
        assert(wide.scissors[i].y>=0&&wide.scissors[i].y+wide.scissors[i].height<=1080);oracle(s,i,960,540,wide,2,120);}
    auto bad=s;bad.eyes[0].pose.orientation={0,std::sin(.00001),0,std::cos(.00001)};reject(saved,bad);
    bad=s;bad.head.orientation={0,0,0,0};reject(saved,bad);
    bad=s;bad.eyes[1].pose.position=bad.eyes[0].pose.position;reject(saved,bad);
    bad=s;bad.eyes[0].fov.up=std::numeric_limits<double>::quiet_NaN();reject(saved,bad);
    bad=s;for(auto&e:bad.eyes)e.pose.position.z=-3;assert(gears_ui_hud::plan(saved,1920,1080,bad,coherent));assert(same(coherent,p));
    bad=s;for(auto&e:bad.eyes)e.pose.position.x+=100;assert(gears_ui_hud::plan(saved,1920,1080,bad,coherent));
    for(unsigned i=0;i<2;++i){near(coherent.viewports[i].x,p.viewports[i].x);near(coherent.viewports[i].y,p.viewports[i].y);}
    reject(saved,s,{0,2});reject(saved,s,{179,2});reject(saved,s,{70,0});reject(saved,s,{70,-1});
    reject(saved,s,{70,std::numeric_limits<double>::infinity()});reject(saved,s,{178.9,2}); // D3D bounds
    auto invalid=saved;invalid.viewport_count=2;reject(invalid,s);
    invalid=saved;invalid.scissor={0,0,0,1080};reject(invalid,s);
    // With a large panel a nonempty edge clip can still miss an eye region.
    invalid=saved;invalid.scissor={0,0,1920,1};
    reject(invalid,s,{120,2});
    unsigned draws=0;gears_viewport::Rect last_view,last_clip;float last_min=0,last_max=0;
    assert(gears_viewport::replay_overlay(saved,p,[&](auto r,float a,float b){last_view=r;last_min=a;last_max=b;},
        [&](auto r){last_clip=r;},[&](unsigned){++draws;})==3);
    assert(draws==2&&last_view==saved.viewport&&last_clip==saved.scissor&&last_min==.2f&&last_max==.8f);
    // 3840x2160 Canvas normalized into a 4096x2048 owned output.
    auto owned=saved;owned.viewport={0,0,4096,2048};owned.scissor=owned.viewport;
    const double source_aspect=3840./2160;gears_viewport::OverlayPlan aspect{};
    assert(gears_ui_hud::plan(owned,4096,2048,symmetric,aspect,{70,2,source_aspect}));
    for(unsigned eye=0;eye<2;++eye){auto v=aspect.viewports[eye];
        near(v.width/v.height,source_aspect);near(v.y+v.height/2,1024);
        // Equal 20-pixel Canvas extents remain square after normalization.
        near(v.width*20/3840,v.height*20/2160);
        auto c=aspect.scissors[eye];assert(c.x>=eye*2048&&c.x+c.width<=(eye+1)*2048&&c.y>=0&&c.y+c.height<=2048);}
    owned.scissor={101,203,701,401};assert(gears_ui_hud::plan(owned,4096,2048,symmetric,aspect,{70,2,source_aspect}));
    for(unsigned eye=0;eye<2;++eye){auto v=aspect.viewports[eye];auto c=aspect.scissors[eye];
        near(c.y,std::floor(double(v.y)+double(v.height)*203/2048),0);
        near(c.y+c.height,std::ceil(double(v.y)+double(v.height)*604/2048),0);}
    for(double invalid_aspect:{-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        const auto old=aspect;assert(!gears_ui_hud::plan(owned,4096,2048,symmetric,aspect,{70,2,invalid_aspect}));assert(same(aspect,old));}
    std::cout<<"UiHudPlanTest passed\n";
}
