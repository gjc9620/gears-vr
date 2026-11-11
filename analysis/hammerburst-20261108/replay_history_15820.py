from pathlib import Path
import re,json,hashlib,subprocess
A=Path(__file__).resolve().parent;P=A/'history-replay-15820';P.mkdir(exist_ok=True);S=A/'withdrawal-clock-r1/source/tools/vr-common'
raw=(A/'runtime-15820-text/playground-15820.log').read_bytes();pg=raw.decode('utf-16' if raw[:2]==b'\xff\xfe' else 'utf-8-sig',errors='replace')
def rows(prefix):return [dict(re.findall(r'(\w+)=([^\s]+)',l)) for l in pg.splitlines() if l.startswith(prefix+' ')]
record=rows('hammerburst_history_rejection')[0];assert record['record']=='1' and record['built']=='0'
frames=[r for r in rows('hammerburst_history_raw') if r['record']=='1'];hands=[r for r in rows('hammerburst_history_hand') if r['record']=='1']
def num(x):
 assert re.fullmatch(r'[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?',x),x
 return x
def pose(row,tag):return '{{'+','.join(num(row[tag+'_'+c]) for c in ('px','py','pz'))+'},{'+','.join(num(row[tag+'_'+c]) for c in ('qx','qy','qz','qw'))+'}}'
code=['#include "controller_relative_motion.h"','#include <cassert>','#include <cstdio>','int main(){','namespace m=gears_hand_continuity;namespace r=gears_controller_relative_motion;','m::TrackedInput previous{},current{};m::MotionHistory history{};']
for f in frames:
 name=f['role'] if f['role']!='history' else 'history.frames['+str(int(f['index']))+']'
 code+=['{auto& v='+name+';auto& s=v.input;','s.source=static_cast<gears_vr::StereoSource>('+num(f['source'])+');']
 for key,field in [('session','session'),('sequence','sequence'),('space','space_epoch'),('profile','profile_generation'),('input_epoch','input_epoch'),('predicted_ns','predicted_time'),('received_ns','received_ns'),('sync_ns','sync_ns'),('focused','focused'),('display','display_mode'),('display_ns','display_observed_ns')]:code+=['s.'+field+'='+num(f[key])+';']
 code+=['v.stream_epoch='+num(f['stream'])+';v.tracking_epoch[0]='+num(f['track_left'])+';v.tracking_epoch[1]='+num(f['track_right'])+';']
 for n in (0,1):
  h=next(h for h in hands if h['role']==f['role'] and h['index']==f['index'] and int(h['hand'])==n)
  code+=['{auto& h=s.hands['+str(n)+'];']
  for key,field in [('grip_active','grip_active'),('grip_flags','grip_flags'),('aim_active','aim_active'),('aim_flags','aim_flags'),('trigger_active','trigger_active'),('trigger','trigger'),('squeeze_active','squeeze_active'),('squeeze','squeeze'),('stick_active','stick_active'),('stick_x','stick_x'),('stick_y','stick_y'),('buttons','buttons'),('buttons_active','buttons_active'),('last_squeeze_flags','last_squeeze_press_flags'),('last_squeeze_sequence','last_squeeze_press_sequence')]:
   value=num(h[key]);code+=['h.'+field+'='+('static_cast<float>('+value+')' if key in ('trigger','squeeze','stick_x','stick_y') else value)+';']
  for tag,field in [('grip','grip'),('aim','aim'),('last_squeeze','last_squeeze_press_grip')]:code+=['h.'+field+'='+pose(h,tag)+';']
  for c in range(8):code+=['h.pressed['+str(c)+']='+num(h['p'+str(c)])+';h.released['+str(c)+']='+num(h['r'+str(c)])+';']
  code+=['}']
 code+=['}']
code+=['history.count='+num(record['history_count'])+';history.reserved='+num(record['history_reserved'])+';', 'gears_vr::Pose mount='+pose(record,'mount')+';double scale='+num(record['scale'])+';', 'const unsigned control='+num(record['control'])+';']
code+=[r'''
 assert(m::tracked_input_shape_valid(previous)&&m::tracked_input_shape_valid(current));
 assert(previous.input.sequence==4628&&current.input.sequence==4633&&history.count==4);
 assert(history.frames[0].input.sequence==4630);
 assert(current.input.sequence-previous.input.sequence<=m::motion_history_capacity);
 assert(m::same_tracking(history.frames[history.count-1],current));
 assert(r::held(previous.input.hands[0],control));
 gears_vr::Vector position{};assert(r::project(previous,mount,scale,position));
 for(unsigned i=0;i<history.count;++i){auto const& next=history.frames[i];
  assert(m::tracked_input_shape_valid(next)&&r::project(next,mount,scale,position));
  assert(r::held(next.input.hands[0],control));
  assert(next.input.hands[0].pressed[control]==previous.input.hands[0].pressed[control]);
  assert(next.input.hands[0].released[control]==previous.input.hands[0].released[control]);
  assert(next.stream_epoch==previous.stream_epoch&&next.tracking_epoch[0]==previous.tracking_epoch[0]&&next.tracking_epoch[1]==previous.tracking_epoch[1]);
  if(i)assert(m::motion_pair(history.frames[i-1],next,0,100000000)&&m::motion_pair(history.frames[i-1],next,1,100000000));
 }
 assert(!m::motion_pair(previous,history.frames[0],0,100000000));
 assert(!m::motion_pair(previous,history.frames[0],1,100000000));
 assert(history.frames[0].input.sync_ns-previous.input.sync_ns<100000000);
 r::Segment segment{};assert(!r::build(previous,current,history,control,mount,scale,segment)&&segment.count==0);
 std::puts("Actual15820 original kernel replay: rejection reproduced. All raw shapes/projections/held counters and retained adjacent pairs pass; previous4628 -> first4630 fails strict consecutive sequence; missing4629. Delta5 within original capacity8. No synthetic missing sample or relaxed gate.");
}
''']
target=P/'actual-history.cpp';target.write_text('\n'.join(code));cmd=P/'actual-history.cmd';exe=P/'actual-history.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{S}" "{target}" /Fo"{P / "actual-history.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'actual-history.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'actual-history.log').read_text(errors='replace'));r.check_returncode()
proof=dict(passed=True,runtime=15820,original_record=record,raw_frames=frames,hand_record_count=len(hands),original_kernel_sha256=hashlib.sha256((S/'controller_relative_motion.h').read_bytes()).hexdigest(),replay_source_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),exact_rejection_reproduced=True,first_failure='Previous4628 to first retained4630 fails consecutive sequence; missing4629. Existing shapes, projection, held counters and all retained adjacent pairs pass.',limits='Snapshot proves missing coverage, not whether4629 was never published, overwritten before history read, or rejected as a raw pair. Original ring latest_sequence and history-stop cause are not in this record. No synthesized point, larger ring or relaxed continuity.')
(P/'result.json').write_text(json.dumps(proof,indent=2))
