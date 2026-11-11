"""Compare actual r163 against query-scoped collider proof revalidation, with real fixtures."""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=H.parent/'component-root-repeat-r165/native-source'
manifest=json.loads((base.parent/'native-source-manifest.json').read_text())['files']
for p,d in manifest.items():assert sha(base/p)==d
out=H/(sys.argv[1] if len(sys.argv)>1 else 'checks-v1');out.mkdir()
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');results=[]
for variant in ('before','after'):
    source=out/(variant+'-src');shutil.copytree(base,source);target=source/'tools/native-capture'
    for p in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(p,target/p.name)
    if variant=='after':
        for changed in ('cover_collider_roles_live.inl','cover_physics_query_live.inl'):shutil.copyfile(R/'tools/native-capture'/changed,target/changed)
    names=['cover_collider_recheck']
    if variant=='after' and '--focused' not in sys.argv:names+=['cover_collider_roles_live','cover_component_roles_live','cover_query_guard_live','cover_physics_query_live','cover_physics_capsule_path_live','cover_physics_overlap_query_live','cover_physics_c0_off']
    for name in names:
        prefix=out/(variant+'-'+name)
        cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',
            '/I'+str(source/'tools/third-party/minhook/include'),str(target/(name+'_test.cpp')),
            '/DGEARS_COLLIDER_RECHECK='+str(int(variant=='after')), '/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
        b=subprocess.run(cmd,cwd=R,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
        Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr);row=dict(variant=variant,name=name,build_exit=b.returncode)
        if not b.returncode:
            run=subprocess.run([str(prefix)+'.exe'],cwd=R,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
            Path(str(prefix)+'-test.log').write_bytes(run.stdout+run.stderr);row.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
            deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']
            row['outside_worktree_dependencies']=[p for p in deps if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(source)]
            assert not row['outside_worktree_dependencies']
        results.append(row);print(json.dumps(row),flush=True)
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
if any(r['build_exit'] or r.get('test_exit',1) for r in results):raise SystemExit(1)
counts={r['variant']:{n:int(c) for n,c in re.findall(r'collider_recheck presence=(\d+) five_reads=(\d+)' ,r['output'])} for r in results if r['name']=='cover_collider_recheck'}
assert counts['before'].keys()==counts['after'].keys()
comparison={k:dict(before=v,after=counts['after'][k],reduction=1-counts['after'][k]/v) for k,v in counts['before'].items()}
assert all(v['after']<v['before'] for v in comparison.values())
(out/'read-comparison.json').write_text(json.dumps(comparison,indent=2)+'\n');print(json.dumps(comparison))
