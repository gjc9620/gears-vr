from pathlib import Path
import json,subprocess
A=Path(__file__).resolve().parent;P=A/'early-released-refire-r1';S=P/'source/tools/playground'
source=(S/'weapon_equipment_runtime_test.cpp').read_text();anchor='int main(){';assert source.count(anchor)==1
extra=r'''
void delayed_pickup_log_semantics(){
 ++cases;Host h;h.integrated_stow=true;h.unholster_changed=true;h.baseline();h.press();h.event(0);
 const auto pending=h.tick();C(pending.status==er::Status::unholster_pending);
 const auto deadline=h.runtime.state().deadline_ms;
 h.runtime.suspend(h.backend());h.advance();const auto held=h.tick();
 C(held.status==er::Status::held&&held.event_serial==pending.event_serial&&held.press_counter==pending.press_counter);
 C(h.model.held()&&h.published_hold&&h.unholster_calls==1&&!h.runtime.state().pending);
 C(h.ms<deadline);
 // A subsequent publication gap resets the input observer, not the model.
 h.runtime.suspend(h.backend());C(h.model.held()&&!h.published_hold);
 h.advance();const auto baseline=h.tick();
 C(baseline.status==er::Status::baseline&&baseline.slot==4&&!baseline.target&&!baseline.event_serial);
 C(h.model.held()&&h.published_hold&&h.unholster_calls==1&&h.switch_calls==0);
 h.advance();C(h.tick().status==er::Status::idle&&h.model.held()&&h.published_hold);
 std::puts("Actual Runtime delayed original pickup + held baseline outcome: same original event completes once; baseline slot4 target0 coexists with held model and freshly republished binding");
}
'''
source=source.replace(anchor,extra+'\n'+anchor+'\n    delayed_pickup_log_semantics();');target=S/'hammerburst_pickup_completion_model_test.cpp';target.write_text(source);cmd=P/'pickup-completion-test.cmd';exe=P/'pickup-completion-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{target}" /Fo"{P / "pickup-completion-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'pickup-completion-test.log').open('wb') as log:r=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'pickup-completion-test.log').read_text(errors='replace'));r.check_returncode()
(P/'pickup-completion-test-result.json').write_text(json.dumps(dict(passed=True,actual_runtime_and_model=True,baseline_outcome_not_model_drop=True,pending_original_completes_after_input_reset=True,extra_unholster_or_input=False,game_execution=False),indent=2))
