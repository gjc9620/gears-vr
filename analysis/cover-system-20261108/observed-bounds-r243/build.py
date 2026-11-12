from pathlib import Path
import json, subprocess, sys, hashlib
r=Path(__file__).resolve().parents[3]; h=Path(__file__).resolve().parent
sys.path.insert(0,str(r/'tools/native-capture'))
from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat')
flags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS
packet=r/'analysis/cover-system-20261108/asset-scope-runtime-r242/runtime-r242/actual-asset-packet.json'
data=json.loads(packet.read_text()); rows=data['shapes']
src=['#include "cover_physx_observed_bounds.h"','#include <cstdio>','namespace b=gears_cover_physx_observed_bounds;','int main(){ b::p::Pose p{}; b::Bounds o{};']
lit=lambda x:float(x).hex()+'f'
for row in rows:
    src+=['p.matrix.values['+str(i)+']='+lit(x)+';' for i,x in enumerate(row['world'])]
    src+=['p.gram_error='+float(row['gram_error'][1]).hex()+';',
          'if(b::primitive(p,static_cast<b::Kind>('+str(row['kind'])+'),{'+','.join(map(lit,row['sizes'][:3]))+'},'+lit(row['sizes'][3])+','+lit(row['sizes'][4])+',o)!=b::Code::bounded)return 1;',
          'std::printf("'+str(row['index'])+' %.17g %.17g %.17g %.17g %.17g %.17g\\n",o.lower[0],o.lower[1],o.lower[2],o.upper[0],o.upper[1],o.upper[2]);']
src+=['}'];(h/'packet-replay.cpp').write_text('\n'.join(src)+'\n')
result={'packet_sha256':hashlib.sha256(packet.read_bytes()).hexdigest(),'cases':{}}
for name,source in [('bounds',r/'tools/native-capture/cover_physx_observed_bounds_test.cpp'),('packet',h/'packet-replay.cpp')]:
    prefix=h/name
    cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT',str(source),'/I'+str(r/'tools/native-capture'),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
    run=subprocess.run(cmd,env=env,capture_output=True,creationflags=flags);Path(str(prefix)+'-build.log').write_bytes(run.stdout+run.stderr)
    item={'build_exit':run.returncode};result['cases'][name]=item
    if run.returncode:continue
    test=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=flags);Path(str(prefix)+'-test.log').write_bytes(test.stdout+test.stderr);item.update(test_exit=test.returncode)
    if name=='bounds':item['output']=test.stdout.decode()
    deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes']+[str(source)]
    item['source_sha256']={str(Path(p).relative_to(r)):hashlib.sha256(Path(p).read_bytes()).hexdigest() for p in deps if Path(p).is_relative_to(r)}
(h/'build-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:{a:b for a,b in v.items() if a!='source_sha256'} for k,v in result['cases'].items()}))
sys.exit(0 if all(v.get('test_exit')==0 for v in result['cases'].values()) else 1)
