#include "ui_plane_projection.h"
#include <cassert>
#include <limits>
#include <cstdio>

using namespace gears_vr;
static bool near(double a,double b,double tolerance=.0005){return std::abs(a-b)<=tolerance;}
static StereoMatrix identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
static StereoMatrix projection(double l=-1,double r=1,double u=1,double d=-1){return {
    float(2/(r-l)),0,0,0,0,float(2/(u-d)),0,0,float(-(r+l)/(r-l)),float(-(u+d)/(u-d)),0,1,0,0,1,0};}
static StereoMatrices parallel(StereoMatrix p){StereoMatrices m{};m.view[0]=m.view[1]=identity();
    m.view[0][12]=.032f;m.view[1][12]=-.032f;m.projection[0]=m.projection[1]=p;return m;}
static bool same(UiPlaneProjection const& a,UiPlaneProjection const& b){
    for(unsigned i=0;i<4;++i)if(a.world_points[i].x!=b.world_points[i].x||a.world_points[i].y!=b.world_points[i].y||a.world_points[i].z!=b.world_points[i].z)return false;
    for(unsigned e=0;e<2;++e){if(a.eyes[e].homography!=b.eyes[e].homography||a.eyes[e].affine!=b.eyes[e].affine||
        a.eyes[e].viewport_scale_translation!=b.eyes[e].viewport_scale_translation)return false;
        for(unsigned n=0;n<4;++n){auto const& x=a.eyes[e].points[n];auto const& y=b.eyes[e].points[n];
            if(x.x!=y.x||x.y!=y.y||x.w!=y.w||x.in_viewport_xy!=y.in_viewport_xy)return false;}}
    return true;
}
static void homography_agrees(UiPlaneProjection const& result,std::array<UiPlanePoint,4> const& source){
    for(unsigned eye=0;eye<2;++eye){auto const& h=result.eyes[eye].homography;
        for(unsigned n=0;n<4;++n){auto p=source[n];double w=p.x*h[2]+p.y*h[5]+h[8];
            assert(near((p.x*h[0]+p.y*h[3]+h[6])/w,result.eyes[eye].points[n].x));
            assert(near((p.x*h[1]+p.y*h[4]+h[7])/w,result.eyes[eye].points[n].y));assert(near(w,result.eyes[eye].points[n].w));}}
}
int main(){
    auto view=identity(),p=projection();auto frozen=parallel(p);UiPlaneProjection out{};
    const std::array<UiPlanePoint,4> source{{{250,200},{500,200},{250,400},{500,400}}};
    assert(project_ui_plane(view,p,frozen,1000,800,2,source,out));
    // Independent pinhole equations: source center unprojects to (0,0,+2).
    assert(out.world_points[3].x==0&&out.world_points[3].y==0&&out.world_points[3].z==2);
    assert(near(out.eyes[0].points[3].x,508)&&near(out.eyes[1].points[3].x,492));
    assert(near(out.eyes[0].points[3].y,400));
    for(unsigned eye=0;eye<2;++eye)assert(out.eyes[eye].affine&&out.eyes[eye].viewport_scale_translation);
    homography_agrees(out,source);
    // Quantized rotations are only approximately orthonormal. Exact inversion
    // of the supplied source matrix preserves a same-eye roundtrip nonetheless.
    auto quantized=identity();const float qc=float(std::cos(.7)),qs=float(std::sin(.7));
    quantized={qc,0,qs,0,0,1,0,0,-qs,0,qc,0,127,-83,209,1};
    auto quantized_eye=frozen;quantized_eye.view[0]=quantized_eye.view[1]=quantized;
    assert(project_ui_plane(quantized,p,quantized_eye,1000,800,2,source,out));
    for(unsigned eye=0;eye<2;++eye)for(unsigned n=0;n<4;++n){assert(near(out.eyes[eye].points[n].x,source[n].x));assert(near(out.eyes[eye].points[n].y,source[n].y));}
    assert(project_ui_plane(view,p,frozen,1000,800,.5,source,out));
    assert(near(out.eyes[0].points[3].x-out.eyes[1].points[3].x,64));

    // Audited asymmetric eye frusta: center of the ORIGINAL camera plane is
    // projected differently into each eye, not duplicated at each image center.
    const double l=std::tan(-.942477763),r=std::tan(.69813168),u=std::tan(.767595768),d=std::tan(-.947190166);
    auto asym=frozen;asym.projection[0]=projection(l,r,u,d);asym.projection[1]=projection(-r,-l,u,d);
    assert(project_ui_plane(view,p,asym,1000,800,2,source,out));
    assert(near(out.eyes[0].points[3].x,1000*(.016-l)/(r-l)));
    assert(near(out.eyes[1].points[3].x,1000*(-.016+r)/(r-l)));
    assert(near(out.eyes[0].points[3].y,800*u/(u-d)));
    // ORIGINAL projection itself may be asymmetric. Its pixel center is not
    // camera-axis forward: hardcoded p8=.2,p9=-.1 gives world(-.4,+.2,+2).
    auto source_asym=p;source_asym[8]=.2f;source_asym[9]=-.1f;
    assert(project_ui_plane(view,source_asym,frozen,1000,800,2,source,out));
    assert(near(out.world_points[3].x,-.4)&&near(out.world_points[3].y,.2));
    assert(near(out.eyes[0].points[3].x,408)&&near(out.eyes[0].points[3].y,360));

    // Source camera at (10,20,30), facing +worldX: symmetric-axis forward would
    // be (12,20,30); the asymmetric pixel center additionally shifts +Y/+Z.
    // Preserve this source origin/orientation rather than taking a latest HMD.
    auto turned=view;turned={0,0,1,0,0,1,0,0,-1,0,0,0,30,-20,-10,1};
    auto same_eye=frozen;same_eye.view[0]=same_eye.view[1]=turned;
    same_eye.projection[0]=same_eye.projection[1]=source_asym;
    assert(project_ui_plane(turned,source_asym,same_eye,1000,800,2,source,out));
    assert(near(out.world_points[3].x,12)&&near(out.world_points[3].y,20.2)&&near(out.world_points[3].z,30.4));
    for(unsigned eye=0;eye<2;++eye)for(unsigned n=0;n<4;++n){assert(near(out.eyes[eye].points[n].x,source[n].x));assert(near(out.eyes[eye].points[n].y,source[n].y));}
    homography_agrees(out,source);
    // Relative physical translation away from the source plane changes scale.
    auto translated=frozen;for(unsigned eye=0;eye<2;++eye){translated.view[eye]=identity();translated.view[eye][12]=-.1f;
        translated.view[eye][13]=-.2f;translated.view[eye][14]=-1;}
    assert(project_ui_plane(view,p,translated,1000,800,2,source,out));
    assert(near(out.eyes[0].points[3].x,450)&&near(out.eyes[0].points[3].y,480));
    assert(near(out.eyes[0].points[0].x,-50)&&out.eyes[0].viewport_scale_translation);

    // Relative yaw tilts the plane: independent ray formulas predict a varying
    // denominator, so viewport scale/translation cannot represent this map.
    auto yawed=frozen;const float c=float(std::cos(.2)),s=float(std::sin(.2));
    for(unsigned eye=0;eye<2;++eye)yawed.view[eye]={c,0,s,0,0,1,0,0,-s,0,c,0,0,0,0,1};
    assert(project_ui_plane(view,p,yawed,1000,800,2,source,out));
    assert(!out.eyes[0].affine&&!out.eyes[0].viewport_scale_translation);
    assert(near(out.eyes[0].points[3].x,500*(1-double(s)/c)));
    // Source(250,200) is camera(-1,+1,+2); yaw gives x=-c-2s,z=-s+2c.
    assert(near(out.eyes[0].points[0].x,500*(1+(-double(c)-2*s)/(-double(s)+2*c))));
    homography_agrees(out,source);
    auto rolled=frozen;for(unsigned eye=0;eye<2;++eye)rolled.view[eye]={0,1,0,0,-1,0,0,0,0,0,1,0,0,0,0,1};
    assert(project_ui_plane(view,p,rolled,1000,800,2,source,out));
    assert(out.eyes[0].affine&&!out.eyes[0].viewport_scale_translation);
    assert(near(out.eyes[0].points[0].x,250)&&near(out.eyes[0].points[0].y,600));
    homography_agrees(out,source);
    // Full-canvas overload includes top-left/bottom-right, in Tile order.
    auto mono=frozen;mono.view[0]=mono.view[1]=view;
    assert(project_ui_plane(view,p,mono,1000,800,2,out));
    assert(near(out.eyes[0].points[0].x,0)&&near(out.eyes[0].points[0].y,0));
    assert(near(out.eyes[0].points[3].x,1000)&&near(out.eyes[0].points[3].y,800));
    auto saved=out;
    for(double distance:{0.,-1.,(std::numeric_limits<double>::quiet_NaN)(),(std::numeric_limits<double>::infinity)()})
        assert(!project_ui_plane(view,p,frozen,1000,800,distance,source,out)&&same(out,saved));
    assert(!project_ui_plane(view,p,frozen,0,800,2,source,out)&&same(out,saved));
    assert(!project_ui_plane(view,p,frozen,1000,0,2,source,out)&&same(out,saved));
    auto invalid=view;invalid[0]=2;assert(!project_ui_plane(invalid,p,frozen,1000,800,2,source,out)&&same(out,saved));
    auto invalid_p=p;invalid_p[0]=0;assert(!project_ui_plane(view,invalid_p,frozen,1000,800,2,source,out)&&same(out,saved));
    auto invalid_eye=frozen;invalid_eye.view[1][14]=-3;
    assert(!project_ui_plane(view,p,invalid_eye,1000,800,2,source,out)&&same(out,saved));
    auto invalid_source=source;invalid_source[3].x=(std::numeric_limits<double>::quiet_NaN)();
    assert(!project_ui_plane(view,p,frozen,1000,800,2,invalid_source,out)&&same(out,saved));
    assert(!project_ui_plane(view,p,frozen,1000,800,(std::numeric_limits<double>::max)(),source,out)&&same(out,saved));
    printf("UI plane: asymmetric source/eye projection, preserved source pose, parallax, translation, projective yaw/affine roll and atomic failures passed\n");
}
