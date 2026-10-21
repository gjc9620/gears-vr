#include "ui_aim_locked_plan.h"
#include "../vr-common/ui_projection.h"
#include <cassert>
#include <cstdio>
using namespace gears_vr;
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static StereoMatrix projection(){return {1,0,0,0,0,1,0,0,0,0,1,1,0,0,-1,0};}
static gears_viewport::DrawState state(){return {true,{0,0,3840,2160},{0,0,3840,2160},1,1,true,0,1};}
static void near(double a,double b){assert(std::abs(a-b)<.003);}
static bool same(gears_viewport::OverlayPlan const& a,gears_viewport::OverlayPlan const& b){
    return a.valid==b.valid&&a.viewports==b.viewports&&a.scissors==b.scissors;}
int main(){unsigned checked=0;
    for(double x:{-.3,0.,.3})for(double y:{-.15,0.,.15})for(double pitch:{-.6,-.3,0.,.3,.6})
    for(double yaw:{-.4,0.,.4})for(double roll:{-.3,0.,.3})for(float depth:{60.f,200.f,800.f}){
        StereoSample sample{};sample.head={{x,1.7+y,0},multiply(
            multiply(Quaternion{0,std::sin(yaw/2),0,std::cos(yaw/2)},
                     Quaternion{std::sin(pitch/2),0,0,std::cos(pitch/2)}),
                     Quaternion{0,0,std::sin(roll/2),std::cos(roll/2)})};
        for(unsigned eye=0;eye<2;++eye){
            auto offset=transform(rotation_matrix(sample.head.orientation),{eye?.032:-.032,0,0});
            sample.eyes[eye].pose={{x+offset.x,1.7+y+offset.y,offset.z},sample.head.orientation};
            sample.eyes[eye].fov=eye?StereoFov{-.70,.94,.77,-.95}:StereoFov{-.94,.70,.77,-.95};
        }
        StereoTicketGeometry g{};g.source_view=identity();g.source_projection=projection();g.aiming=true;
        assert(stereo_matrices(g.source_view,g.source_projection,Pose{{0,1.7,0},{}},sample,100,g.eyes));
        g.finite_aim={true,{0,0,depth},1,2};
        gears_viewport::OverlayPlan actual{},reticle{};
        assert(gears_ui_aim::plan(state(),3840,2160,sample,g,actual,{51,2,16./9}));
        assert(gears_ui_reticle::display_plan(state(),3840,2160,g,reticle,16./9));
        auto narrow=state();narrow.scissor={0,0,100,100};gears_viewport::OverlayPlan clipped{};
        assert(gears_ui_aim::plan(narrow,3840,2160,sample,g,clipped,{51,2,16./9}));
        assert(clipped.viewports==actual.viewports); // Source clipping cannot choose another anchor.
        UiStereoProjection expected{};assert(project_ui_anchor(g.eyes,{0,0,depth},1920,2160,expected));
        for(unsigned eye=0;eye<2;++eye){auto v=actual.viewports[eye];
            near(v.x+v.width/2,eye*1920+expected.eyes[eye].x);
            near(v.y+v.height/2,expected.eyes[eye].y);
            near(v.x+v.width/2,reticle.viewports[eye].x+reticle.viewports[eye].width/2);
            near(v.y+v.height/2,reticle.viewports[eye].y+reticle.viewports[eye].height/2);
            auto s=actual.scissors[eye];assert(s.x>=eye*1920&&s.x+s.width<=(eye+1)*1920);
            assert(s.y>=0&&s.y+s.height<=2160);
        }
        // Hip-fire/menu behavior retains the exact existing head-panel map.
        g.aiming=false;gears_viewport::OverlayPlan before{},after{};
        assert(gears_ui_hud::plan(state(),3840,2160,sample,before,{51,2,16./9}));
        assert(gears_ui_aim::plan(state(),3840,2160,sample,g,after,{51,2,16./9})&&same(before,after));
        // With no finite collision point, both UI and reticle still use the
        // original game's world direction, never the current head forward.
        g.aiming=true;g.finite_aim={};
        assert(gears_ui_aim::plan(state(),3840,2160,sample,g,after,{51,2,16./9}));
        assert(gears_ui_reticle::plan(state(),3840,2160,g,reticle,16./9));
        for(unsigned eye=0;eye<2;++eye){near(after.viewports[eye].x+after.viewports[eye].width/2,
            reticle.viewports[eye].x+reticle.viewports[eye].width/2);
            near(after.viewports[eye].y+after.viewports[eye].height/2,
            reticle.viewports[eye].y+reticle.viewports[eye].height/2);}
        ++checked;
    }
    // An eye looking away from a finite target cannot create a head-locked UI.
    StereoSample sample{};sample.head.orientation={};
    for(unsigned eye=0;eye<2;++eye){sample.eyes[eye].pose={{eye?.032:-.032,0,0},{}};
        sample.eyes[eye].fov={-.8,.8,.8,-.8};}
    StereoTicketGeometry g{};g.source_view=identity();g.source_projection=projection();g.aiming=true;
    for(unsigned eye=0;eye<2;++eye){g.eyes.view[eye]=identity();g.eyes.view[eye][14]=-100;
        g.eyes.projection[eye]=projection();}
    g.finite_aim={true,{0,0,60},1,2};gears_viewport::OverlayPlan hidden{};
    assert(gears_ui_aim::plan(state(),3840,2160,sample,g,hidden));
    for(auto clip:hidden.scissors)assert(clip.width==0&&clip.height==0);
    g.finite_aim={};for(auto& view:g.eyes.view){view=identity();view[0]=-1;view[10]=-1;}
    assert(gears_ui_aim::plan(state(),3840,2160,sample,g,hidden));
    for(auto clip:hidden.scissors)assert(clip.width==0&&clip.height==0);
    std::printf("ADS HUD/reticle anchor: %u head poses/depths passed\n",checked);
}
