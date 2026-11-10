"""Compile and run immutable-data checks only; no game/process opened."""
from pathlib import Path
import hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
out=H/sys.argv[1];out.mkdir(exist_ok=False)
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');results=[]
for folder,name in (('native-capture','cover_measurement_core_observation'),):
    source=R/'tools'/folder/(name+'_test.cpp')
    cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(source),
        '/Fo:'+str(out/(name+'.obj')),'/Fe:'+str(out/(name+'.exe')),'/sourceDependencies',str(out/(name+'-dependencies.json'))]
    build=subprocess.run(cmd,cwd=R,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
    (out/(name+'-build.log')).write_bytes(build.stdout+build.stderr)
    row=dict(name=name,build_exit=build.returncode,source_sha256=hashlib.sha256(source.read_bytes()).hexdigest())
    if not build.returncode:
        run=subprocess.run([str(out/(name+'.exe'))],cwd=R,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(name+'-test.log')).write_bytes(run.stdout+run.stderr)
        row.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'))
    results.append(row);print(json.dumps(row),flush=True)
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
raise SystemExit(any(r['build_exit'] or r.get('test_exit',1) for r in results))
