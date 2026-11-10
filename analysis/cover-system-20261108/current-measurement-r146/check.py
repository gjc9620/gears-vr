"""Compile the actual frozen measurement API and recorded-skeleton checks."""
from pathlib import Path
import hashlib,json,subprocess,sys
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parent
sys.path.insert(0,str(R/'tools/native-capture'))
from build_verified import vc_environment
out=H/sys.argv[1];out.mkdir(exist_ok=False)
source=H/'current_measurement_test.cpp';frozen=H/'native-source/tools/native-capture'
cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',
    '/I'+str(frozen),str(source),'/Fo:'+str(out/'check.obj'),'/Fe:'+str(out/'check.exe'),'/sourceDependencies',str(out/'dependencies.json'),'/link','bcrypt.lib']
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat')
flags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS
build=subprocess.run(cmd,cwd=R,env=env,capture_output=True,creationflags=flags)
(out/'build.log').write_bytes(build.stdout+build.stderr)
record=dict(build_exit=build.returncode,test_source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),command=cmd)
if not build.returncode:
    args=[str(out/'check.exe'),str(R/'analysis/solo-m1-20261028/observed-backend-fixture')]
    run=subprocess.run(args,cwd=R,capture_output=True,creationflags=flags)
    (out/'test.log').write_bytes(run.stdout+run.stderr)
    record.update(test_exit=run.returncode,output=run.stdout.decode('utf-8','replace'),arguments=args)
(out/'result.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
raise SystemExit(record['build_exit'] or record.get('test_exit',1))
