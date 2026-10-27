#include "weapon_main_mount_candidate.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace {
namespace mm=gears_weapon_main_mount;namespace wh=gears_weapon_hand;
unsigned checks{};
void need(bool good){++checks;if(!good){std::fprintf(stderr,"candidate check failed %u\n",checks);std::abort();}}
struct Fixture {
    wh::Frame frame{};wh::GripGameState game{};mm::AssetBinding asset{};mm::BodyBinding body{};
    std::int64_t stamp{1001000000};std::uint64_t generation{9};
    static wh::ObjectIdentity object(std::uint64_t address){return {address,0x8000,0x9000,unsigned(address/8),7,0,1};}
    Fixture(){
        auto& f=frame;f.key={11,22,33,1,1,1,44,55};f.now_ns=1000000000;
        f.flags=wh::scene_owned|wh::input_matched|wh::gameplay_interactable;
        f.world=object(0x1000);f.controller=object(0x2000);f.pawn=object(0x3000);
        f.mesh=object(0x4000);f.weapon=object(0x5000);f.reference_generation=1;
        f.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
        auto& s=f.stereo;s.session=1;s.sequence=100;s.received_ns=990000000;s.predicted_time=123;
        s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;
        s.head={{0,1.6,0},{0,0,0,1}};
        for(unsigned i=0;i<2;++i){s.eyes[i].pose.position={i?.032:-.032,1.6,0};s.eyes[i].pose.orientation={0,0,0,1};s.eyes[i].fov={-.8,.8,.8,-.8};}
        auto& c=f.controllers;c.session=1;c.sequence=100;c.space_epoch=s.space_epoch;c.predicted_time=123;
        c.received_ns=s.received_ns;c.sync_ns=c.received_ns-1;c.focused=1;
        for(auto& h:c.hands){h.grip_active=1;h.grip_flags=15;}
        game.known=game.main_hand_held=true;game.kind=wh::WeaponKind::firearm;
        asset.certified=asset.right_attached=true;asset.key=f.key;asset.weapon=f.weapon;
        asset.component=object(0x6000);asset.asset=object(0x7000);
        body.known=true;body.key=f.key;body.mesh=f.mesh;body.revision=7;body.basis_revision=2;body.basis=mm::Basis::marcus_mesh_face_forward_y;
        body.geometry=gears_hand_authored_mount::marcus115_signature;body.model_scale=1;body.units_per_metre=100;
        need(wh::frame_active(f,stamp));
    }
    mm::Status prepare(mm::Candidate& out){return mm::prepare(frame,game,generation,asset,body,stamp,out);}
};
template<class Change>void rejects(Change change,mm::Status expected){
    Fixture f;mm::Candidate out{};need(f.prepare(out)==mm::Status::ready&&out.right.known);
    change(f);need(f.prepare(out)==expected);
    need(!out.right.known&&!out.weapon.address&&!out.key.frame&&!out.provider_revision&&!out.hold_generation);
}
}
int main(){
    {Fixture f;mm::Candidate out{};need(f.prepare(out)==mm::Status::ready);
        need(out.provider_revision==1&&out.body_revision==7&&out.hold_generation==9&&wh::same(out.weapon,f.frame.weapon));
        need(wh::same(out.key,f.frame.key)&&wh::same(out.component,f.asset.component)&&wh::same(out.asset,f.asset.asset));
        need(!f.game.mount_known); // Does not invent a support/muzzle/fire mount.
        need(out.right.grip_to_wrist_rotation.w==1&&out.right.grip_to_wrist_rotation.x==0);}
    rejects([](auto& f){f.game.main_hand_held=false;},mm::Status::unheld);
    rejects([](auto& f){f.generation=0;},mm::Status::unheld);
    rejects([](auto& f){f.game.animation_owns_hands=true;},mm::Status::animation);
    rejects([](auto& f){f.game.busy=true;},mm::Status::busy);
    rejects([](auto& f){f.game.known=false;},mm::Status::inactive);
    rejects([](auto& f){f.game.kind=wh::WeaponKind::hand_thrown;},mm::Status::not_firearm);
    rejects([](auto& f){f.frame.weapon={};},mm::Status::no_weapon);
    rejects([](auto& f){f.asset.certified=false;},mm::Status::unsupported_asset);
    rejects([](auto& f){f.asset.right_attached=false;},mm::Status::not_attached);
    rejects([](auto& f){f.asset.component={};},mm::Status::unsupported_asset);
    rejects([](auto& f){f.asset.asset={};},mm::Status::unsupported_asset);
    rejects([](auto& f){++f.frame.key.frame;},mm::Status::unsupported_asset);
    rejects([](auto& f){f.frame.weapon=Fixture::object(0xa000);},mm::Status::unsupported_asset);
    rejects([](auto& f){f.body.known=false;},mm::Status::body_unavailable);
    rejects([](auto& f){f.body.revision=0;},mm::Status::body_unavailable);
    rejects([](auto& f){f.body.basis_revision=0;},mm::Status::body_unavailable);
    rejects([](auto& f){f.body.basis_revision=1;},mm::Status::body_mismatch);
    rejects([](auto& f){f.body.basis=mm::Basis::unknown;},mm::Status::body_mismatch);
    rejects([](auto& f){++f.body.key.world_generation;},mm::Status::body_mismatch);
    rejects([](auto& f){f.body.mesh=Fixture::object(0xb000);},mm::Status::body_mismatch);
    rejects([](auto& f){f.body.geometry[0]^=1;},mm::Status::unsupported_geometry);
    rejects([](auto& f){f.body.model_scale=0;},mm::Status::unsupported_geometry);
    rejects([](auto& f){f.body.units_per_metre=std::numeric_limits<double>::quiet_NaN();},mm::Status::unsupported_geometry);
    rejects([](auto& f){f.frame.recentered=1;},mm::Status::inactive);
    rejects([](auto& f){f.frame.heading_deferred=1;},mm::Status::inactive);
    rejects([](auto& f){f.frame.controllers.hands[1].grip_flags=0;},mm::Status::inactive);
    rejects([](auto& f){f.frame.controllers.hands[1].grip_active=0;},mm::Status::inactive);
    rejects([](auto& f){f.stamp+=100000000;},mm::Status::inactive);
    std::printf("Main mount candidate: %u checks passed; owned inputs, no Native contract or writer\n",checks);
}
