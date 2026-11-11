from pathlib import Path
import json,hashlib,shutil,difflib,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'native-render-age-r1';P=A/'native-mutex-r1';S=P/'source'
def main():
 assert not P.exists(),'Preserve candidate identity'
 parent=json.loads((BASE/'prepared.json').read_text());shutil.copytree(BASE/'source/tools',S/'tools');shutil.copytree(BASE/'dependency',P/'dependency');patch=''
 def edit(rel,fn):
  nonlocal patch
  p=S/rel;before=p.read_text();after=fn(before);assert before!=after;p.write_text(after);patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 def bridge(s):
  needle='    std::int64_t selected_sync_ns{};';assert s.count(needle)==1
  s=s.replace(needle,needle+'\n    DWORD mutex_result{};bool mutex_attempted{},mutex_contended{}; // Receiver-local; never a permit.')
  s=s.replace('static bool lock_now(HANDLE mutex,bool* contended=nullptr) noexcept {','static bool lock_now(HANDLE mutex,bool* contended=nullptr,DWORD* observed=nullptr,bool* attempted=nullptr) noexcept {')
  needle='        if(contended)*contended=false;if(!mutex)return false;';assert s.count(needle)==1
  s=s.replace(needle,'        if(attempted)*attempted=false;\n'+needle)
  needle='        const auto result=WaitForSingleObject(mutex,0);';assert s.count(needle)==1
  s=s.replace(needle,needle+'\n        if(observed)*observed=result;if(attempted)*attempted=true;')
  needle='        if(!lock_now(mutex_,contended))return finish(Code::mutex_unavailable);';assert s.count(needle)==1
  s=s.replace(needle,'        const bool locked=lock_now(mutex_,contended,&local.mutex_result,&local.mutex_attempted);\n        local.mutex_contended=local.mutex_attempted&&local.mutex_result==WAIT_TIMEOUT;\n        if(!locked)return finish(Code::mutex_unavailable);')
  return s
 edit('tools/vr-common/controller_input_bridge.h',bridge)
 def summary(s):
  needle='    std::int64_t source_age_ns{},selected_age_ns{};';assert s.count(needle)==1
  s=s.replace(needle,needle+'\n    unsigned mutex_attempted{},mutex_result{},mutex_contended{};')
  needle='matching.selected_sync_ns?stamp-matching.selected_sync_ns:0};';assert s.count(needle)==1
  s=s.replace(needle,'matching.selected_sync_ns?stamp-matching.selected_sync_ns:0,\n                unsigned(matching.mutex_attempted),matching.mutex_result,unsigned(matching.mutex_contended)};')
  needle='static_cast<long long>(r.source_age_ns),static_cast<long long>(r.selected_age_ns));}';assert s.count(needle)==1
  s=s.replace(needle,'static_cast<long long>(r.source_age_ns),static_cast<long long>(r.selected_age_ns));\n        fprintf(file,"shared_grip_handoff_mutex_%u=%u,%u,%u ",i,r.mutex_attempted,r.mutex_result,r.mutex_contended);}')
  return s
 edit('tools/native-capture/cover_shared_input_live.inl',summary)
 (R/'docs/hammerburst-native-mutex.patch').write_text(patch)
 C=P/'project';C.mkdir();(C/'CMakeLists.txt').write_text((BASE/'project/CMakeLists.txt').read_text().replace(str(BASE).replace('\\','/'),str(P).replace('\\','/')))
 commands=[list(c) for c in parent['commands']];commands[0][commands[0].index('-S')+1]=str(C);commands[0][commands[0].index('-B')+1]=str(P/'build');commands[1][commands[1].index('--build')+1]=str(P/'build')
 for label,cmd in zip(('configure','build'),commands):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsCapture6DofM1.dll';proof=dict(parent_native=parent['native_sha256'],frozen_native=parent['frozen_native'],native_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),dll=str(dll),full_native_compiled=True,commands=commands,original_compile_definitions=parent['original_compile_definitions'],runtime_exercised=False,gameplay_accepted=False,root_native_modified=False,xr_changed=False,scope='Receiver-local exact original zero-time mutex return, attempted and contended flags; bounded existing GT handoff summary. No retry/wait change/packet layout/writer/source/action/freshness changes.',policy_cases=parent['policy_cases'],policy_failures=parent['policy_failures'])
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['native_sha256'])
if __name__=='__main__':main()
