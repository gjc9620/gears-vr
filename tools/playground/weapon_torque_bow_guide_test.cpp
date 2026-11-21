#include "../vr-common/weapon_torque_bow_guide.h"
#include "weapon_torque_bow_arrow_tracking_fixture.h"
#include <limits>
namespace g=gears_torque_bow_guide;namespace w=gears_weapon;namespace vr=gears_vr;
#define CHECK BOW_ARROW_CHECK
struct F:bow_arrow_tracking_fixture::Fixture {
    g::Selector selector;g::Sample s{};
    F(){s.source=hand;s.hold=s.config=1;s.now=bow_arrow_tracking_fixture::now;s.stage=w::TorqueBowStage::ready;s.reserve=1;}
    void next(){s.now+=20000000;++s.source.key.frame;++s.source.controllers.sequence;s.source.stereo.sequence=s.source.controllers.sequence;
        s.source.now_ns=s.now-1000000;s.source.controllers.sync_ns=s.now-4000000;s.source.controllers.received_ns=s.now-3000000;s.source.stereo.received_ns=s.now-2000000;}
    g::View run(){return selector.advance(s);}
};
int main(){
    F f;CHECK(f.run().step==g::Step::shoulder);f.next();f.s.stage=w::TorqueBowStage::reload_steps;f.s.held_arrow=true;CHECK(f.run().step==g::Step::feed);
    f.next();f.s.stage=w::TorqueBowStage::reload_commit;CHECK(f.run().step==g::Step::none);
    f.next();f.s.stage=w::TorqueBowStage::ready;f.s.held_arrow=false;f.s.loaded=1;f.s.reserve=0;f.s.receipts.loaded=4;
    CHECK(f.run().step==g::Step::locked);CHECK(f.run().step==g::Step::locked);
    f.next();f.s.stage=w::TorqueBowStage::charging;f.s.charging=true;CHECK(f.run().step==g::Step::none);
    f.s.progress_known=true;f.s.charge_count=.75;f.s.charge_max=1.5;CHECK(f.run().step==g::Step::charging&&f.run().progress==.5);
    f.next();f.s.charge_count=3;CHECK(f.run().progress==1&&f.s.charge_count==3); // Original fatigue time is untouched.
    f.s.charge_max=0;CHECK(f.run().step==g::Step::none);f.s.charge_max=1.5;
    f.next();f.s.stage=w::TorqueBowStage::charge_abort;CHECK(f.run().step==g::Step::none);
    f.next();f.s.stage=w::TorqueBowStage::ready;f.s.charging=false;f.s.receipts.cancelled=9;CHECK(f.run().step==g::Step::cancelled);
    auto old=f.s;f.selector.lost();f.next();CHECK(f.run().step==g::Step::none);f.s=old;CHECK(f.run().step==g::Step::none);
    F shot;shot.s.loaded=1;shot.run();shot.next();shot.s.stage=w::TorqueBowStage::await_original;shot.s.loaded=0;shot.s.receipts.projectile=1;
    CHECK(shot.run().step==g::Step::fired);shot.next();shot.s.stage=w::TorqueBowStage::ready;CHECK(shot.run().step==g::Step::fired);
    for(unsigned i=0;i<36;++i){shot.next();shot.run();}CHECK(shot.run().step==g::Step::shoulder);
    F baseline;baseline.s.loaded=1;baseline.s.receipts={10,20,30};CHECK(baseline.run().step==g::Step::none);
    baseline.next();++baseline.s.config;++baseline.s.receipts.cancelled;CHECK(baseline.run().step==g::Step::none);
    F missing;missing.s.reserve=0;CHECK(missing.run().step==g::Step::no_ammo);missing.next();missing.s.stage=w::TorqueBowStage::interrupted;CHECK(missing.run().step==g::Step::none);
    F stale;stale.run();stale.s.now+=100000001;CHECK(stale.run().step==g::Step::none);
    F receipt;receipt.s.receipts.cancelled=2;receipt.run();receipt.next();receipt.s.receipts.cancelled=1;CHECK(receipt.run().step==g::Step::none);
    const vr::Pose root{{100,200,300},{0,0,0,1}},shoulder{{400,500,600},{0,0,0,1}};g::Shape shape{};
    for(double scale:{.5,1.,2.})for(double units:{30.,100.,400.}){
        CHECK(g::make({g::Step::feed,0},root,scale,units,shoulder,shape)&&shape.count==15);
        const auto& a=shape.lines[12];CHECK(a.start.x>a.end.x); // Original tail-first motion toward -RootX.
        CHECK(std::abs(shape.lines[0].start.x-(root.position.x+58.00706100463867*scale))<1e-9);
        for(auto step:{g::Step::shoulder,g::Step::locked,g::Step::cancelled,g::Step::fired,g::Step::no_ammo,g::Step::charging}){
            CHECK(g::make({step,.5},root,scale,units,shoulder,shape)&&shape.count>0&&shape.count<=40);
            for(unsigned i=0;i<shape.count;++i)CHECK(vr::finite(shape.lines[i].start)&&vr::finite(shape.lines[i].end));
        }
    }
    CHECK(g::make({g::Step::charging,1},root,1,100,shoulder,shape)&&shape.count==32);
    CHECK(!g::make({g::Step::charging,std::numeric_limits<double>::quiet_NaN()},root,1,100,shoulder,shape)&&shape.count==0);
    CHECK(!g::make({g::Step::feed,0},root,0,100,shoulder,shape));
    std::printf("Torque Bow guide selector/geometry: %u checks passed; no Engine/Native/pixels or action authority.\n",bow_arrow_tracking_fixture::checks);
}
