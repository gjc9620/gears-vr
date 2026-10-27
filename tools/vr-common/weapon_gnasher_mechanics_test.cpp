#include "weapon_gnasher_mechanics.h"
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
    c.capacity=8;c.perfect_end_ticks=deadline;
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
        sample.original_ready=true;pose(0);CHECK(step().event==GnasherMechanicalEvent::none);
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
        const auto request=gun.inventory_request();CHECK(request.token!=0);return request;
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
    config=synthetic_config();config.perfect_end_ticks=-1;CHECK(!config.valid());
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
        const auto request=f.stroke(deadline==1000?f.reload_start+1000:-1);
        const auto expected=deadline==1000?ReloadEvent::perfect:ReloadEvent::ordinary;
        CHECK(request.completion==expected);CHECK(f.accept(request).reload_event==expected);
    }
    Fixture late(7,1,1000);late.begin();late.insert();
    CHECK(late.stroke(late.reload_start+1001).completion==ReloadEvent::ordinary);
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
    // Loss, jumps, gaps, release, incorrect geometry and ordering all require
    // a new home -> down -> home stroke; finishing the old return is useless.
    for(int failure=0;failure<12;++failure){
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
    Fixture factory_failure;factory_failure.shot();factory_failure.angle(0);factory_failure.angle(.2);
    factory_failure.angle(.48);factory_failure.gun.tracking_lost();
    for(double a:{.34,.2,.1,0.})CHECK(factory_failure.angle(a).event!=GnasherMechanicalEvent::request_original_cycle);
    CHECK(factory_failure.stroke().completion==ReloadEvent::cycled);
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

int main(){
    calibration_and_binding();counts_and_reload_targets();deadline_and_cancel();
    original_boundaries();interrupted_strokes();identity_and_pose_frames();insertion_and_receipts();
    std::puts("Gnasher 6DoF mechanics: calibration, <=3-round transactions, per-shot lever,");
    std::puts("ordinary/verified-perfect deadlines, original handoff, identity and discontinuity checks passed");
}
