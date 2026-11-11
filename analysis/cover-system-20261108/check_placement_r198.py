from pathlib import Path
import subprocess,json,sys,hashlib
R=Path.cwd();H=R/'analysis/cover-system-20261108/placement-readiness-r198';H.mkdir();fixture=H.parent/'61492-cover-frame';prior=json.loads((H.parent/'c0-target-r99/verification.json').read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for n,d in prior['fixtures'].items():assert sha(fixture/n)==d
sys.path.insert(0,str(R/'tools/native-capture'));from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');rows=[]
for n in ('cover_neutral_goal','cover_c0_target_adapter'):
 p=H/n;c=subprocess.run(['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(R/'tools/native-capture'/(n+'_test.cpp')),'/Fo:'+str(p)+'.obj','/Fe:'+str(p)+'.exe','/sourceDependencies',str(p)+'-dependencies.json'],env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(p)+'-build.log').write_bytes(c.stdout+c.stderr);row=dict(name=n,build_exit=c.returncode)
 if not c.returncode:
  v=subprocess.run([str(p)+'.exe',str(fixture)],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(p)+'-test.log').write_bytes(v.stdout+v.stderr);row.update(test_exit=v.returncode,output=v.stdout.decode('utf-8','replace'));deps=json.loads(Path(str(p)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes'];row['source_sha256']={str(Path(x).relative_to(R)):sha(Path(x)) for x in deps if Path(x).is_relative_to(R)}
 rows.append(row);print(json.dumps({k:v for k,v in row.items() if k!='source_sha256'}),flush=True)
(H/'results.json').write_text(json.dumps(rows,indent=2)+'\n');(H/'fixture-hashes.json').write_text(json.dumps(prior['fixtures'],indent=2)+'\n')
