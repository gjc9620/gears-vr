"""Compare actual frozen r156 against schema-proof reuse and its consumers."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=H.parent/'mesh-role-repeat-r156/native-source';out=H/(sys.argv[1] if len(sys.argv)>1 else 'checks-v1');out.mkdir()
manifest=json.loads((base.parent/'native-source-manifest.json').read_text())['files']
for name,digest in manifest.items():assert sha(base/name)==digest
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');creation=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS
results=[]
for variant in ('before','after'):
    source=out/(variant+'-src');shutil.copytree(base,source);target=source/'tools/native-capture'
    for fixture in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(fixture,target/fixture.name)
    if variant=='after':
        for name in ('cover_physics_roles_live.inl','cover_collider_roles_live.inl'):shutil.copyfile(R/'tools/native-capture'/name,target/name)
    names=['cover_schema_repeat']
    if variant=='after':names+=['cover_physics_roles_live','cover_collider_roles_live','cover_component_roles_live',
        'cover_query_guard_live','cover_physics_query_live','cover_physics_capsule_path_live','cover_physics_overlap_query_live','cover_physics_c0_off']
    for name in names:
        prefix=out/(variant+'-'+name)
        command=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',
            '/I'+str(source/'tools/third-party/minhook/include'),str(target/(name+'_test.cpp')),
            '/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
        build=subprocess.run(command,cwd=R,env=env,capture_output=True,creationflags=creation);Path(str(prefix)+'-build.log').write_bytes(build.stdout+build.stderr)
        row=dict(variant=variant,name=name,build_exit=build.returncode)
        if not build.returncode:
            run=subprocess.run([str(prefix)+'.exe'],cwd=R,capture_output=True,creationflags=creation);Path(str(prefix)+'-test.log').write_bytes(run.stdout+run.stderr)
            row.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
            deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']
            row['outside_worktree_dependencies']=[p for p in deps if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(source)]
            assert not row['outside_worktree_dependencies']
        results.append(row);print(json.dumps(row),flush=True)
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
if any(r['build_exit'] or r.get('test_exit',1) for r in results):raise SystemExit(1)
counts={r['variant']:{kind+'-'+n:int(c) for kind,n,c in re.findall(r'schema_repeat (physics bodies|collider presence)=(\d+) reads=(\d+)',r['output'])} for r in results if r['name']=='cover_schema_repeat'}
assert counts['before'].keys()==counts['after'].keys()
comparison={key:dict(before=value,after=counts['after'][key],reduction=1-counts['after'][key]/value) for key,value in counts['before'].items()}
assert all(v['after']<v['before'] for v in comparison.values())
(out/'read-comparison.json').write_text(json.dumps(comparison,indent=2)+'\n');print(json.dumps(comparison))
