#include "weapon_lancer_charging.h"
#include <cstdio>
#include <cstdlib>
namespace {
unsigned checks{};void check(bool value,char const* name){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
gears_weapon::HammerburstLatchGeometry geometry(){
    gears_weapon::HammerburstLatchGeometry g;g.axis={0,1,0};g.closed_contact={.08,0,0};
    g.open_angle=28.5*3.14159265358979323846/180.;g.current_instance_verified=true;return g;
}
gears_weapon::HammerburstLatchSample hand(double a,unsigned n=1){
    return {{.08*std::cos(a),0,-.08*std::sin(a)},n,1000+75*n,41,true,true};
}
}
int main(){
    using C=gears_weapon::LancerChargingStroke;const auto g=geometry();C c;
    check(c.begin(g,hand(0),0),"fresh closed mechanism");
    check(c.move(hand(g.open_angle*.5,2)),"continuous pull");
    check(c.move(hand(g.open_angle,3)),"full requested pull");
    check(!c.actual_full_pull(),"animation request is not actual completion");
    check(c.release(4)&&c.phase()==C::Phase::waiting_open,"quick release waits for actual open");
    check(!c.advance_return(4.2)&&c.acknowledge_bone(0,1,4.2)&&c.phase()==C::Phase::waiting_open,"time cannot replace missing open bone");
    check(c.acknowledge_bone(g.open_angle,2,4.3)&&c.advance_return(4.5),"actual open starts spring return");
    check(c.acknowledge_bone(0,3,4.5)&&c.acknowledge_bone(0,4,4.6)&&c.phase()==C::Phase::complete,"quick release still requires two actual closed frames");
    c.reset();
    check(c.begin(g,hand(0,4),0),"new explicit grasp");
    check(c.move(hand(g.open_angle*.5,5))&&c.move(hand(g.open_angle,6)),"full second pull");
    check(c.acknowledge_bone(g.open_angle,3)&&c.actual_full_pull(),"actual original receiver reaches open");
    check(c.release(5)&&c.advance_return(5.2),"spring return after release");
    check(c.phase()==C::Phase::returning,"elapsed time cannot unlock");
    check(c.acknowledge_bone(0,4)&&!c.acknowledge_bone(0,4),"duplicate pose cannot count twice");
    check(c.phase()==C::Phase::returning,"requires another actual closed frame");
    check(c.acknowledge_bone(0,5)&&c.phase()==C::Phase::complete,"observed return completes once");
    check(!c.begin(g,hand(0,7),0),"cannot reaward completed action");
    c.reset();check(c.begin(g,hand(0),0)&&c.move(hand(.1,2))&&c.acknowledge_bone(.1,1),"partial pull");
    check(c.release(6)&&c.advance_return(6.2)&&c.acknowledge_bone(0,2)&&c.acknowledge_bone(0,3)&&c.phase()==C::Phase::idle,"partial release resets without unlocking");
    c.reset();check(c.begin(g,hand(0),0),"restart new action only");
    auto bad=hand(.1,2);bad.grip_token=42;
    check(!c.move(bad),"different grab cannot inherit travel");
    c.reset();check(c.begin(g,hand(0),0),"tracking case");bad=hand(.1,2);bad.tracked=false;
    check(!c.move(bad),"tracking loss never moves mechanism");
    c.reset();auto outside=hand(0);outside.hand.x=.3;
    check(!c.begin(g,outside,0),"remote grab rejected");
    c.reset();check(c.begin(g,hand(0),0),"gap case");bad=hand(.1,6);
    check(!c.move(bad),"long observation gap rejected");
    check(c.last_rejection()==gears_weapon::HammerburstLatchRejection::gap,"gap cause retained");
    check(c.cancel(7)&&c.advance_return(7.2)&&c.acknowledge_bone(0,1,7.2)&&
        c.acknowledge_bone(0,2,7.3)&&c.phase()==C::Phase::idle,"gap returns closed without reload completion");
    check(c.begin(g,hand(0,8),0)&&c.move(hand(g.open_angle,9))&&
        c.acknowledge_bone(g.open_angle,3),"new full pull after cancelled gap");
    check(c.cancel(8)&&c.advance_return(8.2)&&c.acknowledge_bone(0,4,8.2)&&
        c.acknowledge_bone(0,5,8.3)&&c.phase()==C::Phase::idle,"cancel cannot award earlier full travel");
    check(!c.cancel(9),"idle cannot synthesize return");
    std::printf("Lancer charging: %u checks passed\n",checks);
}
