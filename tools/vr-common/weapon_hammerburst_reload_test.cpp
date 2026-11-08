#include "weapon_hammerburst_reload.h"
#include "weapon_hammerburst_latch.h"
#include "weapon_hammerburst_latch_curve.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_weapon;
using E=HammerburstReloadEvent;
using S=HammerburstReloadStage;

namespace {
// All counts, ticks, sequences and receipts below are synthetic fixtures, not
// Hammerburst capacity, phase, mechanical travel or perfect-window measurements.
constexpr HammerburstReloadIdentity id{11,12,13,14,15,16,17,18};
HammerburstReloadStamp stamp(std::uint64_t sequence,std::int64_t tick) {
    return {id,sequence,tick};
}
HammerburstReloadStart start(std::uint32_t loaded=3,std::uint32_t reserve=20,
                            bool known=true) {
    return {stamp(1,1000),1,1,10,loaded,reserve,{id,100,known},true,true,true};
}
HammerburstLatchReceipt opening() {
    return {1,1,HammerburstLatchPose::closed,HammerburstLatchPose::open,true,true};
}
HammerburstLatchReceipt closing(std::uint64_t since=4) {
    return {1,since,HammerburstLatchPose::open,HammerburstLatchPose::closed,true,false};
}
HammerburstMagazineReceipt magazine(std::uint64_t item=51) {
    return {item,HammerburstAmmoSource::left_shoulder,
            HammerburstAmmoKind::hammerburst_magazine,true};
}
HammerburstInsertionReceipt insertion(std::uint64_t item=51,std::uint64_t since=3) {
    return {item,since,true,true,true,true,true};
}
void opened(HammerburstReload& model,HammerburstReloadStart const& begin=start()) {
    assert(model.begin_r3(begin)==E::started);
    assert(model.confirm_latch_open(stamp(2,1010),opening())==E::opened);
}
void seated(HammerburstReload& model,HammerburstReloadStart const& begin=start()) {
    opened(model,begin);
    assert(model.grab_left_shoulder(stamp(3,1020),magazine())==E::grabbed);
    assert(model.insert_magazine(stamp(4,1030),insertion())==E::inserted);
}
void completion_and_guide() {
    HammerburstReload model;
    assert(model.guide().part==HammerburstGuidePart::none&&!model.can_fire());
    assert(model.begin_r3(start())==E::started);
    assert(model.loaded()==3&&model.reserve()==20&&model.total()==23);
    auto guide=model.guide();
    assert(guide.part==HammerburstGuidePart::magazine_latch&&guide.red==1&&
           guide.green==1&&guide.blue==0&&guide.alpha==1);
    assert(model.confirm_latch_open(stamp(2,1010),opening())==E::opened);
    assert(!model.feed_present()&&model.loaded()==0&&model.target()==10&&model.total()==23);
    assert(model.guide().part==HammerburstGuidePart::left_shoulder);
    assert(model.grab_left_shoulder(stamp(3,1020),magazine())==E::grabbed);
    assert(model.held()==10&&model.reserve()==13&&model.total()==23);
    assert(model.guide().part==HammerburstGuidePart::feed_port&&!model.can_fire());
    assert(model.insert_magazine(stamp(4,1030),insertion())==E::inserted);
    assert(model.guide().part==HammerburstGuidePart::magazine_latch);
    assert(model.loaded()==10&&model.held()==0&&model.feed_present()&&!model.can_fire());
    assert(model.confirm_latch_closed(stamp(5,1040),closing())==E::perfect);
    assert(model.guide().part==HammerburstGuidePart::none&&model.can_fire());
    assert(model.confirm_latch_closed(stamp(6,1041),closing())==E::rejected);
    HammerburstReloadCompletion commit{};
    assert(model.take_completion(commit)&&commit.identity==id&&commit.original_reload_serial==1);
    assert(commit.result==E::perfect&&commit.loaded==10&&commit.reserve==13&&
           commit.started_tick==1000&&commit.finished_tick==1040);
    assert(!model.take_completion(commit));
}
void deadlines() {
    for(auto finish:{1000LL,1100LL,1101LL,9000LL}){
        HammerburstReload model;
        auto begin=start();
        // A paused game can provide increasing event sequences at one tick.
        assert(model.begin_r3(begin)==E::started);
        assert(model.confirm_latch_open(stamp(2,1000),opening())==E::opened);
        assert(model.grab_left_shoulder(stamp(3,1000),magazine())==E::grabbed);
        assert(model.insert_magazine(stamp(4,1000),insertion())==E::inserted);
        assert(model.stage()==S::closing_latch&&!model.can_fire());
        assert(model.confirm_latch_closed(stamp(5,finish),closing())==
               (finish<=1100?E::perfect:E::ordinary));
    }
    HammerburstReload unknown;
    auto begin=start(3,20,false);begin.perfect.end_ticks=0;
    seated(unknown,begin);
    assert(unknown.confirm_latch_closed(stamp(5,1040),closing())==E::ordinary);
    HammerburstReload long_wait;
    opened(long_wait);
    assert(long_wait.grab_left_shoulder(stamp(3,9000),magazine())==E::grabbed);
    assert(long_wait.stage()==S::holding_magazine&&!long_wait.can_fire());
    assert(long_wait.insert_magazine(stamp(4,10000),insertion())==E::inserted);
    assert(long_wait.confirm_latch_closed(stamp(5,11000),closing())==E::ordinary);
    HammerburstReload huge;
    seated(huge);
    assert(huge.confirm_latch_closed(stamp(5,(std::numeric_limits<std::int64_t>::max)()),
                                     closing())==E::ordinary);
}
void entry_validation() {
    for(unsigned invalid=0;invalid!=12;++invalid){
        HammerburstReload model;auto begin=start();
        switch(invalid){
            case 0:begin.stamp.identity.weapon=0;break;
            case 1:begin.r3_press_edge=0;break;
            case 2:begin.original_reload_serial=0;break;
            case 3:begin.original_class_verified=false;break;
            case 4:begin.original_reload_accepted=false;break;
            case 5:begin.capacity=0;break;
            case 6:begin.loaded=11;break;
            case 7:begin.reserve=1000001;break;
            case 8:begin.perfect.end_ticks=-1;break;
            case 9:begin.perfect.identity.weapon++;break;
            case 10:begin.stamp.game_tick=-1;break;
            case 11:begin.current_instance_sequence_verified=false;break;
        }
        assert(model.begin_r3(begin)==E::invalid&&model.stage()==S::idle);
    }
    HammerburstReload full,dry;
    assert(full.begin_r3(start(10,20))==E::full);
    assert(dry.begin_r3(start(3,0))==E::no_ammo);
    assert(full.guide().part==HammerburstGuidePart::none&&!full.can_fire());
    HammerburstReload active;
    opened(active);
    auto before=active.total();
    assert(active.begin_r3(start())==E::rejected&&active.total()==before);
}
void latch_order_and_receipts() {
    HammerburstReload model;
    assert(model.begin_r3(start())==E::started);
    assert(model.grab_left_shoulder(stamp(2,1001),magazine())==E::rejected);
    assert(model.insert_magazine(stamp(2,1001),insertion())==E::rejected);
    assert(model.confirm_latch_closed(stamp(2,1001),closing(1))==E::rejected);
    for(unsigned invalid=0;invalid!=5;++invalid){
        auto receipt=opening();
        switch(invalid){
            case 0:receipt.original_reload_serial++;break;
            case 1:receipt.measured_hammerburst_travel=false;break;
            case 2:receipt.old_magazine_released=false;break;
            case 3:receipt.from=HammerburstLatchPose::unknown;break;
            case 4:receipt.motion_begin_sequence=2;break;
        }
        assert(model.confirm_latch_open(stamp(2,1010),receipt)==E::rejected);
        assert(model.loaded()==3&&model.total()==23);
    }
    assert(model.confirm_latch_open(stamp(2,1010),opening())==E::opened);
    // An original animation reaching closed before player insertion is ignored.
    assert(model.confirm_latch_closed(stamp(3,1020),closing(2))==E::rejected);
    assert(model.grab_left_shoulder(stamp(3,1020),magazine())==E::grabbed);
    assert(model.confirm_latch_closed(stamp(4,1030),closing(3))==E::rejected);
    assert(model.insert_magazine(stamp(4,1030),insertion())==E::inserted);
    assert(model.confirm_latch_closed(stamp(5,1040),closing(3))==E::rejected);
    assert(model.confirm_latch_closed(stamp(5,1040),closing())==E::perfect);
}
void pickup_and_insertion() {
    HammerburstReload model;opened(model);
    for(unsigned invalid=0;invalid!=4;++invalid){
        auto receipt=magazine();
        switch(invalid){
            case 0:receipt.item=0;break;
            case 1:receipt.source=HammerburstAmmoSource::other;break;
            case 2:receipt.kind=HammerburstAmmoKind::other;break;
            case 3:receipt.tracked_left_hand=false;break;
        }
        assert(model.grab_left_shoulder(stamp(3,1020),receipt)==E::rejected);
        assert(model.reserve()==23&&model.held()==0);
    }
    assert(model.grab_left_shoulder(stamp(3,1020),magazine())==E::grabbed);
    assert(model.grab_left_shoulder(stamp(4,1030),magazine(52))==E::rejected);
    for(unsigned invalid=0;invalid!=8;++invalid){
        auto receipt=insertion();
        switch(invalid){
            case 0:receipt.item++;break;
            case 1:receipt.hammerburst_mount_verified=false;break;
            case 2:receipt.aligned=false;break;
            case 3:receipt.continuous=false;break;
            case 4:receipt.seated=false;break;
            case 5:receipt.tracked_left_hand=false;break;
            case 6:receipt.stroke_begin_sequence=2;break;
            case 7:receipt.stroke_begin_sequence=4;break;
        }
        assert(model.insert_magazine(stamp(4,1030),receipt)==E::rejected);
        assert(model.loaded()==0&&model.held()==10&&model.total()==23);
    }
    assert(model.insert_magazine(stamp(4,1030),insertion())==E::inserted);
    assert(model.insert_magazine(stamp(5,1040),insertion())==E::rejected);
}
void identity_clock_and_release() {
    HammerburstReload model;opened(model);
    for(unsigned invalid=0;invalid!=10;++invalid){
        auto observation=stamp(3,1020);
        switch(invalid){
            case 0:observation.identity.world++;break;
            case 1:observation.identity.pawn++;break;
            case 2:observation.identity.weapon++;break;
            case 3:observation.identity.world_generation++;break;
            case 4:observation.identity.session++;break;
            case 5:observation.identity.space_epoch++;break;
            case 6:observation.identity.input_epoch++;break;
            case 7:observation.identity.profile_generation++;break;
            case 8:observation.sequence=2;break;
            case 9:observation.game_tick=1009;break;
        }
        assert(model.grab_left_shoulder(observation,magazine())==E::rejected);
        assert(model.total()==23&&model.held()==0);
    }
    assert(model.grab_left_shoulder(stamp(3,1020),magazine())==E::grabbed);
    assert(model.release_magazine(stamp(4,1030),52)==E::rejected);
    assert(model.release_magazine(stamp(4,1030),51)==E::released);
    assert(model.total()==23&&model.reserve()==23&&model.held()==0);
    assert(model.insert_magazine(stamp(5,1040),insertion())==E::rejected);
    assert(model.grab_left_shoulder(stamp(5,1040),magazine())==E::rejected);
    assert(model.grab_left_shoulder(stamp(5,1040),magazine(52))==E::grabbed);
    assert(model.insert_magazine(stamp(6,1050),insertion(52,5))==E::inserted);
    assert(model.confirm_latch_closed(stamp(7,1060),closing(6))==E::perfect);
}
void conservation_and_interruptions() {
    for(std::uint32_t loaded=0;loaded<10;++loaded){
        for(std::uint32_t reserve=1;reserve<=12;++reserve){
            HammerburstReload model;
            seated(model,start(loaded,reserve));
            assert(model.loaded()==(std::min)(10u,loaded+reserve));
            assert(model.total()==loaded+reserve);
            assert(model.confirm_latch_closed(stamp(5,1040),closing())==E::perfect);
            assert(model.total()==loaded+reserve&&model.can_fire());
        }
    }
    for(unsigned step=0;step!=5;++step){
        HammerburstReload model;assert(model.begin_r3(start())==E::started);
        if(step>=1)assert(model.confirm_latch_open(stamp(2,1010),opening())==E::opened);
        if(step>=2)assert(model.grab_left_shoulder(stamp(3,1020),magazine())==E::grabbed);
        if(step>=3)assert(model.insert_magazine(stamp(4,1030),insertion())==E::inserted);
        if(step>=4)assert(model.confirm_latch_closed(stamp(5,1040),closing())==E::perfect);
        const auto seated_count=model.loaded();
        assert(model.interrupt()==E::interrupted);
        assert(model.total()==23&&model.loaded()==seated_count&&model.held()==0);
        assert(model.stage()==S::interrupted&&!model.can_fire()&&
               model.guide().part==HammerburstGuidePart::none);
        HammerburstReloadCompletion commit{};
        assert(!model.take_completion(commit));
        assert(model.interrupt()==E::rejected&&model.begin_r3(start())==E::rejected);
        assert(model.confirm_latch_closed(stamp(10,1100),closing())==E::rejected);
    }
}
void start_replay_and_commit() {
    HammerburstReload model;seated(model);
    assert(model.confirm_latch_closed(stamp(5,1040),closing())==E::perfect);
    auto next=start(9,13);next.stamp=stamp(6,1050);
    next.r3_press_edge=2;next.original_reload_serial=2;
    assert(model.begin_r3(next)==E::rejected); // Unclaimed original commit cannot be overwritten.
    HammerburstReloadCompletion commit{};
    assert(model.take_completion(commit));
    auto replay=next;replay.r3_press_edge=1;
    assert(model.begin_r3(replay)==E::rejected);
    replay=next;replay.original_reload_serial=1;
    assert(model.begin_r3(replay)==E::rejected);
    assert(model.begin_r3(next)==E::started);
    assert(model.confirm_latch_open(stamp(7,1060),opening())==E::rejected);
    auto receipt=opening();receipt.original_reload_serial=2;receipt.motion_begin_sequence=6;
    assert(model.confirm_latch_open(stamp(7,1060),receipt)==E::opened);
    model.reset();
    assert(model.stage()==S::idle);
    assert(model.begin_r3(start())==E::rejected); // reset does not revive an old edge
    next.stamp=stamp(8,1070);next.r3_press_edge=3;
    next.original_reload_serial=3;
    assert(model.begin_r3(next)==E::started);
}
}
void physical_start_pause_and_resume() {
    HammerburstReload m;auto b=start();b.r3_press_edge=0;
    assert(m.begin_latch(b,1)==E::started);
    assert(m.confirm_latch_open(stamp(2,1010),opening())==E::opened);
    assert(m.grab_left_shoulder(stamp(3,1020),magazine())==E::grabbed);
    assert(m.suspend()==E::interrupted&&m.stage()==S::paused&&m.held()==0&&m.total()==23);
    assert(!m.resume(stamp(4,1200),2,2)&&!m.resume(stamp(4,1200),1,1));
    assert(m.resume(stamp(4,1200),1,2));
    assert(m.grab_left_shoulder(stamp(5,1210),magazine())==E::rejected);
    assert(m.grab_left_shoulder(stamp(5,1210),magazine(52))==E::grabbed);
    assert(m.insert_magazine(stamp(6,1220),insertion(52,5))==E::inserted);
    assert(m.suspend()==E::interrupted&&m.resume(stamp(7,1230),1,3));
    assert(m.confirm_latch_closed(stamp(8,1240),closing(6))==E::rejected);
    assert(m.confirm_latch_closed(stamp(8,1240),closing(7))==E::ordinary);
    assert(m.loaded()==10&&m.reserve()==13&&m.total()==23);
}
void continuous_hinge_motion() {
    HammerburstLatchGeometry g{{0,0,0},{0,1,0},{.05,0,0},.8,.02,.02,.02,true};
    HammerburstLatchStroke stroke;
    auto sample=[](double angle,std::uint64_t seq,std::uint64_t ms){
        return HammerburstLatchSample{{.05*std::cos(angle),0,-.05*std::sin(angle)},seq,ms,1,true,true};
    };
    assert(stroke.begin(g,sample(0,1,1000),0));
    for(unsigned i=1;i<=4;++i){
        const auto result=stroke.observe(sample(.2*i,i+1,1000+50*i));
        assert(result==(i==4?HammerburstLatchMotion::open:HammerburstLatchMotion::moving));
    }
    for(unsigned i=1;i<=4;++i){
        const auto result=stroke.observe(sample(.8-.2*i,i+5,1200+50*i));
        assert(result==(i==4?HammerburstLatchMotion::closed:HammerburstLatchMotion::moving));
    }
    assert(stroke.observe(sample(.2,10,1600))==HammerburstLatchMotion::rejected&&!stroke.active());
    assert(stroke.begin(g,sample(.4,11,1700),.4));
    auto loss=sample(.5,12,1750);loss.tracked=false;
    assert(stroke.observe(loss)==HammerburstLatchMotion::rejected);
    assert(stroke.begin(g,sample(.4,13,1800),.4));
    auto other=sample(.5,14,1850);other.grip_token=2;
    assert(stroke.observe(other)==HammerburstLatchMotion::rejected);
    auto centre=sample(0,15,1900);centre.hand={0,0,0};
    assert(!stroke.begin(g,centre,0));
    auto unverified=g;unverified.current_instance_verified=false;
    assert(!stroke.begin(unverified,sample(0,16,2000),0));
    g.axis={0,0,1};
    assert(stroke.begin(g,{{.05,0,0},17,2100,3,true,true},0));
    assert(stroke.observe({{.05*std::cos(.4),.05*std::sin(.4),0},18,2150,3,true,true})==HammerburstLatchMotion::moving);
    assert(std::abs(stroke.angle()-.4)<1e-8);
}
int main() {
    { HammerburstLatchCurve curve;double keys[]={0,0,.4,.8,.8,.4,0};float time=-1;
      assert(!curve.bind(keys,7,.7,false)&&!curve.time_for_angle(.4,time));
      assert(curve.bind(keys,7,.7,true)&&curve.time_for_angle(.4,time)&&std::abs(time-.2f)<1e-6);
      assert(curve.time_for_angle(.6,time)&&std::abs(time-.25f)<1e-6);
      assert(curve.time_for_angle(.8,time)&&std::abs(time-.3f)<1e-6);
      assert(curve.time_for_angle(.8,time,HammerburstLatchCurve::Sampling::endpoint_intervals)&&std::abs(time-.35f)<1e-6);
      double sampled{};
      assert(curve.angle_at_time(.3,sampled,HammerburstLatchCurve::Sampling::endpoint_intervals)&&std::abs(sampled-(.4+.4*4./7))<1e-6);
      assert(!curve.time_for_angle(.9,time));
      double reversed[]={0,.4,.2,.8,.8,.4,0};assert(!curve.bind(reversed,7,.7,true));
      keys[2]=std::numeric_limits<double>::quiet_NaN();assert(!curve.bind(keys,7,.7,true)); }
    completion_and_guide();deadlines();entry_validation();latch_order_and_receipts();
    pickup_and_insertion();identity_clock_and_release();conservation_and_interruptions();
    start_replay_and_commit();physical_start_pause_and_resume();continuous_hinge_motion();
    { // Strict acquisition, measured endpoints and default gap handling.
      HammerburstLatchGeometry g{{0,0,0},{0,1,0},{.07,0,0},.8,.035,.035,.03,true};
      HammerburstLatchStroke s;
      const auto point=[](double a,unsigned seq,unsigned ms,double radius=.07,double axial=0.){
          return HammerburstLatchSample{{radius*std::cos(a),axial,-radius*std::sin(a)},seq,ms,99,true,true};};
      auto oversized=g;oversized.contact_radius=.12;
      assert(!s.begin(oversized,point(0,1,1000),0));
      assert(!s.begin(g,point(0,1,1000,.10,.04),0));
      assert(s.begin(g,point(0,1,1000),0));
      assert(s.observe(point(0,2,1050))==HammerburstLatchMotion::closed&&s.angle()==0);
      assert(s.observe(point(.3,3,1100))==HammerburstLatchMotion::moving);
      assert(s.observe(point(.69,4,1150))==HammerburstLatchMotion::moving);
      assert(std::abs(s.angle()-.69)<1e-8);
      assert(s.observe(point(.79,5,1200))==HammerburstLatchMotion::open);
      assert(std::abs(s.angle()-.79)<1e-8);
      assert(s.observe(point(1.05,6,1250))==HammerburstLatchMotion::rejected);
      assert(s.last_rejection()==HammerburstLatchRejection::range);
      assert(s.begin(g,point(.8,7,1300),.8));
      assert(s.observe(point(.8,8,1451))==HammerburstLatchMotion::rejected);
      assert(s.last_rejection()==HammerburstLatchRejection::gap);
      assert(s.begin(g,point(.8,9,1500),.8));
      assert(s.observe(point(.4,10,1550))==HammerburstLatchMotion::moving);
      assert(s.observe(point(.1,11,1600))==HammerburstLatchMotion::moving);
      assert(s.observe(point(.01,12,1650))==HammerburstLatchMotion::closed);
      assert(std::abs(s.angle()-.01)<1e-8);
      auto unheld=point(.0,13,1700);unheld.gripped=false;
      assert(s.observe(unheld)==HammerburstLatchMotion::rejected);
      assert(s.observe(point(0,14,1750),true)==HammerburstLatchMotion::rejected);
      assert(s.last_rejection()==HammerburstLatchRejection::inactive);
    }
    std::puts("Hammerburst latch/magazine reload, timing, conservation and interruption tests passed");
}
