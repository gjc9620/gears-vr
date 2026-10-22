#include "hand_animation_ownership.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace gears_hand_animation;
namespace {
using Phase=OwnershipPhase;
using Reason=OwnershipReason;
using EdgePhase=OwnershipBoundaryPhase;
constexpr std::uintptr_t picked_weapon=0x9000;

OwnershipSnapshot baseline(){
    OwnershipSnapshot s{};
    s.epoch=1;s.tick=1;s.world=100;s.controller=200;s.pawn=300;s.mesh=400;
    s.tree=500;s.weapon=600;s.known=511;s.player_controlled=true;s.health=450;
    s.control_lists_known=true;s.slots=19;
    for(unsigned i=0;i<s.controls.size();++i){
        auto& c=s.controls[i];c.object=0x1000+i;c.component=s.mesh;c.outer=s.tree;
        c.next=i%4==3?0:0x1001+i;c.cls=700;c.registry_index=10+i;c.known=255;
        c.list_mask=i<4?4u:1u;c.name={800+i,0};c.object_name={900+i,0};
    }
    for(unsigned i=0;i<s.slots;++i){
        auto& slot=s.slot[i];slot.presence_known=true;
        if(i==1||i==7){slot.custom_index=slot.target_index=-1;continue;}
        slot.object=0x2000+i;slot.component=s.mesh;
        slot.registry_index=30+i;slot.known=2047;slot.name={100+i,0};
        slot.total=1;slot.children=3;slot.pending=i?.25f:.2f;
        for(unsigned j=0;j<slot.children;++j){
            auto& child=slot.child[j];child.object=0x3000+i*16+j;
            child.known=j?31u:3u;child.weight=child.total=child.target=j?0.f:1.f;
        }
    }
    return s;
}
void animate(OwnershipSnapshot& s,unsigned index,OwnershipAnimation animation){
    auto& slot=s.slot[index];auto& child=slot.child[1];
    slot.custom_index=slot.target_index=1;slot.playing=true;
    child.weight=child.total=child.target=1;child.playing=true;
    child.kind=animation;child.animation={200+unsigned(animation),0};
}
void drain(OwnershipSnapshot& s){
    for(unsigned i=0;i<s.slots;++i){
        auto& slot=s.slot[i];slot.target_index=0;slot.playing=false;slot.blend=0;
        for(unsigned j=1;j<slot.children;++j){
            auto& c=slot.child[j];c.weight=c.total=c.target=0;c.playing=false;
        }
    }
}
struct Trial {
    OwnershipGate gate;
    OwnershipSnapshot s{baseline()};
    std::uint64_t now{1000},call{};
    OwnershipDecision out{};
    Trial(){assert(push().phase==Phase::idle_unproved);}
    OwnershipDecision push(OwnershipBoundary edge={}){
        ++s.serial;s.observed_ms=now;out=gate.observe(s,now,edge);
        assert(!out.ownership_proved());return out;
    }
    void next(unsigned ms=20,bool tick=true){now+=ms;if(tick)++s.tick;}
    OwnershipBoundary edge(Kind kind){
        OwnershipBoundary e{};e.phase=EdgePhase::pre;e.kind=kind;e.call=++call;
        e.exact_owner=e.normal=e.active_state=true;
        e.subject=kind==Kind::use_request?s.controller:
            kind==Kind::on_anim_end||kind==Kind::melee_stop_request?s.pawn:s.weapon;
        if(kind==Kind::use_request){e.source=Source::bridge_invoke;e.pickup_weapon=picked_weapon;}
        if(kind==Kind::on_anim_end)e.sequence_node=s.slot[0].child[1].object;
        return e;
    }
    void pickup(bool companions=false){
        auto e=edge(Kind::use_request);assert(push(e).phase!=Phase::rejected);
        e.phase=EdgePhase::post;s.special=53;s.switching=true;
        animate(s,0,OwnershipAnimation::kickup);
        if(companions)for(const auto index:{3u,10u,14u}){
            animate(s,index,OwnershipAnimation::pickup_holster);
            auto& c=s.slot[index].child[1];c.animation={300+index,0};
            s.slot[index].pending=-1;
            c.weight=c.total=0; // Even an invisible slot's target/playback owns it.
        }
        assert(push(e).phase==Phase::original_animation);
        next();s.weapon=picked_weapon;s.switching=false;
        assert(push().phase==Phase::original_animation);
    }
    void melee(OwnershipAnimation animation=OwnershipAnimation::smack_a){
        s.wants_melee=true;auto e=edge(Kind::melee_state_begin);
        assert(push(e).phase!=Phase::rejected);
        e.phase=EdgePhase::post;s.melee=true;
        animate(s,2,animation);animate(s,5,animation);
        assert(push(e).phase==Phase::original_animation);
    }
    void end(bool pickup,bool drained=true){
        auto e=edge(pickup?Kind::on_anim_end:Kind::melee_state_end);
        assert(push(e).phase==Phase::original_animation);
        e.phase=EdgePhase::post;s.special=0;s.melee=false;s.wants_melee=false;
        if(drained)drain(s);
        assert(push(e).phase==(drained?Phase::waiting_new_tick:Phase::waiting_slot_exit));
    }
    void ready(){next();assert(push().normal_path_ready);}
    void rejected(Reason reason){
        assert(out.phase==Phase::rejected&&out.reason==reason&&!out.normal_path_ready);
        next();assert(push().reason==reason); // Faults never heal from a later idle poll.
    }
};

void normal_paths(){
    Trial pickup;pickup.pickup();pickup.end(true);pickup.ready();
    assert(pickup.s.slot[0].total==1&&pickup.s.slot[0].pending==.2f);
    assert(pickup.s.slot[0].custom_index==1&&pickup.out.episode==1);
    for(unsigned i=0;i<70;++i){pickup.next(80);assert(pickup.push().normal_path_ready);}
    pickup.next();pickup.melee();pickup.end(false);pickup.ready();
    pickup.next();pickup.melee(OwnershipAnimation::smack_c);pickup.end(false);pickup.ready();
    assert(pickup.out.episode==3); // A/C require distinct matched Begin/End cycles.

    Trial fading;fading.melee();fading.end(false,false);
    fading.s.slot[2].child[1].target=0;fading.s.slot[2].blend=.2f;
    fading.next();assert(fading.push().phase==Phase::waiting_slot_exit);
    drain(fading.s);fading.next();assert(fading.push().phase==Phase::waiting_new_tick);
    fading.ready();

    Trial stop;stop.melee();auto e=stop.edge(Kind::melee_stop_request);
    assert(stop.push(e).phase==Phase::original_animation);
    e.phase=EdgePhase::post;stop.s.wants_melee=false;
    assert(stop.push(e).phase==Phase::original_animation); // Stop request cannot release.
    stop.end(false);stop.ready();
}
void release_guards(){
    // Every independent remaining weight, target, playback, blend, intent or
    // original tail strength blocks release even with a valid logical end.
    for(unsigned guard=0;guard<10;++guard){
        Trial t;t.pickup();t.end(true);
        auto& slot=t.s.slot[0];auto& c=slot.child[1];
        if(guard==0)c.weight=.01f;
        if(guard==1)c.total=.01f;
        if(guard==2)c.target=.01f;
        if(guard==3)c.playing=true;
        if(guard==4)slot.blend=.01f;
        if(guard==5)slot.playing=true;
        if(guard==6)slot.target_index=1;
        if(guard==7)t.s.controls[2].target=.01f;
        if(guard==8)t.s.controls[7].blend=.01f;
        if(guard==9)t.s.wants_melee=true;
        t.next();assert(t.push().phase==Phase::waiting_slot_exit&&!t.out.normal_path_ready);
        drain(t.s);t.s.controls[2].target=t.s.controls[7].blend=0;t.s.wants_melee=false;
        t.next();assert(t.push().phase==Phase::waiting_new_tick);t.ready();
    }
    Trial fresh;fresh.pickup();fresh.end(true);
    fresh.next(20,false);assert(fresh.push().phase==Phase::waiting_new_tick);
    fresh.next(0);assert(fresh.push().normal_path_ready);
    assert(fresh.gate.decision(fresh.now+ownership_fresh_ms).normal_path_ready);
    fresh.out=fresh.gate.decision(fresh.now+ownership_fresh_ms+1);fresh.rejected(Reason::stale);
    fresh.gate.reset();fresh.s=baseline();fresh.call=0;
    assert(fresh.push().phase==Phase::idle_unproved&&!fresh.out.normal_path_ready);
    // A new unpaired fixed action cannot borrow an earlier cycle's EndState.
    {Trial t;t.pickup();t.end(true);t.ready();t.s.switching=true;
        t.next();t.push();t.rejected(Reason::missing_start);}
}
void missing_or_wrong_boundaries(){
    {Trial t;t.s.special=53;t.push();t.rejected(Reason::missing_start);}
    {Trial t;t.pickup();drain(t.s);t.s.special=0;t.push();t.rejected(Reason::missing_end);}
    {Trial t;t.melee();drain(t.s);t.s.melee=t.s.wants_melee=false;t.push();t.rejected(Reason::missing_end);}
    {Trial t;auto e=t.edge(Kind::use_request);t.push(e);t.next();t.push();t.rejected(Reason::missing_boundary);}
    {Trial t;auto e=t.edge(Kind::use_request);e.phase=EdgePhase::post;t.push(e);t.rejected(Reason::missing_boundary);}
    {Trial t;auto e=t.edge(Kind::use_request);t.push(e);e.phase=EdgePhase::post;
        t.push(e);t.rejected(Reason::missing_start);} // ServerUse returning idle is not a pickup.
    {Trial t;t.pickup();auto e=t.edge(Kind::on_anim_end);++e.sequence_node;
        t.push(e);t.rejected(Reason::boundary_mismatch);}
    {Trial t;t.pickup();auto e=t.edge(Kind::on_anim_end);t.push(e);e.phase=EdgePhase::post;
        ++e.call;t.push(e);t.rejected(Reason::missing_boundary);}
    {Trial t;t.melee();auto e=t.edge(Kind::melee_state_end);e.source=Source::bridge_invoke;
        t.push(e);t.rejected(Reason::boundary_mismatch);}
    {Trial t;t.melee();auto e=t.edge(Kind::melee_state_end);e.active_state=false;
        t.push(e);t.rejected(Reason::boundary_mismatch);}
    {Trial t;t.melee();auto e=t.edge(Kind::melee_state_end);e.exact_owner=false;
        t.push(e);t.rejected(Reason::boundary_mismatch);}
    {Trial t;t.pickup();auto e=t.edge(Kind::on_anim_end);t.push(e);e.phase=EdgePhase::post;
        t.push(e);t.rejected(Reason::missing_end);} // Matching event name without transition.
}
void unsupported_or_changed(){
    {OwnershipGate gate;auto s=baseline();s.serial=1;s.observed_ms=1000;s.special=53;
        auto out=gate.observe(s,1000);assert(out.reason==Reason::started_active&&!out.normal_path_ready);}
    {Trial t;t.pickup();t.s.special=42;t.push();t.rejected(Reason::unsupported_action);}
    {Trial t;t.melee();t.s.special_attack=true;t.push();t.rejected(Reason::unsupported_action);}
    {Trial t;t.melee();t.s.switching=true;t.push();t.rejected(Reason::unsupported_action);}
    {Trial t;t.melee();auto e=t.edge(Kind::melee_stop_request);e.normal=false;
        t.push(e);t.rejected(Reason::cancelled);}
    {Trial t;t.pickup();auto e=t.edge(Kind::special_stop);t.push(e);t.rejected(Reason::unsupported_action);}
    {Trial t;t.pickup();--t.s.health;t.push();t.rejected(Reason::interrupted);}
    {Trial t;t.melee();t.s.weapon=picked_weapon;t.push();t.rejected(Reason::identity_changed);}
    {Trial t;t.pickup();t.s.weapon=600;t.push();t.rejected(Reason::identity_changed);} // Cannot switch back.
    for(unsigned part=0;part<6;++part){
        Trial t;t.pickup();
        if(part==0)++t.s.world;
        if(part==1)++t.s.controller;
        if(part==2)++t.s.pawn;
        if(part==3){++t.s.mesh;for(auto& c:t.s.controls)c.component=t.s.mesh;
            for(auto& slot:t.s.slot)slot.component=t.s.mesh;}
        if(part==4){++t.s.tree;for(auto& c:t.s.controls)c.outer=t.s.tree;}
        if(part==5)++t.s.epoch;
        t.push();t.rejected(Reason::identity_changed);
    }
    {Trial t;t.pickup();++t.s.controls[0].registry_index;t.push();t.rejected(Reason::controls_changed);}
    {Trial t;t.pickup();++t.s.slot[0].registry_index;t.push();t.rejected(Reason::slot_reused);}
    {Trial t;t.pickup();++t.s.slot[0].child[1].animation.number;t.push();t.rejected(Reason::slot_reused);}
    {Trial t;t.pickup();t.s.slot[0].child[1].object+=100;t.push();t.rejected(Reason::slot_reused);}
    {Trial t;t.pickup();animate(t.s,2,OwnershipAnimation::unsupported);t.push();t.rejected(Reason::unsupported_action);}
    {Trial t;t.melee();animate(t.s,2,OwnershipAnimation::smack_c);t.push();t.rejected(Reason::slot_reused);}
    // Retail PID 57480: array elements 1/7 are null; pickup additionally targets
    // holsters in 3/10/14. At snapshot 26 those same sequence objects become
    // Equip_Lt without matched Play/Stop events. A quiet Kickup cannot repair it.
    {Trial t;t.pickup(true);for(const auto index:{3u,10u,14u}){
            auto& c=t.s.slot[index].child[1];c.animation.index+=50;
            c.kind=OwnershipAnimation::unsupported;
        }
        t.next();t.push();t.rejected(Reason::slot_reused);
        drain(t.s);t.s.special=0;t.next();assert(!t.push().normal_path_ready);}
    {Trial t;t.pickup();t.s.slot[1]=t.s.slot[2];t.s.slot[1].object+=100;
        t.s.slot[1].registry_index+=100;t.s.slot[1].name.index+=100;
        for(auto& c:t.s.slot[1].child)c.object+=1000;
        t.push();t.rejected(Reason::slot_reused);} // A certified hole cannot silently fill.
}
void bad_observations(){
    for(unsigned bad=0;bad<10;++bad){
        Trial t;t.pickup();
        if(bad==0)t.s.known&=~4u;
        if(bad==1)t.s.control_lists_known=false;
        if(bad==2)t.s.controls[7].known&=~32u;
        if(bad==3)t.s.slot[0].child[1].known&=~2u;
        if(bad==4)t.s.slot[0].child[1].weight=std::numeric_limits<float>::quiet_NaN();
        if(bad==5)t.s.slot[0].pending=-.5f;
        if(bad==6)t.s.slots=ownership_slots+1;
        if(bad==7)t.s.slot[0].children=ownership_children+1;
        if(bad==8)t.s.tick=0;
        if(bad==9)t.s.slot[1].presence_known=false;
        t.push();t.rejected(Reason::incomplete_snapshot);
    }
    {Trial t;t.pickup();++t.s.serial;t.push();t.rejected(Reason::sequence_gap);}
    {Trial t;t.pickup();t.next(101);t.push();t.rejected(Reason::sequence_gap);}
    {Trial t;t.pickup();--t.now;t.push();t.rejected(Reason::time_regression);}
    {Trial t;t.pickup();--t.s.tick;t.push();t.rejected(Reason::time_regression);}
    {Trial t;t.pickup();t.s.serial++;t.s.observed_ms=t.now+1;
        t.out=t.gate.observe(t.s,t.now);t.rejected(Reason::stale);}
    {Trial t;t.pickup();for(unsigned i=0;i<63;++i){t.next(80);t.push();}
        t.rejected(Reason::timeout);} // Continuous good polls do not extend an episode.
}
} // namespace
int main(){
    static_assert(!OwnershipDecision::ownership_proved(),"A normal path never authorizes a writer");
    normal_paths();release_guards();missing_or_wrong_boundaries();
    unsupported_or_changed();bad_observations();
    std::puts("HandAnimationOwnershipTest passed: normal pickup/melee, full slot exit, fresh tick, missing/cancelled/changed/timed-out evidence; ownership_proved=0 writes=0");
}
