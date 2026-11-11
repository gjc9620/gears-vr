"""Check copied source-age observations on exact r167, retaining actual reader fixtures."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=H.parent/'collider-recheck-r167/native-source'
manifest=json.loads((base.parent/'native-source-manifest.json').read_text())['files']
for p,d in manifest.items():assert sha(base/p)==d
out=H/(sys.argv[1] if len(sys.argv)>1 else 'checks-v1');out.mkdir();source=out/'source';shutil.copytree(base,source)
target=source/'tools/native-capture'
for p in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(p,target/p.name)
for name in ('body_source_age_observation.h','body_tick_source_live.inl','cover_physics_c0_live.inl'):
    shutil.copyfile(R/'tools/native-capture'/name,target/name)
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');results=[]
names=('body_source_age_observation',) if '--focused' in sys.argv else ('body_source_age_observation','cover_query_source_cause','cover_query_timing','cover_query_guard_live','cover_physics_capsule_path_live','cover_physics_c0_off')
for name in names:
    prefix=out/name
    cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',
        '/I'+str(source/'tools/third-party/minhook/include'),str(target/(name+'_test.cpp')),
        '/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
    build=subprocess.run(cmd,cwd=R,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    Path(str(prefix)+'-build.log').write_bytes(build.stdout+build.stderr);row=dict(name=name,build_exit=build.returncode)
    if not build.returncode:
        run=subprocess.run([str(prefix)+'.exe'],cwd=R,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
        Path(str(prefix)+'-test.log').write_bytes(run.stdout+run.stderr);row.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
        deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']
        row['outside_worktree_dependencies']=[p for p in deps if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(source)]
        assert not row['outside_worktree_dependencies']
    results.append(row);print(json.dumps(row),flush=True)
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
if any(r['build_exit'] or r.get('test_exit',1) for r in results):raise SystemExit(1)
