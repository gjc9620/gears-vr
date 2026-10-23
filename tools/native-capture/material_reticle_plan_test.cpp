#include "material_reticle_plan.h"
#include "material_overlay_plan.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <stdexcept>

using namespace gears_vr;
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static StereoMatrix projection(){return {1,0,0,0,0,1,0,0,0,0,1,1,0,0,-1,0};}
static StereoTicketGeometry geometry(){
    StereoTicketGeometry g{};g.source_view=identity();g.source_projection=projection();g.aiming=true;
    for(unsigned eye=0;eye<2;++eye){g.eyes.view[eye]=identity();g.eyes.view[eye][12]=eye?-.032f:.032f;g.eyes.projection[eye]=projection();}
    g.finite_aim={true,{.4f,.2f,4.f},1,7};return g;
}
struct Fixture {
    gears_ui_material::ViewPlane source{3840,2160,3840.f/2160.f};
    gears_ui_material::Quad quad{};gears_ui_layout::Matrix matrix{};
    unsigned width=5736,height=2302;
    gears_viewport::DrawState saved{true,{0,0,5736,2302},{0,0,5736,2302},1,1,true,.2f,.8f};
    StereoTicketGeometry g=geometry();
    Fixture(float w=3840,float h=2160,float bias=0){
        source={w,h,w/h};matrix=identity();matrix[0]=2/w;matrix[5]=-2/h;matrix[12]=-1+2*bias/w;matrix[13]=1-2*bias/h;
        for(unsigned i=0;i<4;++i){const float xyz[]{i%2?w:0,i/2?h:0,0};std::memcpy(quad.vertices[i].data(),xyz,sizeof(xyz));}
    }
    bool run(gears_material_reticle::Result& out,unsigned kind=1,bool directional=true,gears_material_reticle::Reject* reject=nullptr)const{
        return gears_material_reticle::plan(kind,directional,saved,width,height,source,quad,matrix,&g,out,reject);
    }
};
static bool same(gears_viewport::OverlayPlan const& a,gears_viewport::OverlayPlan const& b){
    return a.valid==b.valid&&a.viewports==b.viewports&&a.scissors==b.scissors;
}
static void near(double a,double b){assert(std::abs(a-b)<.002);}
static void reject(Fixture const& f,gears_material_reticle::Reject reason,unsigned kind=1){
    gears_material_reticle::Result out{};assert(Fixture{}.run(out));const auto old=out;
    gears_material_reticle::Reject actual{};assert(!f.run(out,kind,true,&actual)&&actual==reason);
    assert(same(out.plan,old.plan)&&out.suppressed==old.suppressed);
}
int main(){
    unsigned cases=0;
    for(auto size:{std::array<float,2>{3840,2160},std::array<float,2>{1280,720},std::array<float,2>{1920,1200}})
    for(float bias:{-.5f,0.f,.5f})for(bool finite:{false,true})for(float yaw:{-.2f,0.f,.2f}){
        Fixture f(size[0],size[1],bias);f.g.finite_aim.valid=finite;
        const float c=std::cos(yaw),s=std::sin(yaw);
        for(unsigned eye=0;eye<2;++eye){auto& v=f.g.eyes.view[eye];v[0]=v[10]=c;v[2]=s;v[8]=-s;v[13]=.1f;}
        const auto vertices=f.quad.vertices;const auto point=f.g.finite_aim.point;
        gears_material_reticle::Result out{};assert(f.run(out)&&!out.suppressed);++cases;
        // Independent world-point/ray projection oracle, including head turn
        // and translation. The weapon target stays fixed for both eyes.
        for(unsigned eye=0;eye<2;++eye){
            const double x=finite?point[0]:0,y=finite?point[1]:0,z=finite?point[2]:1;
            const double ex=x*c-z*s+(finite?f.g.eyes.view[eye][12]:0);
            const double ey=y+(finite?.1f:0),ez=x*s+z*c;
            const auto& v=out.plan.viewports[eye];const double ew=f.width/2.;
            near(v.x+v.width/2.,eye*ew+ew*(1+ex/ez)/2);
            near(v.y+v.height/2.,f.height*(1-ey/ez)/2);
            near(v.width/v.height,f.source.aspect);
            const auto& clip=out.plan.scissors[eye];assert(clip.x>=eye*ew&&clip.x+clip.width<=eye*ew+ew);
            assert(clip.y>=0&&clip.y+clip.height<=f.height);
        }
        gears_viewport::OverlayPlan icon{};
        assert(gears_ui_reticle::display_plan(f.saved,f.width,f.height,f.g,icon,f.source.aspect));
        assert(same(out.plan,icon)&&f.quad.vertices==vertices&&f.g.finite_aim.point==point);
        // The old material HUD planner rejected this otherwise certified draw.
        gears_material_overlay::Result old{};assert(!gears_material_overlay::plan(1,true,f.saved,f.width,f.height,{},f.source.aspect,{},{},f.quad,f.matrix,old));
        unsigned draws=0;gears_viewport::Rect viewport{},scissor{};float minimum{},maximum{};
        auto vp=[&](auto r,float lo,float hi){viewport=r;minimum=lo;maximum=hi;};auto sc=[&](auto r){scissor=r;};
        assert(gears_viewport::replay_overlay(f.saved,out.plan,vp,sc,[&](unsigned eye){assert(eye==draws++);})==3&&draws==2);
        assert(viewport==f.saved.viewport&&scissor==f.saved.scissor&&minimum==.2f&&maximum==.8f);
        volatile bool fail_draw=true;
        try{gears_viewport::replay_overlay(f.saved,out.plan,vp,sc,[&](unsigned){if(fail_draw)throw std::runtime_error("draw");});assert(false);}catch(std::runtime_error const&){}
        assert(viewport==f.saved.viewport&&scissor==f.saved.scissor&&minimum==.2f&&maximum==.8f);
    }
    using R=gears_material_reticle::Reject;
    for(unsigned kind:{0u,2u,3u,4u,~0u})reject(Fixture{},R::kind,kind);
    Fixture bad;bad.quad.vertices[3]=bad.quad.vertices[2];reject(bad,R::source_quad);
    bad=Fixture{};float value=2;std::memcpy(bad.quad.vertices[0].data(),&value,4);reject(bad,R::source_quad);
    bad=Fixture{};value=1;std::memcpy(bad.quad.vertices[0].data()+8,&value,4);reject(bad,R::source_quad);
    bad=Fixture{};bad.matrix[0]=std::numeric_limits<float>::quiet_NaN();reject(bad,R::source_quad);
    bad=Fixture{};bad.source.aspect=2;reject(bad,R::source_quad);
    bad=Fixture{};bad.saved.viewport_count=2;reject(bad,R::draw_state);
    bad=Fixture{};bad.g.eyes.view[1][0]=2;reject(bad,R::geometry);
    bad=Fixture{};bad.g.finite_aim.tick=0;reject(bad,R::geometry);
    bad=Fixture{};bad.g.finite_aim.point[0]=std::numeric_limits<float>::infinity();reject(bad,R::geometry);
    Fixture f;gears_material_reticle::Result out{};
    assert(!gears_material_reticle::plan(1,true,f.saved,f.width,f.height,f.source,f.quad,f.matrix,nullptr,out));
    assert(gears_material_reticle::plan(1,false,f.saved,f.width,f.height,f.source,f.quad,f.matrix,nullptr,out));
    assert(same(out.plan,gears_viewport::overlay_plan(f.saved,f.width,f.height)));
    f.height=2303;f.saved.viewport.height=f.saved.scissor.height=2303;assert(f.run(out));
    f.g.finite_aim.point={100,0,4};assert(f.run(out));
    for(auto const& clip:out.plan.scissors)assert(clip.width==0&&clip.height==0);
    f.g.finite_aim.point={0,0,-1};assert(f.run(out)&&out.suppressed);
    for(auto const& clip:out.plan.scissors)assert(clip.width==0&&clip.height==0);
    f.g.finite_aim.valid=false;
    for(auto& v:f.g.eyes.view){v[0]=-1;v[10]=-1;}
    assert(f.run(out)&&out.suppressed);
    // Returning to a visible target after clipping resumes normal scope output.
    f.g=geometry();assert(f.run(out)&&!out.suppressed);
    for(auto const& clip:out.plan.scissors)assert(clip.width>0&&clip.height>0);
    std::printf("MaterialReticlePlanTest passed: %u projection/replay cases, source and ticket rejection, clipping/resume, state restoration\n",cases);
}
