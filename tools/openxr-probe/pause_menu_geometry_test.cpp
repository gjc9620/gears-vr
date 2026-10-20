#include "../native-capture/ui_hud_plan.h"
#include "menu_eye_panel.h"
#include <cassert>
#include <cmath>
#include <cstdio>

// Independent inverse-frustum oracle. UI vertices have already been projected
// by native replay; their XR image must keep that projection, without another
// source-aspect correction on a common quad.
static void verify(unsigned width,unsigned height,double source_aspect,bool asymmetric,double ipd){
    gears_vr::StereoSample sample{};
    sample.head.orientation={};
    constexpr double pi=3.14159265358979323846;
    for(unsigned i=0;i<2;++i){auto& e=sample.eyes[i];
        e.pose.orientation={};
        e.pose.position={i?ipd/2:-ipd/2,1.7,0};
        if(asymmetric)e.fov={(i?-40.:-54.)*pi/180,(i?54.:40.)*pi/180,44*pi/180,-54*pi/180};
        else e.fov={-pi/4,pi/4,pi/4,-pi/4};
    }
    gears_viewport::DrawState saved{true,{0,0,float(width),float(height)},
        {0,0,float(width),float(height)},1,1,true,0,1};
    gears_viewport::OverlayPlan plan{};
    assert(gears_ui_hud::plan(saved,width,height,sample,plan,{51,2,source_aspect}));
    double centres[2]{};
    for(unsigned eye=0;eye<2;++eye){auto v=plan.viewports[eye];auto f=sample.eyes[eye].fov;
        const double range_x=std::tan(f.right)-std::tan(f.left);
        const double range_y=std::tan(f.up)-std::tan(f.down);
        // A square occupying 1% of source width has height 1%*aspect.
        const double native_dx=v.width*.01,native_dy=v.height*.01*source_aspect;
        const double physical_dx=2*range_x*native_dx/(width/2);
        const double physical_dy=2*range_y*native_dy/height;
        assert(std::abs(physical_dx/physical_dy-1)<1e-6);
        const double centre_pixel=v.x+v.width/2-eye*(width/2);
        centres[eye]=sample.eyes[eye].pose.position.x+
            2*(std::tan(f.left)+range_x*centre_pixel/(width/2));
        assert(std::abs(centres[eye])<1e-6);
        if(!asymmetric&&width==2*height){
            gears_menu_panel::Plan old{};assert(gears_menu_panel::make_plan(width,height,old));
            const double old_dx=old.metres_width*native_dx/old.width;
            const double old_dy=old.metres_height*native_dy/old.height;
            assert(std::abs(old_dx/old_dy-2)<1e-6); // Old path visibly flattened glyphs.
        }
    }
    assert(std::abs(centres[0]-centres[1])<1e-6); // Same finite-depth UI plane in both eyes.
}
int main(){
    for(auto size:{std::pair<unsigned,unsigned>{1920,1080},{3840,2160},{1792,980},{4096,2048}})
        for(double aspect:{16./9,4./3,21./9})for(bool asym:{false,true})for(double ipd:{.04,.064,.08})
            verify(size.first,size.second,aspect,asym,ipd);
    std::puts("pause menu geometry: 72 configurations preserve square glyphs and finite-depth binocular alignment; legacy quad stretches glyphs 2x");
}
