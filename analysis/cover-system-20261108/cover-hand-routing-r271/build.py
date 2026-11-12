from pathlib import Path
import sys,subprocess,json,hashlib
root=Path.cwd();out=root/'analysis/cover-system-20261108/cover-hand-routing-r271';name=sys.argv[1]
fixture='analysis/cover-system-20261108/61492-cover-frame'
cases={
 'live':('native-capture/cover_c0_policy_live_test.cpp',[fixture,fixture+'/source-CT0.bin',sys.argv[2] if len(sys.argv)>2 else '0']),
 'input':('native-capture/cover_input_live_test.cpp',[]),
 'store':('native-capture/cover_full_goal_live_test.cpp',[]),
 'points':('native-capture/cover_l2_grab_live_test.cpp',[])}
source,args=cases[name];sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import vc_environment
prefix=out/name;source_path=(out/'host-source'/'tools'/source) if name=='live' else (root/'tools'/source);cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(source_path),'/I'+str(root/'tools/third-party/minhook/include'),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','bcrypt.lib','/sourceDependencies',str(prefix)+'-dependencies.json']
b=subprocess.run(cmd,env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr);result={'build_exit':b.returncode}
if b.returncode:result['errors']=(b.stdout+b.stderr).decode('utf-8','replace')[-4000:]
else:
 r=subprocess.run([str(prefix)+'.exe',*args],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-test.log').write_bytes(r.stdout+r.stderr);result.update(test_exit=r.returncode,output=r.stdout.decode('utf-8','replace')[-4000:])
 deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']+[str(source_path)]
 result['source_sha256']={str(Path(p).relative_to(root)):hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in deps if Path(p).is_relative_to(root)}
Path(str(prefix)+'-result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps({k:v for k,v in result.items() if k!='source_sha256'}))
