// Owned typed source fixture, copied from the sender test; no runtime authority.
#include "body_main_mount_sender.h"
#define main body_main_mount_sender_owned_fixture
#define now fixture_now
#include "weapon_rig_binding_test.cpp"
#undef now
#undef main
namespace {
namespace sender=gears_body_main_mount_sender;
namespace bodywire=gears_body_main_mount_provider;
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
    !out.query_id&&!out.hand_lifecycle&&!out.action_generation&&!out.equipment_begin_ns&&!out.equipment_end_ns&&
    bodywire::same(out.source,bodywire::SourceKey{})&&out.mount.size==sizeof(out.mount)&&
    out.mount.abi_version==oldwire::version&&!out.mount.query_id&&!out.mount.key.frame&&!out.mount.calibration_known&&
    !out.mount.weapon.address&&!out.mount.component.address&&!out.mount.hold_generation;}
}
