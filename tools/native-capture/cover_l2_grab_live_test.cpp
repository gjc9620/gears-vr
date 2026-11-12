#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "anchor_observer.h"
#include "cover_l2_viewpoint_getter.h"
#include "../vr-common/cover_l2_grab_points.h"
#include "../vr-common/cover_input_owner.h"
#include <cstdio>
namespace cover_neutral_eye_live {struct EyeSample{};}
namespace cover_geometry_live {
struct Geometry {struct {struct {bool value=true;} permissions[4];} selected;};
bool available=true;
gears_cover_l2_points::Result source{{100,0,19},{{{100,-70,19},{100,70,19},{100,0,119}}},{{true,true,true}},2};
bool sample_l2_with(gears_anchor::Receipt const*,cover_neutral_eye_live::EyeSample const&,Geometry&,gears_cover_l2_points::Result& out){
    if(!available)return false;out=source;return true;
}
}
namespace cover_input_live {
struct {gears_vr::CoverInputOwnerResult latest{};} diagnostics;
using GrabPointProvider=bool(*)(gears_anchor::Receipt const*,cover_neutral_eye_live::EyeSample const&,gears_vr::CoverInputOwnerFrame&);
GrabPointProvider grab_points_provider{};
void (*grab_point_summary)(FILE*){};
}
#include "cover_l2_grab_live.inl"
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
int main(){
    using namespace gears_vr;
    CoverInputOwnerFrame f{};f.cover.cover_valid=f.cover.interaction_enabled=true;
    f.cover.epochs={1,1,1,1,1,1,1};f.anchor={{100,0,19},{1,0,0},{0,1,0}};
    f.units_per_metre=100;f.stereo.reference.orientation={0,0,0,1};
    ControllerInputFrame hands{};auto& hand=hands.hands[0];hand.grip_active=1;hand.grip_flags=15;
    hand.grip.orientation={0,0,0,1};hand.grip.position={-.7,0,0};f.controllers=&hands;
    cover_neutral_eye_live::EyeSample eye{};gears_anchor::Receipt scene{};
    C(cover_input_live::grab_points_provider!=nullptr);
    C(cover_input_live::grab_points_provider(&scene,eye,f));
    C(f.cover.edge.kind==CoverEdgeKind::left&&f.cover.edge.within_acquisition_reach);
    C(std::abs(f.cover.range.min_right+.7)<1e-9&&std::abs(f.cover.range.max_right-.7)<1e-9&&f.cover.range.max_up==1);
    auto& previous=cover_input_live::diagnostics.latest.interaction;
    previous.phase=CoverPhase::grabbing;previous.edge_id=1;previous.offset={-.3,0};
    hand.grip.position={.7,0,0};
    C(cover_l2_grab_live::prepare(&scene,eye,f));
    C(f.cover.edge.id==1&&!f.cover.edge.within_acquisition_reach);
    previous.phase=CoverPhase::held;hand.grip.position={1,0,0};
    C(cover_l2_grab_live::prepare(&scene,eye,f));C(f.cover.edge.id==2); // .7m world after -.3m held offset.
    ++f.cover.epochs.cover;hand.grip.position={-.7,0,0};
    C(cover_l2_grab_live::prepare(&scene,eye,f));C(f.cover.edge.id==1); // Old cover offset cannot leak.
    cover_geometry_live::source.type=1;hand.grip.position={0,1,0};
    C(cover_l2_grab_live::prepare(&scene,eye,f));C(f.cover.range.max_up==0&&!f.cover.edge.allowed);
    f.controllers=nullptr;C(cover_l2_grab_live::prepare(&scene,eye,f));C(!f.cover.edge.allowed&&f.cover.range.min_right<0);
    cover_geometry_live::available=false;C(!cover_l2_grab_live::prepare(&scene,eye,f));
    std::printf("CoverL2GrabLiveTest checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
