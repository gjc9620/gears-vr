"""Check copied source-age observations on exact r167, retaining actual reader fixtures."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=H.parent/'owner-link-detail-r171/native-source'
manifest=json.loads((base.parent/'native-source-manifest.json').read_text())['files']
for p,d in manifest.items():assert sha(base/p)==d
out=H/(sys.argv[1] if len(sys.argv)>1 else 'checks-v1');out.mkdir();source=out/'source';shutil.copytree(base,source)
target=source/'tools/native-capture'
for p in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(p,target/p.name)
for name in (() if '--baseline' in sys.argv else ('cover_physx_actor_backlink.h','cover_owner_link_observation.h','cover_physx_asset_actors.h','cover_physx_collider.h','cover_physx_components.h','cover_physx_actor_shapes.h','cover_physx_shape_owner.h','cover_physx_self_evidence.h')):
    shutil.copyfile(R/'tools/native-capture'/name,target/name)
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');results=[]
names=('cover_physx_owned_sweep',) if '--owned-only' in sys.argv else ('cover_physics_capsule_path_live',) if '--path-only' in sys.argv else ('cover_owner_link_observation','cover_physics_capsule_path_live','cover_physx_owned_sweep') if '--repair-checks' in sys.argv else ('cover_physx_actor_backlink',) if '--focused' in sys.argv else ('cover_physx_actor_backlink','cover_physx_asset_actors','cover_physx_collider','cover_physx_components','cover_physx_component_assets','cover_physx_actor_attachments','cover_physx_shape_owner','cover_physx_actor_shapes','cover_physics_capsule_path_live','cover_physics_c0_off')
for name in names:
    prefix=out/name
    cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',
        '/I'+str(source/'tools/third-party/minhook/include'),str(target/(name+'_test.cpp')),
        '/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
    if '--baseline' in sys.argv:cmd+=['/DEXPECT_BROKEN_BACKLINK=1']
    cmd+=['/link','bcrypt.lib','psapi.lib']
    build=subprocess.run(cmd,cwd=R,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    Path(str(prefix)+'-build.log').write_bytes(build.stdout+build.stderr);row=dict(name=name,build_exit=build.returncode)
    if not build.returncode:
        run=subprocess.run([str(prefix)+'.exe']+(['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll'] if name in ('cover_physx_shape_owner','cover_physx_actor_shapes','cover_physx_owned_sweep') else []),cwd=R,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
        Path(str(prefix)+'-test.log').write_bytes(run.stdout+run.stderr);row.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
        deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']
        row['outside_worktree_dependencies']=[p for p in deps if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(source)]
        assert not row['outside_worktree_dependencies']
    results.append(row);print(json.dumps(row),flush=True)
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
if any(r['build_exit'] or r.get('test_exit',1) for r in results):raise SystemExit(1)





