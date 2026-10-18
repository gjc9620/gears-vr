#include "material_overlay_plan.h"
#include <cassert>
#include <iostream>
#include <limits>

static constexpr double pi=3.14159265358979323846;
static gears_vr::StereoSample sample(){
    gears_vr::StereoSample s{};s.head.orientation={};
    for(unsigned eye=0;eye<2;++eye){s.eyes[eye].pose.orientation={};
        s.eyes[eye].pose.position={eye?.032:-.032,0,0};
        s.eyes[eye].fov={(eye?-40.:-54.)*pi/180,(eye?54.:40.)*pi/180,44*pi/180,-54*pi/180};}
    return s;
}
static gears_viewport::DrawState state(unsigned height=1400){return {true,{0,0,2560,float(height)},{120,60,2200,1200},1,1,true,.2f,.8f};}
static bool same(gears_viewport::OverlayPlan const& a,gears_viewport::OverlayPlan const& b){
    return a.valid==b.valid&&a.viewports==b.viewports&&a.scissors==b.scissors;
}
static void near(double a,double b){assert(std::abs(a-b)<.001);}
static gears_ui_material::Quad quad(){
    gears_ui_material::Quad q{};const float xy[4][3]{{.4f,.4f,0},{.6f,.4f,0},{.4f,.6f,0},{.6f,.6f,0}};
    for(unsigned i=0;i<4;++i)std::memcpy(q.vertices[i].data(),xy[i],sizeof(xy[i]));return q;
}
int main(){
    const auto saved=state();const auto s=sample();const auto q=quad();
    gears_ui_layout::Matrix matrix{};matrix[0]=matrix[5]=matrix[10]=matrix[15]=1;
    const auto before=q.vertices;unsigned cases=0;
    // Destination dimensions are independent of the original 16:9 source.
    for(unsigned height:{1400u,1401u})for(unsigned kind:{0u,1u,2u,3u,4u})for(bool enabled:{false,true})
    for(double fov:{45.,65.,100.})for(auto layout:{gears_ui_layout::Parameters{},gears_ui_layout::Parameters{1.2,.8}}){
        const auto current=state(height);
        const gears_ui_hud::Parameters hud{fov,2.5,0};gears_material_overlay::Result actual{};
        const bool accepted=gears_material_overlay::plan(kind,enabled,current,2560,height,s,16./9,hud,layout,q,matrix,actual);
        ++cases;
        // Mounted104 policy from game_ui_material_draw.inl: kind 0 is ordinary;
        // 2/3 use the user panel/layout only while the HUD plane is enabled.
        if(kind==1||kind==4){assert(!accepted);continue;}
        assert(accepted);const bool hud_kind=kind!=0&&enabled;
        assert(actual.kind==(hud_kind?gears_game_material::Plan::hud:gears_game_material::Plan::ordinary));
        gears_viewport::OverlayPlan baseline{};
        if(!hud_kind)baseline=gears_viewport::overlay_plan(current,2560,height);
        else {
            assert(gears_ui_hud::plan(current,2560,height,s,baseline,{fov,2.5,16./9}));
            if(!gears_ui_layout::identity(layout)){
                // Identity matrix maps this source quad to normalized bounds
                // [.7,.8] x [.2,.3]. An independent geometry oracle follows.
                gears_ui_layout::Bounds bounds{};assert(gears_ui_layout::quad_bounds(q.vertices,matrix,bounds));
                gears_viewport::OverlayPlan adjusted{};
                assert(gears_ui_layout::compose(current,2560,height,baseline,bounds,layout,adjusted));baseline=adjusted;
            }
        }
        assert(same(actual.plan,baseline));
        assert(actual.layout_considered==(hud_kind&&!gears_ui_layout::identity(layout)));
        assert(actual.layout_applied==actual.layout_considered);
        if(hud_kind){
            // Panel half width uses user FOV and depth; source aspect stays
            // 16:9 even though each destination eye is a tall rectangle.
            const double half=2.5*std::tan(fov*pi/360);
            const double scale=layout.element_scale,cx=.75,cy=.25;
            const double tx=.5+layout.center_scale*(cx-.5)-scale*cx;
            const double ty=.5+layout.center_scale*(cy-.5)-scale*cy;
            for(unsigned eye=0;eye<2;++eye){const auto f=s.eyes[eye].fov;const auto v=actual.plan.viewports[eye];
                for(double x:{.7,.75,.8})for(double y:{.2,.25,.3}){
                    const double px=(2*(tx+scale*x)-1)*half;
                    const double py=(1-2*(ty+scale*y))*half/(16./9);
                    const double expected_x=eye*1280+1280*((px-s.eyes[eye].pose.position.x)/2.5-std::tan(f.left))/(std::tan(f.right)-std::tan(f.left));
                    const double expected_y=height*(std::tan(f.up)-py/2.5)/(std::tan(f.up)-std::tan(f.down));
                    near(v.x+v.width*x,expected_x);near(v.y+v.height*y,expected_y);
                }
            }
        }
        unsigned draws=0;gears_viewport::Rect last_view{},last_clip{};float last_min{},last_max{};
        const auto mask=gears_viewport::replay_overlay(current,actual.plan,
            [&](auto r,float a,float b){last_view=r;last_min=a;last_max=b;},[&](auto r){last_clip=r;},[&](unsigned){++draws;});
        assert(mask==3&&draws==2&&last_view==current.viewport&&last_clip==current.scissor);
        assert(last_min==current.min_depth&&last_max==current.max_depth);
    }
    // Invalid HUD geometry retains Mounted104's panel fallback. It does not
    // affect ordinary kinds and never changes the native upload bytes.
    auto bad=matrix;bad[15]=0;gears_material_overlay::Result fallback{},panel{};
    assert(gears_material_overlay::plan(2,true,saved,2560,1400,s,16./9,{45,2,0},{1.2,.8},q,bad,fallback));
    assert(gears_material_overlay::plan(2,true,saved,2560,1400,s,16./9,{45,2,0},{},q,matrix,panel));
    assert(fallback.layout_considered&&!fallback.layout_applied&&same(fallback.plan,panel.plan));
    assert(q.vertices==before);
    const auto sentinel=panel;
    assert(!gears_material_overlay::plan(1,true,saved,2560,1400,s,16./9,{},{},q,matrix,panel));
    assert(same(panel.plan,sentinel.plan)&&panel.kind==sentinel.kind);
    auto invalid=state();invalid.viewport_count=2;
    assert(!gears_material_overlay::plan(0,false,invalid,2560,1400,s,16./9,{},{},q,matrix,panel));
    assert(same(panel.plan,sentinel.plan));
    // The replaced fixed-65-degree implementation differs for both ordinary
    // Material and an explicit 45-degree HUD preference.
    gears_viewport::OverlayPlan previous{};
    assert(gears_ui_hud::plan(saved,2560,1400,s,previous,{65,2,16./9}));
    assert(!same(previous,gears_viewport::overlay_plan(saved,2560,1400))&&!same(previous,sentinel.plan));
    std::cout<<"MaterialOverlayPlanTest passed: "<<cases<<" policy comparisons; rectangular/odd-height geometry, two-eye replay, layout fallback\n";
}
