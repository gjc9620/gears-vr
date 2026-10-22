#include "weapon_boomshot_interaction.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

using namespace gears_weapon;
using E=BoomshotEvent;
using S=BoomshotStage;
using O=BoomshotOutcome;
static_assert(!std::is_copy_constructible<BoomshotInteraction>::value&&
              !std::is_copy_assignable<BoomshotInteraction>::value,"Transactions must have one owner");
namespace {
unsigned checks{};
void check(bool condition,char const* expression,int line) {
    ++checks;
    if(!condition){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(expression) check(bool(expression),#expression,__LINE__)

// Synthetic certificates, quantities, sequence, geometry IDs and clocks only.
// These fixtures do not describe the retail gun's mechanics or ammunition.
constexpr BoomshotIdentity identity{11,12,13,14};
BoomshotPlan plan(BoomshotCondition condition=BoomshotCondition::partial) {
    BoomshotPlan p{};p.identity=identity;p.config_id=31;p.plan_id=32;p.condition=condition;
    p.current_instance_verified=p.original_order_verified=p.quantity_verified=true;p.count=3;
    p.steps[0]={BoomshotStepKind::move_mechanism,BoomshotPart::barrel,41,51,0,10,false};
    p.steps[1]={BoomshotStepKind::insert_ammunition,BoomshotPart::feed_port,42,52,2,10,false};
    p.steps[2]={BoomshotStepKind::move_mechanism,BoomshotPart::barrel,43,53,0,10,true};
    return p;
}
BoomshotInstanceConfig config(bool shot_cycle=false) {
    BoomshotInstanceConfig c{};c.identity=identity;c.config_id=31;c.ammunition_type=61;
    c.ammunition_kind=BoomshotAmmoKind::projectile;c.capacity=7;c.original_ammo_cost=2;
    c.original_projectile_count=3;c.maximum_sample_gap_ns=1000;
    c.current_instance_verified=c.original_class_verified=c.original_fire_semantics_verified=true;
    c.after_shot=plan(BoomshotCondition::after_shot);c.after_shot.plan_id=33;
    if(shot_cycle){c.after_shot.count=1;c.after_shot.steps[0].return_required=true;}
    else c.after_shot.count=0;
    return c;
}
struct Fixture {
    BoomshotInteraction model{};
    BoomshotInstanceConfig certificate{config()};
    BoomshotPlan reload_plan{plan()};
    std::uint64_t sequence{},event_sequence{},item{100},operation_serial{1};
    std::int64_t tick{100},ns{100};
    Fixture(std::uint32_t loaded=1,std::uint32_t reserve=6,bool shot_cycle=false) {
        certificate=config(shot_cycle);reload_plan=plan(loaded?BoomshotCondition::partial:BoomshotCondition::empty);
        CHECK(model.bind(certificate,{identity,1,loaded,reserve}));
        CHECK(model.observe(frame())==E::motion_reset);
    }
    BoomshotFrame frame(std::int64_t at=-1,std::int64_t delta_ns=20) {
        ++sequence;tick=at<0?tick+1:at;ns+=delta_ns;
        return {{identity,model.binding_token(),sequence,1,tick,ns},model.inventory(),
            true,true,true,true,true,false};
    }
    BoomshotReloadStart start(bool known=true,std::int64_t deadline=30) const {
        return {model.binding_token(),1,1,reload_plan,{identity,31,1,deadline,known},true,true};
    }
    BoomshotMotionReceipt motion(unsigned index,bool cycle=false) const {
        auto const& p=cycle?certificate.after_shot:reload_plan;
        auto const& s=p.steps[index];
        return {model.binding_token(),operation_serial,p.plan_id,s.phase_id,s.geometry_id,1,
            sequence,ns,true,true,true,true,true};
    }
    BoomshotAmmoReceipt ammo() const {
        return {model.binding_token(),1,reload_plan.plan_id,
            reload_plan.steps[model.step_index()].phase_id,item,61,
            reload_plan.steps[model.step_index()].units,BoomshotAmmoKind::projectile,
            BoomshotAmmoSource::left_shoulder,true};
    }
    BoomshotOriginalReceipt native(std::uint64_t token,BoomshotInventory observed,
                                   bool accepted=true,std::int64_t at=-1) {
        auto f=frame(at);
        return {f.stamp,token,++event_sequence,observed,accepted,true};
    }
    BoomshotInsertionReceipt insertion() const {
        return {motion(model.step_index()),item,true,true,true};
    }
    void begin(bool known=true,std::int64_t deadline=30) {
        CHECK(model.begin_r3(frame(),start(known,deadline))==E::started);
    }
    void open() {
        const auto r=motion(0);CHECK(model.complete_mechanism(frame(),r)==E::step_completed);
    }
    void grab() {CHECK(model.grab_left_shoulder(frame(),ammo())==E::grabbed);}
    void propose_insert(std::int64_t at=-1) {
        const auto r=insertion();CHECK(model.insert_ammunition(frame(at),r)==E::request_inventory);
    }
    BoomshotOriginalReceipt inventory_receipt(bool accepted=true,std::int64_t at=-1) {
        auto const request=model.inventory_request();
        auto observed=model.inventory();
        if(accepted){++observed.revision;observed.loaded=request.after_loaded;observed.reserve=request.after_reserve;}
        return native(request.token,observed,accepted,at);
    }
    void seat() {propose_insert();CHECK(model.acknowledge_inventory(inventory_receipt())==E::inventory_committed);}
    void seated(bool known=true,std::int64_t deadline=30) {begin(known,deadline);open();grab();seat();}
};

void certification_and_entry() {
    BoomshotInteraction unknown;
    CHECK(!unknown.bind({},{}));CHECK(unknown.stage()==S::unbound&&!unknown.can_request_fire());
    for(unsigned invalid=0;invalid<17;++invalid){
        BoomshotInteraction m;auto c=config();BoomshotInventory observed{identity,1,1,6};
        switch(invalid){
            case 0:c.identity.weapon=0;break;
            case 1:c.config_id=0;break;
            case 2:c.ammunition_type=0;break;
            case 3:c.ammunition_kind=BoomshotAmmoKind::unknown;break;
            case 4:c.capacity=0;break;
            case 5:c.original_ammo_cost=0;break;
            case 6:c.original_ammo_cost=8;break;
            case 7:c.original_projectile_count=0;break;
            case 8:c.maximum_sample_gap_ns=0;break;
            case 9:c.current_instance_verified=false;break;
            case 10:c.original_class_verified=false;break;
            case 11:c.original_fire_semantics_verified=false;break;
            case 12:c.after_shot.current_instance_verified=false;break;
            case 13:c.after_shot=plan();break;
            case 14:observed.revision=0;break;
            case 15:observed.loaded=8;break;
            case 16:observed.identity.generation++;break;
        }
        CHECK(!m.bind(c,observed));CHECK(m.stage()==S::unbound);
    }
    for(unsigned invalid=0;invalid<18;++invalid){
        Fixture f;auto s=f.start();
        switch(invalid){
            case 0:s.binding++;break;
            case 1:s.r3_press_edge=0;break;
            case 2:s.original_reload_serial=0;break;
            case 3:s.original_reload_accepted=false;break;
            case 4:s.automatic_completion_suppressed=false;break;
            case 5:s.plan.current_instance_verified=false;break;
            case 6:s.plan.original_order_verified=false;break;
            case 7:s.plan.quantity_verified=false;break;
            case 8:s.plan.identity.weapon++;break;
            case 9:s.plan.config_id++;break;
            case 10:s.plan.condition=BoomshotCondition::empty;break;
            case 11:s.plan.steps[1].units=0;break;
            case 12:s.plan.steps[2].phase_id=s.plan.steps[0].phase_id;break;
            case 13:s.plan.steps[0].geometry_id=0;break;
            case 14:s.perfect.original_reload_serial++;break;
            case 15:s.perfect.identity.generation++;break;
            case 16:s.perfect.end_ticks=-1;break;
            case 17:s.plan.count=13;break;
        }
        const auto event=f.model.begin_r3(f.frame(),s);
        CHECK(event==E::rejected||event==E::invalid);CHECK(f.model.stage()==S::ready);
    }
    Fixture full(7),empty(0),dry(0,0),shortage(1,1);
    CHECK(full.model.begin_r3(full.frame(),full.start())==E::full);
    CHECK(dry.model.begin_r3(dry.frame(),dry.start())==E::no_ammo);
    CHECK(shortage.model.begin_r3(shortage.frame(),shortage.start())==E::no_ammo);
    auto no_shot=empty.frame();no_shot.original_can_fire=false;
    CHECK(empty.model.begin_r3(no_shot,empty.start())==E::started);
    Fixture repeat;repeat.begin();
    CHECK(repeat.model.begin_r3(repeat.frame(),repeat.start())==E::rejected);
    CHECK(repeat.model.step_index()==0);
}

void ordered_guidance_and_conservation() {
    Fixture f;
    CHECK(f.model.guide().part==BoomshotPart::none);f.begin();
    auto yellow=f.model.guide();
    CHECK(yellow.part==BoomshotPart::barrel&&yellow.red==1&&yellow.green==1&&yellow.blue==0&&yellow.alpha==1);
    CHECK(f.model.grab_left_shoulder(f.frame(),f.ammo())==E::rejected);
    auto future=f.motion(2);CHECK(f.model.complete_mechanism(f.frame(),future)==E::rejected);
    f.open();CHECK(f.model.guide().part==BoomshotPart::left_shoulder);f.grab();
    CHECK(f.model.guide().part==BoomshotPart::feed_port);
    CHECK(f.model.inventory().loaded==1&&f.model.inventory().reserve==6);
    CHECK(f.model.held_units()==2&&f.model.available_reserve()==4&&!f.model.can_request_fire());
    CHECK(f.model.release_ammunition(f.frame(),f.item)==E::released);
    CHECK(f.model.inventory().reserve==6&&f.model.available_reserve()==6);
    CHECK(f.model.grab_left_shoulder(f.frame(),f.ammo())==E::rejected);
    ++f.item;f.grab();f.propose_insert();
    const auto proposed=f.model.inventory_request();
    CHECK(proposed.token&&proposed.item_serial==f.item&&proposed.before.loaded==1&&
          proposed.before.reserve==6&&proposed.after_loaded==3&&proposed.after_reserve==4);
    CHECK(f.model.inventory_request().token==proposed.token&&f.model.inventory().loaded==1);
    CHECK(f.model.release_ammunition(f.frame(),f.item)==E::rejected);
    auto ack=f.inventory_receipt();
    CHECK(f.model.acknowledge_inventory(ack)==E::inventory_committed);
    CHECK(f.model.inventory().loaded==3&&f.model.inventory().reserve==4&&f.model.held_units()==0);
    CHECK(f.model.guide().part==BoomshotPart::barrel);
    CHECK(f.model.acknowledge_inventory(ack)==E::rejected);
    auto close=f.motion(2);CHECK(f.model.complete_mechanism(f.frame(130),close)==E::request_completion);
    const auto result=f.model.completion_request();
    CHECK(result.outcome==O::perfect&&result.physical_finish_tick==130&&result.started_tick==102);
    CHECK(!f.model.can_request_fire()&&f.model.guide().part==BoomshotPart::none);
    auto completion=f.native(result.token,f.model.inventory(),true,1000);
    CHECK(f.model.acknowledge_completion(completion)==E::reload_completed);
    CHECK(f.model.acknowledge_completion(completion)==E::rejected);
    CHECK(f.model.inventory().loaded+f.model.inventory().reserve==7);
    CHECK(f.model.observe(f.frame())==E::motion_reset);
    CHECK(f.model.observe(f.frame())==E::none&&f.model.can_request_fire());
}

void timing_and_independent_plans() {
    for(auto elapsed:{0LL,30LL,31LL,5000LL}){
        Fixture f;
        // Paused game clock is legal; sequence and monotonic clock still advance.
        const auto begin=f.frame(101);CHECK(f.model.begin_r3(begin,f.start())==E::started);
        auto r=f.motion(0);CHECK(f.model.complete_mechanism(f.frame(101),r)==E::step_completed);
        CHECK(f.model.grab_left_shoulder(f.frame(101),f.ammo())==E::grabbed);
        auto insert=f.insertion();CHECK(f.model.insert_ammunition(f.frame(101),insert)==E::request_inventory);
        CHECK(f.model.acknowledge_inventory(f.inventory_receipt(true,101))==E::inventory_committed);
        r=f.motion(2);CHECK(f.model.complete_mechanism(f.frame(101+elapsed),r)==E::request_completion);
        CHECK(f.model.completion_request().outcome==(elapsed<=30?O::perfect:O::ordinary));
    }
    Fixture unknown;unknown.seated(false,0);
    auto r=unknown.motion(2);CHECK(unknown.model.complete_mechanism(unknown.frame(),r)==E::request_completion);
    CHECK(unknown.model.completion_request().outcome==O::ordinary);
    Fixture insert_only;
    insert_only.reload_plan.steps[0]=insert_only.reload_plan.steps[1];insert_only.reload_plan.count=1;
    CHECK(insert_only.model.begin_r3(insert_only.frame(102),insert_only.start())==E::started);
    CHECK(insert_only.model.guide().part==BoomshotPart::left_shoulder);insert_only.grab();
    insert_only.propose_insert(120);
    CHECK(insert_only.model.acknowledge_inventory(insert_only.inventory_receipt(true,10000))==E::request_completion);
    CHECK(insert_only.model.completion_request().physical_finish_tick==120&&
          insert_only.model.completion_request().outcome==O::perfect);
    Fixture quantities;
    quantities.reload_plan.count=3;
    quantities.reload_plan.steps[0]={BoomshotStepKind::insert_ammunition,BoomshotPart::feed_port,70,71,1,0,false};
    quantities.reload_plan.steps[1]={BoomshotStepKind::move_mechanism,BoomshotPart::magazine,72,73,0,0,false};
    quantities.reload_plan.steps[2]={BoomshotStepKind::insert_ammunition,BoomshotPart::feed_port,74,75,3,0,false};
    quantities.begin();quantities.grab();quantities.seat();
    auto middle=quantities.motion(1);
    CHECK(quantities.model.complete_mechanism(quantities.frame(),middle)==E::step_completed);
    ++quantities.item;quantities.grab();quantities.propose_insert();
    CHECK(quantities.model.acknowledge_inventory(quantities.inventory_receipt())==E::request_completion);
    CHECK(quantities.model.inventory().loaded==5&&quantities.model.inventory().reserve==2);
    Fixture overflow;
    overflow.reload_plan.steps[1].units=7;
    CHECK(overflow.model.begin_r3(overflow.frame(),overflow.start())==E::no_ammo);
    Fixture capacity(1,20);capacity.reload_plan.steps[1].units=7;
    CHECK(capacity.model.begin_r3(capacity.frame(),capacity.start())==E::invalid);
}

void mechanical_and_ammo_proofs() {
    for(unsigned invalid=0;invalid<17;++invalid){
        Fixture f;f.begin();auto r=f.motion(0);auto next=f.frame();
        switch(invalid){
            case 0:r.binding++;break;
            case 1:r.operation_serial++;break;
            case 2:r.plan_id++;break;
            case 3:r.phase_id++;break;
            case 4:r.geometry_id++;break;
            case 5:r.motion_epoch++;break;
            case 6:r.begin_sequence=1;break;
            case 7:r.begin_sequence=next.stamp.sequence;break;
            case 8:r.begin_ns=0;break;
            case 9:r.begin_ns=next.stamp.monotonic_ns;break;
            case 10:r.begin_ns=next.stamp.monotonic_ns-1;break;
            case 11:r.player_driven=false;break;
            case 12:r.continuous=false;break;
            case 13:r.endpoints_and_direction_verified=false;break;
            case 14:r.left_grip_held=false;break;
            case 15:next.left_tracked=false;break;
            case 16:next.right_tracked=false;break;
        }
        const auto e=f.model.complete_mechanism(next,r);
        CHECK(e==E::rejected||e==E::motion_reset);CHECK(f.model.step_index()==0);
    }
    Fixture no_return;no_return.seated();auto r=no_return.motion(2);r.return_verified=false;
    CHECK(no_return.model.complete_mechanism(no_return.frame(),r)==E::rejected);
    for(unsigned invalid=0;invalid<9;++invalid){
        Fixture f;f.begin();f.open();auto a=f.ammo();
        switch(invalid){
            case 0:a.binding++;break;
            case 1:a.original_reload_serial++;break;
            case 2:a.plan_id++;break;
            case 3:a.phase_id++;break;
            case 4:a.item_serial=0;break;
            case 5:a.ammunition_type++;break;
            case 6:a.units++;break;
            case 7:a.source=BoomshotAmmoSource::other;break;
            case 8:a.kind=BoomshotAmmoKind::verified_carrier;break;
        }
        CHECK(f.model.grab_left_shoulder(f.frame(),a)==E::rejected);
        CHECK(!f.model.held_units()&&f.model.available_reserve()==6);
    }
    for(unsigned invalid=0;invalid<4;++invalid){
        Fixture f;f.begin();f.open();f.grab();auto i=f.insertion();
        switch(invalid){
            case 0:i.item_serial++;break;
            case 1:i.approached_from_outside=false;break;
            case 2:i.aligned=false;break;
            case 3:i.seated=false;break;
        }
        CHECK(f.model.insert_ammunition(f.frame(),i)==E::rejected);
        CHECK(f.model.inventory().loaded==1&&!f.model.inventory_request().token);
    }
}

void tracking_clock_and_animation() {
    for(unsigned discontinuity=0;discontinuity<6;++discontinuity){
        Fixture f;f.begin();auto old=f.motion(0);auto bad=f.frame();
        switch(discontinuity){
            case 0:bad.left_tracked=false;break;
            case 1:bad.stamp.motion_epoch=2;break;
            case 2:bad.stamp.monotonic_ns+=2000;f.ns=bad.stamp.monotonic_ns;break;
            case 3:--bad.stamp.sequence;break;
            case 4:bad.stamp.game_tick=1;break;
            case 5:f.model.tracking_lost();break;
        }
        const auto e=f.model.complete_mechanism(bad,old);
        CHECK(e==E::motion_reset||e==E::rejected);
        auto recovery=f.frame();if(discontinuity==1)recovery.stamp.motion_epoch=2;
        const auto recovered=f.model.observe(recovery);
        CHECK(recovered==E::motion_reset||recovered==E::none);
        auto next=f.frame();if(discontinuity==1)next.stamp.motion_epoch=2;
        CHECK(f.model.complete_mechanism(next,old)==E::rejected);
        CHECK(f.model.step_index()==0&&f.model.inventory().loaded==1);
    }
    Fixture lost;lost.begin();lost.open();lost.grab();
    auto held=lost.model.held_units();lost.model.tracking_lost();
    CHECK(lost.model.observe(lost.frame(1000))==E::motion_reset);
    CHECK(lost.model.held_units()==held);lost.propose_insert();
    CHECK(lost.model.acknowledge_inventory(lost.inventory_receipt())==E::inventory_committed);
    auto close=lost.motion(2);CHECK(lost.model.complete_mechanism(lost.frame(),close)==E::request_completion);
    CHECK(lost.model.completion_request().outcome==O::ordinary); // Loss did not pause t0.
    Fixture animation(4);auto entry=animation.frame();entry.original_owns_hands=true;
    entry.original_interaction_allowed=false;entry.left_tracked=entry.right_tracked=false;
    CHECK(animation.model.observe(entry)==E::animation_handoff);
    CHECK(animation.model.animation_owns_hands()&&!animation.model.can_request_fire());
    auto still=animation.frame();still.left_tracked=false;
    CHECK(animation.model.observe(still)==E::motion_reset&&animation.model.animation_owns_hands());
    auto restore=animation.frame();restore.original_can_fire=false;
    CHECK(animation.model.observe(restore)==E::restored&&!animation.model.animation_owns_hands());
    CHECK(!animation.model.can_request_fire());
}

void inventory_receipts_and_rebinding() {
    for(unsigned invalid=0;invalid<5;++invalid){
        Fixture f;f.begin();f.open();f.grab();f.propose_insert();auto ack=f.inventory_receipt();
        switch(invalid){
            case 0:--ack.observed.revision;break;
            case 1:++ack.observed.loaded;break;
            case 2:++ack.observed.reserve;break;
            case 3:ack.observed.identity.weapon++;break;
            case 4:ack.original_effect_applied_once=false;break;
        }
        CHECK(f.model.acknowledge_inventory(ack)==E::interrupted&&f.model.needs_reconciliation());
        CHECK(!f.model.can_request_fire()&&f.model.inventory().loaded==1);
    }
    Fixture retry;retry.begin();retry.open();retry.grab();retry.propose_insert();
    auto old_request=retry.model.inventory_request();
    CHECK(retry.model.acknowledge_inventory(retry.inventory_receipt(false))==E::rejected);
    CHECK(retry.model.stage()==S::holding_ammunition&&retry.model.held_units()==2&&retry.model.inventory().loaded==1);
    retry.propose_insert();CHECK(retry.model.inventory_request().token!=old_request.token);
    auto ack=retry.inventory_receipt();auto stale=ack;stale.request_token=old_request.token;
    CHECK(retry.model.acknowledge_inventory(stale)==E::rejected);
    CHECK(retry.model.acknowledge_inventory(ack)==E::inventory_committed);
    Fixture replacement;replacement.begin();replacement.open();replacement.grab();replacement.propose_insert();
    auto old_ack=replacement.inventory_receipt();auto old_start=replacement.start();
    auto observed=replacement.model.inventory();const auto binding=replacement.model.binding_token();
    replacement.model.reset();
    CHECK(replacement.model.bind(replacement.certificate,observed));
    CHECK(replacement.model.binding_token()!=binding);
    CHECK(replacement.model.observe(replacement.frame())==E::motion_reset);
    CHECK(replacement.model.begin_r3(replacement.frame(),old_start)==E::rejected);
    CHECK(replacement.model.acknowledge_inventory(old_ack)==E::rejected);
    auto current=replacement.start();current.r3_press_edge=2;
    CHECK(replacement.model.begin_r3(replacement.frame(),current)==E::started);replacement.open();
    CHECK(replacement.model.grab_left_shoulder(replacement.frame(),replacement.ammo())==E::rejected);
    ++replacement.item;replacement.grab();replacement.propose_insert();
    auto old_with_new_stamp=old_ack;old_with_new_stamp.stamp=replacement.frame().stamp;
    CHECK(replacement.model.acknowledge_inventory(old_with_new_stamp)==E::rejected);
    CHECK(replacement.model.acknowledge_inventory(replacement.inventory_receipt())==E::inventory_committed);
    Fixture external;external.seated();const auto committed=external.model.inventory();
    auto drift=external.frame();drift.observed.loaded++;
    CHECK(external.model.observe(drift)==E::interrupted);
    CHECK(external.model.inventory().loaded==committed.loaded&&external.model.inventory().reserve==committed.reserve);
    Fixture stow;stow.begin();stow.open();stow.grab();
    CHECK(stow.model.interrupt()==E::interrupted&&stow.model.inventory().reserve==6&&stow.model.held_units()==0);
    CHECK(stow.model.guide().part==BoomshotPart::none&&!stow.model.can_request_fire());
    Fixture pending;pending.begin();pending.open();pending.grab();pending.propose_insert();
    auto pending_ack=pending.inventory_receipt();
    CHECK(pending.model.interrupt()==E::interrupted&&!pending.model.inventory_request().token);
    CHECK(pending.model.acknowledge_inventory(pending_ack)==E::rejected);
}

BoomshotShotReceipt successful_shot(Fixture& f) {
    auto observed=f.model.inventory();++observed.revision;observed.loaded-=f.certificate.original_ammo_cost;
    return {f.native(f.model.fire_request().token,observed),f.certificate.original_projectile_count,true};
}
void original_fire_and_cycles() {
    Fixture f(5);
    CHECK(f.model.request_original_fire(f.frame(),1)==E::request_fire);
    const auto fire=f.model.fire_request();
    CHECK(fire.token&&fire.before.loaded==5&&f.model.inventory().loaded==5&&!f.model.can_request_fire());
    CHECK(f.model.request_original_fire(f.frame(),1)==E::rejected);
    auto shot=successful_shot(f);CHECK(f.model.after_original_shot(shot)==E::shot_recorded);
    CHECK(f.model.inventory().loaded==3&&f.model.inventory().reserve==6&&f.model.stage()==S::ready);
    CHECK(f.model.after_original_shot(shot)==E::rejected);
    CHECK(f.model.observe(f.frame())==E::motion_reset);
    CHECK(f.model.request_original_fire(f.frame(),1)==E::rejected);
    CHECK(f.model.request_original_fire(f.frame(),2)==E::request_fire);
    auto second=successful_shot(f);second.original.original_event_sequence=shot.original.original_event_sequence;
    CHECK(f.model.after_original_shot(second)==E::rejected);
    second.original.original_event_sequence=++f.event_sequence;
    CHECK(f.model.after_original_shot(second)==E::shot_recorded);
    CHECK(f.model.observe(f.frame())==E::motion_reset);
    CHECK(f.model.request_original_fire(f.frame(),3)==E::no_ammo);
    for(unsigned invalid=0;invalid<5;++invalid){
        Fixture bad(4);CHECK(bad.model.request_original_fire(bad.frame(),1)==E::request_fire);
        auto receipt=successful_shot(bad);
        switch(invalid){
            case 0:receipt.original.original_effect_applied_once=false;break;
            case 1:receipt.original_recoil_applied=false;break;
            case 2:receipt.original_projectiles_spawned++;break;
            case 3:receipt.original.observed.loaded++;break;
            case 4:receipt.original.observed.reserve--;break;
        }
        CHECK(bad.model.after_original_shot(receipt)==E::interrupted&&!bad.model.can_request_fire());
    }
    Fixture denied(4);CHECK(denied.model.request_original_fire(denied.frame(),1)==E::request_fire);
    BoomshotShotReceipt no_shot{denied.native(denied.model.fire_request().token,denied.model.inventory(),false),0,false};
    CHECK(denied.model.after_original_shot(no_shot)==E::rejected&&denied.model.inventory().loaded==4);
    Fixture cycle(5,6,true);
    CHECK(cycle.model.request_original_fire(cycle.frame(),1)==E::request_fire);
    auto fired=successful_shot(cycle);cycle.operation_serial=fired.original.original_event_sequence;
    CHECK(cycle.model.after_original_shot(fired)==E::shot_recorded&&cycle.model.stage()==S::mechanism);
    CHECK(cycle.model.guide().part==BoomshotPart::barrel);
    CHECK(cycle.model.request_original_fire(cycle.frame(),2)==E::rejected);
    CHECK(cycle.model.observe(cycle.frame())==E::motion_reset);
    auto stroke=cycle.motion(0,true);
    CHECK(cycle.model.complete_mechanism(cycle.frame(),stroke)==E::request_completion);
    const auto result=cycle.model.completion_request();
    CHECK(result.operation==BoomshotOperation::shot_cycle&&result.outcome==O::none);
    CHECK(cycle.model.acknowledge_completion(cycle.native(result.token,cycle.model.inventory()))==E::cycle_completed);
    CHECK(cycle.model.inventory().loaded==3&&cycle.model.inventory().reserve==6);
    Fixture unavailable(5);auto frame=unavailable.frame();frame.original_can_fire=false;
    CHECK(unavailable.model.request_original_fire(frame,1)==E::rejected);
    CHECK(!unavailable.model.fire_request().token&&unavailable.model.inventory().loaded==5);
}
}
int main() {
    certification_and_entry();ordered_guidance_and_conservation();timing_and_independent_plans();
    mechanical_and_ammo_proofs();tracking_clock_and_animation();inventory_receipts_and_rebinding();
    original_fire_and_cycles();
    std::printf("Boomshot interaction: %u checks passed (synthetic logic only).\n",checks);
}
