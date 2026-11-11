from pathlib import Path
import json,subprocess,sys,hashlib
R=Path.cwd();H=R/'analysis/cover-system-20261108/component-recheck-r192';record=json.loads((H/'pending-verification.json').read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for p,d in record['source_sha256'].items():assert sha(R/p)==d==sha(H/'source'/p)
sys.path.insert(0,str(R/'tools/native-capture'));from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');v=subprocess.run(['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo',str(H/'cases.obj'),'/Fe:'+str(H/'cases.exe')],env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(H/'link.log').write_bytes(v.stdout+v.stderr);result=dict(link_exit=v.returncode,game_started=False,xr_started=False,quest_touched=False)
if not v.returncode:
 t=subprocess.run([str(H/'cases.exe')],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(H/'test.log').write_bytes(t.stdout+t.stderr);result.update(test_exit=t.returncode,output=t.stdout.decode('utf-8','replace'))
(H/'execution-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
