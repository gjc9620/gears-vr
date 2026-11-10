"""Run the actual scoped observer on data only; no game/process opened."""
from pathlib import Path
import hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
out=H/'checks-v1';out.mkdir(exist_ok=False)
source=R/'tools/native-capture/cover_query_source_cause_test.cpp'
cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(source),
    '/Fo:'+str(out/'check.obj'),'/Fe:'+str(out/'check.exe'),'/sourceDependencies',str(out/'dependencies.json')]
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');flags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS
build=subprocess.run(cmd,cwd=R,env=env,capture_output=True,creationflags=flags)
(out/'build.log').write_bytes(build.stdout+build.stderr)
row=dict(build_exit=build.returncode,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest())
if not build.returncode:
    run=subprocess.run([str(out/'check.exe')],cwd=R,capture_output=True,creationflags=flags)
    (out/'test.log').write_bytes(run.stdout+run.stderr)
    row.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
(out/'result.json').write_text(json.dumps(row,indent=2)+'\n');print(json.dumps(row))
raise SystemExit(row['build_exit'] or row.get('test_exit',1))
