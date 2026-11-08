#include "cover_body_target.h"
#include <cstdio>
#include <cstring>
#include <limits>

using namespace gears_vr;
unsigned checks{},failures{};
#define C(value) do{++checks;if(!(value)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#value);}}while(0)
bool near(double a,double b){return std::abs(a-b)<1e-8;}
struct Fixture {
    CoverBodyTargetKey key{{1,2,3,4,5,6,7},0x1000,0x2000,0x3000,0x4000,8,9,10,11,12,13,0};
    CoverViewAnchor anchor{{100,-200,30},{1,0,0},{0,1,0}};
    CoverViewAnchor view_anchor=anchor;
    CoverOffset offset{-.1,.2};CoverViewResult view{};StereoResult stereo{};
    Fixture(){
        key.world_generation=14;
        C(prepare_cover_view(anchor,offset,100,view));stereo.status=StereoStatus::accepted;
        stereo.reference={{0,1.6,0},{}};stereo.reference_generation=key.reference;
        auto& s=stereo.sample;s.source=StereoSource::MetaOperatorSimulated;s.session=key.xr_session;
        s.sequence=key.sequence;s.space_epoch=key.xr_space;s.received_ns=1000000000;
        s.head=stereo.reference;s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;eyes();
    }
    void eyes(){auto& s=stereo.sample;for(unsigned i=0;i<2;++i){
        auto d=transform(rotation_matrix(s.head.orientation),{i?.032:-.032,0,0});
        s.eyes[i].pose={{s.head.position.x+d.x,s.head.position.y+d.y,s.head.position.z+d.z},s.head.orientation};
        s.eyes[i].fov={-.7,.7,.7,-.7};
    }}
    bool run(CoverBodyTarget& out,CoverBodyTargetKey const* expected=nullptr,std::int64_t stamp=1010000000){
        return prepare_cover_body_target(expected?*expected:key,key,anchor,view_anchor,view,offset,stereo,stamp,100,out,true);
    }
};
int main(){
    for(double wall_angle:{0.,1.5707963267948966,3.1415926535897932,-1.5707963267948966})
    for(double view_angle:{0.,.41,-1.1}){
        Fixture f;f.anchor.forward_world={std::cos(wall_angle),std::sin(wall_angle),0};
        f.anchor.right_world={-std::sin(wall_angle),std::cos(wall_angle),0};
        f.view_anchor.forward_world={std::cos(view_angle),std::sin(view_angle),0};
        f.view_anchor.right_world={-std::sin(view_angle),std::cos(view_angle),0};
        C(prepare_cover_view(f.anchor,f.view_anchor,f.offset,100,f.view));
        f.stereo.sample.head.position.x+=.15;f.stereo.sample.head.position.z-=.1;f.eyes();
        CoverBodyTarget target{};const bool accepted=f.run(target);C(accepted);
        if(accepted){
            const auto x=.15*f.view_anchor.right_world.x+.1*f.view_anchor.forward_world.x;
            const auto y=.15*f.view_anchor.right_world.y+.1*f.view_anchor.forward_world.y;
            C(near(target.desired_right_metres,f.offset.right+x*f.anchor.right_world.x+y*f.anchor.right_world.y));
            C(near(target.desired_forward_metres,x*f.anchor.forward_world.x+y*f.anchor.forward_world.y));
            C(near(target.desired_eye_world.x,f.view.base_eye_world.x+100*x)&&
              near(target.desired_eye_world.y,f.view.base_eye_world.y+100*y)&&near(target.desired_eye_world.z,50));
            C(target.view_anchor.forward_world.x==f.view_anchor.forward_world.x);
            C(!target.pose_applied&&!target.collision_verified&&!target.same_pose_epoch_verified&&!target.head_bone_goal_valid);
        }
    }
    // The corners retain both independent axes, including forward motion.
    for(double right:{-.3,0.,.3})for(double up:{-.25,0.,.55,.8})for(double forward:{-.2,0.,.25}){
        Fixture f;f.stereo.sample.head.position={right,1.6+up,-forward};f.eyes();CoverBodyTarget out{};C(f.run(out));
        C(near(out.desired_right_metres,right-.1)&&near(out.desired_up_metres,up+.2)&&near(out.desired_forward_metres,forward));
        C(near(out.desired_eye_world.x,100+100*forward)&&near(out.desired_eye_world.y,-200+100*(right-.1))&&near(out.desired_eye_world.z,30+100*(up+.2)));
        C(!out.head_bone_goal_valid&&!out.pose_applied&&!out.same_pose_epoch_verified&&!out.collision_verified);
        C(out.tracking.sample.sequence==f.stereo.sample.sequence&&same(out.key,f.key));
    }
    for(double angle:{-1.2,0.,1.2}){
        Fixture f;f.stereo.sample.head.orientation={std::sin(angle/2),0,0,std::cos(angle/2)};f.eyes();CoverBodyTarget out{};
        C(f.run(out));C(near(out.desired_eye_world.x,100)&&near(out.desired_eye_world.y,-210)&&near(out.desired_eye_world.z,50));
        f.stereo.sample.head.position={.25,2.15,-.1};f.eyes();C(f.run(out));
        C(near(out.desired_right_metres,.15)&&near(out.desired_up_metres,.75)&&near(out.desired_forward_metres,.1));
    }
    for(unsigned fault=0;fault<23;++fault){
        Fixture f;CoverBodyTarget value{};C(f.run(value));const auto before=value;auto expected=f.key;auto stamp=std::int64_t(1010000000);
        switch(fault){
        case 0:++expected.epochs.world;break;case 1:++expected.pawn;break;case 2:++expected.mesh;break;
        case 3:++expected.link;break;case 4:++expected.slot;break;case 5:++expected.scene;break;case 6:++expected.thread;break;
        case 7:++f.stereo.sample.sequence;break;case 8:++f.stereo.sample.session;break;case 9:++f.stereo.sample.space_epoch;break;
        case 10:++f.stereo.reference_generation;break;case 11:stamp=1200000000;break;case 12:stamp=900000000;break;
        case 13:f.anchor.right_world={1,0,0};break;case 14:f.view.base_eye_world.x+=1;break;
        case 15:f.view.base_view[0]+=1;break;case 16:f.stereo.sample.head.position.x=std::numeric_limits<double>::quiet_NaN();break;
        case 17:f.stereo.heading_deferred=true;break;case 18:f.stereo.sample.view_flags=0;break;
        case 19:f.view_anchor.neutral_eye_world.x+=1;break;
        case 20:f.view_anchor.forward_world={};break;
        case 21:f.view_anchor.forward_world={0,1,0};f.view_anchor.right_world={-1,0,0};break;
        case 22:f.view_anchor.right_world.x=std::numeric_limits<double>::quiet_NaN();break;
        }
        C(!f.run(value,&expected,stamp));C(!std::memcmp(&value,&before,sizeof value));
    }
    Fixture f;CoverBodyTarget value{};C(f.run(value));C(cover_body_target_current(value,f.key,1050000000));
    C(!cover_body_target_current(value,f.key,1300000000));auto next=f.key;++next.reference;
    C(!cover_body_target_current(value,next,1050000000));C(!cover_body_target_current(value,f.key,1000000000));
    // Runtime sources do not gain simulator permission from the payload.
    C(!prepare_cover_body_target(f.key,f.key,f.anchor,f.view_anchor,f.view,f.offset,f.stereo,1010000000,100,value));
    f.stereo.sample.source=StereoSource::RuntimeTracked;
    C(prepare_cover_body_target(f.key,f.key,f.anchor,f.view_anchor,f.view,f.offset,f.stereo,1010000000,100,value));
    f.stereo.sample.head_flags=3;
    C(!prepare_cover_body_target(f.key,f.key,f.anchor,f.view_anchor,f.view,f.offset,f.stereo,1010000000,100,value));
    std::printf("CoverBodyTargetTest: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
