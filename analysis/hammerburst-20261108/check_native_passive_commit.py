from pathlib import Path
import json,hashlib,subprocess
import prepare_native_passive_commit as prep
A=Path(__file__).resolve().parent;P=prep.P;BASE=prep.BASE
s=(P/'source'/prep.REL).read_text()
for addition in (prep.DECL,prep.CAPTURE,'observe_passive_commit();','summarize_passive_commits(f);'):
    assert s.count(addition)==1
    s=s.replace(addition,'')
assert s==(BASE/'source'/prep.REL).read_text()
changed=[]
for p in (BASE/'source/tools').rglob('*'):
    if p.is_file():
        relative=p.relative_to(BASE/'source')
        if p.read_bytes()!=(P/'source'/relative).read_bytes():changed.append(relative.as_posix())
assert changed==[prep.REL],changed
source=(P/'source/tools/native-capture/body_c0_hands_test.cpp').read_text()
anchor='            C(bytes(actual)==input_before&&bytes(work->eye_targets[0].grip_world)==bytes(moved_left));'
assert source.count(anchor)==1
source=source.replace(anchor,anchor+r'''
            // Same production solve with changing passive WORLD goals. Verify
            // the recorded world matrix against independently reconstructed FK.
            auto previous=passive[11];
            for(unsigned step=1;step<=3;++step){
                work->left_visual.grip.position.x=socket.position.x+step*.1;
                adjusted=body_only;
                C(a::compose_common(*model,*frame,actual,asset,base,now,sim,*work,adjusted,
                    a::adapter::MountKind::held_lancer,prepare,true).status==a::Status::okay);
                const auto moved=fk(*model,*frame,adjusted);
                const auto recorded=b::d::pose(work->body_work.pose.world[11]);
                C(work->left_visual_applied&&work->left_blend.position_fitted);
                C(distance(moved[11].position,recorded.position)<.002&&qerror(moved[11].orientation,recorded.orientation)<1e-5);
                C(distance(moved[11].position,previous.position)>.09);
                C(distance(moved[35].position,physical_targets[1].position)<.002&&qerror(moved[35].orientation,physical_targets[1].orientation)<1e-5);
                previous=moved[11];
            }
''')
target=P/'passive-commit-test.cpp';target.write_text(source)
S=P/'source/tools/native-capture';exe=P/'passive-commit-test.exe'
fixture=A.parent/'solo-m1-20261028/observed-backend-fixture'
assert all((fixture/n).exists() for n in ('reference.bin','C0-local.bin','C0-component.bin'))
cmd=P/'passive-commit-test.cmd'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /DGEARS_C0_CONTEXT_GEOMETRY_ONLY /EHsc /I"{S}" "{target}" bcrypt.lib /Fo"{P / "passive-commit-test.obj"}" /Fe"{exe}"\nif errorlevel 1 exit /b %errorlevel%\n"{exe}" "{fixture}"\nexit /b %errorlevel%\n')
with (P/'passive-commit-test.log').open('wb') as log:
    result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
proof=dict(passed=result.returncode==0,exact_policy_reversal=True,changed_source_files=changed,full_actual_c0_fixture=True,moving_passive_goal_steps=3,independent_fk_oracle=True,synthetic_input_seam=True,runtime_proved=False,build_native_sha256=json.loads((P/'prepared.json').read_text())['native_sha256'])
(P/'passive-commit-test-result.json').write_text(json.dumps(proof,indent=2))
print(proof)
assert result.returncode==0,(P/'passive-commit-test.log').read_text(errors='replace')[-5000:]
