#include "weapon_snub_slide.h"
#include "weapon_snub_slide_curve.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
namespace {
unsigned checks{};
void check(bool value,const char* message){++checks;if(!value){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
using S=gears_weapon::SnubSlideStroke;using G=gears_weapon::SnubSlideGeometry;
gears_weapon::HammerburstLatchSample sample(double distance,unsigned sequence=1){
    return {{.1-distance,.02,.03},sequence,1000+75*sequence,41,true,true};
}
G geometry(){G g;g.closed_contact={.1,.02,.03};g.axis={-1,0,0};g.travel=.054;g.current_instance_verified=true;return g;}
}
int main(){
    const auto g=geometry();S s;
    check(s.begin(g,sample(0),0)&&s.move(sample(.025,2))&&s.move(sample(.054,3)),"continuous full pull");
    check(!s.actual_full_pull(),"wanted pose is not an original-bone receipt");
    check(s.release(4)&&s.phase()==S::Phase::waiting_open,"quick release waits for actual open");
    check(!s.advance_return(5)&&s.acknowledge_bone(0,1,5)&&s.phase()==S::Phase::waiting_open,"waiting time cannot charge");
    check(s.acknowledge_bone(.054,2,5.1)&&s.advance_return(5.3),"actual full slide begins spring return");
    check(s.acknowledge_bone(0,3,5.3)&&!s.acknowledge_bone(0,3,5.3),"no duplicate closed frame");
    check(s.phase()==S::Phase::returning&&s.acknowledge_bone(0,4,5.4)&&s.phase()==S::Phase::complete,"two actual closed observations complete");
    check(!s.begin(g,sample(0,9),0),"completed cannot reaward");
    s.reset();check(s.begin(g,sample(0),0)&&s.move(sample(.025,2))&&s.acknowledge_bone(.025,1,6),"partial pull");
    check(s.release(6)&&s.advance_return(6.2)&&s.acknowledge_bone(0,2,6.2)&&s.acknowledge_bone(0,3,6.3)&&s.phase()==S::Phase::idle,"partial returns closed without award");
    s.reset();check(s.begin(g,sample(0),0),"cancel setup");auto wrong=sample(.054,2);wrong.grip_token=99;
    check(!s.move(wrong)&&!s.move(sample(.054,3)),"invalid token poisons stroke until cancellation");
    check(s.release(7)&&s.advance_return(7.2)&&s.acknowledge_bone(0,1,7.2)&&s.acknowledge_bone(0,2,7.3)&&s.phase()==S::Phase::idle,"release of invalid stroke cannot inherit distance");
    s.reset();check(s.begin(g,sample(0),0),"lateral setup");wrong=sample(.01,2);wrong.hand.y+=.04;
    check(!s.move(wrong)&&s.last_rejection()==S::Rejection::lateral,"lateral gesture is not a pull");
    s.reset();check(s.begin(g,sample(0),0)&&!s.move(sample(.02,6))&&s.last_rejection()==S::Rejection::gap,"gap never bridges motion");
    s.reset();wrong=sample(0);wrong.hand.x=.5;check(!s.begin(g,wrong,0),"remote grasp rejected");
    s.reset();check(s.begin(g,sample(0),0),"loss setup");wrong=sample(.01,2);wrong.tracked=false;
    check(!s.move(wrong)&&s.last_rejection()==S::Rejection::sample,"tracking loss cannot move slide");
    s.reset();check(s.begin(g,sample(0),0),"speed setup");wrong=sample(.054,2);wrong.milliseconds=1076;
    check(!s.move(wrong)&&s.last_rejection()==S::Rejection::speed,"teleport cannot charge");
    // Synthetic curve deliberately begins partly open, as the original does.
    // It contains no shipped key data. Native endpoint-law selection is not
    // certified by this arithmetic round trip.
    using C=gears_weapon::SnubSlideCurve;using V=gears_vr::Vector;
    const std::array<V,7> keys{{{.07,.02,.03},{.07,.02,.03},{.046,.02,.03},{.065,.02,.03},{.1,.02,.03},{.1,.02,.03},{.1,.02,.03}}};
    C curve;check(curve.bind(keys.data(),keys.size(),2,{.1,.02,.03},{-1,0,0},true),"derive final original closing branch");
    check(std::abs(curve.travel()-.054)<1e-10&&curve.open_key()==2&&curve.closed_key()==4,"closed reference instead of frame0");
    for(auto law:{C::Sampling::reference_keys,C::Sampling::endpoint_intervals}){
        float time{};double value{};
        for(double distance:{0.,.001,.017,.035,.054})check(curve.time_for_distance(distance,time,law)&&
            curve.distance_at_time(time,value,law)&&std::abs(value-distance)<2e-7,"distance/time round trip on final branch");
        check(curve.time_for_distance(0,time,law)&&time==2,"closed endpoint invariant to sampling law");
    }
    auto bad=keys;bad[3].y+=.001;check(!curve.bind(bad.data(),bad.size(),2,{.1,.02,.03},{-1,0,0},true),"lateral source motion not silently projected");
    bad=keys;bad[3].x=.085;bad[4].x=.08;check(!curve.bind(bad.data(),bad.size(),2,{.1,.02,.03},{-1,0,0},true),"wrong closing branch rejected");
    bad=keys;bad[6].x=.08;check(!curve.bind(bad.data(),bad.size(),2,{.1,.02,.03},{-1,0,0},true),"requires stable final closed plateau");
    bad=keys;bad[0].x=std::numeric_limits<double>::quiet_NaN();check(!curve.bind(bad.data(),bad.size(),2,{.1,.02,.03},{-1,0,0},true),"nonfinite curve rejected");
    std::printf("Snub slide: %u checks passed\n",checks);
}
