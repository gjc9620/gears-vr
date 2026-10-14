#include "ui_projection.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace gears_vr;
static bool near(double actual,double expected,double tolerance=.0003){return std::abs(actual-expected)<=tolerance;}
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static StereoMatrix frustum(double left,double right,double up,double down){
    // Independent canonical perspective fixture: near distance one, reverse Z.
    return {float(2/(right-left)),0,0,0,0,float(2/(up-down)),0,0,
            float(-(right+left)/(right-left)),float(-(up+down)/(up-down)),0,1,0,0,1,0};
}
static StereoMatrices parallel(StereoMatrix left,StereoMatrix right){
    StereoMatrices result{};result.view[0]=result.view[1]=identity();
    // Synthetic metre coordinates for this fixture only; the helper has no scale.
    result.view[0][12]=.032f;result.view[1][12]=-.032f;
    result.projection[0]=left;result.projection[1]=right;return result;
}
static bool same(UiStereoProjection const& a,UiStereoProjection const& b){
    for(unsigned i=0;i<2;++i)if(a.eyes[i].x!=b.eyes[i].x||a.eyes[i].y!=b.eyes[i].y||
        a.eyes[i].w!=b.eyes[i].w||a.eyes[i].in_viewport_xy!=b.eyes[i].in_viewport_xy)return false;
    return true;
}
int main(){
    const double l=std::tan(-.942477763),r=std::tan(.69813168),u=std::tan(.767595768),d=std::tan(-.947190166);
    auto asymmetric=parallel(frustum(l,r,u,d),frustum(-r,-l,u,d));UiStereoProjection result{};
    assert(project_ui_anchor(asymmetric,{0,0,2},1920,1080,result));
    // Analytic ray/frustum interpolation; no matrix multiplication in oracle.
    assert(near(result.eyes[0].x,1920*((.032/2-l)/(r-l))));
    assert(near(result.eyes[1].x,1920*((-.032/2+r)/(r-l))));
    assert(near(result.eyes[0].y,1080*u/(u-d))&&near(result.eyes[1].y,1080*u/(u-d)));
    assert(result.eyes[0].w==2&&result.eyes[1].w==2);
    assert(result.eyes[0].in_viewport_xy&&result.eyes[1].in_viewport_xy);
    assert(std::abs(result.eyes[0].x-960)>200&&std::abs(result.eyes[1].x-960)>200);
    // The audited image-center rays differ by 29.44567 degrees. A single point
    // chosen on the LEFT center ray is not also at the right image center.
    const double center_x=(l+r)/2,center_y=(u+d)/2;
    assert(project_ui_anchor(asymmetric,{-.032+2*center_x,2*center_y,2},1920,1080,result));
    assert(near(result.eyes[0].x,960)&&near(result.eyes[0].y,540));
    assert(near(result.eyes[1].x,1920*((center_x-.064/2+r)/(r-l))));
    assert(std::abs(result.eyes[1].x-960)>400);
    const double forward_depth=-.064/(-2*center_x);
    assert(near(forward_depth,-.1191180287,1e-8));
    auto saved=result;assert(!project_ui_anchor(asymmetric,{0,0,forward_depth},1920,1080,result)&&same(result,saved));

    auto symmetric=parallel(frustum(-1,1,1,-1),frustum(-1,1,1,-1));
    // 64mm baseline yields analytically 64,16,.32 pixels of disparity at the
    // three selected depths for a 1000px, 90-degree horizontal frustum.
    const double depths[]{.5,2,100},expected_disparity[]{64,16,.32};
    for(unsigned i=0;i<3;++i){assert(project_ui_anchor(symmetric,{0,0,depths[i]},1000,800,result));
        assert(near(result.eyes[0].x,500+expected_disparity[i]/2));
        assert(near(result.eyes[1].x,500-expected_disparity[i]/2));
        assert(near(result.eyes[0].x-result.eyes[1].x,expected_disparity[i]));
        assert(near(result.eyes[0].y,400)&&near(result.eyes[1].y,400));}
    auto frozen_before=symmetric;
    // A camera at (10,20,30), turned +90 degrees about Y: forward is world +X;
    // right is world -Z. This chosen world point has local (0.5,1,5).
    auto rotated=symmetric;
    for(unsigned eye=0;eye<2;++eye)rotated.view[eye]={0,0,1,0,0,1,0,0,-1,0,0,0,float(30+(eye?-.032:.032)),-20,-10,1};
    assert(project_ui_anchor(rotated,{15,21,29.5},1000,800,result));
    assert(near(result.eyes[0].x,500+1000*(.5+.032)/10,.0005));
    assert(near(result.eyes[1].x,500+1000*(.5-.032)/10,.0005));
    assert(near(result.eyes[0].y,320)&&near(result.eyes[1].y,320));
    assert(result.eyes[0].w==5&&result.eyes[1].w==5);
    assert(symmetric.view[0]==frozen_before.view[0]&&symmetric.projection[0]==frozen_before.projection[0]);

    // Being outside XY is not a failure or an occlusion claim.
    assert(project_ui_anchor(symmetric,{10,0,2},1000,800,result));
    assert(!result.eyes[0].in_viewport_xy&&!result.eyes[1].in_viewport_xy&&result.eyes[0].x>1000);
    assert(project_ui_anchor(symmetric,{0,10,2},1000,800,result));
    assert(!result.eyes[0].in_viewport_xy&&!result.eyes[1].in_viewport_xy&&result.eyes[0].y<0);
    assert(project_ui_anchor(symmetric,{1.99,0,2},1000,800,result));
    assert(!result.eyes[0].in_viewport_xy&&result.eyes[1].in_viewport_xy);
    // Continuous clip boundaries are inclusive, not integer pixel indices.
    auto mono=symmetric;mono.view[0]=mono.view[1]=identity();
    assert(project_ui_anchor(mono,{-2,2,2},1000,800,result));
    assert(result.eyes[0].x==0&&result.eyes[0].y==0&&result.eyes[0].in_viewport_xy);
    assert(project_ui_anchor(mono,{2,-2,2},1000,800,result));
    assert(result.eyes[0].x==1000&&result.eyes[0].y==800&&result.eyes[0].in_viewport_xy);

    saved=result;
    for(auto world:{Vector{0,0,0},Vector{0,0,-1},Vector{(std::numeric_limits<double>::quiet_NaN)(),0,1},
                    Vector{0,(std::numeric_limits<double>::infinity)(),1}})
        assert(!project_ui_anchor(symmetric,world,1000,800,result)&&same(result,saved));
    assert(!project_ui_anchor(symmetric,{0,0,2},0,800,result)&&same(result,saved));
    assert(!project_ui_anchor(symmetric,{0,0,2},1000,0,result)&&same(result,saved));
    // Right-eye failure must not partially commit an already computed left eye.
    auto invalid=symmetric;invalid.view[1][14]=-3;
    assert(!project_ui_anchor(invalid,{0,0,2},1000,800,result)&&same(result,saved));
    invalid=symmetric;invalid.view[1][0]=2;
    assert(!project_ui_anchor(invalid,{0,0,2},1000,800,result)&&same(result,saved));
    invalid=symmetric;invalid.projection[1][11]=0;
    assert(!project_ui_anchor(invalid,{0,0,2},1000,800,result)&&same(result,saved));
    invalid=symmetric;invalid.projection[1][9]=(std::numeric_limits<float>::quiet_NaN)();
    assert(!project_ui_anchor(invalid,{0,0,2},1000,800,result)&&same(result,saved));
    invalid=symmetric;invalid.projection[1][0]=(std::numeric_limits<float>::max)();
    assert(!project_ui_anchor(invalid,{(std::numeric_limits<double>::max)(),0,2},1000,800,result)&&same(result,saved));
    printf("UI projection: asymmetric frusta, common world point, depth disparity, rigid transforms, XY bounds and unchanged-output failures passed\n");
}
