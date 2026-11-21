#include "weapon_troika_mounted_space.h"
#include <cstdio>
#include <cstdlib>
namespace {unsigned checks{};void check(bool x,int line){++checks;if(!x){std::fprintf(stderr,"line %d\n",line);std::exit(1);}}
#define CHECK(x) check(bool(x),__LINE__)
}
int main(){
    using namespace gears_weapon;namespace r=gears_rig_tracking;namespace v=gears_vr;namespace a=gears_troika_source;
    constexpr int64_t now=2000000000;
    r::Frame entry{};entry.status=r::Status::ready;entry.flags=r::controllers_matched|r::producer_continuity_available;
    entry.native={1,2,3,4,5};entry.publication_sequence=6;entry.scene_invocation=7;entry.world_generation=8;entry.binding_generation=9;
    entry.reference_generation=10;entry.published_ns=now-1000000;entry.scene_qpc=now-2000000;entry.qpc_frequency=1000000000;
    entry.observed={1,1,1,{1,1}};entry.producer={11,{12,13}};entry.native_reference={14,r::reference_stable,0};
    entry.controller_publisher_pid=15;entry.controller_publisher_start=16;entry.units_per_metre=100;entry.view_policy=r::first_person;
    entry.world={0x1000,0,0x9000,1,1,0,1};entry.controller={0x2000,0,0x9000,2,2,0,1};entry.pawn={0x3000,0,0x9000,3,3,0,1};
    entry.mesh={0x4000,0x3000,0x9000,4,4,0,1};entry.weapon={};
    entry.base_view={1,0,0,0,0,1,0,0,0,0,1,0,40,50,60,1};entry.source_view=entry.base_view;
    auto& stereo=entry.stereo;stereo.session=17;stereo.sequence=18;stereo.space_epoch=19;stereo.received_ns=now-3000000;
    stereo.predicted_time=20;stereo.head_flags=stereo.view_flags=15;stereo.focused=stereo.visible=stereo.should_render=1;stereo.head={};
    for(unsigned i=0;i<2;++i){stereo.eyes[i].pose={};stereo.eyes[i].pose.position.x=i?.032:-.032;stereo.eyes[i].fov={-.7,.7,.6,-.6};}
    auto& input=entry.controllers;input.session=17;input.sequence=18;input.space_epoch=19;input.profile_generation=21;input.input_epoch=22;
    input.predicted_time=20;input.received_ns=now-4000000;input.sync_ns=now-5000000;input.focused=1;
    for(unsigned i=0;i<2;++i){auto& hand=input.hands[i];hand.grip_active=hand.aim_active=1;hand.grip_flags=hand.aim_flags=15;
        hand.grip={{.1+double(i)*.2,.4,-.5},{.6,0,0,.8}};hand.aim=hand.grip;
        CHECK(v::rig_tracking::pose(entry.base_view,entry.reference,hand.grip,100,entry.world_grip[i]));}
    CHECK(v::rig_tracking::pose(entry.base_view,entry.reference,stereo.head,100,entry.world_head));entry.world_pose_mask=7;
    CHECK(r::frame_valid(entry,now,r::maximum_age_ns));
    TroikaMountKey owner{entry.native,entry.world,entry.controller,entry.pawn,{0x5000,0,0x9000,5,5,0,1},{0x6000,0,0x9000,6,6,0,1},23,24};
    TroikaMountedSpace space;CHECK(space.capture(owner,entry,now));CHECK(!space.capture(owner,entry,now));
    a::Frame mounted{};mounted.native=entry.native;mounted.world=entry.world;mounted.controller=entry.controller;mounted.driver=entry.pawn;
    mounted.driver_mesh=entry.mesh;mounted.turret=owner.turret;mounted.weapon=owner.weapon;mounted.turret_mesh={0x7000,0x5000,0x9000,7,7,0,1};
    mounted.publication_sequence=1;mounted.source_ticket=2;mounted.scene_invocation=25;mounted.world_generation=9;
    mounted.scene_qpc=entry.scene_qpc;mounted.qpc_frequency=entry.qpc_frequency;mounted.published_ns=entry.published_ns;
    mounted.reference_generation=entry.reference_generation;mounted.native_reference_revision=2;mounted.native_reference=entry.native_reference;
    mounted.producer=entry.producer;mounted.publisher_pid=15;mounted.publisher_start=16;mounted.stereo=stereo;mounted.controllers=input;mounted.reference=entry.reference;
    mounted.control_policy=a::mounted_look_suppressed;
    {auto denied=mounted;denied.control_policy=0;CHECK(!a::frame_valid(denied,now,r::maximum_age_ns));
        denied=mounted;denied.abi_version=1;CHECK(!a::frame_valid(denied,now,r::maximum_age_ns));}
    TroikaMountedMapping first{};CHECK(space.map(owner,mounted,now,first));CHECK(first.new_publication&&first.continuity_changed&&first.hands.tracked_mask==3);
    CHECK(v::stereo_same_pose(first.hands.world[0],entry.world_grip[0]));CHECK(first.hands.input_sync_ns==input.sync_ns);
    TroikaMountedMapping result{};CHECK(space.map(owner,mounted,now,result));CHECK(!result.new_publication&&!result.continuity_changed);
    CHECK(result.units_per_metre==entry.units_per_metre);
    a::ConsumerCursor first_consumer,second_consumer;
    CHECK(!result.new_publication); // Geometry already mapped by another reader.
    CHECK(first_consumer.take(mounted,now));CHECK(!first_consumer.take(mounted,now));
    CHECK(second_consumer.take(mounted,now)); // Each actual consumer still sees this publication once.
    auto next_for_consumer=mounted;++next_for_consumer.publication_sequence;
    CHECK(first_consumer.take(next_for_consumer,now));CHECK(!first_consumer.take(mounted,now));
    CHECK(!first_consumer.take(next_for_consumer,now+100000001));
    gears_vr::Pose press{};CHECK(!space.map_press(owner,mounted,0,now,press));
    auto pressed=mounted;++pressed.publication_sequence;
    for(auto& hand:pressed.controllers.hands){hand.last_squeeze_press_grip=hand.grip;
        hand.last_squeeze_press_sequence=pressed.controllers.sequence;hand.last_squeeze_press_flags=15;hand.grip.position.x+=.2;}
    for(unsigned side=0;side<2;++side){CHECK(space.map_press(owner,pressed,side,now,press));
        CHECK(v::stereo_same_pose(press,entry.world_grip[side]));}
    CHECK(!space.map_press(owner,pressed,2,now,press));
    ++mounted.publication_sequence;++mounted.publication_sequence;
    auto next=mounted;++next.publication_sequence;next.stereo.head.orientation={0,.6,0,.8};
    CHECK(space.map(owner,next,now,result));CHECK(!result.continuity_changed&&result.new_publication);
    CHECK(v::stereo_same_pose(first.hands.world[0],result.hands.world[0])&&v::stereo_same_pose(first.hands.world[1],result.hands.world[1]));
    CHECK(!v::stereo_same_pose(first.world_head,result.world_head));
    ++next.publication_sequence;++next.controllers.input_epoch;CHECK(space.map(owner,next,now,result));CHECK(result.continuity_changed);
    CHECK(v::stereo_same_pose(first.hands.world[0],result.hands.world[0]));CHECK(result.identity.input_epoch==next.controllers.input_epoch);
    const auto sentinel=result;
    for(unsigned fault=0;fault<9;++fault){
        auto bad=next;auto wrong=owner;++bad.publication_sequence;
        if(fault==0)++wrong.token;
        if(fault==1)++bad.native.registration_cookie;
        if(fault==2)++bad.driver.registry_index;
        if(fault==3)++bad.stereo.space_epoch;
        if(fault==4)++bad.reference_generation;
        if(fault==5)bad.reference.position.x=.1;
        if(fault==6)++bad.native_reference.epoch;
        if(fault==7)++bad.publisher_start;
        if(fault==8)bad.controllers.sync_ns=now-100000001;
        result=sentinel;CHECK(!space.map(wrong,bad,now,result));CHECK(result.publication_sequence==sentinel.publication_sequence&&v::stereo_same_pose(result.hands.world[0],sentinel.hands.world[0]));
    }
    ++next.publication_sequence;CHECK(space.map(owner,next,now,result));CHECK(result.continuity_changed); // Fresh restoration cannot reuse old action continuity.
    auto mutated=next;mutated.controllers.hands[1].grip.position.x+=.1;CHECK(!space.map(owner,mutated,now,result));
    CHECK(!space.map(owner,mounted,now,result)); // Old publication cannot replay.
    ++next.publication_sequence;next.controllers.hands[0].grip_active=0;next.controllers.hands[0].grip_flags=0;
    ++next.producer.tracking[0];CHECK(space.map(owner,next,now,result));CHECK(result.continuity_changed&&result.hands.tracked_mask==2);
    space.reset();CHECK(!space.map(owner,next,now,result));
    // The renderer must use the same retained pre-HMD basis as hand mapping.
    // Compose both eyes once; feeding mapped world_head as a new base would
    // add the HMD delta twice. No current turret camera is a view input.
    TroikaMountedSpace visual;CHECK(visual.capture(owner,entry,now));
    v::StereoMatrix base{};CHECK(visual.render_base(owner,mounted,now,base));
    CHECK(base==entry.base_view);
    v::StereoMatrix projection{};projection[0]=projection[5]=1;projection[10]=1;
    projection[11]=1;projection[14]=-1;
    for(unsigned motion=0;motion<6;++motion){
        auto source=mounted;source.publication_sequence=10+motion;
        const double angle=.15*motion;
        source.stereo.head.position={.04*motion,.02*motion,-.03*motion};
        source.stereo.head.orientation={0,std::sin(angle/2),0,std::cos(angle/2)};
        const auto head_rotation=v::rotation_matrix(source.stereo.head.orientation);
        for(unsigned side=0;side<2;++side){
            const auto delta=v::transform(head_rotation,{side?.032:-.032,0,0});
            auto& eye=source.stereo.eyes[side].pose;eye.orientation=source.stereo.head.orientation;
            eye.position={source.stereo.head.position.x+delta.x,source.stereo.head.position.y+delta.y,
                source.stereo.head.position.z+delta.z};
        }
        CHECK(visual.render_base(owner,source,now,base));CHECK(base==entry.base_view);
        v::StereoMatrices rendered{};CHECK(v::stereo_matrices(base,projection,source.reference,source.stereo,100,rendered));
        TroikaMountedMapping mapped{};CHECK(visual.map(owner,source,now,mapped));
        for(unsigned side=0;side<2;++side){
            v::Pose world_eye{};CHECK(v::rig_tracking::pose(base,source.reference,source.stereo.eyes[side].pose,100,world_eye));
            CHECK(v::stereo_distance(world_eye.position,v::stereo_camera_position(rendered.view[side]))<1e-4);
            // View columns yield exactly the world forward/right/up encoded by
            // the tracking mapper, including a nonzero head yaw.
            const auto orientation=v::rotation_matrix(world_eye.orientation);
            for(unsigned axis=0;axis<3;++axis){
                CHECK(std::abs(rendered.view[side][4*axis+2]-orientation[axis][0])<1e-5);
                CHECK(std::abs(rendered.view[side][4*axis]-orientation[axis][1])<1e-5);
                CHECK(std::abs(rendered.view[side][4*axis+1]-orientation[axis][2])<1e-5);
            }
        }
        CHECK(v::stereo_same_pose(mapped.hands.world[0],entry.world_grip[0]));
        CHECK(v::stereo_same_pose(mapped.hands.world[1],entry.world_grip[1]));
    }
    const auto preserved=base;
    for(unsigned fault=0;fault<4;++fault){
        auto bad=mounted;bad.publication_sequence=30+fault;auto wrong=owner;
        if(fault==0)++wrong.token;
        if(fault==1)++bad.reference_generation;
        if(fault==2)bad.controllers.sync_ns=now-100000001;
        if(fault==3)++bad.native_reference.epoch;
        CHECK(!visual.render_base(wrong,bad,now,base));CHECK(base==preserved);
    }
    visual.reset();CHECK(!visual.render_base(owner,mounted,now,base));CHECK(base==preserved);
    std::printf("Troika mounted entry-fixed mapping: %u checks passed; data only, no grip/fire/C0 grant.\n",checks);
}
