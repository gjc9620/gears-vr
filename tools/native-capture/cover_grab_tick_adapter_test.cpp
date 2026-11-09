// Reuse only the existing host file/hash helpers, not its test main.
#define main ExistingBodyGoalMain
#include "body_goal_view_adapter_test.cpp"
#undef main
#include "cover_grab_tick_adapter.h"
namespace tick_grab=gears_cover_grab_tick;
int main(int argc,char** argv){if(argc!=2)return 2;namespace tg=tick_grab;namespace src=tg::source;
    src::Input input{};auto& t=input.observed;auto& s=t.parent_scene;
    C(read(std::string(argv[1])+"/source-CT0.bin",&s,sizeof s));if(failures)return 1;
    s.flags|=src::tick::rig::cover_key_known;s.cover_type=2;s.cover_slot=0;s.cover_link=s.world;
    s.reference_generation+=7; // View reference and Native reference revision are distinct domains.
    const auto now=std::int64_t(s.published_ns)+1000000000;
    input.scene_cache_revision=2;input.metadata_begin_ns=now-10000000;input.metadata_end_ns=now-8000000;
    t.tick=1;t.thread=s.native.game_thread;t.parent_cookie=123;t.begin_ns=now-7000000;t.end_ns=now-3000000;
    t.stereo=s.stereo;t.stereo.sequence+=5;t.stereo.received_ns=now-4000000;t.stereo.predicted_time+=1000000000;
    t.controllers=s.controllers;t.controllers.sequence=t.stereo.sequence;t.controllers.predicted_time=t.stereo.predicted_time;
    t.controllers.received_ns=now-5000000;t.controllers.sync_ns=now-6000000;
    t.controllers.hands[0].squeeze=.3f; // Host selected-down result in the producer's hysteresis band.
    t.continuity.input=t.controllers;t.continuity.stream_epoch=s.producer.stream;
    for(unsigned i=0;i<2;++i)t.continuity.tracking_epoch[i]=s.producer.tracking[i];
    t.reference_revision=s.native_reference_revision;t.native_reference=s.native_reference;
    t.publisher_pid=s.controller_publisher_pid;t.publisher_start=s.controller_publisher_start;
    C(src::classify(input,s,now,true)==src::Verdict::okay);if(failures)return 1;
    tg::body::CoverPlacement placement{};placement.world=s.world;placement.pawn=s.pawn;placement.mesh=s.mesh;placement.link=s.cover_link;
    placement.type=s.cover_type;placement.slot=s.cover_slot;placement.world_generation=s.world_generation;
    placement.scene=s.scene_invocation;placement.publication=s.publication_sequence;
    placement.reference=s.reference_generation;placement.observed_ns=input.metadata_end_ns;
    tg::free::EyeFrame eye{};eye.actual_eye_world={{100,200,300},{}};auto& k=eye.key;
    k.game_pid=s.native.game_pid;k.game_thread=s.native.game_thread;k.game_start=s.native.game_start;k.native_started_qpc=s.native.native_started_qpc;
    k.world_generation=s.world_generation;k.mesh=s.mesh.address;k.binding_generation=s.binding_generation;k.tick=t.tick;k.compose=2;
    k.source=t.stereo.source;k.stereo_session=t.stereo.session;k.stereo_sequence=t.stereo.sequence;k.space_epoch=t.stereo.space_epoch;
    k.reference_revision=t.reference_revision;k.controller_sequence=t.controllers.sequence;k.input_epoch=t.controllers.input_epoch;
    k.profile_generation=t.controllers.profile_generation;k.predicted_time=t.stereo.predicted_time;
    tg::grab::Key key{{1,2,3,4,5,6,7},42,200,3,23};v::CoverObservation cover{};
    cover.sequence=2;cover.observed_ns=placement.observed_ns;cover.epochs=key.epochs;cover.cover_valid=cover.interaction_enabled=true;
    cover.edge={42,v::CoverEdgeKind::top,true,true};v::CoverControllerResult hand{};hand.status=v::CoverControllerStatus::accepted;
    hand.continuity_generation=8;hand.hand={t.controllers.sequence,t.controllers.received_ns,5,6,t.controllers.hands[0].grip.position,true,true,true,true};
    v::CoverResult interaction{};interaction.offset_certified=true;v::CoverOwnership ownership{};
    tg::grab::Frame out{};const auto signature=tg::grab::mount::marcus115_signature;
    C(tg::prepare(input,placement,eye,key,cover,hand,interaction,ownership,signature,tg::grab::Mode::approach,now,true,out)==tg::Status::prepared);
    C(out.hand.observed_ns==t.controllers.received_ns&&out.hand.observed_ns!=now);
    C(out.hand.left_position_metres.x==t.controllers.hands[0].grip.position.x);
    C(out.hand.lgrip_down);C(placement.reference!=t.reference_revision);
    C(out.now_ns==now&&out.model_scale==1&&out.units_per_metre==s.units_per_metre);
    tg::free::HandTarget expected{};tg::grab::mount::Mount mount{};
    C(tg::grab::mount::marcus115(signature,tg::grab::mount::Side::left,1,s.units_per_metre,mount));
    C(tg::free::wrist_from_eye(eye.actual_eye_world,t.stereo.head,t.controllers.hands[0].grip,mount.calibration,s.units_per_metre,expected)==tg::free::HandStatus::ready);
    C(v::stereo_same_pose(out.physical_grip_world,expected.grip_world));
    for(unsigned defect=0;defect<14;++defect){auto in=input;auto p=placement;auto e=eye;auto h=hand;auto c=cover;auto sig=signature;
        switch(defect){case 0:++p.link.name_number;break;case 1:++p.scene;break;case 2:++p.publication;break;
        case 3:++e.key.tick;break;case 4:++e.key.reference_revision;break;case 5:h.hand.observed_ns=now;break;
        case 6:h.hand.left_position_metres.x+=.001;break;case 7:++h.hand.sequence;break;
        case 8:in.observed.controllers.received_ns=now-40000001;break;case 9:++c.epochs.reference;break;
        case 10:sig[0]^=1;break;case 11:h.status=v::CoverControllerStatus::needs_release;break;
        case 12:p.observed_ns=input.metadata_begin_ns-1;break;
        default:p.observed_ns=input.metadata_end_ns+1;c.observed_ns=p.observed_ns;break;}
        const auto before=bytes(out);
        C(tg::prepare(in,p,e,key,c,h,interaction,ownership,sig,tg::grab::Mode::held,now,true,out)!=tg::Status::prepared);
        C(bytes(out)==before);
    }
    {namespace body=gears_full_body_c0;namespace marker=gears_body_head_authored;
        const std::string folder=argv[1];auto frame=std::make_unique<body::Frame>();auto model=std::make_unique<body::Model>();
        auto fk=std::make_unique<body::Workspace>();auto work=std::make_unique<tg::Workspace>();
        C(read(folder+"/reference.bin",frame->reference.data(),sizeof frame->reference));
        C(read(folder+"/C0-local.bin",frame->local.data(),sizeof frame->local));
        C(read(folder+"/C0-component.bin",&frame->component_to_world,sizeof frame->component_to_world));
        frame->units_per_metre=s.units_per_metre;C(body::prepare_model(frame->reference,digest,*model));
        for(unsigned i=0;i<16;++i)fk->component.m[i]=frame->component_to_world.values[i];
        std::copy(frame->local.begin(),frame->local.end(),fk->pose.local);C(body::d::rebuild(*model,*fk));
        body::Output baseline{};baseline.local=frame->local;baseline.root_world=body::d::pose(fk->pose.world[body::root]);
        baseline.pelvis_world=body::d::pose(fk->pose.world[body::pelvis]);baseline.head_world=body::d::pose(fk->pose.world[body::head]);
        for(unsigned side=0;side<2;++side){baseline.feet_world[side]=body::d::pose(fk->pose.world[body::legs[side][2]]);
            baseline.shoulders_world[side]=body::d::pose(fk->pose.world[body::shoulders[side]]);}
        auto marker_work=std::make_unique<marker::Workspace>();marker::Marker mark{};marker::EyeHmdWorld measured_eye{};
        C(marker::prepare(*model,*frame,marker::Basis::marcus_mesh_face_forward_y,*marker_work,mark)==marker::Status::okay);
        C(marker::eye_from_head(mark,*model,*frame,{baseline.head_world},measured_eye)==marker::Status::okay);
        eye.actual_eye_world=measured_eye.pose;
        const auto wrist=body::d::pose(fk->pose.world[11]);
        // HOST input generation: invert the authored mount/eye mapping for a
        // reachable wrist. No runtime tracking or physical calibration claim.
        const auto grip_q=v::multiply(wrist.orientation,v::inverse(mount.calibration.grip_to_wrist_rotation));
        const auto offset=v::transform(v::rotation_matrix(grip_q),mount.calibration.grip_to_wrist_metres);
        const v::Vector grip_p{wrist.position.x-offset.x*s.units_per_metre,
            wrist.position.y-offset.y*s.units_per_metre,wrist.position.z-offset.z*s.units_per_metre};
        const auto relative=v::multiply(v::inverse(eye.actual_eye_world.orientation),grip_q);
        const v::Quaternion xr_q{-relative.y,-relative.z,relative.x,relative.w};
        const v::Vector world_d{(grip_p.x-eye.actual_eye_world.position.x)/s.units_per_metre,
            (grip_p.y-eye.actual_eye_world.position.y)/s.units_per_metre,(grip_p.z-eye.actual_eye_world.position.z)/s.units_per_metre};
        const auto ue=v::transform(v::rotation_matrix(v::inverse(eye.actual_eye_world.orientation)),world_d);
        const auto xr=v::transform(v::rotation_matrix(t.stereo.head.orientation),{ue.y,ue.z,-ue.x});
        t.controllers.hands[0].grip={{t.stereo.head.position.x+xr.x,t.stereo.head.position.y+xr.y,t.stereo.head.position.z+xr.z},
            v::normalized(v::multiply(t.stereo.head.orientation,xr_q))};
        t.continuity.input=t.controllers;hand.hand.left_position_metres=t.controllers.hands[0].grip.position;
        tg::grab::Contact contact{key,wrist,placement.observed_ns};tg::grab::Profile profile{};
        profile.id=key.profile;profile.kind=cover.edge.kind;profile.geometry_signature=model->signature;
        profile.acquire_metres=.1;profile.retain_metres=.16;profile.maximum_wrist_turn=1;
        for(unsigned n=0;n<15;++n){const auto& q=model->reference[12+n].q;profile.approach[n]={q[0],q[1],q[2],q[3]};
            profile.held[n]=v::normalized(v::multiply(profile.approach[n],{0,0,-std::sin(.05),std::cos(.05)}));}
        interaction.phase=v::CoverPhase::grabbing;interaction.owns_left_hand=true;interaction.edge_id=key.edge;interaction.request_id=9;
        ownership={v::CoverLeftHandOwner::cover,9};
        std::array<std::uint64_t,2> available{{UINT64_MAX,(std::uint64_t(1)<<(body::bones-64))-1}};
        gears_cover_grab_transition::State state{};auto candidate=baseline;
        const auto compose=[&](){return tg::compose(input,placement,eye,key,cover,hand,interaction,ownership,
            contact,profile,*model,*frame,available,tg::grab::Mode::held,now,true,state,*work,candidate);};
        auto result=compose();C(result.input==tg::Status::prepared);C(result.pose.status==gears_cover_grab_pose_adapter::Status::prepared);
        C(candidate.report.max_length_error<=.002);C(state.known);
        for(unsigned fault=0;fault<3;++fault){candidate=baseline;const auto before=bytes(candidate);
            if(fault==0)++placement.link.name_number;
            if(fault==1)available={};
            if(fault==2)++contact.key.shape_generation;
            result=compose();C(result.input!=tg::Status::prepared||result.pose.status!=gears_cover_grab_pose_adapter::Status::prepared);
            C(bytes(candidate)==before);C(!state.known);
            if(fault==0)--placement.link.name_number;
            if(fault==1)available={{UINT64_MAX,(std::uint64_t(1)<<(body::bones-64))-1}};
        }
    }
    std::printf("CoverGrabTickAdapterTest checks=%u failures=%u\n",checks,failures);return failures?1:0;
}
