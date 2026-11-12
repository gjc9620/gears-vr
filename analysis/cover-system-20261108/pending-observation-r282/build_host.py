from pathlib import Path
import hashlib,json,shutil,subprocess,sys
from overlay import ROOT as root,REL,apply
out=Path(__file__).resolve().parent;oldroot=out.parent/'recorded-readback-r277'
source=out/'host-source-v2';assert not source.exists()
old=json.loads((oldroot/'host-source-manifest.json').read_text())['files'];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for p,d in old.items():assert sha(oldroot/'host-source'/p)==d,p
shutil.copytree(oldroot/'host-source',source)
p=source/REL;p.write_text(apply(p.read_text(encoding='utf-8')),encoding='utf-8')
for n in ['cover_pending_observation.inl','cover_pending_observation_test.inl','cover_c0_policy_live_test.cpp']:
 shutil.copyfile(root/'tools/native-capture'/n,source/'tools/native-capture'/n)
manifest={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()}
changed=[p for p,d in manifest.items() if old.get(p)!=d];assert len(changed)==4,changed
(out/'host-source-v2-manifest.json').write_text(json.dumps(dict(files=manifest,changed=changed,body_goal_baseline='r277; observer-only Host proof; real r279 heading remains Native build check'),indent=2)+'\n')
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import vc_environment
prefix=out/'live-v2';src=source/'tools/native-capture/cover_c0_policy_live_test.cpp'
cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(src),'/I'+str(root/'tools/third-party/minhook/include'),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','bcrypt.lib','/sourceDependencies',str(prefix)+'-dependencies.json']
b=subprocess.run(cmd,env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
(out/'host-build-v2.log').write_bytes(b.stdout+b.stderr);result=dict(build_exit=b.returncode)
if b.returncode:result['error']=(b.stdout+b.stderr).decode('utf-8','replace')[-3500:]
else:
 fixture=root/'analysis/cover-system-20261108/61492-cover-frame'
 r=subprocess.run([str(prefix)+'.exe',str(fixture),str(fixture/'source-CT0.bin'),'11'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'host-test-v2.log').write_bytes(r.stdout+r.stderr);result.update(test_exit=r.returncode,output=r.stdout.decode('utf-8','replace')[-2500:])
(out/'host-result-v2.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
sys.exit(b.returncode or result.get('test_exit',0))
