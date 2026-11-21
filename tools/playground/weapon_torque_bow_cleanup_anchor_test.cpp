#include "weapon_hand_offline_fixture.h"
namespace {
namespace unified_weapon_fire {
// Boundary of the already tested common metadata-retention helper. The real
// Reader, typed fields, full actor identities and actual HandClient run below.
bool retention=true;
bool retained_metadata(meta::Reader&) noexcept {return retention;}
}
namespace nativeequipment {
bool quiet=true;
bool restoration_quiet() noexcept {return quiet;}
bool current(void*,abi::Frame const& f) noexcept {
    return weaponhand::client.source_current_on_gt(f.key)&&abi::frame_active(f,weaponhand::clock_ns(nullptr));
}
}
namespace torquebow_cleanup_context {
bool available=true;std::uint64_t world_generation=1;
bool current(abi::EquipmentIdentity const& p) noexcept {return available&&p.key.world_generation==world_generation;}
}
#include "weapon_torque_bow_cleanup_anchor.inl"
bool owned=true;
bool owns(abi::Frame const&,std::uint64_t serial) noexcept {return owned&&serial==9;}
void frame(Memory& m){
    auto& f=m.frame;f.flags=abi::scene_owned|abi::input_matched|abi::gameplay_interactable;
    f.key={11,22,33,1,1,1,GetCurrentProcessId(),GetCurrentThreadId()};
    f.now_ns=weaponhand::clock_ns(nullptr);f.reference_generation=1;f.stereo_status=unsigned(gears_vr::StereoStatus::accepted);
    auto& s=f.stereo;s.session=1;s.sequence=100;s.received_ns=f.now_ns;s.predicted_time=1;
    s.head_flags=s.view_flags=15;s.focused=s.visible=s.should_render=1;s.head.orientation={};
    for(unsigned i=0;i<2;++i){s.eyes[i].pose.orientation={};s.eyes[i].pose.position.x=i?.032:-.032;s.eyes[i].fov={-.8,.8,.8,-.8};}
    auto& c=f.controllers;c.session=1;c.sequence=100;c.space_epoch=s.space_epoch;c.predicted_time=1;
    c.focused=1;c.sync_ns=c.received_ns=f.now_ns;
    for(auto& h:c.hands){h.grip_active=1;h.grip_flags=15;}
    f.left_grip_known=f.left_tracked=1;
}
}
int main(){
    auto mem=std::make_unique<Memory>();auto& m=*mem;
    Memory::put(m.concrete,0x48,m.text(L"Weap_Bow_base"));Memory::put(m.pawn,0x100,m.world);m.tables();frame(m);
    auto& client=weaponhand::client;game_thread.store(0); // Loader registration is outside GT, as in production.
    assert(client.begin_connect({11,GetCurrentProcessId()})==abi::Reply::accepted);
    abi::RegistrationResult receipt{};receipt.cookie=33;receipt.native_started_qpc=22;
    assert(client.connected(abi::Reply::accepted,receipt)==abi::Reply::accepted);
    game_thread.store(GetCurrentThreadId());frame(m);
    auto registration=client.registration();abi::Proposal proposal{};
    assert(registration.prepare(registration.context,&m.frame,&proposal)==unsigned(abi::Reply::accepted));
    abi::Decision decision{};decision.key=m.frame.key;decision.proposal_nonce=proposal.nonce;
    decision.now_ns=weaponhand::clock_ns(nullptr);decision.status=abi::DecisionStatus::released;
    assert(registration.commit(registration.context,&m.frame,&proposal,&decision)==unsigned(abi::Reply::accepted));
    assert(nativeequipment::current(nullptr,m.frame));
    abi::ExistingActionLifetime pose_lifetime{};
    assert(client.capture_existing_action_lifetime_on_gt(m.frame.key,pose_lifetime));
    torquebow_cleanup::Anchor cleanup;torquebow_cleanup_context::available=false;
    assert(!cleanup.capture_before_press(m.frame,9,owns));torquebow_cleanup_context::available=true;
    assert(cleanup.capture_before_press(m.frame,9,owns));
    assert(cleanup.current(m.frame,9,owns));assert(!cleanup.current(m.frame,8,owns));
    // REAL soft invalidation: old action/pose lifetime fails and ordinary new
    // input stays unavailable. Only the already captured cleanup record lives.
    abi::Invalidation invalid{};invalid.registration_cookie=33;invalid.native_started_qpc=22;invalid.reason=abi::InvalidateReason::input_lost;
    registration.invalidate(registration.context,&invalid);
    assert(!client.existing_action_lifetime_on_gt(pose_lifetime));assert(!nativeequipment::current(nullptr,m.frame));
    assert(client.registration_alive(33,22)&&cleanup.current(m.frame,9,owns));
    ++torquebow_cleanup_context::world_generation;assert(!cleanup.current(m.frame,9,owns));
    --torquebow_cleanup_context::world_generation; // Same pointer tuple cannot conceal a different world.
    torquebow_cleanup::Anchor late;assert(!late.capture_before_press(m.frame,9,owns));
    for(unsigned link=0;link<6;++link){
        const uptr object=link<2?m.pc:link<5?m.pawn:m.weapon;
        const unsigned offset=link==0?0x110:link==1?0x100:link==2?0x110:link==3?0x100:link==4?0x120:0x110;
        const auto before=Memory::get<uptr>(object,offset);Memory::put(object,offset,uptr{});
        assert(!cleanup.current(m.frame,9,owns));Memory::put(object,offset,before);
    }
    owned=false;assert(!cleanup.current(m.frame,9,owns));owned=true;
    nativeequipment::quiet=false;assert(!cleanup.current(m.frame,9,owns));nativeequipment::quiet=true;
    unified_weapon_fire::retention=false;assert(!cleanup.current(m.frame,9,owns));unified_weapon_fire::retention=true;
    auto other=m.frame;++other.key.lifecycle;assert(!cleanup.current(other,9,owns));
    Memory::put(m.weapon,8,4000u);assert(!cleanup.current(m.frame,9,owns));Memory::put(m.weapon,8,m.frame.weapon.registry_index);
    assert(!cleanup.retire(9,false)&&cleanup.current(m.frame,9,owns));
    invalid.reason=abi::InvalidateReason::native_stopping;registration.invalidate(registration.context,&invalid);
    assert(!cleanup.current(m.frame,9,owns)); // Terminal/reconnect never survives.
    assert(cleanup.retire(9,true));assert(!cleanup.current(m.frame,9,owns));
    std::puts("Bow cleanup anchor: real HandClient soft revoke, no late capture/new input, exact serial/actor links, independent world-incarnation and terminal revoke passed; owned-memory, mock Native context witness/common-retention boundaries only.");
}
