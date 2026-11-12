#include "../vr-common/cover_view_heading.h"
#include <cstdio>
#include <limits>
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
int main(){using namespace gears_vr;
    CoverViewAnchor original{{100,200,19},{0,1,0},{-1,0,0}},heading{};
    CoverViewResult render{};C(prepare_cover_view(original,{},100,render));
    C(cover_level_heading(render.base_view,{4,5,6},heading));
    C(heading.forward_world.y==1&&heading.right_world.x==-1&&heading.neutral_eye_world.x==4);
    // Pitch is removed only from the authenticated pre-HMD base. Raw HMD
    // yaw/pitch and eye translations are subsequently applied exactly once.
    auto pitched=render.base_view;const double s=std::sin(.4),c=std::cos(.4);
    pitched[5]=float(-s);pitched[6]=float(c);pitched[9]=float(c);pitched[10]=float(s);
    C(cover_level_heading(pitched,{7,8,9},heading));C(heading.forward_world.y==1&&heading.neutral_eye_world.z==9);
    const auto saved=heading;
    for(unsigned bad=0;bad<6;++bad){auto m=render.base_view;
        if(bad==0)m[0]=std::numeric_limits<float>::quiet_NaN();
        if(bad==1)m[15]=0;
        if(bad==2)m[0]*=2;
        if(bad==3)m[0]*=-1;
        if(bad==4)m[3]=.1f;
        if(bad==5){m[5]=-1;m[6]=0;m[9]=0;m[10]=1;}
        C(!cover_level_heading(m,{},heading));
        C(heading.neutral_eye_world.x==saved.neutral_eye_world.x&&heading.forward_world.y==saved.forward_world.y);
    }
    // Wall displacement is independent of observation heading.
    CoverViewAnchor wall{original.neutral_eye_world,{1,0,0},{0,1,0}};
    C(prepare_cover_view(wall,original,{-.3,.1},100,render));
    C(render.base_eye_world.x==100&&render.base_eye_world.y==170&&render.base_eye_world.z==29);
    std::printf("CoverViewHeadingTest checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
