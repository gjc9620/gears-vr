from pathlib import Path
import json,shutil,subprocess,sys
R=Path.cwd();H=R/'analysis/cover-system-20261108/component-consumer-r193';out=H/'related';out.mkdir()
src=H.parent/'owner-bracket-r187/native-source';tests=src/'tools/native-capture'
# Tests are separate scratch files; the frozen production manifest is unchanged.
# Keep them outside the frozen build tree and provide all quoted includes there.
scratch=out/'source';shutil.copytree(src,scratch);tests=scratch/'tools/native-capture'
for name in ('cover_component_roles_live.inl','cover_physics_query_live.inl'):shutil.copyfile(R/'tools/native-capture'/name,tests/name)
for test in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(test,tests/test.name)
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');rows=[]
for name in ('cover_physics_capsule_path_live','cover_physics_overlap_query_live'):
    prefix=out/name
    cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT','/I'+str(scratch/'tools/third-party/minhook/include'),str(tests/(name+'_test.cpp')),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
    b=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr)
    row=dict(name=name,build_exit=b.returncode)
    if not b.returncode:
        v=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-test.log').write_bytes(v.stdout+v.stderr);row.update(test_exit=v.returncode,output=v.stdout.decode('utf-8','replace'))
        includes=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes'];row['outside_worktree_dependencies']=[p for p in includes if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(scratch)];assert not row['outside_worktree_dependencies']
    rows.append(row);print(json.dumps(row),flush=True)
(out/'results.json').write_text(json.dumps(rows,indent=2)+'\n');raise SystemExit(int(any(r['build_exit'] or r.get('test_exit',1) for r in rows)))
