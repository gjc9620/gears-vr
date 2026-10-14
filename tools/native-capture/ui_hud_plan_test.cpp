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
    gears_viewport::OverlayPlan const& p, double depth=2,double hfov=70) {
    const double half=depth*std::tan(hfov*pi/360);
    const double px=(2*x/1920-1)*half,py=(1-2*y/1080)*half*1080/1920;
    const auto inv=transpose(rotation_matrix(s.head.orientation));
    const auto delta=transform(inv,{s.eyes[eye].pose.position.x-s.head.position.x,
        s.eyes[eye].pose.position.y-s.head.position.y,s.eyes[eye].pose.position.z-s.head.position.z});
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
    assert(gears_ui_hud::plan(saved,1920,1080,s,p));
    for(unsigned i=0;i<2;++i)for(double x:{0.,384.,960.,1920.})for(double y:{0.,216.,540.,1080.})oracle(s,i,x,y,p);
    // Center is no longer each-eye pixel center with this asymmetric FOV.
    assert(p.viewports[0].x+p.viewports[0].width/2>480);
    assert(p.viewports[1].x+p.viewports[1].width/2<1440);
    assert(p.viewports[0].y+p.viewports[0].height/2<540);
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
    for(unsigned i=0;i<2;++i)oracle(shifted,i,700,300,shifted_plan);assert(shifted_plan.viewports[0].width>p.viewports[0].width);
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
    bad=s;for(auto&e:bad.eyes)e.pose.position.z=-3;reject(saved,bad);
    bad=s;for(auto&e:bad.eyes)e.pose.position.x+=100;reject(saved,bad); // empty fixed-eye intersection
    reject(saved,s,{0,2});reject(saved,s,{179,2});reject(saved,s,{70,0});reject(saved,s,{70,-1});
    reject(saved,s,{70,std::numeric_limits<double>::infinity()});reject(saved,s,{178.9,2}); // D3D bounds
    auto invalid=saved;invalid.viewport_count=2;reject(invalid,s);
    invalid=saved;invalid.scissor={0,0,0,1080};reject(invalid,s);
    // A nonempty source clip can still be wholly outside the fixed eye region.
    invalid=saved;invalid.scissor={0,0,1920,1};
    bad=s;for(auto&e:bad.eyes)e.pose.position.y=-1.5;reject(invalid,bad);
    unsigned draws=0;gears_viewport::Rect last_view,last_clip;float last_min=0,last_max=0;
    assert(gears_viewport::replay_overlay(saved,p,[&](auto r,float a,float b){last_view=r;last_min=a;last_max=b;},
        [&](auto r){last_clip=r;},[&](unsigned){++draws;})==3);
    assert(draws==2&&last_view==saved.viewport&&last_clip==saved.scissor&&last_min==.2f&&last_max==.8f);
    std::cout<<"UiHudPlanTest passed\n";
}
