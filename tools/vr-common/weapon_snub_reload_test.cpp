#include "weapon_snub_reload.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
using E=SnubReloadEvent;using S=SnubReloadStage;
namespace {
// Synthetic receipts only: no travel, count or timing is runtime evidence.
constexpr SnubReloadIdentity id{11,12,13,14};
SnubReloadStamp stamp(std::uint64_t seq,std::int64_t tick=1000,std::uint64_t epoch=1){
    return {id,seq,epoch,tick};
}
SnubReloadStart start(unsigned loaded=4,unsigned reserve=2,bool perfect=true){
    SnubReloadStart s{};s.stamp=stamp(1);s.intent_id=1;s.inventory_revision=1;
    s.old_magazine_item=70;s.capacity=6;s.loaded=loaded;s.reserve=reserve;
    const auto q=(std::min)(6u-loaded,reserve);
    s.original_reload_serial=q?21:0;s.original_reload_accepted=q!=0;
    s.original_class_verified=s.right_hand_equipped=true;s.original_inventory_slot=0;
    s.plan.identity=id;s.plan.id=31;s.plan.feed_mount_id=41;
    s.plan.condition=loaded?SnubReloadCondition::partial:SnubReloadCondition::empty;
    s.plan.count=loaded?2:3;s.plan.original_order_verified=s.plan.current_instance_verified=true;
    s.plan.steps[0]={SnubStepKind::eject_magazine,snub_part(SnubPart::magazine),51,0};
    s.plan.steps[1]={SnubStepKind::insert_magazine,snub_part(SnubPart::feed_port),52,0};
    s.plan.steps[2]={SnubStepKind::move_original_parts,static_cast<SnubPartMask>(
        snub_part(SnubPart::slide)|snub_part(SnubPart::left_wing)|snub_part(SnubPart::right_wing)),53,61};
    s.perfect={id,s.original_reload_serial,perfect&&q?100:0,perfect&&q!=0};return s;
}
SnubEjectionReceipt eject(std::uint64_t serial=21){return {serial,31,51,true,true};}
SnubMagazineReceipt mag(std::uint64_t item=71){return {item,snub_ammo_slot,true,true,true,true};}
SnubInsertionReceipt insert(std::uint64_t item=71,std::uint64_t began=4){
    return {item,41,began,true,true,true,true,true,true};
}
SnubPartMotionReceipt motion(std::uint64_t began=6){
    return {21,31,53,61,began,static_cast<SnubPartMask>(
        snub_part(SnubPart::slide)|snub_part(SnubPart::left_wing)|snub_part(SnubPart::right_wing)),
        true,true,true,true,true,true};
}
SnubInventoryReceipt accepted(SnubInventoryRequest const& q,std::uint64_t revision){
    auto out=q.expected;out.revision=revision;
    return {q.token,true,out,q.result==ReloadTxnResult::inspection?ReloadTxnResult::none:q.result};
}
void opened(SnubReload& m,SnubReloadStart const& s=start()){
    assert(m.begin(s)==E::started&&!m.can_fire());
    assert(m.confirm_ejection(stamp(2),eject(s.original_reload_serial))==E::ejected);
    assert(m.stage()==S::old_magazine&&m.loaded()==s.loaded&&m.reserve()==s.reserve);
}
SnubInventoryRequest pending_insert(SnubReload& m,SnubReloadStart const& s=start(),std::int64_t insert_tick=1000){
    opened(m,s);assert(m.grab_left_shoulder(stamp(3),mag())==E::rejected);
    assert(m.release_original(stamp(3),70)==E::released);
    assert(m.grab_left_shoulder(stamp(4),mag())==E::grabbed);
    const auto target=(std::min)(s.capacity-s.loaded,s.reserve);
    assert(m.loaded()==s.loaded&&m.reserve()==s.reserve&&m.held()==target);
    assert(m.available_reserve()==s.reserve-target&&m.total()==s.loaded+s.reserve);
    assert(m.insert_magazine(stamp(5,insert_tick),insert())==E::insertion_requested);
    assert(m.stage()==S::inventory_pending&&m.loaded()==s.loaded&&!m.can_fire());
    SnubInventoryRequest q{};assert(m.take_inventory_request(q));
    assert(q.call==ReloadTxnCall::insert&&q.before.loaded==s.loaded&&q.before.reserve==s.reserve);
    assert(!m.take_inventory_request(q)&&!m.reset());return q;
}
void finish(SnubReload& m,std::uint64_t seq,std::int64_t tick,E result){
    SnubReloadCompletion out{};assert(m.take_completion(out));
    assert(out.result==result&&out.native_request.call==ReloadTxnCall::finish);
    assert(!m.take_completion(out)&&!m.can_fire()&&!m.reset());
    auto receipt=accepted(out.native_request,m.inventory().revision);
    auto wrong=receipt;++wrong.token;
    assert(m.acknowledge_completion(stamp(seq,tick),wrong,true)==E::rejected);
    assert(m.acknowledge_completion(stamp(seq,tick),receipt,false)==E::rejected);
    assert(m.acknowledge_completion(stamp(seq,tick),receipt,true)==E::committed);
    assert(m.can_fire()&&!m.take_completion(out));
    assert(m.acknowledge_completion(stamp(seq+1,tick),receipt,true)==E::rejected);
}
void partial_and_empty(){
    SnubReload p;const auto q=pending_insert(p);
    assert(q.expected.loaded==6&&q.expected.reserve==0);
    assert(p.acknowledge_inventory(stamp(6,1040),accepted(q,2))==E::perfect);
    assert(p.condition()==SnubReloadCondition::partial&&p.current_step()==2);
    assert(p.move_original_parts(stamp(7,1041),motion())==E::rejected);
    assert(p.acknowledge_inventory(stamp(7,1041),accepted(q,2))==E::rejected);
    finish(p,7,1041,E::perfect);
    SnubReload empty;const auto eq=pending_insert(empty,start(0,4));
    assert(eq.expected.loaded==4&&eq.expected.reserve==0);
    assert(empty.acknowledge_inventory(stamp(6,1040),accepted(eq,2))==E::inserted);
    assert(empty.loaded()==4&&empty.condition()==SnubReloadCondition::empty&&!empty.can_fire());
    SnubReloadCompletion out{};assert(!empty.take_completion(out));
    auto wrong=motion();wrong.endpoints_verified=false;
    assert(empty.move_original_parts(stamp(7,1045),wrong)==E::rejected);
    wrong=motion();wrong.parts=snub_part(SnubPart::slide);
    assert(empty.move_original_parts(stamp(7,1045),wrong)==E::rejected);
    assert(empty.move_original_parts(stamp(7,1045),motion())==E::perfect);
    finish(empty,8,1046,E::perfect);
}
void old_mag_and_inspection(){
    for(auto s:{start(4,2),start(6,5),start(3,0),start(0,0)}){
        SnubReload m;opened(m,s);
        assert(m.restore_original(stamp(3),insert(70,2))==E::cancelled);
        assert(m.loaded()==s.loaded&&m.reserve()==s.reserve&&m.held()==0);
        SnubReloadCompletion out{};
        if(s.original_reload_serial){
            assert(m.take_completion(out)&&out.native_request.call==ReloadTxnCall::exit_without_fill);
            assert(out.native_request.result==ReloadTxnResult::inspection);
            auto bad=accepted(out.native_request,1);bad.result_applied=ReloadTxnResult::perfect;
            auto ambiguous=m; // An inconsistent native result requires reconciliation.
            assert(ambiguous.acknowledge_completion(stamp(4),bad,true)==E::rejected);
            assert(!ambiguous.can_fire()&&!ambiguous.reset());
            assert(m.acknowledge_completion(stamp(4),accepted(out.native_request,1),true)==E::committed);
        }else assert(m.stage()==S::complete&&!m.take_completion(out));
        assert(m.total()==s.loaded+s.reserve&&m.can_fire()==(s.loaded!=0));
    }
}
void release_retry_and_recovery(){
    SnubReload m;opened(m);
    assert(m.release_original(stamp(3),71)==E::rejected);
    assert(m.release_original(stamp(3),70)==E::released);
    assert(m.grab_left_shoulder(stamp(4),mag())==E::grabbed);
    assert(m.release_magazine(stamp(5),72)==E::rejected);
    assert(m.release_magazine(stamp(5),71)==E::released&&m.reserve()==2&&m.held()==0);
    assert(m.grab_left_shoulder(stamp(6),mag())==E::rejected);
    assert(m.grab_left_shoulder(stamp(6),mag(72))==E::grabbed);
    assert(m.interrupt()==E::interrupted&&m.reserve()==2&&m.held()==0&&!m.reset());
    auto observed=m.inventory();auto wrong=observed;++wrong.loaded;
    assert(m.resume(stamp(7,1001,2),2,21,wrong,true)==E::rejected);
    assert(m.resume(stamp(7,1001,2),1,21,observed,true)==E::rejected);
    assert(m.resume(stamp(7,1001,2),2,21,observed,true)==E::resumed);
    assert(m.grab_left_shoulder(stamp(8,1002,2),mag(73))==E::grabbed);
    assert(m.insert_magazine(stamp(9,1003,2),insert(73,8))==E::insertion_requested);
    SnubInventoryRequest q{};assert(m.take_inventory_request(q));
    assert(m.acknowledge_inventory(stamp(10,1004,2),accepted(q,2))==E::ordinary);
    SnubReloadCompletion out{};assert(m.take_completion(out));
    assert(out.native_request.result==ReloadTxnResult::ordinary&&out.started_tick==1000);
}
void late_call_and_reconciliation(){
    SnubReload m;const auto q=pending_insert(m,start(0,4));
    assert(m.interrupt()==E::interrupted&&m.held()==4&&!m.reset());
    assert(m.resume(stamp(6,1001,2),2,21,m.inventory(),true)==E::rejected);
    assert(m.acknowledge_inventory(stamp(6,1001,2),accepted(q,2))==E::interrupted);
    assert(m.loaded()==4&&m.held()==0&&!m.can_fire());
    assert(m.resume(stamp(7,1002,2),2,21,m.inventory(),true)==E::resumed);
    assert(m.move_original_parts(stamp(8,1003,2),motion(6))==E::rejected);
    assert(m.move_original_parts(stamp(8,1003,2),motion(7))==E::ordinary);
    SnubReloadCompletion out{};assert(m.take_completion(out));
    assert(m.interrupt()==E::interrupted&&!m.reset());
    assert(m.acknowledge_completion(stamp(9,1004,2),accepted(out.native_request,2),true)==E::interrupted);
    assert(!m.can_fire()&&m.resume(stamp(10,1005,3),3,21,m.inventory(),true)==E::committed);
    assert(m.can_fire()&&!m.take_completion(out));
    SnubReload bad;const auto bq=pending_insert(bad);
    auto receipt=accepted(bq,2);++receipt.observed.loaded;
    assert(bad.acknowledge_inventory(stamp(6),receipt)==E::rejected&&!bad.can_fire());
    assert(!bad.take_inventory_request(out.native_request)&&!bad.reset());
    assert(bad.interrupt()==E::interrupted);
    assert(bad.acknowledge_inventory(stamp(6),accepted(bq,2))==E::interrupted);
    assert(bad.loaded()==6&&bad.reserve()==0);
}
void windows_plans_and_replay(){
    for(auto end:{1040LL,1100LL,1101LL,50000LL}){
        SnubReload m;const auto q=pending_insert(m,start(),end);
        assert(m.acknowledge_inventory(stamp(6,end+20),accepted(q,2))==(end<=1100?E::perfect:E::ordinary));
    }
    SnubReload delayed;const auto delayed_q=pending_insert(delayed,start(),1040);
    assert(delayed.acknowledge_inventory(stamp(6,50000),accepted(delayed_q,2))==E::perfect);
    SnubReloadCompletion delayed_out{};assert(delayed.take_completion(delayed_out));
    assert(delayed_out.finished_tick==1040);
    SnubReload unknown;const auto q=pending_insert(unknown,start(4,2,false));
    assert(unknown.acknowledge_inventory(stamp(6,1001),accepted(q,2))==E::ordinary);
    auto s=start();s.plan.count=3;SnubReload invalid;assert(invalid.begin(s)==E::invalid);
    s=start(0,4);s.plan.count=2;assert(invalid.begin(s)==E::invalid);
    s=start();s.r3_press_edge=5;s.intent_id=0;assert(invalid.begin(s)==E::invalid);
    s=start();s.r3_press_edge=5;assert(invalid.begin(s)==E::invalid);
    s=start();s.intent_source=ReloadIntentSource::take_ammunition;assert(invalid.begin(s)==E::invalid);
    s=start(6,0);s.original_reload_serial=9;assert(invalid.begin(s)==E::invalid);
    SnubReload m;const auto iq=pending_insert(m);
    assert(m.acknowledge_inventory(stamp(6,1001),accepted(iq,2))==E::perfect);
    finish(m,7,1002,E::perfect);assert(m.reset());
    s=start(1,5);s.stamp=stamp(8,2000);s.intent_id=2;s.old_magazine_item=72;
    s.inventory_revision=3;s.original_reload_serial=22;s.perfect.original_reload_serial=22;
    auto replay=s;replay.intent_id=1;assert(m.begin(replay)==E::rejected);
    replay=s;replay.old_magazine_item=71;assert(m.begin(replay)==E::rejected);
    assert(m.begin(s)==E::started);
}
void cancelled_drop_and_infinite(){
    for(auto s:{start(3,0),start(4,2)}){
        SnubReload m;opened(m,s);assert(m.release_original(stamp(3),70)==E::released);
        assert(m.interrupt()==E::interrupted);
        SnubOriginalReturnReceipt r{70,true,true,true};auto bad=r;bad.mechanism_closed=false;
        assert(m.cancel_to_original(stamp(4,1001,2),2,m.inventory(),bad)==E::rejected);
        assert(m.cancel_to_original(stamp(4,1001,2),2,m.inventory(),r)==E::cancelled);
        assert(m.loaded()==s.loaded&&m.reserve()==s.reserve&&m.outcome()==E::cancelled);
        SnubReloadCompletion out{};
        if(s.original_reload_serial){
            assert(m.take_completion(out)&&out.native_request.call==ReloadTxnCall::exit_without_fill);
            assert(m.acknowledge_completion(stamp(5,1002,2),accepted(out.native_request,1),true)==E::committed);
        }else assert(m.stage()==S::complete);
    }
    auto s=start(4,0,false);s.infinite_reserve=true;s.original_reload_serial=21;
    s.original_reload_accepted=true;SnubReload m;opened(m,s);
    assert(m.release_original(stamp(3),70)==E::released);
    assert(m.grab_left_shoulder(stamp(4),mag())==E::grabbed&&m.held()==2&&m.reserve()==0);
    assert(m.insert_magazine(stamp(5),insert())==E::insertion_requested);
    SnubInventoryRequest q{};assert(m.take_inventory_request(q));
    assert(q.expected.loaded==6&&q.expected.reserve==0&&q.expected.infinite_reserve);
    assert(m.acknowledge_inventory(stamp(6),accepted(q,2))==E::ordinary);
}
void conservation(){
    unsigned cases=0;
    for(unsigned l=0;l<6;++l)for(unsigned r=1;r<20;++r){
        auto s=start(l,r,false);SnubReload m;const auto q=pending_insert(m,s);
        assert(m.acknowledge_inventory(stamp(6),accepted(q,2))==(l?E::ordinary:E::inserted));
        if(!l)assert(m.move_original_parts(stamp(7),motion())==E::ordinary);
        assert(m.loaded()==(std::min)(6u,l+r)&&m.reserve()==r-(m.loaded()-l));
        assert(m.total()==l+r);++cases;
    }
    assert(cases==114);
}
}
int main(){
    partial_and_empty();old_mag_and_inspection();release_retry_and_recovery();
    late_call_and_reconciliation();windows_plans_and_replay();cancelled_drop_and_infinite();conservation();
    std::puts("Snub shared ledger: branches/inspection/recovery/late-call/replay +114 conservation cases passed (synthetic only).");
}
