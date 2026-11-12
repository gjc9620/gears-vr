from pathlib import Path
import json,subprocess,sys,hashlib
root=Path.cwd();folder=root/'analysis/cover-system-20261108/query-headroom-r251';variant=sys.argv[1]
cases={'before':('cover_physx_pose_span_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'span':('cover_physx_pose_span_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'geometry':('cover_query_headroom_test.cpp',[]),'math':('cover_physx_observed_pose_math_test.cpp',[]),'older':('cover_physx_observed_pose_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'replay':('cover_physx_observed_pose_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'actor':('cover_physx_actor_shapes_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'old_fields':('cover_physx_shape_owner_fields_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'counter':('cover_physx_shape_owner_fields_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'engine':('cover_physx_asset_shapes_engine_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'session':('cover_asset_shapes_session_test.cpp',[]),'off':('cover_physics_c0_off_test.cpp',[]),'construction':('cover_physx_shape_construction_test.cpp',[]),'groups_engine':('cover_physx_read_group_engine_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'owned':('cover_physx_owned_sweep_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'overlap':('cover_physx_overlap_query_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']),'path':('cover_physics_capsule_path_live_test.cpp',[]),'before_owned':('../../analysis/cover-system-20261108/query-headroom-r251/before-owned/cover_physx_owned_sweep_test.cpp',['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll'])}
file,args=cases[variant];prefix=folder/variant;source=(root/'tools/native-capture'/file).resolve()
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import vc_environment
flags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS
cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(source),
 '/I'+str(root/'tools/native-capture'),'/I'+str(root/'tools/third-party/minhook/include'),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','bcrypt.lib','/sourceDependencies',str(prefix)+'-dependencies.json']
build=subprocess.run(cmd,env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),capture_output=True,creationflags=flags)
Path(str(prefix)+'-build.log').write_bytes(build.stdout+build.stderr);result=dict(build_exit=build.returncode)
if not build.returncode:
 run=subprocess.run([str(prefix)+'.exe',*[str(root/p) for p in args]],capture_output=True,creationflags=flags)
 Path(str(prefix)+'-test.log').write_bytes(run.stdout+run.stderr);result.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
 deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']+[str(source)]
 result['source_sha256']={str(Path(p).relative_to(root)):hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in deps if Path(p).is_relative_to(root)}
Path(str(prefix)+'-result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:v for k,v in result.items() if k!='source_sha256'}))





