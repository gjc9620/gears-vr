from pathlib import Path
import subprocess,sys,json,hashlib,shutil
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent;sys.path.insert(0,str(r/'tools/native-capture'));from build_verified import vc_environment
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();source=o/'host-source-v4';assert not source.exists()
native=o/'native-source-v3';manifest=json.loads((o/'native-source-v3-manifest.json').read_text())['files']
for rel,digest in manifest.items():assert sha(native/rel)==digest
shutil.copytree(native,source);test='tools/native-capture/cached_pose_overwrite_source_test.cpp';shutil.copyfile(r/test,source/test)
files={p.relative_to(source).as_posix():sha(p) for p in source.rglob('*') if p.is_file()};(o/'host-source-v4-manifest.json').write_text(json.dumps(dict(files=files,baseline='exact r288-v3 candidate + owned test only',root_dirty_phase_header_excluded=True),indent=2)+'\n')
src=source/test;prefix=o/'cached-source-v4'
b=subprocess.run(['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/MT',str(src),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json'],env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr);result=dict(build_exit=b.returncode,host_source=str(source))
if b.returncode:result['error']=(b.stdout+b.stderr).decode('gb18030','replace')[-2000:]
else:
 x=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-test.log').write_bytes(x.stdout+x.stderr);result.update(test_exit=x.returncode,output=x.stdout.decode('utf-8','replace'))
 deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']+[str(src)];result['source_sha256']={Path(p).relative_to(source).as_posix():sha(Path(p)) for p in deps if Path(p).is_relative_to(source)}
 assert all(not Path(p).is_relative_to(r) or Path(p).is_relative_to(source) for p in deps)
Path(str(prefix)+'-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='source_sha256'}));sys.exit(b.returncode or result.get('test_exit',0))
