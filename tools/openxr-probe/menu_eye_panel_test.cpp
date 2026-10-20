#include "menu_eye_panel.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <utility>

static bool near(double a,double b) {
    return std::abs(a-b)<=1e-6*(std::max)(std::abs(a),std::abs(b));
}
static bool same(gears_menu_panel::Plan const& a,gears_menu_panel::Plan const& b) {
    return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height&&
        a.metres_width==b.metres_width&&a.metres_height==b.metres_height;
}
static void valid(unsigned width,unsigned height) {
    gears_menu_panel::Plan plan{};
    assert(gears_menu_panel::make_plan(width,height,plan));
    assert(plan.x==0&&plan.y==0&&plan.width==std::int32_t(width/2)&&plan.height==std::int32_t(height));
    assert(plan.metres_width==1.6f&&std::isfinite(plan.metres_height)&&plan.metres_height>0);
    assert(near(double(plan.metres_width)/plan.metres_height,double(width)/height));
    // A Canvas square becomes half as wide in eye pixels. Physical panel
    // scaling must restore equal X/Y lengths, including non-16:9 sources.
    const double delta=double((std::min)(width,height))/4;
    const double eye_x=delta/2,eye_y=delta;
    const double physical_x=eye_x/plan.width*plan.metres_width;
    const double physical_y=eye_y/plan.height*plan.metres_height;
    assert(near(physical_x,physical_y));
    assert(near(physical_x/plan.metres_width*plan.width*2,delta));
    assert(near(physical_y/plan.metres_height*plan.height,delta));
    // Eye aspect is a concrete factor-two error, not an allowed alternative.
    const double wrong_height=double(plan.metres_width)*plan.height/plan.width;
    assert(near(eye_y/plan.height*wrong_height,physical_x*2));
    assert(!near(eye_y/plan.height*wrong_height,physical_x));
}
int main() {
    for(auto size:{std::pair<unsigned,unsigned>{1920,1080},{3840,2160},{1920,764},{640,480},
                   {1080,1920},{2,1},{8192,8192},{2,8192},{8192,1}})valid(size.first,size.second);
    gears_menu_panel::Plan full_hd{},four_k{};
    assert(gears_menu_panel::make_plan(1920,1080,full_hd));
    assert(gears_menu_panel::make_plan(3840,2160,four_k));
    assert(near(full_hd.metres_height,.9)&&full_hd.metres_height==four_k.metres_height);
    const gears_menu_panel::Plan sentinel{-3,7,11,13,2.5f,3.5f};
    const auto maximum=(std::numeric_limits<unsigned>::max)();
    for(auto size:{std::pair<unsigned,unsigned>{0,1080},{1920,0},{0,0},{1,1},{3,1080},
                   {8193,1080},{8194,1080},{1920,8193},{maximum,1080},{1920,maximum}}) {
        auto output=sentinel;
        assert(!gears_menu_panel::make_plan(size.first,size.second,output));
        assert(same(output,sentinel));
    }
    std::puts("menu eye panel: source aspect, left-eye bounds, forward/inverse UI scale and failure preservation passed");
}
