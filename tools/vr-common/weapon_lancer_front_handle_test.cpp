#include "weapon_lancer_front_handle.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace h=gears_weapon::lancer_front;
unsigned checks{};
#define C(v) do{++checks;if(!(v)){std::printf("FAIL line%d\n",__LINE__);std::exit(1);}}while(false)
int main(){
    gears_weapon::SnubSlideGeometry g{};C(h::geometry(100,g));h::Stroke s;
    const double full=.23041412353515625;
    C(std::abs(g.travel-full)<1e-12);
    C(std::abs((57.83992385864258-h::travel_raw)-h::slot_rear_raw.y)<1e-10);
    const auto sample=[&](double travel,unsigned n){return gears_weapon::HammerburstLatchSample{
        {g.closed_contact.x,g.closed_contact.y-travel,g.closed_contact.z},n,1000+60*n,71,true,true};};
    // Owner requests easier acquisition. This increases hover/grab only;
    // release, actual part stroke/return and the separate radial hold policy
    // remain independently checked below.
    C(std::abs(g.contact_radius-.18)<1e-12);
    auto wide=sample(0,1);wide.hand.x+=.16;C(s.begin(g,wide,0));s.reset();
    wide=sample(0,1);wide.hand.x+=.181;C(!s.begin(g,wide,0));
    C(s.begin(g,sample(0,1),0));C(s.move(sample(full*.5,2)));C(s.move(sample(full,3)));
    C(!s.actual_full_pull()&&s.release(1)&&s.phase()==h::Stroke::Phase::waiting_open);
    C(s.acknowledge_visible_part(0,1,1.1)&&s.phase()==h::Stroke::Phase::waiting_open);
    C(s.acknowledge_visible_part(full,2,1.2)&&s.phase()==h::Stroke::Phase::returning);
    C(s.advance_return(1.4)&&s.acknowledge_visible_part(0,3,1.4));
    C(!s.acknowledge_visible_part(0,3,1.4)&&s.phase()==h::Stroke::Phase::returning);
    C(s.acknowledge_visible_part(0,4,1.5)&&s.phase()==h::Stroke::Phase::complete);
    // The actual owner15215 held/release route peaks at14.14cm in today's
    //35deg basis. The previous17cm physical gate could not finish it even
    // after correcting its old per-frame range cancellations.
    s.reset();C(s.begin(g,sample(0,20),0));
    for(unsigned n=1;n<=7;++n)C(s.move(sample(.02*n,20+n)));
    C(std::abs(s.wanted_distance()-full)<1e-9&&!s.actual_full_pull());
    C(s.acknowledge_visible_part(.14,1,2.)&&!s.actual_full_pull());
    C(s.acknowledge_visible_part(full,2,2.1)&&s.actual_full_pull());
    C(s.release(2.2)&&s.advance_return(2.4)&&s.acknowledge_visible_part(0,3,2.4)&&
        s.acknowledge_visible_part(0,4,2.5)&&s.phase()==h::Stroke::Phase::complete);
    // A nearby but incomplete real pull remains ordinary uncharged state,
    // even if an unrelated delayed upload reports a full-open mesh.
    s.reset();C(s.begin(g,sample(0,30),0));
    for(unsigned n=1;n<=6;++n)C(s.move(sample(.02*n,30+n)));
    C(s.wanted_distance()<full-.0015&&!s.actual_full_pull());
    C(s.acknowledge_visible_part(full,1,3.)&&!s.actual_full_pull());
    C(s.release(3.1)&&s.advance_return(3.3)&&s.acknowledge_visible_part(0,2,3.3)&&
        s.acknowledge_visible_part(0,3,3.4)&&s.phase()==h::Stroke::Phase::idle);
    s.reset();C(s.begin(g,sample(0,1),0)&&s.move(sample(.01,2)));
    C(s.acknowledge_visible_part(.01,1,1)&&s.release(1)&&s.advance_return(1.2));
    C(s.acknowledge_visible_part(0,2,1.2)&&s.acknowledge_visible_part(0,3,1.3)&&s.phase()==h::Stroke::Phase::idle);
    auto old=sample(0,4);old.hand={.04665,.38716,.07247};C(!s.begin(g,old,0)); // old B surface
    old.hand={.05066,.3250145,.0942102};C(!s.begin(g,old,0)); // old wrong cap
    s.reset();C(s.begin(g,sample(0,1),0));
    auto forgiving=sample(.30,3);forgiving.hand.x+=.05;
    C(s.move(forgiving)&&std::abs(s.wanted_distance()-full)<1e-9);
    C(!s.actual_full_pull()); // Long input still requires actual visible mechanical feedback.
    // Representative measured physical13780 motion: a short initial
    // opposite adjustment, a broad/curved long pull, then actual release.
    // Neither a normal overshoot nor leaving hover may demand another grab.
    s.reset();C(s.begin(g,sample(0,10),0));C(s.move(sample(-.02022,11)));
    C(s.move(sample(.035,12)));auto human=sample(.15,13);human.hand.x+=.06546;
    C(s.move(human)&&s.phase()==h::Stroke::Phase::pulling&&s.last_rejection()==h::Stroke::Rejection::none);
    human=sample(.20452,14);human.hand.x+=.03996;
    C(s.move(human)&&std::abs(s.wanted_distance()-full)<1e-9&&!s.actual_full_pull());
    auto outside=human;outside.sequence=15;outside.milliseconds+=200;outside.hand.x+=.2;
    auto departed=s;
    C(!departed.move(outside)&&departed.last_rejection()==h::Stroke::Rejection::lateral);
    C(departed.cancel(2.)&&departed.advance_return(2.2));
    C(departed.acknowledge_visible_part(0,10,2.2)&&departed.acknowledge_visible_part(0,11,2.3)&&departed.phase()==h::Stroke::Phase::idle);
    C(s.acknowledge_visible_part(full,10,2.)&&s.actual_full_pull());
    C(s.release(2.1)&&s.advance_return(2.3)&&s.acknowledge_visible_part(0,11,2.3)&&
        s.acknowledge_visible_part(0,12,2.4)&&s.phase()==h::Stroke::Phase::complete);
    s.reset();
    // A genuine, continuous60cm axial pull keeps the same grasp while the
    // actual part remains23.04cm long. It still cannot complete without
    // observed full part travel, actual release and mechanical return.
    C(s.begin(g,sample(0,100),0));
    for(unsigned i=1;i<=20;++i){C(s.move(sample(.03*i,100+i)));C(s.phase()==h::Stroke::Phase::pulling);}
    C(std::abs(s.wanted_distance()-full)<1e-9&&!s.actual_full_pull());
    C(s.release(10.)&&s.phase()==h::Stroke::Phase::waiting_open);
    C(s.acknowledge_visible_part(full,1,10.1)&&s.phase()==h::Stroke::Phase::returning);
    C(s.advance_return(10.3)&&s.acknowledge_visible_part(0,2,10.3)&&s.acknowledge_visible_part(0,3,10.4)&&s.phase()==h::Stroke::Phase::complete);
    s.reset();
    C(s.begin(g,sample(0,4),0)&&s.move(sample(full,5))&&s.acknowledge_visible_part(full,4,1.4));
    C(s.cancel(2)&&s.advance_return(2.2)&&s.acknowledge_visible_part(0,5,2.2)&&
        s.acknowledge_visible_part(0,6,2.3)&&s.phase()==h::Stroke::Phase::idle);
    for(unsigned i=0;i<=64;++i){
        const double distance=h::travel_raw*double(i)/64;std::array<float,2> w{};C(h::weights(distance,w));
        // Independent exact translation oracle, also checks neutral normals:
        // both immutable shapes translate by full travel; weights sum to t.
        const double actual_y=58.4-(double(w[0])+double(w[1]))*h::travel_raw;
        C(std::abs(actual_y-(58.4-distance))<1e-6);
    }
    std::array<float,2> w{};C(!h::weights(-.1,w)&&!h::weights(h::travel_raw+.1,w)&&!h::weights(std::numeric_limits<double>::quiet_NaN(),w));
    {double raw{};
        C(.035*100>3.5); // Preserve the earlier short-stroke representation regression.
        C(h::raw_distance(100,.035,raw)&&std::abs(raw-3.5)<1e-12&&h::weights(raw,w));
        for(double units:{10.,37.,99.9,100.,173.25,499.9,500.}){
            C(h::raw_distance(units,h::travel_raw/units,raw)&&raw<=h::travel_raw&&raw>h::travel_raw-1e-12);
            C(h::raw_distance(units,1./units,raw)&&std::abs(raw-1)<1e-12);
            C(!h::raw_distance(units,h::travel_raw/units+.00001,raw));
        }
        C(!h::raw_distance(100,-.001,raw)&&!h::raw_distance(0,.01,raw)&&
          !h::raw_distance(100,std::numeric_limits<double>::infinity(),raw));
    }
    std::printf("Lancer front handle: %u actual-feedback/partial/cancel/old-B-rejection/translation checks; CPU only\n",checks);
}
