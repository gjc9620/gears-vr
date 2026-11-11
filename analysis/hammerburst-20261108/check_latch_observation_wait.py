from pathlib import Path
import json,subprocess,hashlib
A=Path(__file__).resolve().parent;P=A/'latch-observation-wait-r1'
# Reuse the original exact predicate/caller fixture, changing only its now
# intentional observation-retention expectation. No game or engine invoked.
text=(A/'check_latch_seek_gap.py').read_text().split('# The real pure stroke')[0]
text=text.replace("P=A/'latch-seek-gap-r1'","P=A/'latch-observation-wait-r1'")
text=text.replace(' const auto stroke_before=stroke;const auto history_before=latch_history::previous;\n','')
text=text.replace('!cleaned&&stroke.value==1&&latch_history::previous==2','!cleaned&&stroke.value==99&&latch_history::previous==88&&angle==0&&stage==2')
text=text.replace('Exact latch pre-dispatch rollback block','Exact latch pre-dispatch observation-retention block')
exec(compile(text,str(A/'check_latch_seek_gap.py'),'exec'),{'__file__':str(__file__),'__name__':'__main__'})
s=(P/'source/tools/playground/weapon_hammerburst_runtime.inl').read_text()
start=s.index('        if(moved==gears_weapon::HammerburstLatchMotion::held_stationary){');end=s.index('        if(moved==gears_weapon::HammerburstLatchMotion::rejected){',start)
stationary=s[start:end]
code=r'''
#include "weapon_hammerburst_latch_history.h"
#include "controller_relative_motion.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
namespace relative=gears_controller_relative_motion;
namespace input=gears_hand_continuity;
HammerburstLatchStroke stroke;
HammerburstLatchMotion moved;
double angle;unsigned latch_stationary_holds;bool reached_seek;
void stationary_gate(){@STATIONARY@ reached_seek=true;}
HammerburstLatchSample sample(double a,std::uint64_t seq,std::uint64_t ms){return {{.05*std::cos(a),0,-.05*std::sin(a)},seq,ms,7,true,true};}
input::TrackedInput raw(double a,std::uint64_t seq,std::int64_t ns){
 input::TrackedInput r{};r.stream_epoch=r.tracking_epoch[0]=r.tracking_epoch[1]=1;
 auto& f=r.input;f.session=f.space_epoch=1;f.sequence=seq;f.focused=1;
 f.sync_ns=f.received_ns=f.predicted_time=ns;
 for(auto& h:f.hands){h.grip_active=1;h.grip_flags=gears_vr::controller_pose_required_flags;h.grip.orientation.w=1;}
 f.hands[0].grip.position={0,-.05*std::sin(a),-.05*std::cos(a)};
 f.hands[0].squeeze_active=1;f.hands[0].squeeze=1;f.hands[0].pressed[gears_vr::control_squeeze]=7;f.hands[0].released[gears_vr::control_squeeze]=6;
 assert(input::tracked_input_shape_valid(r));return r;
}
input::MotionHistory history(double a,double b,std::uint64_t seq,std::int64_t ns,std::int64_t end,unsigned n){
 input::MotionHistory h{};h.count=n;
 for(unsigned i=1;i<=n;++i)h.frames[i-1]=raw(a+(b-a)*i/n,seq+i,ns+(end-ns)*i/n);
 return h;
}
HammerburstLatchMotion observe(relative::Segment const& seg){
 std::array<HammerburstLatchSample,9> points{};
 for(unsigned i=0;i<seg.count;++i)points[i]={seg.points[i].position,seg.points[i].sequence,std::uint64_t(seg.points[i].sync_ns/1000000),7,true,true};
 return hammerburst_latch_history(stroke,points[seg.count-1],points,seg.count);
}
int main(){
 const HammerburstLatchGeometry g{{0,0,0},{0,1,0},{.05,0,0},.8,.02,.02,.02,true};
 const auto t=std::int64_t(162870638000000);const double a=.000645288956,b=.00481488532,c=.009;
 auto old=raw(a,4679,t),accepted=raw(b,4683,t+108000000),current=raw(c,4689,t+245000000);
 relative::Segment first{},next{};auto h1=history(a,b,4679,t,t+108000000,4),h2=history(b,c,4683,t+108000000,t+245000000,6);
 const gears_vr::Pose mount{};
 assert(input::motion_history_capacity==8); // Original frozen capacity, unchanged.
 assert(!relative::build(old,current,h2,gears_vr::control_squeeze,mount,1,next)); // Delta10 exceeds8.
 assert(relative::build(old,accepted,h1,gears_vr::control_squeeze,mount,1,first));
 assert(stroke.begin(g,sample(a,4679,std::uint64_t(t/1000000)),a));angle=a;
 assert(observe(first)!=HammerburstLatchMotion::rejected);assert(stroke.last_observation_ms()==std::uint64_t(t/1000000)+108);
 assert(std::abs(stroke.angle()-b)<1e-10&&angle==a); // No seek: actual angle unchanged.
 assert(relative::build(accepted,current,h2,gears_vr::control_squeeze,mount,1,next));
 assert(observe(next)!=HammerburstLatchMotion::rejected&&angle==a);
 assert(std::abs(stroke.angle()-c)<1e-10);angle=stroke.angle(); // Fake successful setter only.
 for(double target:{0.,.3,.8}){
  auto p=sample(target,100,1000);assert(stroke.begin(g,p,target));angle=target==0?.024:0.;
  auto same=p;same.sequence++;same.milliseconds+=137;moved=stroke.observe(same,true);
  assert(moved==HammerburstLatchMotion::held_stationary);reached_seek=false;stationary_gate();
  assert(reached_seek&&moved==stroke.observed_motion());
  angle=target;reached_seek=false;moved=HammerburstLatchMotion::held_stationary;stationary_gate();assert(!reached_seek);
 }
 for(unsigned bad=0;bad<3;++bad){
  auto p=sample(.3,100,1000);assert(stroke.begin(g,p,.3));angle=0.;auto q=sample(.31,101,1050);
  if(bad==0)q.tracked=false;if(bad==1)q.gripped=false;if(bad==2)q.grip_token++;
  assert(stroke.observe(q,true)==HammerburstLatchMotion::rejected&&!stroke.active()&&angle==0.);
 }
 auto broken=h2;broken.frames[2].tracking_epoch[0]++;
 assert(!relative::build(accepted,current,broken,gears_vr::control_squeeze,mount,1,next));
 broken=h2;broken.frames[2].input.hands[0].released[gears_vr::control_squeeze]++;
 assert(!relative::build(accepted,current,broken,gears_vr::control_squeeze,mount,1,next));
 std::puts("Actual relative/history/stroke kernels: original sequence10 rollback refusal; retained6 successor; no actual-angle commit; stationary closed/moving/open; committed no-redispatch; lost tracking/release/token/epoch/edge rejection passed. Synthetic positions and intermediate timestamps, not captured full producer history or engine proof.");
}
'''.replace('@STATIONARY@',stationary)
target=P/'observation-wait-test.cpp';target.write_text(code);cmd=P/'observation-wait-test.cmd';exe=P/'observation-wait-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{P / "source/tools/vr-common"}" "{target}" /Fo"{P / "observation-wait-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'observation-wait-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'observation-wait-test.log').read_text(errors='replace'));r.check_returncode()
write=P/'observation-wait-test-result.json';write.write_text(json.dumps(dict(passed=True,original_capacity=8,rollback_sequence_gap=10,retained_sequence_gap=6,original_max_sample_gap_ms=100,exact_stationary_caller=True,actual_kernel=True,source_sha256=hashlib.sha256(code.encode()).hexdigest(),game_started=False,limits='Endpoint sequence/time anchored to4400; intermediate history/positions synthetic. Engine setter/grant integration not exercised.'),indent=2))
for name in ('weapon_hammerburst_reload_test','weapon_hammerburst_latch_history_test'):
 target=P/'source/tools/vr-common'/(name+'.cpp');assert target.exists()
 cmd=P/(name+'.cmd');exe=P/(name+'.exe')
 cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{target}" /Fo"{P / (name+".obj")}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
 with (P/(name+'.log')).open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 print((P/(name+'.log')).read_text(errors='replace'));r.check_returncode()
proof=json.loads(write.read_text());proof['existing_latch_and_history_tests_passed']=True;write.write_text(json.dumps(proof,indent=2))
