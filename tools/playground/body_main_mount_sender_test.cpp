#include "body_main_mount_sender.h"
#define main body_main_mount_sender_owned_fixture
#define now fixture_now
#include "weapon_rig_binding_test.cpp"
#undef now
#undef main
namespace {
namespace sender=gears_body_main_mount_sender;
namespace bodywire=gears_body_main_mount_provider;
static_assert(bodywire::version==5,"This fixture checks the explicit visual hold context ABI");
namespace oldwire=gears_weapon_main_mount_provider;
struct BodyFixture {
    Fixture owned;bodywire::Request request{};sender::Parent parent{};
    wh::EquipmentIdentity identity{};wh::EquipmentObservation observation{};
    explicit BodyFixture(unsigned selected=0):owned(selected){
        request.query_id=901;request.hand_lifecycle=owned.hand.key.lifecycle;request.action_generation=7;
        auto& s=request.source;s.original_scene=owned.rig;auto& o=s.original_scene;
        o.qpc_frequency=1000000000;o.scene_qpc=fixture_now-20000000;o.published_ns=fixture_now-10000000;
        o.stereo.received_ns=fixture_now-70000000;o.controllers.received_ns=fixture_now-71000000;o.controllers.sync_ns=fixture_now-72000000;
        s.scene_ticket=41;s.scene_cache_revision=51;auto& t=s.tracking;
        s.metadata_begin_ns=fixture_now-9000000;s.metadata_end_ns=fixture_now-8000000;
        t.tick=61;t.thread=o.native.game_thread;t.parent_cookie=o.native.registration_cookie;
        t.begin_ns=fixture_now-7000000;t.end_ns=fixture_now-3000000;t.stereo=o.stereo;t.stereo.sequence+=5;
        t.stereo.received_ns=fixture_now-4000000;t.controllers=o.controllers;t.controllers.sequence=t.stereo.sequence;
        t.controllers.received_ns=fixture_now-5000000;t.controllers.sync_ns=fixture_now-6000000;
        t.continuity.input=t.controllers;t.continuity.stream_epoch=o.producer.stream;
        t.continuity.tracking_epoch[0]=o.producer.tracking[0];t.continuity.tracking_epoch[1]=o.producer.tracking[1];
        t.native_reference=o.native_reference;t.reference_revision=302; // Native private revision, NOT reference_generation=111.
        t.publisher_pid=o.controller_publisher_pid;t.publisher_start=o.controller_publisher_start;
        auto& b=s.basis;b.compose=71;b.capture_begin_ns=fixture_now-2000000;b.component_to_world=o.base_view;b.base_view=o.base_view;
        b.geometry.mesh=o.mesh;b.geometry.geometry=gears_hand_authored_mount::marcus115_signature;
        b.geometry.model_scale=1;b.geometry.units_per_metre=o.units_per_metre;b.geometry.revision=o.binding_generation;
        b.geometry.basis_revision=2;b.geometry.basis=oldwire::Basis::marcus_mesh_face_forward_y;
        parent={o.native.registration_cookie,o.native.native_started_qpc,o.native.game_pid,o.native.game_thread};
        CHECK(sender::context(request,parent,fixture_now,identity)==bodywire::Status::ready);
        observation.begin_ns=fixture_now-8000000;observation.end_ns=fixture_now-1000000;
        observation.game.known=observation.game.main_hand_held=true;observation.game.kind=wh::WeaponKind::firearm;
        observation.hold_generation=9;observation.component=object(0x7000,8);observation.asset=object(0x7100,9);
    }
    bodywire::Status respond(bodywire::Response& out){return sender::response(request,parent,identity,observation,fixture_now,out);}
};
bool empty(bodywire::Response const& out){return out.size==sizeof(out)&&out.abi_version==bodywire::version&&
    out.hold_kind==bodywire::HoldKind::unknown&&!out.reserved&&
    !out.query_id&&!out.hand_lifecycle&&!out.action_generation&&!out.equipment_begin_ns&&!out.equipment_end_ns&&
    bodywire::same(out.source,bodywire::SourceKey{})&&out.mount.size==sizeof(out.mount)&&
    out.mount.abi_version==oldwire::version&&!out.mount.query_id&&!out.mount.key.frame&&!out.mount.calibration_known&&
    !out.mount.weapon.address&&!out.mount.component.address&&!out.mount.hold_generation;}
template<class Change>void reject(Change change){BodyFixture f;bodywire::Response out{};
    CHECK(f.respond(out)==bodywire::Status::ready&&out.mount.calibration_known);change(f);
    CHECK(f.respond(out)!=bodywire::Status::ready&&empty(out));}
}
int main(){
    {BodyFixture f;bodywire::Response out{};CHECK(f.respond(out)==bodywire::Status::ready);
        CHECK(out.hold_kind==bodywire::HoldKind::firearm&&!out.reserved);
        f.observation.game.kind=wh::WeaponKind::hand_thrown;
        CHECK(f.respond(out)==bodywire::Status::ready&&out.hold_kind==bodywire::HoldKind::frag);
        f.observation.game.kind=wh::WeaponKind::unknown;
        CHECK(f.respond(out)==bodywire::Status::not_firearm&&empty(out));}
    for(unsigned selected=0;selected<2;++selected){BodyFixture f(selected);bodywire::Response out{};CHECK(f.respond(out)==bodywire::Status::ready);
        CHECK(out.query_id==901&&out.mount.query_id==901&&out.hand_lifecycle==f.request.hand_lifecycle&&out.action_generation==7);
        CHECK(bodywire::same(out.source,bodywire::source_key(f.request.source))&&wh::same(out.mount.key,f.identity.key));
        CHECK(out.mount.key.frame==f.request.source.original_scene.scene_invocation&&out.mount.calibration_known==1);
        CHECK(out.equipment_begin_ns==f.observation.begin_ns&&out.equipment_end_ns==f.observation.end_ns);
        CHECK(f.request.source.tracking.reference_revision!=f.request.source.original_scene.reference_generation);
        CHECK(!rt::frame_valid(f.request.source.original_scene,fixture_now,50000000,selected==1));
        oldwire::Request legacy{};legacy.query_id=901;legacy.hand_lifecycle=f.request.hand_lifecycle;
        legacy.retained=f.request.source.original_scene;legacy.body=f.request.source.basis.geometry;
        gears_weapon_main_mount_source::BodyContext context{};
        CHECK(gears_weapon_main_mount_sender::context(legacy,f.parent,fixture_now,context)==oldwire::Status::source_mismatch);
    }
    for(std::int64_t age:{40000000LL,40000001LL}){BodyFixture f;auto& t=f.request.source.tracking;
        t.begin_ns=t.end_ns=t.stereo.received_ns=t.controllers.received_ns=t.controllers.sync_ns=fixture_now-age;
        auto& s=f.request.source;s.metadata_begin_ns=t.begin_ns-1000000;s.metadata_end_ns=t.begin_ns;
        s.original_scene.scene_qpc=s.metadata_begin_ns-2000000;s.original_scene.published_ns=s.metadata_begin_ns-1000000;
        t.continuity.input=t.controllers;f.request.source.basis.capture_begin_ns=t.end_ns;
        bodywire::Response out{};CHECK((f.respond(out)==bodywire::Status::ready)==(age==40000000));if(age>40000000)CHECK(empty(out));}
    for(std::int64_t age:{50000000LL,50000001LL,250000000LL}){BodyFixture f;auto& o=f.request.source.original_scene;
        o.scene_qpc=f.request.source.metadata_begin_ns-age;o.published_ns=std::uint64_t(o.scene_qpc);bodywire::Response out{};
        CHECK(f.respond(out)==bodywire::Status::ready);CHECK(out.abi_version==5);}
    {BodyFixture f;auto& s=f.request.source;s.original_scene.scene_qpc=fixture_now-55000000;
        s.original_scene.published_ns=fixture_now-54000000;bodywire::Response out{};
        CHECK(f.respond(out)==bodywire::Status::ready); // actual GT metadata is 9 ms old, Scene was 46 ms at its BEGIN.
        CHECK(out.source.metadata_begin_ns==s.metadata_begin_ns&&out.source.metadata_end_ns==s.metadata_end_ns);
        CHECK(out.abi_version==5);for(unsigned version:{1u,2u,3u,4u}){auto legacy=f.request;legacy.abi_version=version;wh::EquipmentIdentity identity{};
            CHECK(sender::context(legacy,f.parent,fixture_now,identity)==bodywire::Status::source_mismatch);}}
    for(std::int64_t age:{50000000LL,50000001LL}){BodyFixture f;auto& s=f.request.source;
        s.metadata_begin_ns=fixture_now-age;s.metadata_end_ns=s.metadata_begin_ns;
        s.original_scene.scene_qpc=s.metadata_begin_ns-1000000;s.original_scene.published_ns=s.metadata_begin_ns;
        bodywire::Response out{};CHECK((f.respond(out)==bodywire::Status::ready)==(age==50000000));if(age>50000000)CHECK(empty(out));}
    for(unsigned field=0;field<5;++field)reject([field](auto& f){auto& s=f.request.source;
        switch(field){case 0:s.metadata_begin_ns=0;break;case 1:s.metadata_end_ns=s.metadata_begin_ns-1;break;
        case 2:s.metadata_end_ns=s.tracking.begin_ns+1;break;case 3:s.metadata_begin_ns=s.metadata_end_ns=fixture_now+1;break;
        case 4:s.original_scene.published_ns=std::uint64_t(s.metadata_begin_ns+1);break;}});
    for(std::int64_t age:{50000000LL,50000001LL}){BodyFixture f;f.observation.begin_ns=fixture_now-age;bodywire::Response out{};
        CHECK((f.respond(out)==bodywire::Status::ready)==(age==50000000));if(age>50000000)CHECK(empty(out));}
    for(unsigned field=0;field<7;++field)reject([field](auto& f){auto& s=f.request.source;
        switch(field){case 0:s.tracking.begin_ns=fixture_now+1;break;case 1:s.tracking.end_ns=s.tracking.begin_ns-1;break;
        case 2:s.tracking.controllers.sync_ns=fixture_now+1;s.tracking.continuity.input=s.tracking.controllers;break;
        case 3:s.basis.capture_begin_ns=fixture_now+1;break;case 4:s.basis.capture_begin_ns=s.tracking.end_ns-1;break;
        case 5:f.observation.end_ns=f.observation.begin_ns-1;break;case 6:f.observation.end_ns=fixture_now+1;break;}});
    for(unsigned field=0;field<13;++field)reject([field](auto& f){auto& t=f.request.source.tracking;
        switch(field){case 0:t.stereo.focused=0;break;case 1:t.controllers.hands[1].grip_active=0;break;
        case 2:++t.stereo.space_epoch;break;case 3:++t.stereo.session;break;case 4:++t.controllers.input_epoch;break;
        case 5:++t.controllers.profile_generation;break;case 6:++t.continuity.stream_epoch;break;
        case 7:--t.continuity.tracking_epoch[1];break;case 8:++t.publisher_start;break;case 9:++t.native_reference.epoch;break;
        case 10:t.reference_revision=0;break;case 11:t.reference_revision=303;break;
        case 12:t.stereo.source=t.controllers.source=gears_vr::StereoSource::MetaOperatorSimulated;break;}});
    for(unsigned side=0;side<2;++side){BodyFixture f;auto& t=f.request.source.tracking;
        ++t.continuity.tracking_epoch[side];bodywire::Response out{};
        CHECK(f.respond(out)==bodywire::Status::ready);
        // Right-hand equipment does not require a currently tracked left hand.
        t.controllers.hands[0].grip_active=0;t.controllers.hands[0].grip_flags=0;t.continuity.input=t.controllers;
        CHECK(f.respond(out)==bodywire::Status::ready);
        t.stereo.sequence=t.controllers.sequence=f.request.source.original_scene.controllers.sequence;
        t.continuity.input=t.controllers;CHECK(f.respond(out)==bodywire::Status::source_mismatch&&empty(out));}
    {BodyFixture f;auto& o=f.request.source.original_scene;
        o.flags&=~gears_rig_tracking::producer_continuity_available;o.producer={};
        bodywire::Response out{};CHECK(f.respond(out)==bodywire::Status::ready);
        for(unsigned field=0;field<8;++field){BodyFixture bad;auto& s=bad.request.source;
            s.original_scene.flags&=~gears_rig_tracking::producer_continuity_available;s.original_scene.producer={};
            switch(field){case 0:s.tracking.continuity.stream_epoch=0;break;case 1:s.tracking.continuity.tracking_epoch[1]=0;break;
            case 2:++s.tracking.publisher_start;break;case 3:++s.tracking.continuity.input.sequence;break;
            case 4:++s.tracking.controllers.input_epoch;s.tracking.continuity.input=s.tracking.controllers;break;
            case 5:++s.tracking.native_reference.epoch;break;case 6:s.original_scene.producer.stream=1;break;
            case 7:s.original_scene.flags&=~gears_rig_tracking::controllers_matched;break;}
            CHECK(bad.respond(out)==bodywire::Status::source_mismatch&&empty(out));
        }
    }
    {BodyFixture f;auto clear=[](auto& o){o.flags&=~(gears_rig_tracking::controllers_matched|gears_rig_tracking::producer_continuity_available);
            o.controllers={};o.producer={};o.controller_publisher_pid=0;o.controller_publisher_start=0;
            o.world_pose_mask=gears_rig_tracking::head_pose;o.world_grip[0]={};o.world_grip[1]={};};
        clear(f.request.source.original_scene);bodywire::Response out{};
        CHECK(f.respond(out)==bodywire::Status::ready);
        for(unsigned field=0;field<8;++field){BodyFixture bad;auto& s=bad.request.source;clear(s.original_scene);
            switch(field){case 0:s.original_scene.controllers.sequence=1;break;case 1:s.original_scene.controller_publisher_pid=1;break;
            case 2:s.original_scene.producer.stream=1;break;case 3:s.original_scene.world_pose_mask|=gears_rig_tracking::right_grip_pose;break;
            case 4:s.tracking.publisher_pid=0;break;case 5:s.tracking.continuity.stream_epoch=0;break;
            case 6:s.tracking.controllers.sync_ns=fixture_now-40000001;s.tracking.continuity.input=s.tracking.controllers;break;
            case 7:++s.tracking.native_reference.epoch;break;}
            CHECK(bad.respond(out)==bodywire::Status::source_mismatch&&empty(out));}
    }
    reject([](auto& f){++f.identity.key.lifecycle;});reject([](auto& f){++f.identity.mesh.name_number;});
    reject([](auto& f){f.observation.game.main_hand_held=false;});reject([](auto& f){f.observation.component={};});
    {BodyFixture f;f.request.query_id=902;bodywire::Response out{};CHECK(f.respond(out)==bodywire::Status::ready&&out.query_id==902&&out.mount.query_id==902);}
    std::printf("BodyMainMountSenderTest: %u checks passed; typed source/metadata bounds only, owned data\n",checks);return 0;
}
