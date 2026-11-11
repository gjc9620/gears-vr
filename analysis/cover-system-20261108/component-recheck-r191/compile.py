from pathlib import Path
import json,shutil,subprocess,sys,hashlib
R=Path.cwd();H=R/'analysis/cover-system-20261108/component-recheck-r191';base=H.parent/'owner-bracket-r187/native-source';src=H/'compile-source';shutil.copytree(base,src)
for test in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(test,src/'tools/native-capture'/test.name)
name='tools/native-capture/cover_component_roles_live.inl';shutil.copyfile(R/name,src/name)
unit=src/'tools/native-capture/compile_component_recheck.cpp';unit.write_text('#define GEARS_COMPONENT_ROLES_TEST_MAIN PriorComponentMain\n#include "cover_component_roles_live_test.cpp"\nvoid compile_recheck_api(catalog::Snapshot& output){ (void)catalog::recheck(h::Binding{},[](){return true;},catalog_work,output); }\n')
sys.path.insert(0,str(R/'tools/native-capture'));from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT','/c',str(unit),'/I'+str(src/'tools/third-party/minhook/include'),'/Fo:'+str(H/'component-recheck.obj'),'/sourceDependencies',str(H/'compile-dependencies.json')]
v=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(H/'compile.log').write_bytes(v.stdout+v.stderr)
row=dict(compile_exit=v.returncode,linked=False,program_executed=False,game_started=False,product_source_sha256=hashlib.sha256((R/name).read_bytes()).hexdigest())
if not v.returncode:
 deps=json.loads((H/'compile-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes'];row['outside_worktree_dependencies']=[p for p in deps if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(src)];assert not row['outside_worktree_dependencies']
(H/'compile-result.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(row));sys.exit(v.returncode)
