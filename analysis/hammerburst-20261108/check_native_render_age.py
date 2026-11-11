from pathlib import Path
import json,hashlib,subprocess
A=Path(__file__).resolve().parent;P=A/'native-render-age-r1';S=P/'source/tools/native-capture'
code=r'''
#include "stereo_hand_age_policy.h"
#include <cassert>
#include <cstdio>
#include <limits>
using namespace gears_vr;
StereoSample sample(){
 StereoSample s{};s.source=StereoSource::MetaOperatorSimulated;s.session=1;s.sequence=1;s.space_epoch=1;
 s.received_ns=1000000000;s.predicted_time=123;s.focused=s.visible=s.should_render=1;s.head_flags=s.view_flags=15;
 s.head={{0,1.7,0},{0,0,0,1}};s.eyes[0].pose={{-.032,1.7,0},{0,0,0,1}};
 s.eyes[1].pose={{.032,1.7,0},{0,0,0,1}};
 for(auto& eye:s.eyes)eye.fov={-.8,.8,.8,-.8};return s;
}
int main(){
 unsigned cases{};
 for(unsigned mutation=0;mutation<16;++mutation){
  StereoGate gate;auto s=sample();assert(gate.update(s,s.received_ns+1,true));
  ++s.sequence;auto now=s.received_ns+100000001;
  if(mutation==1)s.head_flags=s.view_flags=0;
  if(mutation==2)s.focused=s.visible=0;
  if(mutation==3)++s.session;
  if(mutation==4)++s.space_epoch;
  if(mutation==5)s.received_ns=now+1;
  if(mutation==6)s.eyes[0].fov.right=s.eyes[0].fov.left;
  if(mutation==7)s.head.position.x=std::numeric_limits<double>::quiet_NaN();
  if(mutation==8)gate.request_recenter();
  if(mutation==9)s.focused=0; // Visible but no current input focus.
  if(mutation==10)s.sequence=0;
  if(mutation==11)now=s.received_ns+100000000;
  if(mutation==12)now=0;
  if(mutation==13)s.received_ns=0;
  if(mutation==14)s.should_render=0;
  auto before=gate.continuity();auto result=gate.update(s,now,true);auto after=gate.continuity();
  if(mutation==15)++after.epoch;
  const bool kept=gears_stereo_hand_age::preserve(s,now,result,before,after);
  assert(kept==(mutation==0));
  if(kept){assert(!result&&result.status==StereoStatus::stale&&before.stable&&after.stable);}
  ++cases;
 }
 {StereoGate gate;auto s=sample();const auto before=gate.continuity();auto result=gate.update(s,s.received_ns+100000001,true);
  assert(!gears_stereo_hand_age::preserve(s,s.received_ns+100000001,result,before,gate.continuity()));++cases;}
 std::printf("Original StereoGate + logical-age policy: %u boundary/tracking/focus/identity/origin/geometry/replay/recenter cases passed; stale render remains rejected\n",cases);
}
'''
(P/'render-age-test.cpp').write_text(code);cmd=P/'render-age-test.cmd';exe=P/'render-age-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{S}" "{P / "render-age-test.cpp"}" /Fo"{P / "render-age-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'render-age-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'render-age-test.log').read_text(errors='replace'));r.check_returncode()
s=(S/'capture.cpp').read_text();assert 'if(weapon_hand_runtime::enabled&&!age_only)cover_shared_input_live::invalidate' in s
block=s[s.index('        if(!result){++stereo_rejections'):s.index('        // The game tick has already authenticated the L3 edge.')]
assert 'return false;' in block and 'request_session_stop' in block
v=dict(exit_code=0,cases=17,original_gate_tested=True,render_still_rejected=True,session_stop_preserved=True,game_started=False,source_sha256=hashlib.sha256(code.encode()).hexdigest())
(P/'render-age-test-result.json').write_text(json.dumps(v,indent=2))
p=P/'prepared.json';proof=json.loads(p.read_text());proof.update(policy_cases=17,policy_failures=0);p.write_text(json.dumps(proof,indent=2))
