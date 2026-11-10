"""Offline compiler/data checks only; never loads PhysX or launches the game."""
import hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tools/native-capture'))
from build_verified import vc_environment
out=Path(__file__).parent/sys.argv[1]
out.mkdir(exist_ok=False)
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat')
records=[]
for name in sys.argv[2:]:
    source=ROOT/'tools/native-capture'/(name+'_test.cpp')
    command=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT','/I'+str(ROOT/'tools/third-party/minhook/include'),str(source),'/Fo:'+str(out/(name+'.obj')),'/sourceDependencies',str(out/(name+'-dependencies.json')),'/Fe:'+str(out/(name+'.exe')),'/link','bcrypt.lib','psapi.lib']
    flags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS
    result=subprocess.run(command,cwd=ROOT,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=flags)
    (out/(name+'-build.log')).write_bytes(result.stdout)
    record={'name':name,'build_exit':result.returncode,'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'command':command}
    if not result.returncode:
        args=[str(out/(name+'.exe'))]
        if name in ('cover_physx_query','cover_physx_owned_sweep','cover_physx_sphere_overlap','cover_physx_overlap_query'):args+=['J:/gearsVr/Gears.of.War.Reloaded.v1.0.1.3816947-P2P/Gears of War Reloaded/Binaries_x64/PhysXCore.dll']
        result=subprocess.run(args,cwd=ROOT,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=flags)
        (out/(name+'-test.log')).write_bytes(result.stdout)
        record['test_exit']=result.returncode
    print(name,record.get('build_exit'),record.get('test_exit'),result.stdout.decode('utf8','replace')[-1800:])
    records.append(record)
(out/'results.json').write_text(json.dumps(records,indent=2)+'\n',encoding='utf8')
sys.exit(any(r['build_exit'] or r.get('test_exit',1) for r in records))
