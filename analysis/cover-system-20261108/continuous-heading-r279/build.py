from pathlib import Path
import hashlib,json,subprocess,sys
root=Path(__file__).resolve().parents[3];out=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import vc_environment
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat')
for name in ['cover_view_heading','cover_l2_grab_live','cover_input_live','cover_shared_input_live','cover_full_goal_live']:
 source=root/'tools/native-capture'/(name+'_test.cpp');prefix=out/name
 cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(source),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','bcrypt.lib','/sourceDependencies',str(prefix)+'-dependencies.json']
 b=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
 Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr);result={'build_exit':b.returncode}
 if b.returncode:result['errors']=(b.stdout+b.stderr).decode('utf-8','replace')[-3000:]
 else:
  r=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
  Path(str(prefix)+'-test.log').write_bytes(r.stdout+r.stderr);result.update(test_exit=r.returncode,output=r.stdout.decode('utf-8','replace')[-3000:])
  deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']+[str(source)]
  result['source_sha256']={str(Path(p).relative_to(root)):sha(Path(p)) for p in deps if Path(p).is_relative_to(root)}
 Path(str(prefix)+'-result.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({'name':name,**{k:v for k,v in result.items() if k!='source_sha256'}}),flush=True)
 if b.returncode or result.get('test_exit'):sys.exit(1)
