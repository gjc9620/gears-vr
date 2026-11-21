#include <type_traits>
#include "weapon_boomshot_reload_accounting.h"
#include "weapon_boomshot_interaction.h"
using namespace gears_weapon;
namespace {
constexpr BoomshotIdentity boom_id{1,2,3,4};
constexpr BoomshotPlan boom_plan(){
    BoomshotPlan p{};p.identity=boom_id;p.config_id=5;p.plan_id=6;p.condition=BoomshotCondition::empty;
    p.current_instance_verified=p.original_order_verified=p.quantity_verified=true;p.count=3;
    p.direct_exchange=false;
    p.steps[0]={BoomshotStepKind::move_mechanism,BoomshotPart::barrel,7,8,0,10,false};
    p.steps[1]={BoomshotStepKind::insert_ammunition,BoomshotPart::feed_port,9,10,1,10,false};
    p.steps[2]={BoomshotStepKind::move_mechanism,BoomshotPart::barrel,11,12,0,10,true};return p;
}
constexpr BoomshotInstanceConfig boom_config(){
    BoomshotInstanceConfig c{};c.identity=boom_id;c.config_id=5;c.ammunition_type=13;
    c.ammunition_kind=BoomshotAmmoKind::verified_carrier;c.capacity=c.original_ammo_cost=c.original_projectile_count=1;
    c.maximum_sample_gap_ns=1000;c.after_shot=boom_plan();c.after_shot.count=0;c.after_shot.condition=BoomshotCondition::after_shot;
    c.current_instance_verified=c.original_class_verified=c.original_fire_semantics_verified=true;return c;
}
constexpr BoomshotPlan boom_exchange_plan(){
    BoomshotPlan p{};p.identity=boom_id;p.config_id=5;p.plan_id=6;p.condition=BoomshotCondition::empty;
    p.current_instance_verified=p.original_order_verified=p.quantity_verified=p.direct_exchange=true;p.count=1;
    p.steps[0]={BoomshotStepKind::insert_ammunition,BoomshotPart::feed_port,7,8,1,0,false};return p;
}
constexpr bool boom_valid(){auto p=boom_plan();unsigned units{};return p.valid(boom_id,5,BoomshotCondition::empty,units)&&units==1&&boom_config().valid();}
constexpr bool boom_exchange_valid(){
    auto p=boom_exchange_plan();unsigned units{};
    if(!p.valid(boom_id,5,BoomshotCondition::empty,units)||units!=1)return false;
    for(unsigned n=0;n<7;++n){auto bad=p;
        if(n==0)bad.count=2;if(n==1)bad.steps[0].kind=BoomshotStepKind::move_mechanism;
        if(n==2)bad.steps[0].part=BoomshotPart::barrel;if(n==3)bad.steps[0].units=2;
        if(n==4)bad.condition=BoomshotCondition::inspection;if(n==5)bad.direct_exchange=false;
        if(n==6){bad.count=1;bad.steps[0].return_required=true;}
        if(bad.valid(boom_id,5,n==4?BoomshotCondition::inspection:BoomshotCondition::empty,units))return false;
    }return true;
}
constexpr bool boom_plan_rejects(){
    for(unsigned n=0;n<10;++n){auto p=boom_plan();unsigned units{};
        if(n==0){p.count=1;p.steps[0]=p.steps[1];}
        if(n==1)p.count=2;if(n==2)p.steps[1].units=2;if(n==3)p.steps[0].part=BoomshotPart::magazine;
        if(n==4)p.steps[2].part=BoomshotPart::magazine;if(n==5)p.steps[1].part=BoomshotPart::barrel;
        if(n==6)p.steps[2].phase_id=p.steps[0].phase_id;if(n==7)p.steps[1].geometry_id=0;
        if(n==8)p.steps[1].return_required=true;if(n==9)p.steps[0].minimum_motion_ns=-1;
        if(p.valid(boom_id,5,BoomshotCondition::empty,units))return false;
    }return true;
}
constexpr bool boom_config_rejects(){
    for(unsigned n=0;n<7;++n){auto c=boom_config();
        if(n==0)c.capacity=7;if(n==1)c.original_ammo_cost=2;if(n==2)c.ammunition_kind=BoomshotAmmoKind::projectile;
        if(n==3)c.after_shot.count=1;if(n==4)c.original_projectile_count=0;
        if(n==5)c.after_shot.quantity_verified=false;if(n==6)c.original_class_verified=false;
        if(c.valid())return false;
    }return true;
}
constexpr bool boom_no_partial(){auto p=boom_plan();p.condition=BoomshotCondition::partial;unsigned units{};
    return !p.valid(boom_id,5,BoomshotCondition::partial,units);}
static_assert(boom_valid());static_assert(boom_exchange_valid());static_assert(boom_plan_rejects());static_assert(boom_config_rejects());static_assert(boom_no_partial());
}

namespace {
constexpr BoomshotFrame boom_frame(std::uint64_t binding,std::uint64_t seq){
    BoomshotFrame f{};f.stamp={boom_id,binding,seq,1,static_cast<std::int64_t>(seq),static_cast<std::int64_t>(seq*10)};
    f.observed={boom_id,1,0,11};
    f.left_tracked=f.right_tracked=f.right_hand_equipped=f.original_interaction_allowed=true;
    return f;
}
constexpr BoomshotReloadStart boom_start(std::uint64_t binding){
    BoomshotReloadStart s{};s.binding=binding;s.original_reload_serial=1;s.plan=boom_plan();
    s.original_reload_accepted=s.automatic_completion_suppressed=true;return s;
}
constexpr bool boom_physical_start(){
    BoomshotInteraction w;if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
    auto b=w.binding_token();auto s=boom_start(b);
    if(w.observe(boom_frame(b,1))!=BoomshotEvent::motion_reset)return false;
    if(w.begin_reload(boom_frame(b,2),s,1,ReloadIntentSource::mechanism_open)!=BoomshotEvent::started)return false;
    if(w.stage()!=BoomshotStage::mechanism||w.step_index()!=0||s.r3_press_edge)return false;
    s.r3_press_edge=2;
    return w.begin_r3(boom_frame(b,3),s)==BoomshotEvent::rejected&&w.step_index()==0;
}
constexpr bool boom_invalid_starts(){
    for(unsigned n=0;n<10;++n){
        BoomshotInteraction w;if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
        auto b=w.binding_token();auto s=boom_start(b);auto f=boom_frame(b,2);
        w.observe(boom_frame(b,1));std::uint64_t intent=1;
        auto source=ReloadIntentSource::mechanism_open;
        if(n==0)f.left_tracked=false;if(n==1)f.right_tracked=false;
        if(n==2)s.original_reload_accepted=false;if(n==3)s.automatic_completion_suppressed=false;
        if(n==4)s.original_reload_serial=0;if(n==5)intent=0;
        if(n==6)source=ReloadIntentSource::take_ammunition;if(n==7)++s.binding;
        if(n==8)f.original_owns_hands=true;if(n==9)f.stamp.sequence=1;
        if(w.begin_reload(f,s,intent,source)==BoomshotEvent::started||w.stage()!=BoomshotStage::ready)return false;
    }return true;
}
constexpr bool boom_auxiliary_start(){
    BoomshotInteraction w;if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
    auto b=w.binding_token();w.observe(boom_frame(b,1));auto s=boom_start(b);s.r3_press_edge=5;
    return w.begin_r3(boom_frame(b,2),s)==BoomshotEvent::started&&w.step_index()==0;
}
constexpr bool boom_tracking_recovery(){
    BoomshotInteraction w;if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
    auto b=w.binding_token();auto s=boom_start(b);w.observe(boom_frame(b,1));
    auto lost=boom_frame(b,2);lost.left_tracked=false;
    if(w.begin_reload(lost,s,1,ReloadIntentSource::mechanism_open)!=BoomshotEvent::motion_reset)return false;
    if(w.begin_reload(boom_frame(b,3),s,1,ReloadIntentSource::mechanism_open)!=BoomshotEvent::motion_reset)return false;
    return w.begin_reload(boom_frame(b,4),s,1,ReloadIntentSource::mechanism_open)==BoomshotEvent::started;
}
static_assert(boom_physical_start());
static_assert(boom_invalid_starts());
static_assert(boom_auxiliary_start());
static_assert(boom_tracking_recovery());
}

namespace {
constexpr BoomshotAmmoReceipt boom_carrier(std::uint64_t item=100){
    return {1,1,6,9,item,13,1,BoomshotAmmoKind::verified_carrier,BoomshotAmmoSource::left_shoulder,true};
}
constexpr bool boom_account_begin(BoomshotReloadAccounting& a,bool infinite=false){
    auto f=boom_frame(1,2);f.observed.infinite_reserve=infinite;
    if(infinite)f.observed.reserve=0;
    auto s=boom_start(1);s.perfect={boom_id,5,1,10,true};
    return a.begin(boom_config(),f,s,1,ReloadIntentSource::mechanism_open);
}
constexpr bool boom_late_insert(){
    BoomshotReloadAccounting a;BoomshotReloadAccounting::Request q{},saved{};
    if(!boom_account_begin(a)||!a.reserve(boom_carrier())||!a.insert(100)||!a.take(q))return false;
    if(!a.interrupt()||a.take(saved)||!a.dispatched(saved)||saved.token!=q.token)return false;
    auto i=q.expected;i.revision=q.before.revision+1;
    if(!a.acknowledge({q.token,true,i,ReloadTxnResult::none})||a.inventory().loaded!=1||a.ready())return false;
    if(boom_account_begin(a)||a.resume(a.inventory(),2,2,1)||a.resume(a.inventory(),2,1,2))return false;
    return a.resume(a.inventory(),2,1,1)&&a.close(8)&&a.take(q)&&q.result==ReloadTxnResult::perfect&&
        a.acknowledge({q.token,true,q.before,q.result})&&a.ready()&&!a.take(saved);
}
constexpr bool boom_infinite_account(){
    BoomshotReloadAccounting a;BoomshotReloadAccounting::Request q{};
    if(!boom_account_begin(a,true)||!a.reserve(boom_carrier())||a.available()!=0||!a.insert(100)||!a.take(q))return false;
    auto i=q.expected;i.revision=q.before.revision+1;
    return !i.reserve&&i.infinite_reserve&&a.acknowledge({q.token,true,i,ReloadTxnResult::none})&&
        a.inventory().loaded==1&&a.inventory().infinite_reserve;
}
constexpr bool boom_bad_receipt(){
    BoomshotReloadAccounting a;BoomshotReloadAccounting::Request q{},saved{};
    if(!boom_account_begin(a)||!a.reserve(boom_carrier())||!a.insert(100)||!a.take(q))return false;
    auto i=q.expected;i.revision=q.before.revision+1;i.infinite_reserve=true;
    return !a.acknowledge({q.token,true,i,ReloadTxnResult::none})&&a.stage()==ReloadTxnStage::reconciliation_required&&
        a.dispatched(saved)&&saved.token==q.token&&!a.take(saved);
}
constexpr bool boom_reject_insert(){
    BoomshotReloadAccounting a;BoomshotReloadAccounting::Request q{};
    if(!boom_account_begin(a)||!a.reserve(boom_carrier())||!a.insert(100)||!a.take(q))return false;
    return a.acknowledge({q.token,false,q.before,ReloadTxnResult::none})&&a.reserved()==0&&
        !a.reserve(boom_carrier())&&a.reserve(boom_carrier(101));
}
constexpr bool boom_late_close(){
    BoomshotReloadAccounting a;BoomshotReloadAccounting::Request q{};
    if(!boom_account_begin(a)||!a.reserve(boom_carrier())||!a.insert(100)||!a.take(q))return false;
    auto i=q.expected;i.revision=q.before.revision+1;
    if(!a.acknowledge({q.token,true,i,ReloadTxnResult::none})||!a.close(4000003)||!a.take(q)||!a.interrupt())return false;
    return q.result==ReloadTxnResult::ordinary&&a.acknowledge({q.token,true,q.before,q.result})&&!a.ready()&&
        a.resume(a.inventory(),2,1,1)&&a.ready()&&!a.take(q);
}
constexpr bool boom_bad_carriers(){
    for(unsigned n=0;n<9;++n){BoomshotReloadAccounting a;if(!boom_account_begin(a))return false;
        auto r=boom_carrier();if(n==0)++r.binding;if(n==1)++r.original_reload_serial;
        if(n==2)++r.plan_id;if(n==3)++r.phase_id;if(n==4)++r.ammunition_type;
        if(n==5)r.units=2;if(n==6)r.kind=BoomshotAmmoKind::projectile;
        if(n==7)r.source=BoomshotAmmoSource::other;if(n==8)r.left_grip_held=false;
        if(a.reserve(r)||a.reserved()!=0||!a.reserve(boom_carrier()))return false;
    }return true;
}
static_assert(boom_late_insert());
static_assert(boom_infinite_account());
static_assert(boom_bad_receipt());
static_assert(boom_reject_insert());
static_assert(boom_late_close());
static_assert(boom_bad_carriers());
}

namespace {
constexpr BoomshotFrame live_frame(BoomshotInteraction const& w,std::uint64_t seq){
    auto f=boom_frame(w.binding_token(),seq);f.observed=w.inventory();return f;
}
constexpr BoomshotMotionReceipt boom_motion(BoomshotInteraction const& w,std::uint64_t begin){
    const auto step=boom_plan().steps[w.step_index()];
    return {w.binding_token(),1,6,step.phase_id,step.geometry_id,1,begin,
        static_cast<std::int64_t>(begin*10),true,true,true,true,true};
}
constexpr bool to_insertion(BoomshotInteraction& w,bool infinite=false){
    if(!w.bind(boom_config(),{boom_id,1,0,infinite?0u:11u,infinite}))return false;
    auto s=boom_start(w.binding_token());s.perfect={boom_id,5,1,10,true};
    w.observe(live_frame(w,1));
    if(w.begin_reload(live_frame(w,2),s,1,ReloadIntentSource::mechanism_open)!=BoomshotEvent::started)return false;
    if(w.complete_mechanism(live_frame(w,3),boom_motion(w,2))!=BoomshotEvent::step_completed)return false;
    if(w.grab_left_shoulder(live_frame(w,4),boom_carrier())!=BoomshotEvent::grabbed)return false;
    return w.insert_ammunition(live_frame(w,5),{boom_motion(w,4),100,true,true,true})==BoomshotEvent::request_inventory;
}
constexpr bool boom_exchange_sequence(){
    BoomshotInteraction w;
    if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
    auto s=boom_start(w.binding_token());s.plan=boom_exchange_plan();s.perfect={boom_id,5,1,10,true};
    w.observe(live_frame(w,1));
    if(w.begin_reload(live_frame(w,2),s,1,ReloadIntentSource::mechanism_open)!=BoomshotEvent::started||
       w.stage()!=BoomshotStage::take_ammunition||w.step_index()!=0)return false;
    auto carrier=boom_carrier();carrier.plan_id=s.plan.plan_id;carrier.phase_id=s.plan.steps[0].phase_id;
    if(w.grab_left_shoulder(live_frame(w,3),carrier)!=BoomshotEvent::grabbed)return false;
    const auto began=live_frame(w,3).stamp;
    const BoomshotMotionReceipt motion{w.binding_token(),1,s.plan.plan_id,s.plan.steps[0].phase_id,
        s.plan.steps[0].geometry_id,began.motion_epoch,began.sequence,began.monotonic_ns,true,true,true,false,true};
    if(w.insert_ammunition(live_frame(w,4),{motion,100,true,true,true})!=BoomshotEvent::request_inventory)return false;
    BoomshotReloadAccounting::Request q{};
    if(!w.take_reload_request(q)||q.call!=ReloadTxnCall::insert)return false;
    auto counts=q.expected;counts.revision=q.before.revision+1;
    const BoomshotInventory observed{counts.identity,counts.revision,counts.loaded,counts.reserve,counts.infinite_reserve};
    const BoomshotOriginalReceipt inserted{live_frame(w,5).stamp,q.token,1,observed,true,true,ReloadTxnResult::none,true};
    if(w.acknowledge_inventory(inserted)!=BoomshotEvent::inventory_committed||w.stage()!=BoomshotStage::completion_commit)return false;
    BoomshotReloadAccounting::Request finish{};
    if(w.take_reload_request(finish))return false;
    auto done_stamp=live_frame(w,6).stamp;
    const BoomshotOriginalReceipt seated{done_stamp,inserted.request_token,2,observed,true,true,ReloadTxnResult::none,true};
    if(w.acknowledge_seated(seated)!=BoomshotEvent::request_completion||!w.take_reload_request(finish)||finish.call!=ReloadTxnCall::finish)return false;
    const BoomshotOriginalReceipt done{live_frame(w,7).stamp,finish.token,3,observed,true,true,finish.result,true};
    return w.acknowledge_completion(done)==BoomshotEvent::reload_completed&&w.stage()==BoomshotStage::ready&&
        w.inventory().loaded==1&&w.inventory().reserve==10;
}
constexpr bool boom_exchange_seat_boundary(bool late){
    BoomshotInteraction w;
    if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
    auto s=boom_start(w.binding_token());s.plan=boom_exchange_plan();s.perfect={boom_id,5,1,10,true};
    w.observe(live_frame(w,1));
    if(w.begin_reload(live_frame(w,2),s,1,ReloadIntentSource::mechanism_open)!=BoomshotEvent::started)return false;
    auto carrier=boom_carrier();carrier.plan_id=s.plan.plan_id;carrier.phase_id=s.plan.steps[0].phase_id;
    if(w.grab_left_shoulder(live_frame(w,3),carrier)!=BoomshotEvent::grabbed)return false;
    const auto began=live_frame(w,3).stamp;
    const BoomshotMotionReceipt motion{w.binding_token(),1,s.plan.plan_id,s.plan.steps[0].phase_id,
        s.plan.steps[0].geometry_id,began.motion_epoch,began.sequence,began.monotonic_ns,true,true,true,false,true};
    if(w.insert_ammunition(live_frame(w,4),{motion,100,true,true,true})!=BoomshotEvent::request_inventory)return false;
    BoomshotReloadAccounting::Request q{};
    if(!w.take_reload_request(q))return false;
    auto counts=q.expected;counts.revision=q.before.revision+1;
    const BoomshotInventory observed{counts.identity,counts.revision,counts.loaded,counts.reserve,counts.infinite_reserve};
    const BoomshotOriginalReceipt inserted{live_frame(w,5).stamp,q.token,1,observed,true,true,ReloadTxnResult::none,true};
    if(w.acknowledge_inventory(inserted)!=BoomshotEvent::inventory_committed||w.take_reload_request(q))return false;
    // The consumed inventory receipt is not a seating proof.
    if(w.acknowledge_seated(inserted)!=BoomshotEvent::rejected||w.take_reload_request(q))return false;
    auto stamp=live_frame(w,6).stamp;stamp.game_tick=live_frame(w,2).stamp.game_tick+(late?4000001:4000000);
    const BoomshotOriginalReceipt seated{stamp,inserted.request_token,2,observed,true,true,ReloadTxnResult::none,true};
    if(w.acknowledge_seated(seated)!=BoomshotEvent::request_completion||!w.take_reload_request(q)||q.call!=ReloadTxnCall::finish)return false;
    if(q.result!=(late?ReloadTxnResult::ordinary:ReloadTxnResult::perfect))return false;
    // A second seat receipt cannot create another finish request.
    return w.acknowledge_seated(seated)==BoomshotEvent::rejected&&!w.take_reload_request(q);
}
static_assert(boom_exchange_seat_boundary(false));static_assert(boom_exchange_seat_boundary(true));
constexpr BoomshotOriginalReceipt boom_ack(BoomshotInteraction const& w,
    BoomshotInteraction::ReloadRequest const& q,std::uint64_t seq,bool accepted=true){
    auto i=accepted&&q.call==ReloadTxnCall::insert?q.expected:q.before;
    i.revision=q.before.revision+(accepted&&q.call==ReloadTxnCall::insert?1:0);
    return {live_frame(w,seq).stamp,q.token,seq,{i.identity,i.revision,i.loaded,i.reserve,i.infinite_reserve},
        accepted,accepted,accepted?q.result:ReloadTxnResult::none,accepted};
}
constexpr bool boom_full_mechanical(bool infinite){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{},again{};
    if(!to_insertion(w,infinite)||!w.take_reload_request(q)||w.take_reload_request(again))return false;
    if(w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed||w.inventory().loaded!=1||w.step_index()!=2)return false;
    if(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q))return false;
    if(q.result!=ReloadTxnResult::perfect||w.acknowledge_completion(boom_ack(w,q,8))!=BoomshotEvent::reload_completed)return false;
    return w.stage()==BoomshotStage::ready&&w.inventory().reserve==(infinite?0u:10u)&&w.inventory().infinite_reserve==infinite;
}
constexpr bool boom_mechanical_late_insert(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{};
    if(!to_insertion(w)||!w.take_reload_request(q)||w.interrupt()!=BoomshotEvent::interrupted||w.reset())return false;
    if(w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed||w.stage()!=BoomshotStage::interrupted)return false;
    if(w.resume(live_frame(w,7),2,1,1,false)||!w.resume(live_frame(w,7),2,1,2,false))return false;
    if(w.observe(live_frame(w,8))!=BoomshotEvent::motion_reset)return false;
    return w.complete_mechanism(live_frame(w,9),boom_motion(w,8))==BoomshotEvent::request_completion&&
        w.take_reload_request(q)&&w.acknowledge_completion(boom_ack(w,q,10))==BoomshotEvent::reload_completed;
}
constexpr bool boom_mechanical_late_complete(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{};
    if(!to_insertion(w)||!w.take_reload_request(q)||w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed)return false;
    if(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q))return false;
    if(w.interrupt()!=BoomshotEvent::interrupted||w.acknowledge_completion(boom_ack(w,q,8))!=BoomshotEvent::reload_completed)return false;
    if(w.stage()!=BoomshotStage::interrupted||w.resume(live_frame(w,9),2,1,3,false))return false;
    return w.resume(live_frame(w,9),2,1,3,true)&&w.stage()==BoomshotStage::ready&&!w.take_reload_request(q);
}
constexpr bool boom_rejected_close_keeps_time(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{},retry{};
    if(!to_insertion(w)||!w.take_reload_request(q)||w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed)return false;
    if(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q))return false;
    if(w.acknowledge_completion(boom_ack(w,q,8,false))!=BoomshotEvent::interrupted)return false;
    return w.resume(live_frame(w,20),2,1,3,false)&&w.take_reload_request(retry)&&retry.token!=q.token&&
        retry.result==ReloadTxnResult::perfect&&w.acknowledge_completion(boom_ack(w,retry,21))==BoomshotEvent::reload_completed;
}
constexpr bool boom_shoot_after_reload(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{};
    if(!to_insertion(w)||!w.take_reload_request(q)||w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed)return false;
    if(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q))return false;
    if(w.acknowledge_completion(boom_ack(w,q,8))!=BoomshotEvent::reload_completed)return false;
    w.observe(live_frame(w,9));auto f=live_frame(w,10);f.original_can_fire=true;
    if(w.request_original_fire(f,1)!=BoomshotEvent::request_fire)return false;
    BoomshotFireRequest shot{};if(!w.take_fire_request(shot))return false;
    auto i=w.inventory();++i.revision;i.loaded=0;
    BoomshotOriginalReceipt r{live_frame(w,11).stamp,shot.token,11,i,true,true};
    return w.after_original_shot({r,1,true})==BoomshotEvent::shot_recorded&&w.inventory().loaded==0&&
        w.stage()==BoomshotStage::ready&&w.step_index()==0;
}
static_assert(boom_full_mechanical(false));
static_assert(boom_full_mechanical(true));
static_assert(boom_mechanical_late_insert());
static_assert(boom_mechanical_late_complete());
static_assert(boom_rejected_close_keeps_time());
static_assert(boom_shoot_after_reload());
constexpr bool boom_rejected_insert_returns_to_carrier(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{};
    if(!to_insertion(w)||!w.take_reload_request(q)||w.acknowledge_inventory(boom_ack(w,q,6,false))!=BoomshotEvent::rejected)return false;
    if(w.stage()!=BoomshotStage::take_ammunition||w.held_units()!=0||w.inventory().loaded!=0)return false;
    if(w.grab_left_shoulder(live_frame(w,7),boom_carrier())!=BoomshotEvent::rejected)return false;
    return w.grab_left_shoulder(live_frame(w,8),boom_carrier(101))==BoomshotEvent::grabbed;
}
constexpr bool boom_unverified_completion_retained(){
    for(unsigned n=0;n<3;++n){
        BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{},saved{};
        if(!to_insertion(w)||!w.take_reload_request(q)||w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed)return false;
        if(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q))return false;
        auto r=boom_ack(w,q,8);if(n==0)r.original_ready=false;
        if(n==1)r.result_applied=ReloadTxnResult::none;if(n==2)r.original_effect_applied_once=false;
        if(w.acknowledge_completion(r)!=BoomshotEvent::interrupted||!w.dispatched_reload_request(saved)||saved.token!=q.token||w.take_reload_request(saved))return false;
    }return true;
}
static_assert(boom_rejected_insert_returns_to_carrier());
static_assert(boom_unverified_completion_retained());
}

namespace {
constexpr bool request_loaded_shot(BoomshotInteraction& w){
    if(!w.bind(boom_config(),{boom_id,1,1,11}))return false;
    w.observe(live_frame(w,1));auto f=live_frame(w,2);f.original_can_fire=true;
    return w.request_original_fire(f,1)==BoomshotEvent::request_fire;
}
constexpr BoomshotShotReceipt shot_result(BoomshotInteraction const& w,BoomshotFireRequest const& q,
                                        std::uint64_t seq,bool accepted){
    auto i=w.inventory();if(accepted){++i.revision;i.loaded=0;}
    return {{live_frame(w,seq).stamp,q.token,seq,i,accepted,accepted},accepted?1u:0u,accepted};
}
constexpr bool interrupted_shot(bool accepted){
    BoomshotInteraction w;BoomshotFireRequest q{},again{};
    if(!request_loaded_shot(w)||!w.take_fire_request(q)||w.take_fire_request(again))return false;
    if(w.interrupt()!=BoomshotEvent::interrupted||w.reset()||w.resume_ready(live_frame(w,3),true,true))return false;
    auto r=shot_result(w,q,3,accepted);
    if(w.after_original_shot(r)!=(accepted?BoomshotEvent::shot_recorded:BoomshotEvent::rejected))return false;
    if(w.stage()!=BoomshotStage::interrupted||w.inventory().loaded!=(accepted?0u:1u))return false;
    if(w.after_original_shot(r)!=BoomshotEvent::rejected||w.resume_ready(live_frame(w,4),false,true)||
       w.resume_ready(live_frame(w,4),true,false))return false;
    return w.resume_ready(live_frame(w,4),true,true)&&w.stage()==BoomshotStage::ready&&!w.take_fire_request(again);
}
constexpr bool undispatched_shot_cancelled(){
    BoomshotInteraction w;BoomshotFireRequest q{};
    if(!request_loaded_shot(w))return false;
    q=w.fire_request();
    if(w.after_original_shot(shot_result(w,q,3,true))!=BoomshotEvent::rejected)return false;
    if(w.interrupt()!=BoomshotEvent::interrupted||w.fire_request().token||w.take_fire_request(q))return false;
    return w.inventory().loaded==1&&w.resume_ready(live_frame(w,3),true,true);
}
constexpr bool wrong_shot_keeps_request(){
    BoomshotInteraction w;BoomshotFireRequest q{};
    if(!request_loaded_shot(w)||!w.take_fire_request(q))return false;
    auto wrong=shot_result(w,q,3,true);wrong.original_projectiles_spawned=2;
    if(w.after_original_shot(wrong)!=BoomshotEvent::interrupted||w.fire_request().token!=q.token||
       w.resume_ready(live_frame(w,4),true,true)||w.take_fire_request(q))return false;
    return w.after_original_shot(shot_result(w,q,4,true))==BoomshotEvent::shot_recorded&&
        w.resume_ready(live_frame(w,5),true,true);
}
constexpr bool reload_after_shot(){
    BoomshotInteraction w;BoomshotFireRequest q{};
    if(!request_loaded_shot(w)||!w.take_fire_request(q)||
       w.after_original_shot(shot_result(w,q,3,true))!=BoomshotEvent::shot_recorded)return false;
    w.observe(live_frame(w,4));auto s=boom_start(w.binding_token());
    return w.begin_reload(live_frame(w,5),s,1,ReloadIntentSource::mechanism_open)==BoomshotEvent::started&&
        w.step_index()==0&&w.inventory().loaded==0;
}
constexpr bool closed_rebind_preserves_tokens(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest old{},q{};
    if(!to_insertion(w)||!w.take_reload_request(old)||w.acknowledge_inventory(boom_ack(w,old,6))!=BoomshotEvent::inventory_committed)return false;
    if(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q)||
       w.acknowledge_completion(boom_ack(w,q,8))!=BoomshotEvent::reload_completed||!w.reset())return false;
    auto c=boom_config();++c.identity.generation;c.after_shot.identity=c.identity;
    if(!w.bind(c,{c.identity,1,0,11})||w.binding_token()==1)return false;
    auto f=live_frame(w,1);f.stamp.identity=c.identity;
    w.observe(f);f.stamp.sequence=2;f.stamp.game_tick=2;f.stamp.monotonic_ns=20;
    auto s=boom_start(w.binding_token());s.plan.identity=c.identity;
    if(w.begin_reload(f,s,2,ReloadIntentSource::mechanism_open)!=BoomshotEvent::started)return false;
    f.stamp.sequence=3;f.stamp.game_tick=3;f.stamp.monotonic_ns=30;
    if(w.complete_mechanism(f,boom_motion(w,2))!=BoomshotEvent::step_completed)return false;
    auto carrier=boom_carrier();carrier.binding=w.binding_token();
    f.stamp.sequence=4;f.stamp.game_tick=4;f.stamp.monotonic_ns=40;
    if(w.grab_left_shoulder(f,carrier)!=BoomshotEvent::rejected)return false;
    carrier.item_serial=101;f.stamp.sequence=5;f.stamp.game_tick=5;f.stamp.monotonic_ns=50;
    if(w.grab_left_shoulder(f,carrier)!=BoomshotEvent::grabbed)return false;
    f.stamp.sequence=6;f.stamp.game_tick=6;f.stamp.monotonic_ns=60;
    if(w.insert_ammunition(f,{boom_motion(w,5),101,true,true,true})!=BoomshotEvent::request_inventory||!w.take_reload_request(q))return false;
    return q.token>old.token&&q.identity==c.identity;
}
static_assert(interrupted_shot(true));
static_assert(interrupted_shot(false));
static_assert(undispatched_shot_cancelled());
static_assert(wrong_shot_keeps_request());
static_assert(reload_after_shot());
static_assert(closed_rebind_preserves_tokens());
}

namespace {
constexpr bool repeat_reload_fire_reload(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{};
    if(!to_insertion(w)||!w.take_reload_request(q)||w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed)return false;
    if(w.complete_mechanism(live_frame(w,7),boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q)||
       w.acknowledge_completion(boom_ack(w,q,8))!=BoomshotEvent::reload_completed)return false;
    w.observe(live_frame(w,9));auto f=live_frame(w,10);f.original_can_fire=true;BoomshotFireRequest fire{};
    if(w.request_original_fire(f,1)!=BoomshotEvent::request_fire||!w.take_fire_request(fire)||
       w.after_original_shot(shot_result(w,fire,11,true))!=BoomshotEvent::shot_recorded)return false;
    w.observe(live_frame(w,12));auto s=boom_start(w.binding_token());
    // Prior serial remains forbidden even with a fresh input intent.
    if(w.begin_reload(live_frame(w,13),s,2,ReloadIntentSource::mechanism_open)!=BoomshotEvent::rejected)return false;
    s.original_reload_serial=2;s.perfect={boom_id,5,2,3,true};
    if(w.begin_reload(live_frame(w,14),s,2,ReloadIntentSource::mechanism_open)!=BoomshotEvent::started)return false;
    auto m=boom_motion(w,14);m.operation_serial=2;
    if(w.complete_mechanism(live_frame(w,15),m)!=BoomshotEvent::step_completed)return false;
    auto a=boom_carrier(101);a.original_reload_serial=2;
    if(w.grab_left_shoulder(live_frame(w,16),a)!=BoomshotEvent::grabbed)return false;
    m=boom_motion(w,16);m.operation_serial=2;
    if(w.insert_ammunition(live_frame(w,17),{m,101,true,true,true})!=BoomshotEvent::request_inventory||!w.take_reload_request(q))return false;
    if(q.reload_serial!=2||q.before.loaded||q.before.reserve!=10||
       w.acknowledge_inventory(boom_ack(w,q,18))!=BoomshotEvent::inventory_committed)return false;
    m=boom_motion(w,18);m.operation_serial=2;
    if(w.complete_mechanism(live_frame(w,19),m)!=BoomshotEvent::request_completion||!w.take_reload_request(q))return false;
    return q.reload_serial==2&&q.result==ReloadTxnResult::perfect&&
        w.acknowledge_completion(boom_ack(w,q,20))==BoomshotEvent::reload_completed&&
        w.inventory().loaded==1&&w.inventory().reserve==9&&w.stage()==BoomshotStage::ready;
}
static_assert(repeat_reload_fire_reload());
}

namespace {
// Ported from production mechanical_and_ammo_proofs and receipt guards.
// Values follow W07 (one carrier/round), not the old synthetic capacity-seven fixture.
constexpr bool legacy_begin(BoomshotInteraction& w){
    if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
    w.observe(live_frame(w,1));return w.begin_r3(live_frame(w,2),
        {w.binding_token(),1,1,boom_plan(),{},true,true})==BoomshotEvent::started;
}
constexpr bool legacy_motion_guards(){
    for(unsigned n=0;n<17;++n){BoomshotInteraction w;if(!legacy_begin(w))return false;
        auto r=boom_motion(w,2);auto f=live_frame(w,3);
        if(n==0)++r.binding;if(n==1)++r.operation_serial;if(n==2)++r.plan_id;
        if(n==3)++r.phase_id;if(n==4)++r.geometry_id;if(n==5)++r.motion_epoch;
        if(n==6)r.begin_sequence=1;if(n==7)r.begin_sequence=3;if(n==8)r.begin_ns=0;
        if(n==9)r.begin_ns=30;if(n==10)r.begin_ns=29;if(n==11)r.player_driven=false;
        if(n==12)r.continuous=false;if(n==13)r.endpoints_and_direction_verified=false;
        if(n==14)r.left_grip_held=false;if(n==15)f.left_tracked=false;if(n==16)f.right_tracked=false;
        auto e=w.complete_mechanism(f,r);
        if((e!=BoomshotEvent::rejected&&e!=BoomshotEvent::motion_reset)||w.step_index()!=0)return false;
    }return true;
}
constexpr bool legacy_insert_guards(){
    for(unsigned n=0;n<4;++n){BoomshotInteraction w;if(!legacy_begin(w))return false;
        if(w.complete_mechanism(live_frame(w,3),boom_motion(w,2))!=BoomshotEvent::step_completed||
           w.grab_left_shoulder(live_frame(w,4),boom_carrier())!=BoomshotEvent::grabbed)return false;
        BoomshotInsertionReceipt r{boom_motion(w,4),100,true,true,true};
        if(n==0)++r.item_serial;if(n==1)r.approached_from_outside=false;if(n==2)r.aligned=false;if(n==3)r.seated=false;
        BoomshotInteraction::ReloadRequest q{};
        if(w.insert_ammunition(live_frame(w,5),r)!=BoomshotEvent::rejected||w.inventory().loaded||w.take_reload_request(q))return false;
    }return true;
}
constexpr bool legacy_inventory_guards(){
    for(unsigned n=0;n<5;++n){BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{},saved{};
        if(!to_insertion(w)||!w.take_reload_request(q))return false;auto r=boom_ack(w,q,6);
        if(n==0)--r.observed.revision;if(n==1)++r.observed.loaded;if(n==2)++r.observed.reserve;
        if(n==3)++r.observed.identity.weapon;if(n==4)r.original_effect_applied_once=false;
        if(w.acknowledge_inventory(r)!=BoomshotEvent::interrupted||w.inventory().loaded||
           !w.dispatched_reload_request(saved)||saved.token!=q.token||w.take_reload_request(saved))return false;
    }return true;
}
constexpr bool legacy_fire_guards(){
    for(unsigned n=0;n<5;++n){BoomshotInteraction w;BoomshotFireRequest q{};
        if(!request_loaded_shot(w)||!w.take_fire_request(q))return false;auto r=shot_result(w,q,3,true);
        if(n==0)r.original.original_effect_applied_once=false;if(n==1)r.original_recoil_applied=false;
        if(n==2)++r.original_projectiles_spawned;if(n==3)++r.original.observed.loaded;if(n==4)--r.original.observed.reserve;
        if(w.after_original_shot(r)!=BoomshotEvent::interrupted||w.can_request_fire()||w.fire_request().token!=q.token)return false;
    }return true;
}
constexpr bool legacy_tracking_guards(){
    for(unsigned n=0;n<6;++n){BoomshotInteraction w;if(!legacy_begin(w))return false;
        auto old=boom_motion(w,2);auto bad=live_frame(w,3);
        if(n==0)bad.left_tracked=false;if(n==1)bad.stamp.motion_epoch=2;
        if(n==2)bad.stamp.monotonic_ns+=2000;if(n==3)--bad.stamp.sequence;
        if(n==4)bad.stamp.game_tick=1;if(n==5)w.tracking_lost();
        auto e=w.complete_mechanism(bad,old);
        if(e!=BoomshotEvent::rejected&&e!=BoomshotEvent::motion_reset)return false;
        auto recovered=live_frame(w,4);auto next=live_frame(w,5);
        if(n==1){recovered.stamp.motion_epoch=2;next.stamp.motion_epoch=2;}
        if(n==2){recovered.stamp.monotonic_ns+=2000;next.stamp.monotonic_ns+=2000;}
        e=w.observe(recovered);if(e!=BoomshotEvent::none&&e!=BoomshotEvent::motion_reset)return false;
        if(w.complete_mechanism(next,old)!=BoomshotEvent::rejected||w.step_index()!=0||w.inventory().loaded)return false;
    }return true;
}
constexpr bool legacy_closing_proof(){
    BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{};
    if(!to_insertion(w)||!w.take_reload_request(q)||w.acknowledge_inventory(boom_ack(w,q,6))!=BoomshotEvent::inventory_committed)return false;
    auto r=boom_motion(w,6);r.return_verified=false;
    return w.complete_mechanism(live_frame(w,7),r)==BoomshotEvent::rejected&&w.step_index()==2&&!w.take_reload_request(q);
}
static_assert(legacy_motion_guards());
static_assert(legacy_insert_guards());
static_assert(legacy_inventory_guards());
static_assert(legacy_fire_guards());
static_assert(legacy_tracking_guards());
static_assert(legacy_closing_proof());
constexpr bool legacy_timing_edges(){
    for(unsigned n=0;n<5;++n){
        BoomshotInteraction w;BoomshotInteraction::ReloadRequest q{};
        if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
        auto frame=[&](std::uint64_t seq){auto f=live_frame(w,seq);f.stamp.game_tick=101;return f;};
        w.observe(frame(1));auto s=boom_start(w.binding_token());s.perfect={boom_id,5,1,30,n!=4};
        if(w.begin_reload(frame(2),s,1,ReloadIntentSource::mechanism_open)!=BoomshotEvent::started||
           w.complete_mechanism(frame(3),boom_motion(w,2))!=BoomshotEvent::step_completed||
           w.grab_left_shoulder(frame(4),boom_carrier())!=BoomshotEvent::grabbed||
           w.insert_ammunition(frame(5),{boom_motion(w,4),100,true,true,true})!=BoomshotEvent::request_inventory||
           !w.take_reload_request(q))return false;
        auto ack=boom_ack(w,q,6);ack.stamp.game_tick=101;
        if(w.acknowledge_inventory(ack)!=BoomshotEvent::inventory_committed)return false;
        auto close=frame(7);close.stamp.game_tick+=n==0?0:n==1?4000000:n==2?4000001:n==3?5000000:1;
        if(w.complete_mechanism(close,boom_motion(w,6))!=BoomshotEvent::request_completion||!w.take_reload_request(q))return false;
        if(q.result!=((n<2||n==4)?ReloadTxnResult::perfect:ReloadTxnResult::ordinary))return false;
    }return true;
}
constexpr bool legacy_animation_handoff(){
    BoomshotInteraction w;if(!w.bind(boom_config(),{boom_id,1,1,11}))return false;
    auto f=live_frame(w,1);f.original_owns_hands=true;f.original_interaction_allowed=false;
    f.left_tracked=f.right_tracked=false;
    if(w.observe(f)!=BoomshotEvent::animation_handoff||!w.animation_owns_hands()||w.can_request_fire())return false;
    f=live_frame(w,2);f.left_tracked=false;
    if(w.observe(f)!=BoomshotEvent::motion_reset||!w.animation_owns_hands())return false;
    f=live_frame(w,3);f.original_can_fire=false;
    return w.observe(f)==BoomshotEvent::restored&&!w.animation_owns_hands()&&!w.can_request_fire();
}
static_assert(legacy_timing_edges());
static_assert(legacy_animation_handoff());
}

namespace {
using E=BoomshotEvent;using S=BoomshotStage;
constexpr bool legacy_begin_setup(BoomshotInteraction& w){
 if(!w.bind(boom_config(),{boom_id,1,0,11}))return false;
 return w.observe(live_frame(w,1))==E::motion_reset;
}
constexpr bool legacy_entry_matrix(){
    for(unsigned invalid=0;invalid<17;++invalid){
        BoomshotInteraction m;auto c=boom_config();BoomshotInventory observed{boom_id,1,0,11};
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
            case 13:c.after_shot=boom_plan();break;
            case 14:observed.revision=0;break;
            case 15:observed.loaded=8;break;
            case 16:observed.identity.generation++;break;
        }
        if(!(!m.bind(c,observed)))return false;if(!(m.stage()==S::unbound))return false;
    }
    for(unsigned invalid=0;invalid<18;++invalid){
        BoomshotInteraction w;if(!legacy_begin_setup(w))return false;auto s=boom_start(w.binding_token());s.r3_press_edge=1;s.perfect={boom_id,5,1,30,true};
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
            case 10:s.plan.condition=BoomshotCondition::partial;break;
            case 11:s.plan.steps[1].units=0;break;
            case 12:s.plan.steps[2].phase_id=s.plan.steps[0].phase_id;break;
            case 13:s.plan.steps[0].geometry_id=0;break;
            case 14:s.perfect.original_reload_serial++;break;
            case 15:s.perfect.identity.generation++;break;
            case 16:s.perfect.end_ticks=-1;break;
            case 17:s.plan.count=13;break;
        }
        const auto event=w.begin_r3(live_frame(w,2),s);
        if(!(event==E::rejected||event==E::invalid))return false;if(!(w.stage()==S::ready))return false;
    }
return true;}
static_assert(legacy_entry_matrix());
constexpr bool legacy_guidance_conservation(){
 BoomshotInteraction w;if(!legacy_begin_setup(w)||w.guide().part!=BoomshotPart::none)return false;
 if(w.begin_reload(live_frame(w,2),boom_start(w.binding_token()),1,ReloadIntentSource::mechanism_open)!=E::started)return false;
 auto g=w.guide();if(g.part!=BoomshotPart::barrel||g.red!=1||g.green!=1||g.blue!=0||g.alpha!=1)return false;
 if(w.complete_mechanism(live_frame(w,3),boom_motion(w,2))!=E::step_completed||w.guide().part!=BoomshotPart::left_shoulder)return false;
 if(w.grab_left_shoulder(live_frame(w,4),boom_carrier())!=E::grabbed||w.guide().part!=BoomshotPart::feed_port||
    w.held_units()!=1||w.available_reserve()!=10||w.inventory().loaded||w.inventory().reserve!=11)return false;
 if(w.release_ammunition(live_frame(w,5),100)!=E::released||w.available_reserve()!=11||w.inventory().reserve!=11)return false;
 if(w.grab_left_shoulder(live_frame(w,6),boom_carrier())!=E::rejected||
    w.grab_left_shoulder(live_frame(w,7),boom_carrier(101))!=E::grabbed)return false;
 if(w.insert_ammunition(live_frame(w,8),{boom_motion(w,7),101,true,true,true})!=E::request_inventory)return false;
 BoomshotInteraction::ReloadRequest q{},saved{};if(!w.take_reload_request(q)||w.take_reload_request(saved)||
    !w.dispatched_reload_request(saved)||saved.token!=q.token||q.before.loaded||q.before.reserve!=11||q.expected.loaded!=1||q.expected.reserve!=10)return false;
 auto ack=boom_ack(w,q,9);
 if(w.acknowledge_inventory(ack)!=E::inventory_committed||w.acknowledge_inventory(ack)!=E::rejected||
    w.guide().part!=BoomshotPart::barrel||w.inventory().loaded+w.inventory().reserve!=11)return false;
 if(w.complete_mechanism(live_frame(w,10),boom_motion(w,9))!=E::request_completion||!w.take_reload_request(q)||
    w.guide().part!=BoomshotPart::none||w.can_request_fire())return false;
 return w.acknowledge_completion(boom_ack(w,q,11))==E::reload_completed&&w.inventory().loaded+w.inventory().reserve==11;
}
constexpr bool legacy_full_dry(){
 BoomshotInteraction full,dry;
 if(!full.bind(boom_config(),{boom_id,1,1,11})||!dry.bind(boom_config(),{boom_id,1,0,0}))return false;
 full.observe(live_frame(full,1));dry.observe(live_frame(dry,1));
 return full.begin_reload(live_frame(full,2),boom_start(full.binding_token()),1,ReloadIntentSource::mechanism_open)==E::full&&
   dry.begin_reload(live_frame(dry,2),boom_start(dry.binding_token()),1,ReloadIntentSource::mechanism_open)==E::no_ammo;
}
static_assert(legacy_guidance_conservation());
static_assert(legacy_full_dry());
}

static_assert(!std::is_copy_constructible<gears_weapon::BoomshotInteraction>::value);
static_assert(!std::is_copy_assignable<gears_weapon::BoomshotInteraction>::value);
int main(){
    const bool a=legacy_timing_edges(),b=closed_rebind_preserves_tokens(),c=boom_exchange_sequence();
    const bool d=boom_late_insert(),e=boom_late_close();
    std::printf("d3 exchange checks: timing=%u rebind=%u exchange=%u late_insert=%u late_close=%u\n",
        unsigned(a),unsigned(b),unsigned(c),unsigned(d),unsigned(e));
    return a&&b&&c&&d&&e?0:1;
}

