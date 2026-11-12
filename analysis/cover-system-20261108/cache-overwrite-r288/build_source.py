from pathlib import Path
import subprocess,sys,json,hashlib
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent;sys.path.insert(0,str(r/'tools/native-capture'));from build_verified import vc_environment
src=r/'tools/native-capture/cached_pose_overwrite_source_test.cpp';prefix=o/'cached-source-v3'
b=subprocess.run(['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/MT',str(src),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json'],env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr);result=dict(build_exit=b.returncode)
if b.returncode:result['error']=(b.stdout+b.stderr).decode('gb18030','replace')[-2000:]
else:
 x=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-test.log').write_bytes(x.stdout+x.stderr);result.update(test_exit=x.returncode,output=x.stdout.decode('utf-8','replace'))
 deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']+[str(src)]
 result['source_sha256']={Path(p).relative_to(r).as_posix():hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in deps if Path(p).is_relative_to(r)}
Path(str(prefix)+'-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='source_sha256'}));sys.exit(b.returncode or result.get('test_exit',0))
