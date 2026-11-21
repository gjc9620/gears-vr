#ifdef NDEBUG
#undef NDEBUG // Assertions are executable tests even in a release build.
#endif
#include "weapon_boltok_reload.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_weapon;
using E=BoltokReloadEvent;
using S=BoltokReloadStage;

namespace {
// Every count (including capacity SEVEN), state, part attribution, trajectory,
// deadline and sequence here is SYNTHETIC. Neither the three-step fixture nor
// its alternate orders describe the original Boltok mechanism or ammunition.
constexpr BoltokReloadIdentity identity{11,12,13,14,15,16,17,18,19};
BoltokReloadStamp stamp(std::uint64_t sequence,std::int64_t tick=1000) {
    return {identity,sequence,1000000000+static_cast<std::int64_t>(sequence)*10000000,
        tick,true,true,true};
}
BoltokInventory inventory(std::uint32_t loaded=3,std::uint32_t reserve=9,
                          std::uint32_t capacity=7,std::uint64_t revision=11) {
    return {identity,revision,77,capacity,loaded,reserve};
}
BoltokBinding binding(std::uint32_t loaded=3,std::uint32_t reserve=9,
                      std::uint32_t capacity=7,std::uint32_t cost=1) {
    return {stamp(1),inventory(loaded,reserve,capacity),cost,0,true,true,true};
}
void bind(BoltokReload& model,BoltokBinding const& b=binding()) {
    assert(model.bind(b,b.stamp.observed_ns));
}
BoltokReloadStart start(BoltokInventory const& inv=inventory(),bool known=true) {
    BoltokReloadStart s{};
    s.stamp=stamp(2);s.inventory=inv;s.r3_press_edge=1;s.accepted_begin_tick=1000;
    s.mechanical_ownership_verified=true;
    s.original_reload_accepted=s.automatic_refill_held=true;
    s.plan.identity=identity;s.plan.generation=31;s.plan.original_reload_serial=21;
    s.plan.initial_state=1001;s.plan.final_ready_state=1004;s.plan.count=3;
    s.plan.condition=inv.loaded?BoltokReloadCondition::partial:BoltokReloadCondition::empty;
    s.plan.current_instance_verified=s.plan.original_order_verified=true;
    s.plan.steps[0]={BoltokStepKind::move_original_parts,boltok_object_part(2),501,601,701,1001,1002,0};
    s.plan.steps[1]={BoltokStepKind::insert_ammunition,boltok_object_part(3),502,602,702,1002,1003,
        (std::min)(inv.capacity-inv.loaded,inv.reserve)};
    s.plan.steps[2]={BoltokStepKind::move_original_parts,boltok_object_part(1),503,603,703,1003,1004,0};
    s.perfect={identity,21,31,known?100:0,known};
    return s;
}
BoltokMotionReceipt motion(BoltokReloadStart const& s,unsigned step,std::uint64_t began) {
    auto const& p=s.plan.steps[step];
    return {s.plan.original_reload_serial,s.plan.generation,p.phase_id,p.contact_id,p.travel_id,
        p.before_state,p.after_state,began,p.parts,stamp(began).observed_ns,10000000,2,
        true,true,true,true,true,true};
}
BoltokAmmoReceipt ammo(BoltokReloadStart const& s,unsigned step=1,std::uint64_t item=81) {
    return {item,s.plan.original_reload_serial,s.plan.generation,s.plan.steps[step].phase_id,
        s.inventory.ammunition_type,s.plan.steps[step].rounds,boltok_ammo_slot,
        BoltokAmmoKind::live_boltok_ammunition,true,true};
}
E begin(BoltokReload& m,BoltokReloadStart const& s) {
    return m.begin_r3(s,s.stamp.observed_ns);
}
E move(BoltokReload& m,BoltokReloadStart const& s,unsigned step,std::uint64_t seq,
       std::uint64_t began,std::int64_t tick=1000) {
    auto frame=stamp(seq,tick);return m.move_original_parts(frame,motion(s,step,began),frame.observed_ns);
}
E grab(BoltokReload& m,BoltokReloadStart const& s,std::uint64_t seq=4,unsigned step=1,
       std::uint64_t item=81) {
    auto frame=stamp(seq);return m.grab_left_shoulder(frame,ammo(s,step,item),frame.observed_ns);
}
E insert(BoltokReload& m,BoltokReloadStart const& s,std::uint64_t seq=5,unsigned step=1,
         std::uint64_t began=4,std::uint64_t item=81,std::int64_t tick=1000) {
    auto frame=stamp(seq,tick);
    return m.insert_ammunition(frame,{item,motion(s,step,began)},frame.observed_ns);
}
BoltokOriginalReceipt inventory_receipt(BoltokReload const& m,std::uint64_t event=1) {
    auto const request=m.inventory_request();auto observed=m.inventory();
    observed.loaded=request.after_loaded;observed.reserve=request.after_reserve;
    observed.revision=request.before_revision+1;
    return {request.token,request.original_reload_serial,request.plan_generation,event,observed,
        true,true,false,0,E::rejected};
}
E acknowledge_inventory(BoltokReload& m,std::uint64_t seq=6,std::int64_t tick=1000,
                        std::uint64_t event=1) {
    auto frame=stamp(seq,tick);
    return m.acknowledge_inventory(frame,inventory_receipt(m,event),frame.observed_ns);
}
BoltokOriginalReceipt completion_receipt(BoltokReload const& m,std::uint64_t event=2) {
    auto const request=m.completion_request();
    return {request.token,request.original_reload_serial,request.plan_generation,event,m.inventory(),
        true,true,true,request.final_ready_state,request.proposed_result};
}
void holding(BoltokReload& m,BoltokReloadStart const& s=start()) {
    bind(m,{stamp(1),s.inventory,1,0,true,true,true});
    assert(begin(m,s)==E::started);
    assert(move(m,s,0,3,2)==E::step_completed);
    assert(grab(m,s)==E::grabbed);
}
void seated(BoltokReload& m,BoltokReloadStart const& s=start()) {
    holding(m,s);assert(insert(m,s)==E::inventory_requested);
    assert(acknowledge_inventory(m)==E::inventory_committed);
}
void complete_and_original_fire() {
    assert(boltok_body_slot==BodySlot::pistol&&original_inventory_slot(boltok_body_slot)==0);
    assert(boltok_ammo_slot==BodySlot::shoulder_ammo);
    assert(boltok_object_part(0)==0&&boltok_object_part(12)==0&&boltok_object_part(11)==1024);
    BoltokReload m;assert(m.stage()==S::unbound&&!m.can_fire());
    auto s=start();bind(m);assert(m.can_fire());
    assert(begin(m,s)==E::started&&!m.can_fire());
    auto guide=m.guide();
    assert(guide.region==BoltokGuideRegion::weapon_part&&guide.parts==boltok_object_part(2)&&
        guide.contact_id==601&&guide.red==1&&guide.green==1&&guide.blue==0&&guide.alpha==1);
    assert(move(m,s,0,3,2)==E::step_completed&&m.loaded()==3&&m.total()==12);
    assert(m.guide().region==BoltokGuideRegion::left_shoulder&&m.guide().parts==0);
    assert(grab(m,s)==E::grabbed&&m.held()==4&&m.reserve()==5&&m.loaded()==3&&m.total()==12);
    assert(m.guide().parts==boltok_object_part(3)&&m.guide().contact_id==602);
    assert(insert(m,s)==E::inventory_requested);
    assert(m.stage()==S::inventory_pending&&m.inventory().reserve==9&&m.reserve()==5&&m.loaded()==3);
    auto request=m.inventory_request();
    assert(request.token&&request.before_revision==11&&request.before_loaded==3&&
        request.before_reserve==9&&request.after_loaded==7&&request.after_reserve==5);
    assert(!m.can_fire()&&m.guide().region==BoltokGuideRegion::none);
    auto receipt=inventory_receipt(m);auto frame=stamp(6);
    assert(m.acknowledge_inventory(frame,receipt,frame.observed_ns)==E::inventory_committed);
    assert(m.loaded()==7&&m.reserve()==5&&m.held()==0&&m.total()==12&&!m.can_fire());
    assert(m.acknowledge_inventory(stamp(7),receipt,stamp(7).observed_ns)==E::rejected);
    assert(m.guide().parts==boltok_object_part(1));
    assert(move(m,s,2,7,6,1100)==E::completion_requested);
    auto completion=m.completion_request();
    assert(completion.started_tick==1000&&completion.finished_tick==1100&&
        completion.proposed_result==E::perfect&&completion.final_ready_state==1004&&!m.can_fire());
    auto done=completion_receipt(m);frame=stamp(8,1200);
    assert(m.acknowledge_completion(frame,done,frame.observed_ns)==E::perfect&&m.can_fire());
    assert(!m.completion_request().token&&!m.inventory_request().token&&m.total()==12);
    assert(m.acknowledge_completion(stamp(9,1200),done,stamp(9,1200).observed_ns)==E::rejected);
    frame=stamp(9,1200);
    assert(m.request_original_fire(frame,1,frame.observed_ns)==E::fire_requested&&!m.can_fire());
    auto fire=m.fire_request();auto observed=m.inventory();--observed.loaded;++observed.revision;
    BoltokShotReceipt shot{{fire.token,0,0,3,observed,true,true,false,0,E::rejected},true,true,true};
    frame=stamp(10,1200);
    assert(m.after_original_shot(frame,shot,frame.observed_ns)==E::shot_recorded);
    assert(m.loaded()==6&&m.reserve()==5&&m.total()==11&&!m.can_fire());
    frame=stamp(11,1200);
    assert(m.after_original_shot(frame,shot,frame.observed_ns)==E::rejected&&m.total()==11);
    assert(m.observe_original_ready(frame,m.inventory(),true,frame.observed_ns)==E::ready_observed);
    assert(m.can_fire());
}
void deadline_boundary_unknown_and_pause() {
    for(auto tick:{1000LL,1099LL,1100LL,1101LL,9000LL,
                   (std::numeric_limits<std::int64_t>::max)()}){
        BoltokReload m;seated(m);
        assert(move(m,start(),2,7,6,tick)==E::completion_requested);
        auto expected=tick<=1100?E::perfect:E::ordinary;
        assert(m.completion_request().proposed_result==expected);
    }
    for(bool populated:{false,true}){
        BoltokReload m;auto s=start(inventory(),false);if(populated)s.perfect.end_ticks=100;
        seated(m,s);assert(move(m,s,2,7,6)==E::completion_requested);
        assert(m.completion_request().proposed_result==E::ordinary);
    }
    // The final insertion is the last necessary action in this synthetic plan.
    // An asynchronous inventory response does not move the finish timestamp.
    auto s=start();s.plan.count=1;s.plan.steps[0]=s.plan.steps[1];
    s.plan.steps[0].before_state=1001;s.plan.final_ready_state=1003;
    BoltokReload m;bind(m);assert(begin(m,s)==E::started);
    assert(grab(m,s,3,0)==E::grabbed);
    assert(insert(m,s,4,0,3,81,1100)==E::inventory_requested);
    assert(acknowledge_inventory(m,5,9000)==E::completion_requested);
    assert(m.completion_request().finished_tick==1100&&m.completion_request().proposed_result==E::perfect);
}
void invalid_binding_and_start() {
    for(unsigned bad=0;bad!=16;++bad){
        auto b=binding();BoltokReload m;
        switch(bad){
            case 0:b.stamp.identity.weapon=0;break;
            case 1:b.stamp.sequence=0;break;
            case 2:b.stamp.observed_ns=0;break;
            case 3:b.stamp.game_tick=-1;break;
            case 4:b.stamp.right_tracked=false;break;
            case 5:b.stamp.weapon_owned=false;break;
            case 6:b.inventory.identity.pawn++;break;
            case 7:b.inventory.revision=0;break;
            case 8:b.inventory.ammunition_type=0;break;
            case 9:b.inventory.capacity=0;break;
            case 10:b.inventory.loaded=8;break;
            case 11:b.inventory.reserve=1000001;break;
            case 12:b.original_class_verified=false;break;
            case 13:b.inventory_reconciled=false;break;
            case 14:b.original_inventory_slot=2;break;
            case 15:b.original_shot_ammo_cost=0;break;
        }
        assert(!m.bind(b,b.stamp.observed_ns)&&m.stage()==S::unbound);
    }
    for(unsigned bad=0;bad!=24;++bad){
        BoltokReload m;bind(m);auto s=start();
        switch(bad){
            case 0:s.r3_press_edge=0;break;
            case 1:s.inventory.loaded++;break;
            case 2:s.inventory.revision++;break;
            case 3:s.original_reload_accepted=false;break;
            case 4:s.automatic_refill_held=false;break;
            case 5:s.plan.current_instance_verified=false;break;
            case 6:s.plan.original_order_verified=false;break;
            case 7:s.plan.identity.weapon++;break;
            case 8:s.plan.generation=0;break;
            case 9:s.plan.original_reload_serial=0;break;
            case 10:s.plan.count=0;break;
            case 11:s.plan.count=25;break;
            case 12:s.plan.condition=BoltokReloadCondition::empty;break;
            case 13:s.plan.steps[0].parts=0x800;break;
            case 14:s.plan.steps[0].travel_id=0;break;
            case 15:s.plan.steps[1].phase_id=501;break;
            case 16:s.plan.steps[2].before_state=999;break;
            case 17:s.plan.steps[1].rounds=3;break;
            case 18:s.plan.steps[0].rounds=1;break;
            case 19:s.plan.steps[0].kind=BoltokStepKind::unknown;break;
            case 20:s.perfect.end_ticks=-1;break;
            case 21:s.perfect.identity.input_epoch++;break;
            case 22:s.perfect.original_reload_serial++;break;
            case 23:s.perfect.plan_generation++;break;
        }
        assert(begin(m,s)==E::invalid&&m.stage()==S::ready&&m.total()==12);
    }
    BoltokReload full;bind(full,binding(7));auto s=start(inventory(7));
    assert(begin(full,s)==E::full&&full.total()==16);
    s.stamp=stamp(3);assert(begin(full,s)!=E::full); // Same edge emits no second feedback.
    BoltokReload dry;bind(dry,binding(3,0));s=start(inventory(3,0));
    assert(begin(dry,s)==E::no_ammo&&dry.total()==3);
    s.stamp=stamp(3);assert(begin(dry,s)!=E::no_ammo);
}
void wrong_ammo_and_continuous_geometry() {
    for(unsigned bad=0;bad!=11;++bad){
        BoltokReload m;auto s=start();bind(m);assert(begin(m,s)==E::started);
        assert(move(m,s,0,3,2)==E::step_completed);auto r=ammo(s);auto f=stamp(4);
        switch(bad){
            case 0:r.item_serial=0;break;
            case 1:r.original_reload_serial++;break;
            case 2:r.plan_generation++;break;
            case 3:r.phase_id++;break;
            case 4:r.ammunition_type++;break;
            case 5:r.rounds++;break;
            case 6:r.source=BodySlot::weapon_2;break;
            case 7:r.kind=BoltokAmmoKind::spent_case;break;
            case 8:r.kind=BoltokAmmoKind::snub_magazine;break;
            case 9:r.live_ammunition_verified=false;break;
            case 10:r.left_grip_held=false;break;
        }
        assert(m.grab_left_shoulder(f,r,f.observed_ns)==E::rejected&&m.total()==12&&m.held()==0);
    }
    for(unsigned bad=0;bad!=20;++bad){
        BoltokReload m;auto s=start();bind(m);assert(begin(m,s)==E::started);
        auto r=motion(s,0,2);auto f=stamp(3);
        switch(bad){
            case 0:r.original_reload_serial++;break;
            case 1:r.plan_generation++;break;
            case 2:r.phase_id++;break;
            case 3:r.parts=boltok_object_part(1);break;
            case 4:r.contact_id++;break;
            case 5:r.travel_id++;break;
            case 6:r.before_state++;break;
            case 7:r.after_state++;break;
            case 8:r.motion_begin_sequence=1;break;
            case 9:r.motion_begin_sequence=3;break;
            case 10:r.motion_begin_ns--;break;
            case 11:r.maximum_sample_gap_ns=100000001;break;
            case 12:r.tracked_samples=1;break;
            case 13:r.measured_6dof_travel=false;break;
            case 14:r.continuous=false;break;
            case 15:r.complete_travel=false;break;
            case 16:r.aligned=false;break;
            case 17:r.player_driven=false;break;
            case 18:r.left_grip_held=false;break;
            case 19:f.left_tracked=false;break;
        }
        assert(m.move_original_parts(f,r,f.observed_ns)==E::rejected&&m.current_step()==0);
    }
    BoltokReload m;auto s=start();holding(m,s);auto f=stamp(5);
    assert(m.release_ammunition(f,82,f.observed_ns)==E::rejected);
    assert(m.release_ammunition(f,81,f.observed_ns)==E::released&&m.reserve()==9&&m.total()==12);
    assert(grab(m,s,6,1,81)==E::rejected); // Released item serial cannot be recycled.
    assert(grab(m,s,6,1,82)==E::grabbed);
    assert(insert(m,s,7,1,4,82)==E::rejected); // Stroke predating new grab.
    assert(insert(m,s,7,1,6,82)==E::inventory_requested);
    BoltokReload gap;bind(gap);assert(begin(gap,s)==E::started);
    auto r=motion(s,0,2);f=stamp(5); // Two samples cannot cover three claimed gaps.
    assert(gap.move_original_parts(f,r,f.observed_ns)==E::rejected);
    r.tracked_samples=4;
    assert(gap.move_original_parts(f,r,f.observed_ns)==E::step_completed);
}
void original_inventory_and_reward_gates() {
    for(unsigned bad=0;bad!=5;++bad){
        BoltokReload m;holding(m);assert(insert(m,start())==E::inventory_requested);
        auto r=inventory_receipt(m);auto f=stamp(6);
        switch(bad){
            case 0:r.token++;break;case 1:r.original_reload_serial++;break;
            case 2:r.plan_generation++;break;case 3:r.original_event_sequence=0;break;
            case 4:r.observed.identity.weapon++;break;
        }
        assert(m.acknowledge_inventory(f,r,f.observed_ns)==E::rejected);
        assert(m.stage()==S::inventory_pending&&m.total()==12&&!m.can_fire());
    }
    for(unsigned bad=0;bad!=6;++bad){
        BoltokReload m;holding(m);assert(insert(m,start())==E::inventory_requested);
        auto r=inventory_receipt(m);auto f=stamp(6);
        switch(bad){
            case 0:r.observed.loaded--;break;
            case 1:r.observed.reserve++;break;
            case 2:r.observed.revision=11;break;
            case 3:r.observed.ammunition_type++;break;
            case 4:r.original_effect_applied_once=false;break;
            case 5:r.observed.capacity++;break;
        }
        assert(m.acknowledge_inventory(f,r,f.observed_ns)==E::interrupted);
        assert(m.needs_reconciliation()&&!m.can_fire()&&m.total()==12);
    }
    BoltokReload denied;holding(denied);assert(insert(denied,start())==E::inventory_requested);
    auto request=denied.inventory_request();auto r=inventory_receipt(denied);auto f=stamp(6);
    r.accepted=r.original_effect_applied_once=false;r.observed=denied.inventory();
    assert(denied.acknowledge_inventory(f,r,f.observed_ns)==E::rejected);
    assert(denied.stage()==S::holding_ammunition&&denied.held()==4&&denied.total()==12);
    assert(insert(denied,start(),7,1,6)==E::inventory_requested);
    assert(denied.inventory_request().token>request.token);
    r.token=request.token;f=stamp(8);
    assert(denied.acknowledge_inventory(f,r,f.observed_ns)==E::rejected); // Old request.
    for(unsigned bad=0;bad!=7;++bad){
        BoltokReload m;seated(m);assert(move(m,start(),2,7,6)==E::completion_requested);
        auto done=completion_receipt(m);f=stamp(8);
        switch(bad){
            case 0:done.accepted=false;break;
            case 1:done.original_effect_applied_once=false;break;
            case 2:done.original_ready=false;break;
            case 3:done.observed.loaded--;break;
            case 4:done.observed.revision--;break;
            case 5:done.original_ready_state++;break;
            case 6:done.applied_reload_result=E::ordinary;break;
        }
        assert(m.acknowledge_completion(f,done,f.observed_ns)==E::interrupted&&!m.can_fire());
    }
}
void identity_input_freshness_and_rebind() {
    for(unsigned field=0;field!=9;++field){
        BoltokReload m;holding(m);auto f=stamp(5);auto& id=f.identity;
        switch(field){
            case 0:++id.world;break;case 1:++id.pawn;break;case 2:++id.weapon;break;
            case 3:++id.world_generation;break;case 4:++id.binding_generation;break;
            case 5:++id.session;break;case 6:++id.space_epoch;break;
            case 7:++id.input_epoch;break;case 8:++id.profile_generation;break;
        }
        assert(m.insert_ammunition(f,{81,motion(start(),1,4)},f.observed_ns)==E::rejected);
        assert(m.observe_frame(f,f.observed_ns)==E::interrupted&&!m.can_fire()&&m.total()==12);
    }
    BoltokReload fresh;bind(fresh);auto f=stamp(2);
    assert(fresh.can_fire(f,f.observed_ns+100000000));
    assert(!fresh.can_fire(f,f.observed_ns+100000001));
    assert(fresh.observe_frame(f,f.observed_ns+100000001)==E::interrupted);
    BoltokReload back;bind(back);auto s=start();s.stamp.game_tick=999;
    assert(begin(back,s)==E::rejected);s=start();s.stamp.sequence=1;
    assert(begin(back,s)==E::rejected);
    BoltokReload lost;bind(lost);f=stamp(2);f.right_tracked=false;
    assert(lost.observe_frame(f,f.observed_ns)==E::interrupted&&!lost.can_fire());
    BoltokReload left_gap;bind(left_gap);s=start();assert(begin(left_gap,s)==E::started);
    f=stamp(3);f.left_tracked=false;
    assert(left_gap.observe_frame(f,f.observed_ns)==E::ready_observed);
    assert(move(left_gap,s,0,4,2)==E::rejected); // Path cannot bridge lost left tracking.
    assert(move(left_gap,s,0,4,3)==E::rejected); // Missing sample cannot start a new path.
    assert(move(left_gap,s,0,5,4)==E::step_completed);
    BoltokReload m;holding(m);assert(insert(m,start())==E::inventory_requested);
    auto old=inventory_receipt(m);auto old_token=m.inventory_request().token;
    assert(m.interrupt()==E::interrupted&&m.total()==12);
    assert(!m.bind(binding(),stamp(1).observed_ns)); // Reusing binding is forbidden.
    auto b=binding();b.stamp.identity.binding_generation++;
    b.inventory.identity=b.stamp.identity;assert(m.bind(b,b.stamp.observed_ns));
    auto newer=start();newer.stamp.identity=b.stamp.identity;
    newer.inventory.identity=newer.plan.identity=newer.perfect.identity=b.stamp.identity;
    assert(begin(m,newer)==E::started);
    f=stamp(3);f.identity=b.stamp.identity;auto path=motion(newer,0,2);
    assert(m.move_original_parts(f,path,f.observed_ns)==E::step_completed);
    f=stamp(4);f.identity=b.stamp.identity;
    assert(m.grab_left_shoulder(f,ammo(newer),f.observed_ns)==E::grabbed);
    f=stamp(5);f.identity=b.stamp.identity;
    assert(m.insert_ammunition(f,{81,motion(newer,1,4)},f.observed_ns)==E::inventory_requested);
    assert(m.inventory_request().token>old_token);
    f=stamp(6);f.identity=b.stamp.identity;old.observed.identity=b.stamp.identity;
    assert(m.acknowledge_inventory(f,old,f.observed_ns)==E::rejected&&m.total()==12);
}
void independent_verified_plan_and_conservation() {
    // Two separate load operations with a verified motion between them. This
    // proves no Snub magazine ejection/order is secretly imposed by the model.
    auto s=start(inventory(2,2));s.plan.count=4;s.plan.initial_state=1001;s.plan.final_ready_state=1005;
    s.plan.steps[0]={BoltokStepKind::insert_ammunition,boltok_object_part(8),801,901,10001,1001,1002,1};
    s.plan.steps[1]={BoltokStepKind::move_original_parts,boltok_object_part(11),802,902,10002,1002,1003,0};
    s.plan.steps[2]={BoltokStepKind::insert_ammunition,boltok_object_part(8),803,903,10003,1003,1004,1};
    s.plan.steps[3]={BoltokStepKind::move_original_parts,boltok_object_part(5),804,904,10004,1004,1005,0};
    BoltokReload m;bind(m,binding(2,2));assert(begin(m,s)==E::started&&m.total()==4);
    assert(grab(m,s,3,0)==E::grabbed&&m.reserve()==1&&m.held()==1&&m.total()==4);
    assert(insert(m,s,4,0,3)==E::inventory_requested);
    assert(acknowledge_inventory(m,5)==E::inventory_committed&&m.loaded()==3&&m.reserve()==1);
    assert(move(m,s,1,6,5)==E::step_completed);
    assert(grab(m,s,7,2,82)==E::grabbed&&m.reserve()==0&&m.total()==4);
    assert(insert(m,s,8,2,7,82)==E::inventory_requested);
    assert(acknowledge_inventory(m,9,1000,2)==E::inventory_committed&&m.loaded()==4&&m.reserve()==0);
    assert(move(m,s,3,10,9,1200)==E::completion_requested);
    assert(m.completion_request().proposed_result==E::ordinary&&m.total()==4);
    for(auto loaded:{0u,2u,6u})for(auto reserve:{1u,2u,10u}){
        BoltokReload sample;auto fixture=start(inventory(loaded,reserve));
        seated(sample,fixture);auto count=loaded+(std::min)(7-loaded,reserve);
        assert(sample.loaded()==count&&sample.total()==loaded+reserve&&sample.held()==0);
    }
}
void physical_open_requires_real_motion() {
    auto s=start(inventory(5,9,6));s.r3_press_edge=0;
    BoltokReload m;bind(m,binding(5,9,6));
    auto opening=motion(s,0,1);
    assert(m.begin_grab_open(s,1,opening,s.stamp.observed_ns)==E::started);
    assert(m.current_step()==1&&m.stage()==S::take_ammunition&&!m.can_fire());
    assert(grab(m,s,3)==E::grabbed&&m.held()==1&&m.total()==14);
    assert(insert(m,s,4,1,3)==E::inventory_requested);
    assert(acknowledge_inventory(m,5)==E::inventory_committed);
    assert(move(m,s,2,6,5,1100)==E::completion_requested);
    auto f=stamp(7,1100);
    assert(m.acknowledge_completion(f,completion_receipt(m),f.observed_ns)==E::perfect);
    assert(m.loaded()==6&&m.reserve()==8&&m.total()==14&&m.can_fire());
    for(unsigned bad=0;bad!=15;++bad){
        BoltokReload rejected;bind(rejected,binding(5,9,6));auto path=opening;auto input=s;
        std::uint64_t token=1;
        switch(bad){
            case 0:path.left_grip_held=false;break;
            case 1:path.continuous=false;break;
            case 2:path.complete_travel=false;break;
            case 3:path.parts=boltok_object_part(11);break;
            case 4:path.motion_begin_sequence=0;break;
            case 5:path.tracked_samples=1;break;
            case 6:input.stamp.left_tracked=false;break;
            case 7:token=0;break;
            case 8:input.stamp.identity.weapon++;break;
            case 9:input.plan.steps[1].rounds=2;break;
            case 10:input.accepted_begin_tick=-1;break;
            case 11:input.accepted_begin_tick=1001;break;
            case 12:input.inventory.capacity=7;break;
            case 13:input.plan.steps[2].kind=BoltokStepKind::insert_ammunition;break;
            case 14:input.mechanical_ownership_verified=false;break;
        }
        assert(rejected.begin_grab_open(input,token,path,input.stamp.observed_ns)!=E::started);
        assert(rejected.stage()==S::ready&&rejected.loaded()==5&&rejected.can_fire());
        // Rejection must not consume the valid stroke's frame or grip token.
        assert(rejected.begin_grab_open(s,1,opening,s.stamp.observed_ns)==E::started);
        assert(rejected.begin_grab_open(s,1,opening,s.stamp.observed_ns)==E::rejected);
    }
    // Opening travel consumes the original reward window; its endpoint cannot
    // move the accepted start forward and manufacture a perfect reload.
    BoltokReload delayed;bind(delayed,binding(5,9,6));s.stamp.game_tick=1050;
    assert(delayed.begin_grab_open(s,1,opening,s.stamp.observed_ns)==E::started);
    auto held_frame=stamp(3,1050);
    assert(delayed.grab_left_shoulder(held_frame,ammo(s),held_frame.observed_ns)==E::grabbed);
    assert(insert(delayed,s,4,1,3,81,1050)==E::inventory_requested);
    assert(acknowledge_inventory(delayed,5,1050)==E::inventory_committed);
    assert(move(delayed,s,2,6,5,1101)==E::completion_requested);
    assert(delayed.completion_request().started_tick==1000&&
        delayed.completion_request().proposed_result==E::ordinary);
}
void partial_close_and_inspection() {
    auto s=start(inventory(3,2,6));s.r3_press_edge=0;s.plan.count=4;s.plan.final_ready_state=1005;
    s.plan.steps[1].rounds=1;
    s.plan.steps[2]={BoltokStepKind::insert_ammunition,boltok_object_part(2),503,603,703,1003,1004,1};
    s.plan.steps[3]={BoltokStepKind::move_original_parts,boltok_object_part(3),504,604,704,1004,1005,0};
    s.plan.early_closures[1]={BoltokStepKind::move_original_parts,boltok_object_part(3),601,701,801,1002,1005,0};
    s.plan.early_closures[2]={BoltokStepKind::move_original_parts,boltok_object_part(3),602,702,802,1003,1005,0};
    for(bool inserted:{false,true}){
        BoltokReload m;bind(m,binding(3,2,6));
        assert(m.begin_grab_open(s,1,motion(s,0,1),s.stamp.observed_ns)==E::started);
        unsigned seq=3,cursor=1;
        if(inserted){
            assert(grab(m,s,3)==E::grabbed);
            assert(m.close_early(stamp(4),{},stamp(4).observed_ns)==E::rejected);
            assert(insert(m,s,4,1,3)==E::inventory_requested);
            assert(m.close_early(stamp(5),{},stamp(5).observed_ns)==E::rejected);
            assert(acknowledge_inventory(m,5)==E::inventory_committed);seq=6;cursor=2;
        }
        auto branch=s;branch.plan.steps[cursor]=s.plan.early_closures[cursor];
        auto close=motion(branch,cursor,seq-1);auto f=stamp(seq,1001);
        auto wrong=close;wrong.before_state++;
        assert(m.close_early(f,wrong,f.observed_ns)==E::rejected);
        assert(m.close_early(f,close,f.observed_ns)==E::completion_requested);
        assert(!m.can_fire()&&m.total()==5);
        const auto result=inserted?E::ordinary:E::inspection_completed;
        assert(m.completion_request().proposed_result==result);
        f=stamp(seq+1,1001);
        assert(m.acknowledge_completion(f,completion_receipt(m),f.observed_ns)==result);
        assert(m.loaded()==(inserted?4u:3u)&&m.reserve()==(inserted?1u:2u)&&m.can_fire());
    }
    // A normal final close is not automatically a legal shortcut from an
    // earlier state. No authenticated branch means no early completion.
    s.plan.early_closures={};BoltokReload no_branch;bind(no_branch,binding(3,2,6));
    assert(no_branch.begin_grab_open(s,1,motion(s,0,1),s.stamp.observed_ns)==E::started);
    assert(no_branch.close_early(stamp(3),motion(s,3,2),stamp(3).observed_ns)==E::rejected);
    assert(no_branch.stage()==S::take_ammunition&&!no_branch.can_fire());
}
void full_and_dry_inspection() {
    for(unsigned loaded:{0u,3u,6u})for(unsigned reserve:{0u,2u}){
        if(reserve&&loaded!=6)continue;
        auto s=start(inventory(loaded,reserve,6));s.r3_press_edge=0;s.original_reload_accepted=false;
        s.plan.count=2;s.plan.steps[1]={BoltokStepKind::move_original_parts,boltok_object_part(3),
            502,602,702,1002,1004,0};
        BoltokReload m;bind(m,binding(loaded,reserve,6));
        auto invalid=s;invalid.mechanical_ownership_verified=false;
        assert(m.begin_grab_open(invalid,1,motion(s,0,1),s.stamp.observed_ns)==E::invalid);
        assert(m.begin_grab_open(s,1,motion(s,0,1),s.stamp.observed_ns)==E::started);
        assert(m.stage()==S::mechanism&&!m.can_fire());
        assert(move(m,s,1,3,2,1001)==E::completion_requested);
        assert(m.completion_request().proposed_result==E::inspection_completed);
        auto f=stamp(4,1001);
        assert(m.acknowledge_completion(f,completion_receipt(m),f.observed_ns)==E::inspection_completed);
        assert(m.loaded()==loaded&&m.reserve()==reserve&&m.total()==loaded+reserve);
        assert(m.can_fire()==bool(loaded));
    }
}
void fire_receipts_and_native_cadence() {
    for(unsigned bad=0;bad!=7;++bad){
        BoltokReload m;bind(m,binding(3,9,7,2));auto f=stamp(2);
        assert(m.request_original_fire(f,1,f.observed_ns)==E::fire_requested);
        auto observed=m.inventory();observed.loaded=1;++observed.revision;
        BoltokShotReceipt shot{{m.fire_request().token,0,0,1,observed,true,true,true,0,E::rejected},
            true,true,true};
        switch(bad){
            case 0:shot.original.observed.loaded=2;break;
            case 1:shot.original.observed.reserve--;break;
            case 2:shot.original.observed.revision--;break;
            case 3:shot.original.original_effect_applied_once=false;break;
            case 4:shot.original_shot_verified=false;break;
            case 5:shot.original_ballistics_retained=false;break;
            case 6:shot.original_recoil_retained=false;break;
        }
        f=stamp(3);
        assert(m.after_original_shot(f,shot,f.observed_ns)==E::interrupted&&!m.can_fire());
    }
    BoltokReload m;bind(m,binding(3,9,7,2));auto f=stamp(2);
    assert(m.request_original_fire(f,1,f.observed_ns)==E::fire_requested);
    auto observed=m.inventory();observed.loaded=1;++observed.revision;
    BoltokShotReceipt shot{{m.fire_request().token,0,0,1,observed,true,true,true,0,E::rejected},true,true,true};
    f=stamp(3);assert(m.after_original_shot(f,shot,f.observed_ns)==E::shot_recorded&&m.total()==10);
    f=stamp(4);assert(m.request_original_fire(f,1,f.observed_ns)==E::rejected); // Same trigger.
    assert(m.request_original_fire(f,2,f.observed_ns)==E::no_ammo&&m.total()==10);
    f=stamp(5);assert(m.request_original_fire(f,2,f.observed_ns)==E::rejected);
    BoltokReload denied;bind(denied);f=stamp(2);
    assert(denied.request_original_fire(f,1,f.observed_ns)==E::fire_requested);
    BoltokShotReceipt refused{{denied.fire_request().token,0,0,1,denied.inventory(),false,false,false,
        0,E::rejected},false,false,false};
    f=stamp(3);assert(denied.after_original_shot(f,refused,f.observed_ns)==E::rejected);
    assert(denied.total()==12&&!denied.can_fire());
    f=stamp(4);assert(denied.request_original_fire(f,2,f.observed_ns)==E::rejected);
    auto inconsistent=denied.inventory();inconsistent.loaded++;
    assert(denied.observe_original_ready(f,inconsistent,true,f.observed_ns)==E::interrupted);
}
} // namespace
int main() {
    complete_and_original_fire();deadline_boundary_unknown_and_pause();invalid_binding_and_start();
    wrong_ammo_and_continuous_geometry();original_inventory_and_reward_gates();
    identity_input_freshness_and_rebind();independent_verified_plan_and_conservation();
    fire_receipts_and_native_cadence();physical_open_requires_real_motion();partial_close_and_inspection();
    full_and_dry_inspection();
    std::puts("Boltok reload protocol tests passed (synthetic evidence only).");
}
