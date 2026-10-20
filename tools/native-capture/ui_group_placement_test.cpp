#include "ui_group_placement.h"
#include <cassert>
#include <iostream>
#include <limits>
using gears_viewport::OverlayPlan;
using gears_ui_group::Recipe;
static void near(double a,double b){assert(std::abs(a-b)<.001);}
static gears_viewport::DrawState source(){return {true,{0,0,1920,1080},{200,100,1200,600},1,1,true,.2f,.8f};}
static OverlayPlan panel(){return {true,{{{40,100,800,800},{1000,80,880,900}}},{{{0,0,960,1080},{960,0,960,1080}}}};}
static bool same(OverlayPlan const& a,OverlayPlan const& b){return a.valid==b.valid&&a.viewports==b.viewports&&a.scissors==b.scissors;}
static void shared_translation(){
    const auto saved=source();const auto base=panel();OverlayPlan moved{};
    assert(gears_ui_group::compose(saved,1920,1080,base,{1,0,.1},moved));
    // Three arbitrary primitives keep their relative layout under one recipe.
    for(unsigned eye=0;eye<2;++eye){auto v=base.viewports[eye],m=moved.viewports[eye];
        near(m.x,v.x);near(m.width,v.width);near(m.height,v.height);
        for(double y:{.1,.4,.9}){
            const double old_y=v.y+v.height*y,new_y=m.y+m.height*y;
            near(new_y-old_y,v.height*.1);
        }
    }
    // Independent pixel oracle, including conservative integer rounding.
    assert((moved.scissors[0]==gears_viewport::Rect{123,254,501,445}));
    assert((moved.scissors[1]==gears_viewport::Rect{1091,253,551,501}));
    // Asymmetric eye maps remain independent, while sharing source recipe.
    near(moved.viewports[0].y,180);near(moved.viewports[1].y,170);
    auto full=saved;full.scissor={0,0,1920,1080};OverlayPlan larger_clip{};
    assert(gears_ui_group::compose(full,1920,1080,base,{1,0,.1},larger_clip));
    assert(moved.scissors[0].height<larger_clip.scissors[0].height);
}
static void scale_and_edges(){
    auto saved=source();auto base=panel();OverlayPlan result{};
    assert(gears_ui_group::compose(saved,1920,1080,base,{1.2,-.1,.2},result));
    near(result.viewports[0].x,-40);near(result.viewports[0].y,260);
    near(result.viewports[0].width,960);near(result.viewports[0].height,960);
    near(result.viewports[1].x,912);near(result.viewports[1].y,260);
    near(result.viewports[1].width,1056);near(result.viewports[1].height,1080);
    saved.scissor={0,0,1920,1080};
    assert(gears_ui_group::compose(saved,1920,1080,base,{1,0,.8},result));
    // Viewports are not clamped: only the transformed clip intersects the eye.
    near(result.viewports[0].y,740);near(result.viewports[0].height,800);
    assert((result.scissors[0]==gears_viewport::Rect{40,740,800,340}));
    assert((result.scissors[1]==gears_viewport::Rect{1000,800,880,280}));
    assert(gears_ui_group::compose(saved,1920,1080,base,{1,-.2,0},result));
    near(result.viewports[0].x,-120);near(result.viewports[1].x,824);
    assert(result.scissors[0].x==0&&result.scissors[1].x==960);
    // A boundary at exactly one pixel stays inside its own eye.
    auto edge=gears_viewport::overlay_plan(saved,1920,1080);
    assert(gears_ui_group::compose(saved,1920,1080,edge,{1,0,1079./1080},result));
    for(auto c:result.scissors)assert(c.y==1079&&c.height==1);
}
static void failures_are_atomic(){
    const auto saved=source();const auto base=panel();OverlayPlan out=base;
    out.scissors[0]={7,8,9,10};const auto sentinel=out;
    const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
    for(Recipe r:{Recipe{0,0,0},{-1,0,0},{nan,0,0},{inf,0,0},{1,nan,0},{1,0,inf},
                  {1e100,0,0},{1,1e100,0},{1,0,2},{1e-300,0,0}}){
        assert(!gears_ui_group::compose(saved,1920,1080,base,r,out));assert(same(out,sentinel));
    }
    auto bad=base;bad.viewports[1].y=2000;
    assert(!gears_ui_group::compose(saved,1920,1080,bad,{1,0,.01},out));assert(same(out,sentinel));
    bad=base;bad.viewports[1].width=std::numeric_limits<float>::quiet_NaN();
    assert(!gears_ui_group::compose(saved,1920,1080,bad,{1,0,.01},out));assert(same(out,sentinel));
    bad=base;bad.valid=false;
    assert(!gears_ui_group::compose(saved,1920,1080,bad,{},out));assert(same(out,sentinel));
    auto invalid=saved;invalid.scissor.width=0;
    assert(!gears_ui_group::compose(invalid,1920,1080,base,{},out));assert(same(out,sentinel));
    assert(!gears_ui_group::compose(saved,1919,1080,base,{},out));assert(same(out,sentinel));
    assert(gears_ui_group::compose(saved,1920,1080,base,{},out));assert(same(out,base));
}
static void whole_group_fit(){
    using gears_ui_group::Bounds;
    // Two unrelated source positions share one target policy; a title, choices
    // and hints in one group retain their relative offsets and original size.
    const Bounds target{.15,.3,.85,.7,true};
    for(Bounds group:{Bounds{.2,.02,.8,.18,true},Bounds{.3,.75,.7,.95,true}}){
        Recipe r{99,98,97};assert(gears_ui_group::fit(group,target,1,r));
        near(r.scale,1);near((group.left+group.right)*r.scale/2+r.translation_x,.5);
        near((group.top+group.bottom)*r.scale/2+r.translation_y,.5);
        const double old_spacing=group.bottom-group.top;
        near((group.bottom*r.scale+r.translation_y)-(group.top*r.scale+r.translation_y),old_spacing);
        auto saved=source();saved.scissor={0,0,1920,1080};auto base=panel();OverlayPlan mapped{};
        assert(gears_ui_group::compose(saved,1920,1080,base,r,mapped));
        for(unsigned eye=0;eye<2;++eye){const auto v=mapped.viewports[eye],b=base.viewports[eye];
            near(v.y+v.height*(group.top+group.bottom)/2,b.y+b.height*.5);
            near(v.x+v.width*(group.left+group.right)/2,b.x+b.width*.5);}
    }
    // Wide/large groups shrink uniformly to fit without independent line moves.
    Recipe wide{};assert(gears_ui_group::fit({-.5,.1,1.5,.9,true},target,1,wide));
    near(wide.scale,.35);near(-.5*wide.scale+wide.translation_x,.15);
    near(1.5*wide.scale+wide.translation_x,.85);
    Recipe tall{};assert(gears_ui_group::fit({.4,-1,.6,2,true},target,2,tall));
    near(tall.scale,.4/3);near(-tall.scale+tall.translation_y,.3);
    near(2*tall.scale+tall.translation_y,.7);
    Recipe enlarged{};assert(gears_ui_group::fit({.4,.4,.6,.6,true},target,1.5,enlarged));near(enlarged.scale,1.5);
    // Empty, unknown, nonfinite or invalid target policies never publish output.
    const Recipe sentinel{3,4,5};Recipe out=sentinel;const double nan=std::numeric_limits<double>::quiet_NaN();
    for(Bounds bad:{Bounds{},Bounds{0,0,0,1,true},Bounds{1,0,0,1,true},Bounds{0,nan,1,1,true}}){
        assert(!gears_ui_group::fit(bad,target,1,out));assert(out.scale==3&&out.translation_x==4&&out.translation_y==5);}
    for(Bounds bad:{Bounds{-.1,0,1,1,true},Bounds{0,0,1,1.1,true},Bounds{0,0,1,1,false}})
        assert(!gears_ui_group::fit(target,bad,1,out));
    for(double bad:{0.,-1.,nan,std::numeric_limits<double>::infinity()})assert(!gears_ui_group::fit(target,target,bad,out));
    assert(out.scale==sentinel.scale&&out.translation_x==sentinel.translation_x&&out.translation_y==sentinel.translation_y);
}
int main(){shared_translation();scale_and_edges();failures_are_atomic();whole_group_fit();
    std::cout<<"PASS: shared group affine placement, saved clipping, asymmetric eyes, limits and atomic rejection\n";
}
