#include "core.h"
#include "interaction.h"
#include "storage.h"
#include <cassert>
#include <filesystem>
#include <iostream>
using namespace workbench;
Hand sample(){Hand h{};h.active=true;for(unsigned i=0;i<26;++i){h.joints[i].flags=15;h.joints[i].radius=.005;h.joints[i].pose={{.1+double(i)*.002,.2,-.4},{}};}return h;}
int main(int argc,char** argv){
    assert(argc==2);std::filesystem::create_directories(argv[1]);
    P object{{.3,.7,-1},axis({0,1,0},.7)},wrist{{.4,.9,-.8},axis({1,0,0},.4)};
    auto local=relative(object,wrist);auto round=compose(object,local);assert(length(sub(round.position,wrist.position))<1e-10);
    auto hand=sample();assert(valid(hand)&&tracked(hand));hand.joints[10].flags=3;assert(valid(hand)&&!tracked(hand));hand.joints[10].flags=0;assert(!valid(hand));
    Volume volume;volume.radius=.1;volume.local.position={.1,0,0};auto center=compose(object,volume.local).position;assert(contains(volume,object,center));assert(!contains(volume,object,add(center,{1,0,0})));
    P target{center,{}};volume.snap_distance=.08;volume.release_distance=.15;volume.snap_angle=.3;
    assert(snap(volume,object,center,{add(center,{.07,0,0}),{}},target,false));
    assert(!snap(volume,object,center,{add(center,{.10,0,0}),{}},target,false));
    assert(snap(volume,object,center,{add(center,{.10,0,0}),{}},target,true));
    assert(!snap(volume,object,center,{add(center,{.16,0,0}),{}},target,true));
    assert(!snap(volume,object,center,{center,axis({0,1,0},.4)},target,false));
    Frame f{};f.time_ns=1'000'000;f.epoch=1;f.head={{0,1.7,0},{}};f.object=object;f.hands={sample(),sample()};
    GrabState grab;assert(begin_grab(grab,f.object,f.hands[1],1));auto before=f.object;f.hands[1].joints[1].pose.position.x+=.25;assert(follow(grab,f.hands,f.object));assert(std::abs(f.object.position.x-before.position.x-.25)<1e-10);
    // A controller can move the object without producing any raw hand joints.
    GrabState cg;P cp{{.2,1,-.4},{}};assert(begin_grab_at(cg,object,cp,1));auto moved=cp;moved.position.z-=.2;P result{};assert(follow_at(cg,{P{},moved},{false,true},result));assert(std::abs(result.position.z-object.position.z+.2)<1e-10);assert(!follow_at(cg,{P{},moved},{false,false},result));
    ButtonEdge edge;assert(!edge.update(2,1));assert(!edge.update(2,1));assert(!edge.update(2,0));assert(edge.update(2,1));assert(!edge.update(2,1));assert(!edge.update(0,0));assert(!edge.update(2,1));assert(!edge.update(2,0));assert(edge.update(2,.8));
    P panel{{-.3,1.2,-1.1},axis({0,1,0},.4)};P ray=compose(panel,{{0,0,1},{}});assert(std::abs(ray_rectangle(ray,panel,.5,.1)-1)<1e-10);ray.position=compose(panel,{{.3,0,1},{}}).position;assert(ray_rectangle(ray,panel,.5,.1)<0);
    assert(slider_value(compose(panel,{{0,0,1},{}}),panel,.5)==.5);assert(slider_value(compose(panel,{{1,0,1},{}}),panel,.5)==1);assert(slider_value(compose(panel,{{-1,0,1},{}}),panel,.5)==0);
    assert(!hand_menu_allowed(0,0,true,false)&&hand_menu_allowed(1,0,true,false));assert(!hand_menu_allowed(1,0,true,true)&&!hand_menu_allowed(1,0,false,false));
    RotationDrag drag;P ring{{0,1,-1},{}};assert(drag.begin(1,compose(ring,{{.065,0,1},{}}),ring,Q{},0));assert(drag.component==2);Q spun{};assert(drag.update(compose(ring,{{0,.065,1},{}}),spun));assert(length(sub(rotate(spun,{1,0,0}),{0,1,0}))<1e-9);
    {auto folder=std::filesystem::path(argv[1]);StorageWorker worker;std::string one=(folder/"capture-1.json").string(),two=(folder/"capture-2.json").string(),latest=(folder/"latest.json").string();assert(worker.submit("1","first",[=]{write_json_atomic(one,Json{{"side","left"},{"id",1}},false);write_json_atomic(latest,Json{{"id",1}});}));assert(worker.submit("2","second",[=]{write_json_atomic(two,Json{{"side","left"},{"id",2}},false);write_json_atomic(latest,Json{{"id",2}});}));worker.finish();auto results=worker.poll();assert(results.size()==2&&results[0].success&&results[1].success);Json a,b,last;std::ifstream(one)>>a;std::ifstream(two)>>b;std::ifstream(latest)>>last;assert(a.at("id")==1&&b.at("id")==2&&last.at("id")==2);try{write_json_atomic(one,Json{{"id",3}},false);assert(false);}catch(std::runtime_error const&){}std::ifstream(one)>>a;assert(a.at("id")==1);}
    f.starter_angle=.8;f.charging_distance=.12;f.hands[0].data_source=1;
    Rig rig{};rig.parent[0]=-1;for(unsigned i=1;i<16;++i){rig.parent[i]=i%3==1?0:int(i-1);rig.reference[i].position={.02,double(i/3)*.002,0};}
    Calibration c{};auto h=sample();h.joints[1].pose={{},{}};h.joints[7].pose.position={.03,0,-.03};h.joints[12].pose.position={.06,0,0};h.joints[22].pose.position={.03,0,.03};assert(calibrate(rig,h,c));std::array<Q,15> joints{};P out{};assert(retarget(rig,c,h,joints,out));for(auto q:joints)assert(std::abs(q.w-1)<1e-9);auto targets=fk(rig,joints);for(unsigned i=1;i<16;++i)assert(std::abs(length(sub(targets[i].position,targets[rig.parent[i]].position))-length(rig.reference[i].position))<1e-9);
    f.hands[0]=h;f.hands[1]=h;auto captured=capture(f,0,rig,c);assert(captured.mapped&&captured.fully_tracked);// Reposition/rotate the isolated item AND the owner's hand in world space.
    // Saved item-local wrists, raw joints and finger rotations must stay identical.
    {auto original=f;P shift{{-.45,.22,.37},axis({.2,1,.4},.9)};f.object=compose(shift,f.object);for(auto& moved_hand:f.hands)for(auto& joint:moved_hand.joints)joint.pose=compose(shift,joint.pose);auto again=capture(f,0,rig,c);assert(length(sub(again.wrist_in_object.position,captured.wrist_in_object.position))<1e-9);auto same_rotation=[](Q a,Q b){return std::abs(std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)-1)<1e-9;};assert(same_rotation(again.wrist_in_object.orientation,captured.wrist_in_object.orientation));for(unsigned i=0;i<26;++i){assert(length(sub(again.raw_in_object[i].pose.position,captured.raw_in_object[i].pose.position))<1e-9);assert(again.raw_in_object[i].flags==captured.raw_in_object[i].flags);}for(unsigned i=0;i<15;++i)assert(same_rotation(again.local[i],captured.local[i]));f=original;}
    auto old=captured.wrist_in_object;f.object.position.x+=10;assert(length(sub(captured.wrist_in_object.position,old.position))==0);
    f.hands[0].data_source=2;try{capture(f,0,rig,c);assert(false);}catch(std::runtime_error const&){}
    std::cout<<"PASS: object-local capture, tracking flags, shape boundary, pickup follow, calibrated retarget identity and fixed bone lengths. CPU only.\n";
}
