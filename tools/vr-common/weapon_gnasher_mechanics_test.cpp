#include "weapon_gnasher_cycle_commit.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

using namespace gears_weapon;
using namespace gears_vr;

static void check(bool condition,char const* expression,int line){
    if(!condition){std::fprintf(stderr,"line %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(expression) check(bool(expression),#expression,__LINE__)
static constexpr GnasherIdentity identity{1,2,3,4,5,6,7,8};
static bool close(double a,double b){return std::abs(a-b)<1e-8;}

static GnasherMechanicalConfig synthetic_config(std::int64_t deadline=1000){
    GnasherMechanicalConfig c{};
    c.calibration_id=1;c.lever_calibrated=c.feed_calibrated=true;
    c.capacity=8;c.game_ticks_per_second=deadline;
    c.lever.left_front_grip_in_main.position={0,0,.08};
    c.lever.pivot_in_main={0,0,0};
    c.lever.right_handle_home_in_main={0,-.2,0};
    c.lever.hinge_axis_in_main={1,0,0};
    return c;
}
struct Fixture {
    GnasherMechanicalInteraction gun{};
    GnasherMechanicalConfig config{};
    GnasherMechanicalSample sample{};
    Vector base{0,1.2,0};
    Quaternion body{};
    std::uint64_t round_serial{},shot_serial{},cycle_serial{};
    std::int64_t reload_start{};
    explicit Fixture(std::uint32_t loaded=2,std::uint32_t reserve=10,
                     std::int64_t deadline=1000):config(synthetic_config(deadline)){
        CHECK(config.valid());CHECK(gun.bind(identity,config,{identity,1,loaded,reserve}));
        sample.identity=identity;sample.monotonic_ns=1000000000;sample.game_tick=1000;
        sample.left_tracked=sample.right_tracked=sample.right_handle_held=true;
        sample.support_grasp_token=1;sample.original_ready=true;pose(0);CHECK(step().event==GnasherMechanicalEvent::none);
    }
    void pose(double angle){
        const Quaternion hinge{std::sin(angle*.5),0,0,std::cos(angle*.5)};
        sample.left_grip={sum(base,rotate(body,config.lever.left_front_grip_in_main.position)),body};
        sample.right_grip={sum(base,rotate(body,{0,-.2*std::cos(angle),-.2*std::sin(angle)})),
            normalized(multiply(body,hinge))};
        sample.left_front_held=true;sample.left_round_serial=0;
    }
    GnasherMechanicalDecision step(std::int64_t ns=20000000,std::int64_t ticks=10){
        ++sample.sequence;sample.monotonic_ns+=ns;sample.game_tick+=ticks;
        return gun.observe(sample);
    }
    GnasherMechanicalDecision angle(double value,std::int64_t ns=20000000){
        pose(value);return step(ns);
    }
    void begin(){
        reload_start=sample.game_tick;
        CHECK(gun.begin_reload(identity,sample.monotonic_ns,reload_start).event==
            GnasherMechanicalEvent::reload_started);
    }
    void grab(){
        CHECK(gun.grab_round(identity,++round_serial,sample.monotonic_ns).event==
            GnasherMechanicalEvent::round_grabbed);
        sample.left_front_held=false;sample.left_round_serial=round_serial;
    }
    GnasherMechanicalDecision depth(double value,std::int64_t ns=20000000){
        sample.left_front_held=false;sample.left_round_serial=round_serial;
        sample.left_grip={sum(base,rotate(body,{0,0,value})),body};return step(ns);
    }
    GnasherInventoryRequest insert_request(){
        CHECK(depth(.07).event==GnasherMechanicalEvent::none);
        CHECK(depth(.05).event==GnasherMechanicalEvent::none);
        CHECK(depth(.025).event==GnasherMechanicalEvent::none);
        CHECK(depth(.002).event==GnasherMechanicalEvent::request_inventory_commit);
        const auto request=gun.inventory_request();CHECK(request.token!=0);
        CHECK(request.input_sequence==sample.sequence&&request.physical_completion_ns==sample.monotonic_ns);
        CHECK(request.reference_generation==sample.reference_generation);return request;
    }
    GnasherMechanicalDecision commit(GnasherInventoryRequest const& request){
        return gun.acknowledge_inventory({request.token,true,{identity,request.before_revision+1,
            request.after_loaded,request.after_reserve}});
    }
    void insert(){grab();const auto request=insert_request();CHECK(commit(request).event==
        GnasherMechanicalEvent::inventory_committed);}
    void shot(){
        auto counts=gun.inventory();CHECK(counts.loaded>0);--counts.loaded;++counts.revision;
        GnasherShotReceipt receipt{identity,++shot_serial,sample.monotonic_ns,
            sample.game_tick,counts};
        CHECK(gun.after_original_shot(receipt).event==GnasherMechanicalEvent::shot_recorded);
        CHECK(gun.after_original_shot(receipt).event==GnasherMechanicalEvent::rejected);
        CHECK(gun.mechanism_pending()&&!gun.can_request_fire(sample.monotonic_ns));
        CHECK(angle(0).event==GnasherMechanicalEvent::restored);
    }
    GnasherCycleRequest stroke(std::int64_t final_tick=-1){
        const double path[]={0,.1,.2,.34,.48,.34,.2,.1,0};
        GnasherMechanicalDecision d{};
        for(unsigned i=0;i<sizeof(path)/sizeof(*path);++i){
            pose(path[i]);
            if(i==8&&final_tick>=0){CHECK(final_tick>=sample.game_tick);sample.game_tick=final_tick-10;}
            d=step();
            CHECK(d.drive_lever);
            CHECK(close(d.lever_angle_radians,path[i]));
            CHECK(d.lever_returning==(i>=4));
            CHECK(d.event==(i==8?GnasherMechanicalEvent::request_original_cycle:
                GnasherMechanicalEvent::none));
        }
        const auto request=gun.cycle_request();CHECK(request.token==d.request_token);
        CHECK(!gun.can_request_fire(sample.monotonic_ns));return request;
    }
    GnasherMechanicalDecision accept(GnasherCycleRequest const& request){
        return gun.acknowledge_cycle({identity,request.token,++cycle_serial,true},gun.inventory());
    }
    void recover(){
        CHECK(!gun.can_request_fire(sample.monotonic_ns));
        CHECK(angle(0).event==GnasherMechanicalEvent::restored);
        CHECK(!gun.animation_owns_hands());
    }
};

static void calibration_and_binding(){
    GnasherMechanicalInteraction gun;
    GnasherMechanicalConfig config{};
    CHECK(!config.valid());CHECK(!gun.bind(identity,config,{identity,1,2,10}));
    config=synthetic_config();config.lever_calibrated=false;
    CHECK(!gun.bind(identity,config,{identity,1,2,10}));
    config=synthetic_config();config.calibration_id=0;CHECK(!config.valid());
    config=synthetic_config();config.capacity=0;CHECK(!config.valid());
    config=synthetic_config();config.game_ticks_per_second=-1;CHECK(!config.valid());
    config=synthetic_config();config.right_grip_home_in_main={0,0,0,0};CHECK(!config.valid());
    config=synthetic_config();CHECK(!gun.bind(identity,config,{identity,1,9,10}));
    CHECK(!gun.bind(identity,config,{identity,0,2,10}));
    auto wrong=identity;wrong.world_generation=0;
    CHECK(!gun.bind(wrong,config,{wrong,1,2,10}));
    CHECK(gun.bind(identity,config,{identity,1,2,10}));
    CHECK(!gun.bind(identity,config,{identity,1,2,10}));
    CHECK(!gun.can_request_fire(1000000000));
    Fixture f;CHECK(f.gun.can_request_fire(f.sample.monotonic_ns));
    auto counts=f.gun.inventory();CHECK(f.gun.can_request_fire(f.sample.monotonic_ns,counts));
    ++counts.reserve;CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns,counts));
    counts=f.gun.inventory();++counts.revision;
    CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns,counts));
    CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns+100000001));
    f.shot();CHECK(!f.gun.bind(identity,f.config,f.gun.inventory()));
    CHECK(f.gun.mechanism_pending());
}

static void counts_and_reload_targets(){
    struct Example{unsigned loaded,reserve,target;};
    for(auto row:{Example{2,10,3},Example{6,9,2},Example{7,9,1},Example{2,1,1},Example{0,2,2}}){
        Fixture f(row.loaded,row.reserve);f.begin();CHECK(f.gun.target()==row.target);
        CHECK(f.gun.guide()==GuidePart::left_shoulder);
        CHECK(f.gun.begin_reload(identity,f.sample.monotonic_ns,f.sample.game_tick+1).event==
            GnasherMechanicalEvent::rejected);
        // A complete early lever stroke cannot replace the remaining rounds.
        for(double a:{0.,.1,.2,.34,.48,.34,.2,.1,0.})
            CHECK(f.angle(a).event!=GnasherMechanicalEvent::request_original_cycle);
        for(unsigned i=0;i<row.target;++i){
            const auto before=f.gun.inventory();f.grab();CHECK(f.gun.inventory().reserve==before.reserve);
            CHECK(f.gun.guide()==GuidePart::feed_port);
            const auto request=f.insert_request();
            CHECK(f.gun.inventory().loaded==before.loaded&&f.gun.inventory().reserve==before.reserve);
            CHECK(f.gun.cancel_reload(identity).event==GnasherMechanicalEvent::rejected);
            CHECK(f.gun.grab_round(identity,999,f.sample.monotonic_ns).event==GnasherMechanicalEvent::rejected);
            const auto d=f.commit(request);
            CHECK(d.reload_event==(i+1==row.target?ReloadEvent::mechanism_needed:ReloadEvent::inserted));
            CHECK(f.commit(request).event==GnasherMechanicalEvent::rejected);
            CHECK(f.gun.inventory().loaded==before.loaded+1&&f.gun.inventory().reserve==before.reserve-1);
            CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns));
        }
        CHECK(f.gun.guide()==GuidePart::lever_handle);
        CHECK(f.gun.grab_round(identity,999,f.sample.monotonic_ns).event==GnasherMechanicalEvent::rejected);
        const auto request=f.stroke();CHECK(request.completion==ReloadEvent::perfect);
        CHECK(f.gun.inventory().loaded==row.loaded+row.target);
        CHECK(f.gun.inventory().reserve==row.reserve-row.target);
        const auto ack=f.accept(request);CHECK(ack.event==GnasherMechanicalEvent::cycle_accepted);
        CHECK(ack.reload_event==ReloadEvent::perfect);f.recover();
        CHECK(f.gun.can_request_fire(f.sample.monotonic_ns));
        CHECK(f.accept(request).event==GnasherMechanicalEvent::rejected);
    }
    Fixture full(8,10);CHECK(full.gun.begin_reload(identity,full.sample.monotonic_ns,
        full.sample.game_tick).reload_event==ReloadEvent::full);
    Fixture empty_reserve(1,0);CHECK(empty_reserve.gun.begin_reload(identity,
        empty_reserve.sample.monotonic_ns,empty_reserve.sample.game_tick).reload_event==ReloadEvent::no_ammo);
    Fixture repeated(0,8);repeated.begin();for(int i=0;i<3;++i)repeated.insert();
    auto request=repeated.stroke();CHECK(repeated.accept(request).reload_event==ReloadEvent::perfect);
    repeated.recover();repeated.begin();CHECK(repeated.gun.target()==3);
    CHECK(repeated.gun.inventory().loaded==3&&repeated.gun.inventory().reserve==5);
}

static void deadline_and_cancel(){
    for(auto deadline:{std::int64_t(0),std::int64_t(1),std::int64_t(1000)}){
        Fixture f(7,1,deadline);f.begin();f.insert();
        const auto request=f.stroke(deadline==1000?f.reload_start+4000:-1);
        const auto expected=deadline==1000?ReloadEvent::perfect:ReloadEvent::ordinary;
        CHECK(request.completion==expected);CHECK(f.accept(request).reload_event==expected);
    }
    Fixture late(7,1,1000);late.begin();late.insert();
    CHECK(late.stroke(late.reload_start+4001).completion==ReloadEvent::ordinary);
    Fixture cancelled(2,5);cancelled.begin();cancelled.insert();cancelled.grab();
    const auto retained=cancelled.gun.inventory();
    CHECK(cancelled.gun.cancel_reload(identity).reload_event==ReloadEvent::cancelled);
    CHECK(cancelled.gun.inventory().loaded==retained.loaded&&cancelled.gun.inventory().reserve==retained.reserve);
    CHECK(cancelled.gun.held_round_serial()==0&&cancelled.gun.mechanism_pending());
    CHECK(cancelled.stroke().completion==ReloadEvent::cycled);
    Fixture released;released.begin();released.grab();const auto serial=released.round_serial;
    const auto inventory=released.gun.inventory();
    CHECK(released.gun.release_round(identity,serial+1).event==GnasherMechanicalEvent::rejected);
    CHECK(released.gun.release_round(identity,serial).reload_event==ReloadEvent::released);
    CHECK(released.gun.inventory().reserve==inventory.reserve);
    CHECK(released.gun.grab_round(identity,serial,released.sample.monotonic_ns).event==GnasherMechanicalEvent::rejected);
    CHECK(released.gun.cancel_reload(identity).reload_event==ReloadEvent::cancelled);
    CHECK(!released.gun.mechanism_pending()&&released.gun.can_request_fire(released.sample.monotonic_ns));
    // A final reload stroke also clears a preceding shot's cycle requirement.
    Fixture combined(2,1);combined.shot();combined.begin();combined.insert();
    const auto request=combined.stroke();CHECK(combined.accept(request).reload_event==ReloadEvent::perfect);
    combined.recover();CHECK(combined.gun.can_request_fire(combined.sample.monotonic_ns));
    CHECK(!combined.gun.mechanism_pending());
}

static void early_close_and_clock(){
    for(unsigned count:{1u,2u}){
        Fixture f(2,10);f.begin();for(unsigned i=0;i<count;++i)f.insert();
        CHECK(f.gun.request_close(identity,f.sample.monotonic_ns).reload_event==ReloadEvent::mechanism_needed);
        CHECK(f.gun.target()==3&&f.gun.inserted()==count);
        CHECK(f.gun.request_close(identity,f.sample.monotonic_ns).event==GnasherMechanicalEvent::rejected);
        CHECK(f.gun.stage()==GnasherMechanicalStage::lever);
        const auto r=f.stroke(f.reload_start+4000);CHECK(r.completion==ReloadEvent::ordinary);
        CHECK(f.accept(r).reload_event==ReloadEvent::ordinary);f.recover();
        CHECK(f.gun.inventory().loaded==2+count&&f.gun.inventory().reserve==10-count);
        CHECK(!f.gun.mechanism_pending()&&f.gun.can_request_fire(f.sample.monotonic_ns));
    }
    Fixture held;held.begin();held.grab();
    CHECK(held.gun.request_close(identity,held.sample.monotonic_ns).event==GnasherMechanicalEvent::rejected);
    const auto r=held.insert_request();
    CHECK(held.gun.request_close(identity,held.sample.monotonic_ns).event==GnasherMechanicalEvent::rejected);
    CHECK(held.commit(r).event==GnasherMechanicalEvent::inventory_committed);
    Fixture empty;empty.begin();CHECK(empty.gun.request_close(identity,empty.sample.monotonic_ns).reload_event==ReloadEvent::cancelled);
    CHECK(empty.gun.inventory().loaded==2&&empty.gun.inventory().reserve==10);
    CHECK(empty.gun.can_request_fire(empty.sample.monotonic_ns));
    Fixture combined;combined.shot();combined.begin();combined.insert();
    CHECK(combined.gun.request_close(identity,combined.sample.monotonic_ns).reload_event==ReloadEvent::mechanism_needed);
    CHECK(combined.accept(combined.stroke()).reload_event==ReloadEvent::ordinary);combined.recover();
    CHECK(!combined.gun.mechanism_pending());
    Fixture recovered(7,1);recovered.begin();recovered.insert();
    recovered.gun.tracking_lost();recovered.pose(0);recovered.step();
    CHECK(recovered.stroke(recovered.reload_start+4001).completion==ReloadEvent::ordinary);
}

static void adopt_original_shot(){
    Fixture f(2,0);f.shot();
    GnasherShotReceipt receipt{identity,1,f.sample.monotonic_ns,f.sample.game_tick,f.gun.inventory()};
    GnasherMechanicalInteraction adopted;
    auto invalid=receipt;invalid.original_event_sequence=0;CHECK(!adopted.bind_after_original_shot(f.config,invalid));
    invalid=receipt;++invalid.observed.identity.input_epoch;CHECK(!adopted.bind_after_original_shot(f.config,invalid));
    CHECK(adopted.bind_after_original_shot(f.config,receipt));
    CHECK(!adopted.can_request_fire(receipt.monotonic_ns)&&adopted.mechanism_pending());
    CHECK(!adopted.bind_after_original_shot(f.config,receipt));
    CHECK(adopted.after_original_shot(receipt).event==GnasherMechanicalEvent::rejected);
    f.gun=adopted;CHECK(f.angle(0).event==GnasherMechanicalEvent::restored);
    CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns)&&f.gun.mechanism_pending());
    const auto cycle=f.stroke();CHECK(f.accept(cycle).event==GnasherMechanicalEvent::cycle_accepted);f.recover();
    CHECK(f.gun.can_request_fire(f.sample.monotonic_ns));
}

static void atomic_closed_cycle(){
    Fixture f(2,0);GnasherFireCycle fire;CHECK(fire.bind(f.gun.inventory()));
    f.shot();CHECK(fire.returned(1,f.sample.monotonic_ns,f.sample.game_tick));
    CHECK(fire.confirm(f.gun.inventory()));const auto request=f.stroke();
    GnasherCycleReceipt receipt{identity,request.token,1,true};
    auto wrong=f.gun.inventory();++wrong.reserve;
    CHECK(!gnasher_commit_closed_cycle(f.gun,fire,receipt,wrong));
    CHECK(f.gun.mechanism_pending()&&fire.blocked());CHECK(f.gun.cycle_request().token==request.token);
    CHECK(gnasher_commit_closed_cycle(f.gun,fire,receipt,f.gun.inventory()));
    CHECK(!f.gun.mechanism_pending()&&!fire.blocked());
    CHECK(!gnasher_commit_closed_cycle(f.gun,fire,receipt,f.gun.inventory()));f.recover();
    CHECK(f.gun.can_request_fire(f.sample.monotonic_ns));
    Fixture invalid(2,0);GnasherFireCycle other;CHECK(other.bind(invalid.gun.inventory()));invalid.shot();
    CHECK(other.returned(1,invalid.sample.monotonic_ns,invalid.sample.game_tick));
    CHECK(!other.returned(2,invalid.sample.monotonic_ns+1,invalid.sample.game_tick));
    auto r=invalid.stroke();CHECK(!gnasher_commit_closed_cycle(invalid.gun,other,{identity,r.token,1,true},invalid.gun.inventory()));
    CHECK(invalid.gun.mechanism_pending()&&other.blocked());
}


static void atomic_reload_inventory_and_cycle(){
    for(bool preceding_shot : {false,true})for(unsigned count : {1u,2u,3u}){
        Fixture f(2,10);GnasherFireCycle fire;CHECK(fire.bind(f.gun.inventory()));
        if(preceding_shot){f.shot();CHECK(fire.returned(1,f.sample.monotonic_ns,f.sample.game_tick));
            CHECK(fire.confirm(f.gun.inventory()));}
        const auto original_shot=fire.receipt();f.begin();
        for(unsigned n=0;n<count;++n){
            f.grab();const auto request=f.insert_request();
            GnasherInventoryReceipt receipt{request.token,true,{identity,request.before_revision+2,
                request.after_loaded,request.after_reserve}};
            auto wrong=receipt;wrong.observed.reserve=request.before_reserve;
            const auto before=f.gun.inventory();CHECK(!gnasher_commit_inserted_round(f.gun,fire,wrong));
            CHECK(f.gun.inventory().loaded==before.loaded&&fire.inventory().loaded==before.loaded);
            CHECK(f.gun.inventory_request().token==request.token);
            CHECK(gnasher_commit_inserted_round(f.gun,fire,receipt));
            CHECK(!gnasher_commit_inserted_round(f.gun,fire,receipt));
            CHECK(fire.inventory().loaded==f.gun.inventory().loaded&&fire.inventory().reserve==f.gun.inventory().reserve);
            CHECK(fire.reload_cycle_pending()&&fire.blocked()&&fire.can_request_cycle());
            CHECK(fire.confirmed()==preceding_shot);
            CHECK(fire.receipt().original_event_sequence==original_shot.original_event_sequence);
            CHECK(fire.receipt().observed.loaded==original_shot.observed.loaded); // Historical shot is not rewritten.
        }
        if(count<3)CHECK(f.gun.request_close(identity,f.sample.monotonic_ns).reload_event==ReloadEvent::mechanism_needed);
        const auto closure=f.stroke();GnasherCycleReceipt receipt{identity,closure.token,1,true};
        const auto expected=count==3?ReloadEvent::perfect:ReloadEvent::ordinary;
        CHECK(closure.completion==expected);
        CHECK(gnasher_commit_closed_cycle(f.gun,fire,receipt,f.gun.inventory()));
        CHECK(!fire.blocked()&&!fire.reload_cycle_pending()&&!fire.confirmed()&&!f.gun.mechanism_pending());
        CHECK(!gnasher_commit_closed_cycle(f.gun,fire,receipt,f.gun.inventory()));f.recover();
        CHECK(f.gun.inventory().loaded==2-unsigned(preceding_shot)+count&&f.gun.inventory().reserve==10-count);
        CHECK(fire.prepare(f.gun.inventory()));CHECK(f.gun.can_request_fire(f.sample.monotonic_ns,fire.inventory()));
        // Exactly one real next shot; neither ledger retains pre-reload counts.
        f.shot();CHECK(fire.returned(20,f.sample.monotonic_ns,f.sample.game_tick));CHECK(fire.confirm(f.gun.inventory()));
        CHECK(fire.blocked()&&f.gun.mechanism_pending());
    }
    Fixture failed(2,10);GnasherFireCycle fire;CHECK(fire.bind(failed.gun.inventory()));
    failed.begin();failed.grab();const auto request=failed.insert_request();
    CHECK(fire.unresolved_return(1));
    const GnasherInventoryReceipt receipt{request.token,true,{identity,request.before_revision+1,
        request.after_loaded,request.after_reserve}};
    CHECK(!gnasher_commit_inserted_round(failed.gun,fire,receipt));
    CHECK(failed.gun.inventory().loaded==2&&failed.gun.inventory().reserve==10&&failed.gun.inventory_request().token==request.token);
    // A known rejected/compensated original write returns only this proposal;
    // it must not clear a preceding shot or release the still-held round.
    Fixture rejected(2,10);GnasherFireCycle prior;CHECK(prior.bind(rejected.gun.inventory()));
    rejected.shot();CHECK(prior.returned(1,rejected.sample.monotonic_ns,rejected.sample.game_tick));CHECK(prior.confirm(rejected.gun.inventory()));
    const auto history=prior.receipt();rejected.begin();rejected.grab();const auto attempt=rejected.insert_request();
    const GnasherInventoryReceipt denial{attempt.token,false,{identity,attempt.before_revision+2,attempt.before_loaded,attempt.before_reserve}};
    CHECK(gnasher_reject_inserted_round(rejected.gun,prior,denial));
    CHECK(prior.blocked()&&prior.confirmed()&&prior.receipt().original_event_sequence==history.original_event_sequence);
    CHECK(rejected.gun.held_round_serial()==attempt.round_serial&&!rejected.gun.inventory_request().token);
    const auto retry=rejected.insert_request();CHECK(retry.token>attempt.token&&retry.before_revision==denial.observed.revision);
    CHECK(gnasher_commit_inserted_round(rejected.gun,prior,{retry.token,true,{identity,retry.before_revision+2,retry.after_loaded,retry.after_reserve}}));
    // A mechanical-only commit is insufficient to bypass the paired ledger.
    Fixture unpaired(2,10);GnasherFireCycle observer;CHECK(observer.bind(unpaired.gun.inventory()));
    unpaired.begin();unpaired.insert();CHECK(unpaired.gun.request_close(identity,unpaired.sample.monotonic_ns).reload_event==ReloadEvent::mechanism_needed);
    const auto closure=unpaired.stroke();CHECK(!gnasher_commit_closed_cycle(unpaired.gun,observer,
        {identity,closure.token,1,true},unpaired.gun.inventory()));CHECK(unpaired.gun.mechanism_pending());
}

static void original_boundaries(){
    Fixture f(2,0);
    for(int i=0;i<2;++i){
        f.shot();const auto before=f.gun.inventory();const auto request=f.stroke();
        CHECK(request.action==MechanismAction::lever_down_return&&request.completion==ReloadEvent::cycled);
        CHECK(f.angle(0).event==GnasherMechanicalEvent::none); // proposal emits once
        CHECK(f.gun.inventory().loaded==before.loaded&&f.gun.inventory().reserve==before.reserve);
        CHECK(f.gun.acknowledge_cycle({identity,request.token+1,1,true},f.gun.inventory()).event==
            GnasherMechanicalEvent::rejected);
        CHECK(f.accept(request).reload_event==ReloadEvent::cycled);
        f.sample.original_animation_active=true;f.sample.left_tracked=false;
        auto d=f.step();CHECK(d.event==GnasherMechanicalEvent::animation_handoff);
        CHECK(d.animation_owns_hands&&!d.drive_lever&&!f.gun.can_request_fire(f.sample.monotonic_ns));
        f.sample.original_animation_active=false;f.sample.original_ready=true;
        CHECK(f.step().animation_owns_hands); // tracking outage cannot restore hands
        f.sample.left_tracked=true;f.sample.original_ready=false;
        CHECK(f.step().animation_owns_hands); // original recovery is separate
        f.sample.original_ready=true;f.recover();
        CHECK(f.gun.can_request_fire(f.sample.monotonic_ns)==(i==0));
    }
    Fixture rejected;rejected.shot();auto request=rejected.stroke();
    CHECK(rejected.gun.acknowledge_cycle({identity,request.token,1,false},
        rejected.gun.inventory()).event==GnasherMechanicalEvent::rejected);
    CHECK(rejected.gun.mechanism_pending());rejected.recover();
    request=rejected.stroke();CHECK(request.completion==ReloadEvent::cycled);
    Fixture divergent;auto counts=divergent.gun.inventory();counts.revision++;counts.loaded=0;
    CHECK(divergent.gun.after_original_shot({identity,1,divergent.sample.monotonic_ns,
        divergent.sample.game_tick,counts}).event==GnasherMechanicalEvent::invalidated);
    CHECK(!divergent.gun.can_request_fire(divergent.sample.monotonic_ns));
    Fixture unexpected;unexpected.shot();counts=unexpected.gun.inventory();--counts.loaded;++counts.revision;
    CHECK(unexpected.gun.after_original_shot({identity,2,unexpected.sample.monotonic_ns,
        unexpected.sample.game_tick,counts}).event==GnasherMechanicalEvent::invalidated);
}

static void interrupted_strokes(){
    // Discontinuous geometry/replay still invalidates the stroke. Ordinary
    // tracking/grip loss freezes it and needs the separate fresh regrasp path.
    for(int failure : {4,5,7,8,9,10,11}){
        Fixture f;f.shot();CHECK(f.angle(0).drive_lever);CHECK(f.angle(.2).drive_lever);
        CHECK(f.angle(.48).drive_lever);f.pose(.34);
        switch(failure){
            case 0:f.sample.left_tracked=false;break;
            case 1:f.sample.right_tracked=false;break;
            case 2:f.sample.left_front_held=false;break;
            case 3:f.sample.right_handle_held=false;break;
            case 4:f.sample.left_grip.position.x+=2;f.sample.right_grip.position.x+=2;break;
            case 5:f.sample.left_grip.orientation={0,1,0,0};break;
            case 6:f.sample.monotonic_ns+=200000000;break;
            case 7:f.sample.right_grip.position.x+=.06;break;
            case 8:f.sample.right_grip.orientation={0,1,0,0};break;
            case 9:f.sample.right_grip.position.x=std::numeric_limits<double>::quiet_NaN();break;
            case 10:f.sample.sequence-=2;break;
            case 11:f.sample.monotonic_ns-=20000000;break;
        }
        CHECK(f.step().event!=GnasherMechanicalEvent::request_original_cycle);
        f.sample.left_tracked=f.sample.right_tracked=f.sample.right_handle_held=true;
        if(failure==10)f.sample.sequence+=2;
        for(double a:{.2,.1,0.})CHECK(f.angle(a).event!=GnasherMechanicalEvent::request_original_cycle);
        CHECK(f.gun.mechanism_pending());CHECK(f.stroke().completion==ReloadEvent::cycled);
    }
    Fixture backwards;backwards.shot();
    for(double a:{0.,-.1,-.2,-.34,-.48,-.34,-.2,-.1,0.})
        CHECK(backwards.angle(a).event!=GnasherMechanicalEvent::request_original_cycle);
    Fixture overswing;overswing.shot();
    for(double a:{0.,.1,.2,.3,.4,.5,.6,.72,.6,.5,.4,.3,.2,.1,0.})
        CHECK(overswing.angle(a).event!=GnasherMechanicalEvent::request_original_cycle);
    Fixture too_fast;too_fast.shot();
    for(double a:{0.,.1,.2,.3,.4,.48,.4,.3,.2,.1,0.})
        CHECK(too_fast.angle(a,10000000).event!=GnasherMechanicalEvent::request_original_cycle);

}


static void frozen_regrasp(){
    // Pause on the down and return branches. The exact original phase/angle
    // remains; old held input and regrasp at a skipped angle advance nothing.
    for(double frozen : {.2,.48})for(int failure=0;failure<6;++failure){
        Fixture f;f.shot();CHECK(f.angle(0).drive_lever);CHECK(f.angle(.2).drive_lever);
        if(frozen>.2){CHECK(f.angle(.34).drive_lever);CHECK(f.angle(frozen).drive_lever);}
        auto lost=f.sample;
        if(failure==0)lost.left_tracked=false;
        if(failure==1)lost.right_tracked=false;
        if(failure==2)lost.left_front_held=false;
        if(failure==3)lost.right_handle_held=false;
        if(failure==4)lost.monotonic_ns+=200000000;
        if(failure<5){++lost.sequence;lost.monotonic_ns+=20000000;lost.game_tick+=10;
            CHECK(f.gun.observe(lost).event!=GnasherMechanicalEvent::request_original_cycle);
            f.sample=lost;}
        else f.gun.tracking_lost(); // Failed factory / unavailable source.
        f.sample.left_tracked=f.sample.right_tracked=f.sample.right_handle_held=true;
        for(double a:{frozen,.1,0.}){const auto d=f.angle(a);if(d.drive_lever)std::fprintf(stderr,"unexpected resume frozen=%g failure=%d angle=%g\n",frozen,failure,a);CHECK(!d.drive_lever);}
        CHECK(f.gun.mechanism_pending()&&!f.gun.cycle_request().token);
        ++f.sample.support_grasp_token;
        // Fresh grip at home cannot teleport the frozen mechanism to home.
        CHECK(!f.angle(0).drive_lever);CHECK(!f.gun.cycle_request().token);
        // Return physically to the frozen contact before adopting the new grip.
        CHECK(f.angle(.1).event!=GnasherMechanicalEvent::request_original_cycle);
        if(frozen>.2){CHECK(f.angle(.2).event!=GnasherMechanicalEvent::request_original_cycle);
            CHECK(f.angle(.34).event!=GnasherMechanicalEvent::request_original_cycle);
            CHECK(!f.angle(frozen).drive_lever);}
        else CHECK(!f.angle(frozen).drive_lever);
        // The regrasp frame itself cannot complete or count unseen movement.
        CHECK(f.gun.mechanism_pending()&&!f.gun.cycle_request().token);
        if(frozen<.4){CHECK(f.angle(.34).drive_lever);CHECK(f.angle(.48).drive_lever);}
        for(double a:{.34,.2,.1})CHECK(f.angle(a).drive_lever);
        const auto d=f.angle(0);CHECK(d.event==GnasherMechanicalEvent::request_original_cycle);
        CHECK(f.accept(f.gun.cycle_request()).reload_event==ReloadEvent::cycled);
        f.recover();CHECK(f.gun.can_request_fire(f.sample.monotonic_ns));
    }
    // Keep committed rounds and the ORIGINAL four-second deadline across loss.
    Fixture reload(2,10);reload.begin();reload.insert();reload.insert();
    CHECK(reload.gun.request_close(identity,reload.sample.monotonic_ns).reload_event==ReloadEvent::mechanism_needed);
    CHECK(reload.angle(0).drive_lever);CHECK(reload.angle(.2).drive_lever);
    reload.gun.tracking_lost();reload.sample.monotonic_ns+=5000000000;
    reload.sample.game_tick=reload.reload_start+5000;++reload.sample.support_grasp_token;
    CHECK(!reload.angle(.2).drive_lever);CHECK(reload.gun.inserted()==2&&reload.gun.target()==3);
    for(double a:{.34,.48,.34,.2,.1})CHECK(reload.angle(a).drive_lever);
    CHECK(reload.angle(0).reload_event==ReloadEvent::ordinary);
    CHECK(reload.accept(reload.gun.cycle_request()).reload_event==ReloadEvent::ordinary);
    CHECK(reload.gun.inventory().loaded==4&&reload.gun.inventory().reserve==8);
}

static void identity_and_pose_frames(){
    for(int field=0;field<8;++field){
        Fixture f;f.shot();f.angle(0);f.angle(.2);f.angle(.48);
        switch(field){
            case 0:++f.sample.identity.world;break;
            case 1:++f.sample.identity.pawn;break;
            case 2:++f.sample.identity.weapon;break;
            case 3:++f.sample.identity.world_generation;break;
            case 4:++f.sample.identity.session;break;
            case 5:++f.sample.identity.space_epoch;break;
            case 6:++f.sample.identity.input_epoch;break;
            case 7:++f.sample.identity.profile_generation;break;
        }
        CHECK(f.step().event==GnasherMechanicalEvent::invalidated);
        CHECK(f.gun.stage()==GnasherMechanicalStage::unbound);
    }
    // A continuously translated/rotated complete gun retains the local hinge.
    Fixture f;f.shot();
    for(int i=1;i<=20;++i){
        f.body={0,std::sin(i*.01),0,std::cos(i*.01)};f.base.x=i*.003;
        CHECK(f.angle(0).event!=GnasherMechanicalEvent::request_original_cycle);
    }
    CHECK(f.stroke().completion==ReloadEvent::cycled);
    ControllerInputFrame hands{};StereoSample stereo{};
    hands.session=stereo.session=identity.session;hands.space_epoch=stereo.space_epoch=identity.space_epoch;
    hands.sequence=stereo.sequence=1;hands.input_epoch=identity.input_epoch;
    hands.profile_generation=identity.profile_generation;hands.predicted_time=stereo.predicted_time=10;
    hands.focused=stereo.focused=1;hands.sync_ns=100;hands.received_ns=200;
    for(auto& hand:hands.hands){hand.grip_active=1;hand.grip_flags=15;}
    GnasherMechanicalSample sample{};
    CHECK(gnasher_mechanical_sample_from_hands(hands,stereo,identity,300,0,true,true,0,false,true,sample));
    CHECK(sample.left_tracked&&sample.right_tracked&&sample.identity==identity);
    CHECK(sample.monotonic_ns==hands.sync_ns); // Never refresh source age during conversion.
    hands.hands[0].grip_flags=3;
    CHECK(gnasher_mechanical_sample_from_hands(hands,stereo,identity,300,0,true,true,0,false,true,sample));
    CHECK(!sample.left_tracked);hands.hands[0].grip_flags=15;
    ++hands.input_epoch;
    CHECK(!gnasher_mechanical_sample_from_hands(hands,stereo,identity,300,0,true,true,0,false,true,sample));
    --hands.input_epoch;stereo.head.position={4,5,6};
    CHECK(gnasher_mechanical_sample_from_hands(hands,stereo,identity,300,0,true,true,0,false,true,sample));
    ++stereo.sequence;
    CHECK(!gnasher_mechanical_sample_from_hands(hands,stereo,identity,300,0,true,true,0,false,true,sample));
}

static void original_reload_acceptance(){
    for(unsigned field=0;field<9;++field){
        Fixture f;
        GnasherReloadAcceptance receipt{identity,f.sample.sequence,f.sample.monotonic_ns,
            f.sample.monotonic_ns+100000000,f.sample.game_tick,f.gun.inventory(),true};
        switch(field){
            case 0:receipt.accepted=false;break;
            case 1:++receipt.identity.input_epoch;break;
            case 2:++receipt.source_sequence;break;
            case 3:--receipt.source_sync_ns;break;
            case 4:receipt.source_admitted_ns=receipt.source_sync_ns-1;break;
            case 5:++receipt.source_admitted_ns;break;
            case 6:--receipt.observed.revision;break;
            case 7:++receipt.observed.loaded;break;
            case 8:--receipt.original_start_game_tick;break;
        }
        CHECK(f.gun.accept_original_reload(receipt).event==GnasherMechanicalEvent::rejected);
        CHECK(!f.gun.reload_active()&&f.gun.inventory().revision==1);
    }
    Fixture f;
    const auto original_start=f.sample.game_tick+500;
    GnasherReloadAcceptance receipt{identity,f.sample.sequence,f.sample.monotonic_ns,
        f.sample.monotonic_ns+100000000,original_start,f.gun.inventory(),true};
    ++receipt.observed.revision;
    CHECK(f.gun.accept_original_reload(receipt).event==GnasherMechanicalEvent::reload_started);
    CHECK(f.gun.reload_active()&&f.gun.inventory().revision==2&&f.gun.target()==3);
    CHECK(f.gun.grab_round(identity,1,receipt.source_admitted_ns+1).event==GnasherMechanicalEvent::rejected);
    // A delayed receipt neither refreshes the admitted raw sample nor restarts
    // the original game-clock window when observed a second time.
    receipt.original_start_game_tick+=10000;
    CHECK(f.gun.accept_original_reload(receipt).event==GnasherMechanicalEvent::rejected);
    f.sample.game_tick=original_start;CHECK(f.angle(0).event==GnasherMechanicalEvent::none);
    f.insert();f.insert();f.insert();
    CHECK(f.stroke(original_start+4000).completion==ReloadEvent::perfect);
    CHECK(f.accept(f.gun.cycle_request()).reload_event==ReloadEvent::perfect);
    CHECK(!f.gun.reload_active());
}

static void parked_reload(){
    Fixture f;GnasherFireCycle fire;CHECK(fire.bind(f.gun.inventory()));f.begin();
    for(unsigned i=0;i<2;++i){f.grab();const auto request=f.insert_request();
        CHECK(gnasher_commit_inserted_round(f.gun,fire,{request.token,true,{identity,
            request.before_revision+1,request.after_loaded,request.after_reserve}}));}
    CHECK(f.gun.request_close(identity,f.sample.monotonic_ns).reload_event==ReloadEvent::mechanism_needed);
    CHECK(f.angle(0).drive_lever);CHECK(f.angle(.2).drive_lever);
    CHECK(f.gun.suspend_hold()&&f.gun.hold_suspended());
    const auto clock=f.gun.reload_started_game_tick();const auto before=f.gun.inventory();
    f.sample.game_tick=clock+6000;f.sample.monotonic_ns+=6000000000;
    CHECK(f.angle(.48).event==GnasherMechanicalEvent::rejected&&f.gun.hold_suspended());
    auto wrong=before;++wrong.loaded;
    CHECK(!gnasher_resume_held_consumers(f.gun,fire,wrong)&&f.gun.hold_suspended());
    wrong=before;++wrong.identity.space_epoch;
    CHECK(!gnasher_resume_held_consumers(f.gun,fire,wrong)&&f.gun.hold_suspended());
    auto observed=before;++observed.revision;
    CHECK(gnasher_resume_held_consumers(f.gun,fire,observed));
    CHECK(!f.gun.hold_suspended()&&!f.gun.can_request_fire(f.sample.monotonic_ns));
    CHECK(f.gun.reload_started_game_tick()==clock&&f.gun.target()==3&&f.gun.inserted()==2);
    ++f.sample.support_grasp_token;CHECK(!f.angle(.2).drive_lever);
    for(double a:{.34,.48,.34,.2,.1})CHECK(f.angle(a).drive_lever);
    CHECK(f.angle(0).reload_event==ReloadEvent::ordinary);
    CHECK(gnasher_commit_closed_cycle(f.gun,fire,{identity,f.gun.cycle_request().token,1,true},observed));
    CHECK(!fire.blocked()&&f.gun.inventory().loaded==4&&f.gun.inventory().reserve==8);
    Fixture held;held.begin();held.grab();CHECK(!held.gun.suspend_hold());
    const auto request=held.insert_request();CHECK(!held.gun.suspend_hold());
    CHECK(held.commit(request).event==GnasherMechanicalEvent::inventory_committed);
    CHECK(held.gun.suspend_hold());
    Fixture completed;completed.shot();completed.stroke();CHECK(!completed.gun.suspend_hold());
}

static void insertion_and_receipts(){
    Fixture f;f.begin();f.grab();
    CHECK(f.depth(.002).event==GnasherMechanicalEvent::rejected); // seated without approach
    CHECK(f.depth(.07).event==GnasherMechanicalEvent::none);
    CHECK(f.depth(.002,1000000).event==GnasherMechanicalEvent::rejected); // teleport
    CHECK(f.gun.inventory_request().token==0);
    f.depth(.002);f.depth(.07);f.sample.left_round_serial=f.round_serial+1;
    CHECK(f.step().event==GnasherMechanicalEvent::rejected);
    CHECK(f.gun.inventory_request().token==0);
    auto request=f.insert_request();auto before=f.gun.inventory();
    CHECK(f.gun.acknowledge_inventory({request.token,false,before}).event==GnasherMechanicalEvent::rejected);
    CHECK(f.gun.inventory().loaded==before.loaded&&f.gun.held_round_serial()==f.round_serial);
    const auto retry=f.insert_request();CHECK(retry.token>request.token);
    CHECK(f.commit(request).event==GnasherMechanicalEvent::rejected);
    CHECK(f.commit(retry).event==GnasherMechanicalEvent::inventory_committed);
    Fixture wrong_counts;wrong_counts.begin();wrong_counts.grab();request=wrong_counts.insert_request();
    CHECK(wrong_counts.gun.acknowledge_inventory({request.token,true,{identity,
        request.before_revision+1,request.after_loaded,request.before_reserve}}).event==
        GnasherMechanicalEvent::invalidated);
    Fixture wrong_cycle;wrong_cycle.shot();const auto cycle=wrong_cycle.stroke();
    before=wrong_cycle.gun.inventory();++before.reserve;
    CHECK(wrong_cycle.gun.acknowledge_cycle({identity,cycle.token,1,true},before).event==
        GnasherMechanicalEvent::invalidated);
    // Reset/rebind cannot recycle a request token from a delayed receipt.
    Fixture rebound;rebound.begin();rebound.grab();request=rebound.insert_request();
    before=rebound.gun.inventory();rebound.gun.reset();
    CHECK(rebound.gun.bind(identity,rebound.config,before));rebound.pose(0);rebound.step();
    rebound.begin();rebound.grab();const auto newer=rebound.insert_request();CHECK(newer.token>request.token);
    CHECK(rebound.commit(request).event==GnasherMechanicalEvent::rejected);
    CHECK(rebound.commit(newer).event==GnasherMechanicalEvent::inventory_committed);
}

static void accepted_original_shot_latency(){
    Fixture f(8,10);const auto before=f.gun.inventory();
    const GnasherShotAcceptance accepted{identity,99,f.sample.sequence,f.sample.monotonic_ns,
        f.sample.monotonic_ns+1,before,true};
    const GnasherShotReceipt returned{identity,99,f.sample.monotonic_ns+2000000000,
        f.sample.game_tick,{identity,before.revision+1,7,10}};
    CHECK(f.gun.accept_original_shot(returned,accepted).event==GnasherMechanicalEvent::shot_recorded);
    CHECK(f.gun.inventory().loaded==7&&f.gun.mechanism_pending());
    CHECK(!f.gun.can_request_fire(returned.monotonic_ns));
    CHECK(f.gun.accept_original_shot(returned,accepted).event==GnasherMechanicalEvent::rejected);
    for(unsigned field=0;field<7;++field){Fixture bad(8,10);auto a=accepted;auto r=returned;
        if(field==0)++a.source_sequence;if(field==1)++a.source_sync_ns;if(field==2)a.source_admitted_ns+=100000000;
        if(field==3)++a.before.loaded;if(field==4)++a.original_event_sequence;if(field==5)a.accepted=false;
        if(field==6)r.monotonic_ns=a.source_admitted_ns-1;
        CHECK(bad.gun.accept_original_shot(r,a).event==GnasherMechanicalEvent::rejected);
        CHECK(bad.gun.inventory().loaded==8&&!bad.gun.mechanism_pending());
    }
}
static void rejected_unpublished_cycle(){
    Fixture f;f.shot();const auto request=f.stroke();const auto counts=f.gun.inventory();
    CHECK(!f.gun.reject_unpublished_cycle(request.token+1));
    CHECK(f.gun.reject_unpublished_cycle(request.token));
    CHECK(f.gun.mechanism_pending()&&!f.gun.can_request_fire(f.sample.monotonic_ns));
    CHECK(f.gun.inventory().loaded==counts.loaded&&f.gun.inventory().reserve==counts.reserve);
    CHECK(!f.gun.reject_unpublished_cycle(request.token));
    const auto next=f.stroke();CHECK(next.token>request.token);
    CHECK(f.accept(request).event==GnasherMechanicalEvent::rejected);
    CHECK(f.accept(next).event==GnasherMechanicalEvent::cycle_accepted);
}
static void idle_cadence_baseline(){
    for(const auto gap:{120000000ll,200000000ll,500000000ll}){
        Fixture f(8,31);const auto before=f.gun.inventory();const auto d=f.step(gap);
        CHECK(d.event==GnasherMechanicalEvent::none&&!d.drive_lever);
        CHECK(f.gun.can_request_fire(f.sample.monotonic_ns+20000000,before));
        CHECK(f.gun.source_matches(f.sample.sequence,f.sample.monotonic_ns));
        CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns+100000001,before));
        CHECK(!f.gun.mechanism_pending()&&!f.gun.cycle_request().token&&!f.gun.inventory_request().token);
        CHECK(f.gun.inventory().loaded==8&&f.gun.inventory().reserve==31);
        CHECK(f.gun.observe(f.sample).event==GnasherMechanicalEvent::rejected);
        CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns,before)); // Replay still revokes tracking.
    }
    {Fixture f;f.shot();CHECK(f.step(200000000).event==GnasherMechanicalEvent::rejected);
     CHECK(f.gun.mechanism_pending()&&!f.gun.can_request_fire(f.sample.monotonic_ns)&&!f.gun.cycle_request().token);}
    {Fixture f;f.begin();const auto start=f.gun.reload_started_game_tick();const auto target=f.gun.target();
     CHECK(f.step(200000000).event==GnasherMechanicalEvent::rejected);
     CHECK(f.gun.reload_active()&&f.gun.reload_started_game_tick()==start&&f.gun.target()==target&&!f.gun.can_request_fire(f.sample.monotonic_ns));}
    {Fixture f;f.sample.original_ready=false;CHECK(f.step(200000000).event==GnasherMechanicalEvent::rejected);CHECK(!f.gun.can_request_fire(f.sample.monotonic_ns));}
}
int main(){
    idle_cadence_baseline();
    accepted_original_shot_latency();
    rejected_unpublished_cycle();
    calibration_and_binding();counts_and_reload_targets();deadline_and_cancel();original_reload_acceptance();parked_reload();
    early_close_and_clock();adopt_original_shot();atomic_closed_cycle();atomic_reload_inventory_and_cycle();original_boundaries();interrupted_strokes();frozen_regrasp();identity_and_pose_frames();insertion_and_receipts();
    std::puts("Gnasher 6DoF mechanics: calibration, <=3-round transactions, per-shot lever,");
    std::puts("inclusive four-second game-clock deadlines and partial ordinary closure, original handoff, identity and discontinuity checks passed");
}
