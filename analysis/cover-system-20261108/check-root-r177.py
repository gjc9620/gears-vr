from pathlib import Path
import json,shutil,subprocess,sys,hashlib
R=Path.cwd(); H=R/'analysis/cover-system-20261108/root-schema-r177';H.mkdir()
base=H.parent/'physics-recheck-r175/native-source'
manifest=json.loads((base.parent/'native-source-manifest.json').read_text())['files']
for p,d in manifest.items():assert hashlib.sha256((base/p).read_bytes()).hexdigest()==d
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');rows=[]
for variant in ('before','after'):
    src=H/variant;shutil.copytree(base,src)
    target=src/'tools/native-capture'
    shutil.copyfile(R/'tools/native-capture/cover_root_schema_test.cpp',target/'cover_root_schema_test.cpp')
    if variant=='after':shutil.copyfile(R/'tools/native-capture/cover_component_roles_live.inl',target/'cover_component_roles_live.inl')
    prefix=H/(variant+'-root')
    cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT','/I'+str(src/'tools/third-party/minhook/include'),str(target/'cover_root_schema_test.cpp'),'/DGEARS_ROOT_BATCH='+str(int(variant=='after')),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe']
    build=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    Path(str(prefix)+'-build.log').write_bytes(build.stdout+build.stderr)
    row=dict(variant=variant,build_exit=build.returncode)
    if not build.returncode:
        run=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
        Path(str(prefix)+'-test.log').write_bytes(run.stdout+run.stderr)
        row.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
    rows.append(row);print(json.dumps(row),flush=True)
(H/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
raise SystemExit(int(any(x['build_exit'] or x.get('test_exit',1) for x in rows)))
