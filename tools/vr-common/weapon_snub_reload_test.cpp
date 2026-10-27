#include "weapon_snub_reload.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_weapon;
using E=SnubReloadEvent;
using S=SnubReloadStage;

namespace {
// ALL plans, counts, ticks, geometry IDs and motion receipts in these tests are
// synthetic. In particular, slide-after-insertion and the alternative wing
// order below are NOT claims about the original Snub animation or mechanics.
constexpr SnubReloadIdentity id{11,12,13,14};
SnubReloadStamp stamp(std::uint64_t seq,std::int64_t tick=1000,std::uint64_t epoch=1) {
    return {id,seq,epoch,tick};
}
SnubReloadStart start(std::uint32_t loaded=2,std::uint32_t reserve=9,bool known=true) {
    SnubReloadStart s{};
    s.stamp=stamp(1);s.r3_press_edge=1;s.original_reload_serial=21;
    s.capacity=6;s.loaded=loaded;s.reserve=reserve;
    s.plan.identity=id;s.plan.id=31;s.plan.feed_mount_id=41;s.plan.count=3;
    s.plan.condition=loaded?SnubReloadCondition::partial:SnubReloadCondition::empty;
    s.plan.original_order_verified=s.plan.current_instance_verified=true;
    s.plan.steps[0]={SnubStepKind::eject_magazine,snub_part(SnubPart::magazine),51,0};
    s.plan.steps[1]={SnubStepKind::insert_magazine,snub_part(SnubPart::feed_port),52,0};
    s.plan.steps[2]={SnubStepKind::move_original_parts,snub_part(SnubPart::slide),53,61};
    s.perfect={id,21,known?100:0,known};
    s.original_inventory_slot=0;
    s.original_class_verified=s.right_hand_equipped=s.original_reload_accepted=true;
    return s;
}
SnubEjectionReceipt eject() {return {21,31,51,true,true};}
SnubMagazineReceipt magazine(std::uint64_t item=71) {
    return {item,snub_ammo_slot,true,true,true,true};
}
SnubInsertionReceipt insertion(std::uint64_t item=71,std::uint64_t since=3) {
    return {item,41,since,true,true,true,true,true,true};
}
SnubPartMotionReceipt movement(std::uint64_t since=4,std::uint64_t phase=53,
                               SnubPartMask parts=snub_part(SnubPart::slide),
                               std::uint64_t travel=61) {
    return {21,31,phase,travel,since,parts,true,true,true,true,true,true};
}
void ejected(SnubReload& m,SnubReloadStart const& s=start()) {
    assert(m.begin_r3(s)==E::started);
    assert(m.confirm_ejection(stamp(2),eject())==E::ejected);
}
void seated(SnubReload& m,SnubReloadStart const& s=start()) {
    ejected(m,s);
    assert(m.grab_left_shoulder(stamp(3),magazine())==E::grabbed);
    assert(m.insert_magazine(stamp(4),insertion())==E::inserted);
}
void complete_and_commit() {
    assert(snub_body_slot==BodySlot::pistol&&original_inventory_slot(snub_body_slot)==0);
    SnubReload m;assert(!m.can_fire()&&!m.guide().parts);
    assert(m.begin_r3(start())==E::started);
    auto guide=m.guide();
    assert(guide.parts==snub_part(SnubPart::magazine)&&guide.red==1&&guide.green==1&&
           guide.blue==0&&guide.alpha==1);
    assert(m.loaded()==2&&m.reserve()==9&&m.total()==11);
    assert(m.confirm_ejection(stamp(2),eject())==E::ejected);
    assert(m.loaded()==0&&m.reserve()==11&&!m.feed_present()&&m.total()==11);
    assert(m.guide().parts==snub_part(SnubPart::left_shoulder));
    assert(m.grab_left_shoulder(stamp(3),magazine())==E::grabbed);
    assert(m.held()==6&&m.reserve()==5&&m.loaded()==0&&!m.can_fire());
    assert(m.guide().parts==snub_part(SnubPart::feed_port));
    assert(m.insert_magazine(stamp(4),insertion())==E::inserted);
    assert(m.loaded()==6&&m.held()==0&&m.feed_present()&&!m.can_fire());
    assert(m.guide().parts==snub_part(SnubPart::slide));
    assert(m.move_original_parts(stamp(5,1040),movement())==E::perfect);
    assert(m.stage()==S::pending_completion&&!m.guide().parts&&!m.can_fire());
    assert(m.acknowledge_completion(stamp(6,1041),21,6,5,true,true)==E::rejected);
    SnubReloadCompletion out{};
    assert(m.take_completion(out)&&out.identity==id&&out.original_reload_serial==21&&
           out.loaded==6&&out.reserve==5&&out.started_tick==1000&&
           out.finished_tick==1040&&out.result==E::perfect);
    assert(!m.take_completion(out));
    assert(m.acknowledge_completion(stamp(6,1041),22,6,5,true,true)==E::rejected);
    assert(m.acknowledge_completion(stamp(6,1041),21,5,5,true,true)==E::rejected);
    assert(m.acknowledge_completion(stamp(6,1041),21,6,5,false,true)==E::rejected);
    assert(m.acknowledge_completion(stamp(6,1041),21,6,5,true,false)==E::rejected);
    assert(m.acknowledge_completion(stamp(6,1041),21,6,5,true,true)==E::committed);
    assert(m.can_fire()&&m.stage()==S::complete);
    assert(m.move_original_parts(stamp(7,1042),movement())==E::rejected);
    assert(m.acknowledge_completion(stamp(7,1042),21,6,5,true,true)==E::rejected);
    assert(!m.take_completion(out));
}
void deadlines_and_pause() {
    for(auto end:{1000LL,1099LL,1100LL,1101LL,5000LL}){
        SnubReload m;seated(m);
        assert(m.move_original_parts(stamp(5,end),movement())==(end<=1100?E::perfect:E::ordinary));
    }
    SnubReload unknown;seated(unknown,start(2,9,false));
    assert(unknown.move_original_parts(stamp(5),movement())==E::ordinary);
    SnubReload not_verified;auto s=start(2,9,false);s.perfect.end_ticks=100;
    seated(not_verified,s);
    assert(not_verified.move_original_parts(stamp(5),movement())==E::ordinary);
    SnubReload long_wait;ejected(long_wait);
    assert(long_wait.grab_left_shoulder(stamp(3,50000),magazine())==E::grabbed);
    assert(long_wait.stage()==S::holding_magazine&&!long_wait.can_fire());
    assert(long_wait.insert_magazine(stamp(4,50000),insertion())==E::inserted);
    assert(long_wait.move_original_parts(stamp(5,50000),movement())==E::ordinary);
    SnubReload huge;seated(huge);
    assert(huge.move_original_parts(stamp(5,(std::numeric_limits<std::int64_t>::max)()),movement())==E::ordinary);
}
void invalid_start_and_plan() {
    for(unsigned bad=0;bad!=25;++bad){
        SnubReload m;auto s=start();
        switch(bad){
            case 0:s.stamp.identity.weapon=0;break;
            case 1:s.r3_press_edge=0;break;
            case 2:s.original_reload_serial=0;break;
            case 3:s.original_class_verified=false;break;
            case 4:s.right_hand_equipped=false;break;
            case 5:s.original_reload_accepted=false;break;
            case 6:s.original_inventory_slot=2;break;
            case 7:s.capacity=0;break;
            case 8:s.loaded=7;break;
            case 9:s.reserve=1000001;break;
            case 10:s.plan.current_instance_verified=false;break;
            case 11:s.plan.original_order_verified=false;break;
            case 12:s.plan.identity.weapon++;break;
            case 13:s.plan.condition=SnubReloadCondition::empty;break;
            case 14:s.plan.count=11;break;
            case 15:s.plan.steps[1].phase_id=51;break;
            case 16:s.plan.steps[2].measured_travel_id=0;break;
            case 17:s.plan.steps[2].parts=128;break;
            case 18:s.plan.steps[1]=s.plan.steps[2];break;
            case 19:s.perfect.end_ticks=-1;break;
            case 20:s.perfect.identity.weapon++;break;
            case 21:s.perfect.original_reload_serial++;break;
            case 22:s.perfect.end_ticks=0;break;
            case 23:s.stamp.game_tick=-1;break;
            case 24:s.stamp.motion_epoch=0;break;
        }
        assert(m.begin_r3(s)==E::invalid&&m.stage()==S::idle&&m.total()==0);
    }
    SnubReload full,dry;
    assert(full.begin_r3(start(6,9))==E::full);
    assert(dry.begin_r3(start(2,0))==E::no_ammo);
    SnubReload active;ejected(active);auto total=active.total();
    assert(active.begin_r3(start())==E::rejected&&active.total()==total);
}
void required_order_and_receipts() {
    SnubReload m;assert(m.begin_r3(start())==E::started);
    assert(m.grab_left_shoulder(stamp(2),magazine())==E::rejected);
    assert(m.move_original_parts(stamp(2),movement(1))==E::rejected);
    for(unsigned bad=0;bad!=5;++bad){
        auto r=eject();
        switch(bad){case 0:r.original_reload_serial++;break;case 1:r.plan_id++;break;
            case 2:r.phase_id++;break;case 3:r.original_magazine_detached=false;break;
            case 4:r.snub_magazine_binding_verified=false;break;}
        assert(m.confirm_ejection(stamp(2),r)==E::rejected&&m.loaded()==2);
    }
    assert(m.confirm_ejection(stamp(2),eject())==E::ejected);
    for(unsigned bad=0;bad!=6;++bad){
        auto r=magazine();
        switch(bad){case 0:r.source=BodySlot::pistol;break;case 1:r.item_serial=0;break;
            case 2:r.snub_magazine_verified=false;break;case 3:r.left_hand_tracked=false;break;
            case 4:r.left_grip_held=false;break;case 5:r.right_hand_tracked=false;break;}
        assert(m.grab_left_shoulder(stamp(3),r)==E::rejected&&m.held()==0);
    }
    assert(m.grab_left_shoulder(stamp(3),magazine())==E::grabbed);
    for(unsigned bad=0;bad!=9;++bad){
        auto r=insertion();
        switch(bad){case 0:r.item_serial++;break;case 1:r.feed_mount_id++;break;
            case 2:r.aligned=false;break;case 3:r.continuous=false;break;
            case 4:r.seated=false;break;case 5:r.left_hand_tracked=false;break;
            case 6:r.left_grip_held=false;break;case 7:r.right_hand_tracked=false;break;
            case 8:r.motion_begin_sequence=2;break;}
        assert(m.insert_magazine(stamp(4),r)==E::rejected&&m.loaded()==0&&m.total()==11);
    }
    assert(m.insert_magazine(stamp(4),insertion())==E::inserted);
    for(unsigned bad=0;bad!=13;++bad){
        auto r=movement();
        switch(bad){case 0:r.original_reload_serial++;break;case 1:r.plan_id++;break;
            case 2:r.phase_id++;break;case 3:r.measured_travel_id++;break;
            case 4:r.parts=snub_part(SnubPart::left_wing);break;
            case 5:r.endpoints_verified=false;break;case 6:r.continuous=false;break;
            case 7:r.player_driven=false;break;case 8:r.left_hand_tracked=false;break;
            case 9:r.left_grip_held=false;break;case 10:r.right_hand_tracked=false;break;
            case 11:r.motion_begin_sequence=3;break;case 12:r.motion_begin_sequence=5;break;}
        assert(m.move_original_parts(stamp(5),r)==E::rejected&&m.stage()==S::mechanism&&!m.can_fire());
    }
    for(unsigned bad=0;bad!=4;++bad){
        auto t=stamp(5);
        switch(bad){case 0:t.sequence=4;break;case 1:t.identity.world_generation++;break;
            case 2:t.game_tick=999;break;case 3:t.motion_epoch=2;break;}
        assert(m.move_original_parts(t,movement())==E::rejected);
    }
    assert(m.move_original_parts(stamp(5),movement())==E::perfect);
}
void distinct_verified_plans() {
    // Neither a universal final slide action nor independently operated wings
    // are assumed. Each accepted phase highlights exactly its configured parts.
    auto s=start(0,4);s.plan.count=4;
    auto coupled=static_cast<SnubPartMask>(snub_part(SnubPart::left_wing)|snub_part(SnubPart::right_wing));
    s.plan.steps[1]={SnubStepKind::move_original_parts,coupled,54,62};
    s.plan.steps[2]={SnubStepKind::insert_magazine,snub_part(SnubPart::feed_port),52,0};
    s.plan.steps[3]={SnubStepKind::move_original_parts,snub_part(SnubPart::slide),53,61};
    SnubReload m;ejected(m,s);
    assert(m.guide().parts==coupled);
    assert(m.grab_left_shoulder(stamp(3),magazine())==E::rejected);
    assert(m.move_original_parts(stamp(3),movement(2,54,coupled,62))==E::mechanism_completed);
    assert(m.grab_left_shoulder(stamp(4),magazine())==E::grabbed);
    assert(m.insert_magazine(stamp(5),insertion(71,4))==E::inserted);
    assert(m.move_original_parts(stamp(6),movement(5))==E::perfect&&m.loaded()==4);
    auto only_magazine=start();only_magazine.plan.count=2;
    SnubReload no_final;ejected(no_final,only_magazine);
    assert(no_final.grab_left_shoulder(stamp(3),magazine())==E::grabbed);
    assert(no_final.insert_magazine(stamp(4),insertion())==E::perfect);
}
void discontinuities_and_interruptions() {
    SnubReload m;ejected(m);
    assert(m.grab_left_shoulder(stamp(3),magazine())==E::grabbed);
    assert(m.motion_discontinuity(stamp(4,1050,2))==E::motion_reset);
    assert(m.insert_magazine(stamp(5,1051),insertion())==E::rejected);
    assert(m.insert_magazine(stamp(5,1051,2),insertion())==E::rejected);
    assert(m.insert_magazine(stamp(5,1051,2),insertion(71,4))==E::inserted);
    assert(m.motion_discontinuity(stamp(6,1100,1))==E::rejected);
    assert(m.motion_discontinuity(stamp(6,1100,3))==E::motion_reset);
    assert(m.move_original_parts(stamp(7,1101,3),movement(5))==E::rejected);
    assert(m.move_original_parts(stamp(7,1101,3),movement(6))==E::ordinary);
    SnubReload held;ejected(held);
    assert(held.grab_left_shoulder(stamp(3),magazine())==E::grabbed);
    assert(held.interrupt()==E::interrupted&&held.held()==0&&held.reserve()==11&&held.total()==11);
    assert(!held.guide().parts&&!held.can_fire()&&held.interrupt()==E::rejected);
    assert(held.resume(stamp(4,1001,2),21,0,10,true)==E::rejected);
    assert(held.resume(stamp(4,1001,2),22,0,11,true)==E::rejected);
    assert(held.resume(stamp(4,1001,2),21,0,11,false)==E::rejected);
    auto wrong_world=stamp(4,1001,2);wrong_world.identity.world++;
    assert(held.resume(wrong_world,21,0,11,true)==E::rejected);
    assert(held.resume(stamp(4,1001,2),21,0,11,true)==E::resumed);
    assert(held.grab_left_shoulder(stamp(5,1001,2),magazine())==E::rejected);
    assert(held.grab_left_shoulder(stamp(5,1001,2),magazine(72))==E::grabbed);
    assert(held.insert_magazine(stamp(6,1001,2),insertion(72,5))==E::inserted);
    assert(held.move_original_parts(stamp(7,1001,2),movement(6))==E::ordinary);
    SnubReload seated_gun;seated(seated_gun);
    assert(seated_gun.interrupt()==E::interrupted&&seated_gun.loaded()==6&&seated_gun.reserve()==5);
    assert(seated_gun.resume(stamp(5,1001,2),21,6,5,true)==E::resumed);
    assert(seated_gun.move_original_parts(stamp(6,1001,2),movement(5))==E::ordinary);
    assert(seated_gun.interrupt()==E::interrupted);
    SnubReloadCompletion out{};
    assert(!seated_gun.take_completion(out));
    assert(seated_gun.resume(stamp(7,1002,3),21,6,5,true)==E::rejected);
}
void conservation_and_drop_retry() {
    unsigned cases=0;
    for(std::uint32_t loaded=0;loaded<6;++loaded)for(std::uint32_t reserve=1;reserve<20;++reserve){
        SnubReload m;ejected(m,start(loaded,reserve,false));
        assert(m.total()==loaded+reserve);
        assert(m.grab_left_shoulder(stamp(3),magazine())==E::grabbed);
        assert(m.release_magazine(stamp(4),72)==E::rejected);
        assert(m.release_magazine(stamp(4),71)==E::released&&m.total()==loaded+reserve);
        assert(m.grab_left_shoulder(stamp(5),magazine())==E::rejected);
        assert(m.grab_left_shoulder(stamp(5),magazine(72))==E::grabbed);
        assert(m.insert_magazine(stamp(6),insertion(72,5))==E::inserted);
        assert(m.loaded()==(std::min)(6U,loaded+reserve)&&m.total()==loaded+reserve);
        assert(m.insert_magazine(stamp(7),insertion(72,5))==E::rejected);
        assert(m.move_original_parts(stamp(7),movement(6))==E::ordinary);
        ++cases;
    }
    assert(cases==114);
}
void new_attempt_identity_and_edge() {
    SnubReload m;seated(m);
    assert(m.move_original_parts(stamp(5),movement())==E::perfect);
    SnubReloadCompletion out{};assert(m.take_completion(out));
    auto next=start(1,8);next.stamp=stamp(7,2000);next.r3_press_edge=2;
    next.original_reload_serial=22;next.perfect.original_reload_serial=22;
    assert(m.begin_r3(next)==E::rejected); // Pending original result cannot be replaced.
    assert(m.acknowledge_completion(stamp(6),21,6,5,true,true)==E::committed);
    auto old=next;old.r3_press_edge=1;assert(m.begin_r3(old)==E::rejected);
    old=next;old.original_reload_serial=21;old.perfect.original_reload_serial=21;
    assert(m.begin_r3(old)==E::rejected);
    old=next;old.stamp.identity.weapon++;old.plan.identity=old.stamp.identity;
    old.perfect.identity=old.stamp.identity;assert(m.begin_r3(old)==E::rejected);
    assert(m.begin_r3(next)==E::started);
    m.reset();
    assert(m.stage()==S::idle&&m.begin_r3(next)==E::rejected);
    next.stamp=stamp(8,3000);next.r3_press_edge=3;
    next.original_reload_serial=23;next.perfect.original_reload_serial=23;
    assert(m.begin_r3(next)==E::started);
}
}
int main() {
    complete_and_commit();deadlines_and_pause();invalid_start_and_plan();
    required_order_and_receipts();distinct_verified_plans();
    discontinuities_and_interruptions();conservation_and_drop_retry();
    new_attempt_identity_and_edge();
    std::puts("Snub reload: 8 groups and 114 conservation/retry cases passed (synthetic receipts only).");
}
