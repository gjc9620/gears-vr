#include "weapon_main_mount_sender.h"
#define main weapon_main_mount_sender_binding_fixture
#include "weapon_rig_binding_test.cpp"
#undef main
namespace sender=gears_weapon_main_mount_sender;
namespace wire=gears_weapon_main_mount_provider;
namespace policy=gears_weapon_main_mount;
namespace source=gears_weapon_main_mount_source;
static_assert(sizeof(wire::Request)==1968&&offsetof(wire::Request,body)==1856&&sizeof(wire::Response)==504);
namespace {
wire::Request request(Fixture const& f){
    wire::Request r{};r.query_id=1;r.hand_lifecycle=f.hand.key.lifecycle;r.retained=f.rig;
    r.body.mesh=f.rig.mesh;r.body.geometry=gears_hand_authored_mount::marcus115_signature;
    r.body.model_scale=1;r.body.units_per_metre=100;r.body.revision=f.rig.binding_generation;
    r.body.basis_revision=wire::supported_basis_revision;r.body.basis=wire::Basis::marcus_mesh_face_forward_y;return r;
}
sender::Parent parent(Fixture const& f){auto const& n=f.rig.native;return {n.registration_cookie,n.native_started_qpc,n.game_pid,n.game_thread};}
policy::Candidate candidate(Fixture const& f,wire::Request const& r){
    source::BodyContext context{};CHECK(sender::context(r,parent(f),now,context)==wire::Status::ready);
    policy::BodyBinding body{};er::Status agreement{};
    CHECK(source::bind(f.hand,f.rig,context,now,f.source,body,agreement)==source::SourceStatus::ready);
    policy::AssetBinding asset{};asset.certified=asset.right_attached=true;asset.key=f.hand.key;asset.weapon=f.hand.weapon;
    asset.component=object(0x7000,8);asset.asset=object(0x7100,9);
    wh::GripGameState game{};game.known=game.main_hand_held=true;game.kind=wh::WeaponKind::firearm;
    policy::Candidate out{};CHECK(policy::prepare(f.hand,game,9,asset,body,now,out)==policy::Status::ready);return out;
}
bool empty(wire::Response const& r){return r.size==sizeof(r)&&r.abi_version==wire::version&&!r.query_id&&
    wh::same(r.key,wh::FrameKey{})&&!r.world.address&&!r.controller.address&&!r.pawn.address&&!r.mesh.address&&
    !r.weapon.address&&!r.component.address&&!r.asset.address&&!r.hold_generation&&!r.provider_revision&&!r.body_revision&&
    !r.basis_revision&&r.basis==wire::Basis::unknown&&!r.reserved&&r.geometry==wire::Signature{}&&
    r.model_scale==0&&r.units_per_metre==0&&!r.calibration_known&&!r.calibration_reserved&&
    r.grip_to_wrist_metres.x==0&&r.grip_to_wrist_metres.y==0&&r.grip_to_wrist_metres.z==0&&
    r.grip_to_wrist_rotation.x==0&&r.grip_to_wrist_rotation.y==0&&r.grip_to_wrist_rotation.z==0&&r.grip_to_wrist_rotation.w==0;}
}
int main(){
    for(unsigned selected=0;selected<2;++selected){Fixture f(selected);auto r=request(f);const auto c=candidate(f,r);
        wire::Response out{};CHECK(sender::response(r,parent(f),c,now,out)==wire::Status::ready);
        CHECK(out.query_id==r.query_id&&wh::same(out.key,f.hand.key)&&out.calibration_known==1&&out.hold_generation==9);
        CHECK(out.grip_to_wrist_metres.x==c.right.grip_to_wrist_metres.x&&out.grip_to_wrist_rotation.w==c.right.grip_to_wrist_rotation.w);
        const auto original=r;CHECK(sender::response(r,parent(f),c,now+50000000,out)!=wire::Status::ready&&empty(out));
        CHECK(r.query_id==original.query_id&&r.retained.scene_invocation==original.retained.scene_invocation);}
    for(unsigned changed=0;changed<15;++changed){Fixture f;auto r=request(f);const auto c=candidate(f,r);auto p=parent(f);
        wire::Response out{};CHECK(sender::response(r,p,c,now,out)==wire::Status::ready);
        switch(changed){case 0:++r.size;break;case 1:++r.abi_version;break;case 2:r.query_id=0;break;
        case 3:r.hand_lifecycle=0;break;case 4:++r.maximum_age_ns;break;case 5:++p.cookie;break;
        case 6:++p.native_started_qpc;break;case 7:++p.game_pid;break;case 8:++p.game_thread;break;
        case 9:++r.body.reserved;break;case 10:++r.body.revision;break;case 11:++r.body.mesh.name_number;break;
        case 12:++r.body.basis_revision;break;case 13:r.body.model_scale=2;break;case 14:r.body.geometry[0]^=1;break;}
        CHECK(sender::response(r,p,c,now,out)!=wire::Status::ready&&empty(out));}
    for(unsigned changed=0;changed<12;++changed){Fixture f;auto r=request(f);auto c=candidate(f,r);wire::Response out{};
        CHECK(sender::response(r,parent(f),c,now,out)==wire::Status::ready);
        switch(changed){case 0:++c.key.lifecycle;break;case 1:++c.key.frame;break;case 2:++c.key.registration_cookie;break;
        case 3:++c.weapon.name_number;break;case 4:c.component={};break;case 5:c.asset={};break;case 6:c.hold_generation=0;break;
        case 7:++c.provider_revision;break;case 8:++c.body_revision;break;case 9:c.right.known=false;break;
        case 10:c.right.grip_to_wrist_metres.x+=.01;break;case 11:c.right.grip_to_wrist_rotation.w+=.01;break;}
        CHECK(sender::response(r,parent(f),c,now,out)!=wire::Status::ready&&empty(out));}
    CHECK(sender::translate(policy::Status::unheld)==wire::Status::unheld);
    CHECK(sender::translate(policy::Status::busy)==wire::Status::busy);
    CHECK(sender::translate(policy::Status::unsupported_geometry)==wire::Status::unsupported_geometry);
    std::printf("WeaponMainMountSenderTest: %u checks passed; pure POD mapping with owned fixture only\n",checks);return 0;
}
