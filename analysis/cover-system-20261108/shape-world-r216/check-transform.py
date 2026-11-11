from pathlib import Path
import json,hashlib,subprocess,sys
root=Path.cwd();folder=root/'analysis/cover-system-20261108/shape-world-r216';prefix=folder/'transform'
sys.path.insert(0,str(root/'tools/native-capture'));from build_verified import vc_environment
cl=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',
 str(root/'tools/native-capture/cover_physx_transform_test.cpp'),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
build=subprocess.run(cl,env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat'),capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
(folder/'transform-build.log').write_bytes(build.stdout+build.stderr);result=dict(build_exit=build.returncode,tests=[])
if not build.returncode:
 old=json.loads((folder.parent/'physx-transform-r74/verification.json').read_text())
 for n,test in enumerate(old['tests']):
  file=root/test['fixture'];digest=hashlib.sha256(file.read_bytes()).hexdigest();assert digest==test['fixture_sha256']
  run=subprocess.run([str(prefix)+'.exe',str(file)],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW)
  (folder/('transform-'+str(n)+'-test.log')).write_bytes(run.stdout+run.stderr)
  assert hashlib.sha256(file.read_bytes()).hexdigest()==digest
  result['tests'].append(dict(fixture=test['fixture'],sha256=digest,exit=run.returncode,output=run.stdout.decode('utf-8','replace')))
(folder/'transform-result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8');print(json.dumps(result))
