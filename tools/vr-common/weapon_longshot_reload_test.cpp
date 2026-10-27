#include "weapon_longshot_reload.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <utility>
using namespace gears_weapon;
using E=LongshotReloadEvent;
using S=LongshotReloadStage;
using A=LongshotAction;
namespace {
unsigned checks{};
void check(bool ok,char const* message){
    ++checks;if(!ok){std::fprintf(stderr,"Longshot reload failed: %s\n",message);std::abort();}
}
// Synthetic externally authenticated plan, NOT measured Longshot order/travel.
constexpr LongshotReloadIdentity id{1,2,3,4,5,6,7,8,9};
constexpr std::int64_t now=1000000000;
LongshotReloadStamp stamp(std::uint64_t sequence,std::int64_t tick=1000){
    return {id,sequence,now,tick,true,true,true};
}
LongshotReloadStart start(bool known=true){
    LongshotReloadStart s{};s.stamp=stamp(1);s.r3_press_edge=1;
    s.plan.identity=id;s.plan.generation=10;s.plan.original_reload_serial=1;
    s.plan.initial_state=20;s.plan.final_ready_state=24;s.plan.count=5;
    s.plan.current_instance_verified=true;
    s.plan.steps[0]={A::open_breech,51,61,20,21,true};
    s.plan.steps[1]={A::eject_case,51,61,21,22,true};
    s.plan.steps[2]={A::take_round,0,0,22,22,true};
    s.plan.steps[3]={A::insert_round,52,62,22,23,true};
    s.plan.steps[4]={A::close_breech,51,61,23,24,true};
    s.inventory={id,1,0,5};s.perfect={id,1,10,known?100:0,known};
    s.original_class_verified=s.single_round_capacity_verified=s.original_reload_accepted=
        s.automatic_refill_held=s.inventory_reconciled=true;
    return s;
}
LongshotRoundReceipt round(std::uint64_t item=101){
    return {item,LongshotAmmoSource::left_shoulder,LongshotAmmoKind::live_longshot_round,true};
}
LongshotStepReceipt receipt(LongshotReloadStart const& s,unsigned i,std::uint64_t since){
    const auto p=s.plan.steps[i];
    return {s.plan.generation,s.plan.original_reload_serial,p.part,p.calibration,since,
        p.before_state,p.after_state,101,i,p.action,true,true,true,true};
}
void opened(LongshotReload& m,LongshotReloadStart const& b=start()){
    check(m.begin_r3(b,now)==E::started,"R3 accepted");
    check(m.complete_step(stamp(2),receipt(b,0,1),now)==E::step_completed,"external opening receipt");
    check(m.complete_step(stamp(3),receipt(b,1,2),now)==E::step_completed,"external ejection receipt");
}
void holding(LongshotReload& m,LongshotReloadStart const& b=start()){
    opened(m,b);check(m.grab_left_shoulder(stamp(4),round(),now)==E::round_grabbed,"single left-shoulder round reserved");
}
LongshotInventoryRequest pending(LongshotReload& m,LongshotReloadStart const& b=start()){
    holding(m,b);
    check(m.complete_step(stamp(5),receipt(b,3,4),now)==E::inventory_requested,"seated round requests original inventory transfer");
    LongshotInventoryRequest q{};check(m.take_inventory_request(q),"inventory request delivered once");
    check(!m.take_inventory_request(q),"inventory request not redispatched");return q;
}
void seated(LongshotReload& m,LongshotReloadStart const& b=start()){
    const auto q=pending(m,b);
    check(m.acknowledge_inventory(stamp(6),{q.token,1,true,{id,2,1,4}},now)==E::inventory_committed,
        "exact original inventory receipt acknowledges one round");
}
LongshotCompletionRequest physical_finish(LongshotReload& m,LongshotReloadStart const& b=start(),std::int64_t finish=1100){
    seated(m,b);
    check(m.complete_step(stamp(7,finish),receipt(b,4,6),now)==E::completion_requested,"last necessary action requests original completion");
    LongshotCompletionRequest q{};check(m.take_completion_request(q),"completion request delivered once");
    check(!m.take_completion_request(q),"completion request never redispatched");return q;
}
LongshotCompletionReceipt completion(LongshotCompletionRequest q){
    return {q.token,q.original_reload_serial,q.plan_generation,q.final_ready_state,1,true,true,{id,2,1,4}};
}
void normal_flow(){
    LongshotReload m;auto b=start();
    check(!m.vr_reload_gate_clear()&&m.guide().region==LongshotGuideRegion::none,"unbound reload has no fire authority or guide");
    check(m.begin_r3(b,now)==E::started,"start empty single-shot gun");
    auto g=m.guide();check(g.action==A::open_breech&&g.part==51&&g.red==1&&g.green==1&&g.blue==0&&g.alpha==1,"yellow current mechanical part");
    check(m.begin_r3(b,now)==E::rejected,"repeat R3 cannot reset clock or steps");
    check(m.complete_step(stamp(2),receipt(b,0,1),now)==E::step_completed,"open");
    check(m.guide().action==A::eject_case,"ejection not skipped");
    check(m.complete_step(stamp(3),receipt(b,1,2),now)==E::step_completed,"eject");
    check(m.guide().region==LongshotGuideRegion::left_shoulder,"yellow left shoulder only at take step");
    check(m.grab_left_shoulder(stamp(4),round(),now)==E::round_grabbed,"grab");
    check(m.held_round()==101&&m.reserved_rounds()==1&&m.available_reserve()==4&&m.inventory().reserve==5&&m.inventory().loaded==0,
        "reservation cannot create loaded ammo or alter authoritative reserve");
    check(m.guide().part==52&&m.guide().action==A::insert_round,"yellow certified feed part");
    check(m.complete_step(stamp(5),receipt(b,3,4),now)==E::inventory_requested,"insertion");
    check(m.inventory().loaded==0&&!m.vr_reload_gate_clear()&&m.guide().region==LongshotGuideRegion::none,"unacknowledged commit grants no ammo/fire/next guide");
    LongshotInventoryRequest q{};check(m.take_inventory_request(q),"inventory commit intent");
    check(q.before_loaded==0&&q.before_reserve==5&&q.after_loaded==1&&q.after_reserve==4&&q.item_serial==101,"single round conservation");
    check(m.acknowledge_inventory(stamp(6),{q.token,1,true,{id,2,1,4}},now)==E::inventory_committed,"authoritative original commit");
    check(m.inventory().loaded==1&&m.inventory().reserve==4&&!m.held_round()&&!m.vr_reload_gate_clear(),"seated round still waits for final mechanical and original state");
    check(m.guide().action==A::close_breech,"yellow closure follows certified plan");
    check(m.complete_step(stamp(7,1100),receipt(b,4,6),now)==E::completion_requested,"inclusive endpoint physical finish");
    LongshotCompletionRequest finish{};check(m.take_completion_request(finish),"take final original handoff");
    check(finish.proposed_result==E::perfect&&finish.started_tick==1000&&finish.finished_tick==1100,"current-instance inclusive perfect endpoint");
    check(!m.vr_reload_gate_clear(),"proposal alone cannot clear reload gate");
    check(m.acknowledge_completion(stamp(8,9000),completion(finish),now)==E::perfect,"native acknowledgment delay does not change physical result");
    check(m.vr_reload_gate_clear()&&m.guide().region==LongshotGuideRegion::none,"original ready receipt clears only VR reload gate");
    check(m.acknowledge_completion(stamp(9,9000),completion(finish),now)==E::rejected,"original completion/reward not accepted twice");
}
void timing(){
    for(auto end:{1000LL,1100LL,1101LL,9000LL}){
        LongshotReload m;auto q=physical_finish(m,start(),end);
        check(q.proposed_result==(end<=1100?E::perfect:E::ordinary),"earlier/equal/later completion follows verified game clock");
    }
    LongshotReload unknown;check(physical_finish(unknown,start(false)).proposed_result==E::ordinary,"unknown perfect endpoint permits ordinary reload");
    LongshotReload m;auto b=start();opened(m,b);
    auto paused=stamp(4);check(m.observe(paused,now)==E::none,"tracking update while game clock paused does not advance plan");
    check(m.step_index()==2,"observation cannot finish mechanical step");
    // A certified plan can identify insertion itself as the last required
    // physical step. A delayed native ack must not replace its completion time.
    b=start();b.plan.count=2;
    b.plan.steps[0]={A::take_round,0,0,20,20,true};
    b.plan.steps[1]={A::insert_round,52,62,20,24,true};
    LongshotReload insertion_last;
    check(insertion_last.begin_r3(b,now)==E::started,"no unverified mandatory closure is invented");
    check(insertion_last.grab_left_shoulder(stamp(2),round(),now)==E::round_grabbed,"short external plan grab");
    check(insertion_last.complete_step(stamp(3,1100),receipt(b,1,2),now)==E::inventory_requested,"insertion is certified last physical action");
    LongshotInventoryRequest iq{};check(insertion_last.take_inventory_request(iq),"take final-insertion commit");
    check(insertion_last.acknowledge_inventory(stamp(4,9000),{iq.token,1,true,{id,2,1,4}},now)==E::completion_requested,"delayed last-insertion receipt");
    LongshotCompletionRequest cq{};check(insertion_last.take_completion_request(cq)&&cq.finished_tick==1100&&cq.proposed_result==E::perfect,
        "inventory acknowledgment delay does not consume physical perfect window");
}
void plan_and_ammo_gates(){
    auto reject=[](LongshotReloadStart b){LongshotReload m;return m.begin_r3(b,now);};
    auto b=start();b.plan.current_instance_verified=false;check(reject(b)==E::invalid,"static track names cannot certify live order");
    b=start();b.plan.steps[1].before_state=99;check(reject(b)==E::invalid,"disconnected mechanical state chain");
    b=start();b.plan.steps[2].action=A::insert_round;check(reject(b)==E::invalid,"insert before grabbing rejected");
    b=start();b.plan.steps[1].action=A::unknown;check(reject(b)==E::invalid,"unexplained phase rejected");
    b=start();b.single_round_capacity_verified=false;check(reject(b)==E::invalid,"capacity must be current-instance certified");
    b=start();b.automatic_refill_held=false;check(reject(b)==E::invalid,"automatic original refill conflict rejected");
    b=start();b.inventory.loaded=1;check(reject(b)==E::full,"loaded Longshot cannot create another round");
    b=start();b.inventory.reserve=0;check(reject(b)==E::no_ammo,"zero reserve returns no-ammo without fake round");
    b=start();++b.perfect.original_reload_serial;check(reject(b)==E::invalid,"perfect deadline from older/different reload rejected");
    b=start();++b.perfect.plan_generation;check(reject(b)==E::invalid,"perfect deadline bound to current mechanical plan");
    LongshotReload m;opened(m);auto r=round();r.kind=LongshotAmmoKind::spent_case;
    check(m.grab_left_shoulder(stamp(4),r,now)==E::rejected,"spent Longshot case is not live ammunition");
    r=round();r.source=LongshotAmmoSource::other;check(m.grab_left_shoulder(stamp(4),r,now)==E::rejected,"wrong body slot");
    r=round();r.live_round_identity_verified=false;check(m.grab_left_shoulder(stamp(4),r,now)==E::rejected,"unverified round resource");
    check(m.grab_left_shoulder(stamp(4),round(),now)==E::round_grabbed,"valid round after rejected inputs");
    check(m.release_round(stamp(5),101,now)==E::round_released&&m.inventory().reserve==5&&!m.reserved_rounds()&&m.needs_inventory_reconciliation(),
        "released round restores reservation and requires new plan instead of guessing mechanical undo");
    // Different certified phase order is deliberately accepted. This fixture
    // does NOT assert that taking a round first is the actual game's order.
    b=start();std::swap(b.plan.steps[0].action,b.plan.steps[1].action);
    LongshotReload other;check(other.begin_r3(b,now)==E::started,"external order is not replaced by a hardcoded opening/ejection assumption");
}
void receipts_tracking_and_replay(){
    auto b=start();LongshotReload m;opened(m,b);
    check(m.complete_step(stamp(4),receipt(b,4,3),now)==E::rejected,"out-of-order closing receipt");
    check(m.grab_left_shoulder(stamp(4),round(),now)==E::round_grabbed,"grab for insertion checks");
    auto r=receipt(b,3,4);r.calibration=99;check(m.complete_step(stamp(5),r,now)==E::rejected,"wrong feed calibration");
    r=receipt(b,3,4);r.item_serial=102;check(m.complete_step(stamp(5),r,now)==E::rejected,"wrong held item");
    r=receipt(b,3,4);r.motion_begin_sequence=1;check(m.complete_step(stamp(5),r,now)==E::rejected,"stroke from earlier stage cannot insert");
    r=receipt(b,3,4);r.continuous=false;check(m.complete_step(stamp(5),r,now)==E::rejected,"teleport/non-continuous hand motion");
    r=receipt(b,3,4);r.complete_travel=false;check(m.complete_step(stamp(5),r,now)==E::rejected,"incomplete travel");
    auto stale=stamp(5);stale.observed_ns-=100000001;check(m.complete_step(stale,receipt(b,3,4),now)==E::rejected,"old pose receipt");
    auto foreign=stamp(5);++foreign.identity.space_epoch;check(m.complete_step(foreign,receipt(b,3,4),now)==E::rejected,"recenter changed identity");
    auto time_back=stamp(5,999);check(m.complete_step(time_back,receipt(b,3,4),now)==E::rejected,"game clock reversal");
    auto lost=stamp(5);lost.left_tracked=false;check(m.observe(lost,now)==E::interrupted,"loss of tracking ends reload without pausing clock");
    check(!m.held_round()&&m.inventory().reserve==5&&!m.vr_reload_gate_clear(),"tracking loss releases reservation without granting ammo");
    check(m.complete_step(stamp(6),receipt(b,3,4),now)==E::rejected,"old stroke cannot resume interrupted reload");
    check(m.begin_r3(b,now)==E::rejected,"old R3 cannot revive attempt");
    auto fresh=b;fresh.stamp=stamp(6);fresh.r3_press_edge=2;fresh.plan.original_reload_serial=2;
    fresh.perfect.original_reload_serial=2;
    check(m.begin_r3(fresh,now)==E::started,"fresh accepted original reload after reconciliation");
    check(m.complete_step(stamp(7),receipt(b,0,6),now)==E::rejected,"prior original reload serial rejected");
    auto live=stamp(8);++live.identity.input_epoch;check(m.observe(live,now)==E::interrupted,"live input epoch change invalidates attempt");
    auto older=fresh;older.stamp=stamp(9);older.r3_press_edge=3;older.plan.original_reload_serial=3;
    older.perfect.original_reload_serial=3;--older.stamp.identity.binding_generation;
    older.plan.identity=older.inventory.identity=older.perfect.identity=older.stamp.identity;
    check(m.begin_r3(older,now)==E::rejected,"old binding never revives prior world");
}
void original_commit_gates(){
    auto b=start();LongshotReload m;auto q=pending(m,b);
    auto ack=LongshotInventoryReceipt{q.token,1,true,{id,2,1,4}};
    auto bad=ack;++bad.token;check(m.acknowledge_inventory(stamp(6),bad,now)==E::rejected,"wrong inventory request token");
    bad=ack;bad.observed.reserve=5;check(m.acknowledge_inventory(stamp(6),bad,now)==E::interrupted,"duplicated ammo receipt invalidates transaction");
    check(m.acknowledge_inventory(stamp(7),ack,now)==E::rejected,"late native receipt after interruption rejected");
    LongshotReload denied;q=pending(denied,b);
    check(denied.acknowledge_inventory(stamp(6),{q.token,1,false,{id,1,0,5}},now)==E::rejected&&denied.stage()==S::step&&denied.held_round()==101,
        "denied unchanged inventory keeps held reservation for a new insertion stroke");
    check(denied.complete_step(stamp(7),receipt(b,3,4),now)==E::rejected,"old insertion motion cannot retry native commit");
    check(denied.complete_step(stamp(8),receipt(b,3,6),now)==E::inventory_requested,"fresh insertion can retry after rejection");
    LongshotInventoryRequest retry{};check(denied.take_inventory_request(retry)&&retry.token!=q.token,"retry receives fresh token");
    check(denied.acknowledge_inventory(stamp(9),{q.token,1,true,{id,2,1,4}},now)==E::rejected,"old denied request cannot satisfy retry");
    LongshotReload completion_model;const auto c=physical_finish(completion_model);
    auto cack=completion(c);cack.original_ready=false;
    check(completion_model.acknowledge_completion(stamp(8,1100),cack,now)==E::interrupted,"no ready proof means no completion/fire permission");
    LongshotReload no_equality;auto inv=pending(no_equality);ack={inv.token,1,true,{id,1,1,4}};
    check(no_equality.acknowledge_inventory(stamp(6),ack,now)==E::interrupted,"inventory commit requires newer authoritative revision");
}
} // namespace
int main(){normal_flow();timing();plan_and_ammo_gates();receipts_tracking_and_replay();original_commit_gates();
    std::printf("Longshot single-round reload: %u synthetic plan, ammo, inventory, deadline and replay checks passed\n",checks);
}
