from pathlib import Path
import subprocess,sys,json
R=Path.cwd();H=R/'analysis/cover-system-20261108/link-path-proof-r183';source=H/'checks-v2/after';target=source/'tools/native-capture/cover_context_live_test.cpp';prefix=H/'context-off'
sys.path.insert(0,str(R/'tools/native-capture'));from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat')
cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT','/DGEARS_COVER_BODY_PROTOTYPE=0','/I'+str(source/'tools/third-party/minhook/include'),str(target),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe']
b=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr);assert b.returncode==0
v=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-test.log').write_bytes(v.stdout+v.stderr);result=dict(build_exit=b.returncode,test_exit=v.returncode,output=v.stdout.decode('utf-8','replace'));(H/'off-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));assert v.returncode==0
