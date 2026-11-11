from pathlib import Path
import hashlib,json,shutil,subprocess,sys
H=Path(__file__).resolve().parent;R=H.parents[2]
base=H.parent/'root-schema-r177/native-source'
manifest=json.loads((base.parent/'native-source-manifest.json').read_text())['files']
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for p,d in manifest.items():assert sha(base/p)==d
source=H/'source';shutil.copytree(base,source);target=source/'tools/native-capture'
for p in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(p,target/p.name)
shutil.copyfile(H/'probe.cpp',target/'cover_recheck_cost_test.cpp')
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');prefix=H/'probe'
cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT','/I'+str(source/'tools/third-party/minhook/include'),str(target/'cover_recheck_cost_test.cpp'),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
b=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(H/'build.log').write_bytes(b.stdout+b.stderr)
assert b.returncode==0,b.returncode
v=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(H/'probe.log').write_bytes(v.stdout+v.stderr)
text=v.stdout.decode('utf-8','replace');print(text,flush=True)
deps=json.loads((H/'probe-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']
outside=[p for p in deps if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(source)]
rows=[json.loads(line) for line in text.splitlines() if line.startswith('{')]
result=dict(build_exit=b.returncode,test_exit=v.returncode,rows=rows,outside_worktree_dependencies=outside,probe_sha256=sha(H/'probe.cpp'),game_started=False,original_query_called=False,production_changed=False)
(H/'results.json').write_text(json.dumps(result,indent=2)+'\n')
assert v.returncode==0 and len(rows)==2 and not outside
