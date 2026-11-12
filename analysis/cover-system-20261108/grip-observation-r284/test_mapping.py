import json,math
from pathlib import Path
from grip_mapping import calibration,raw_delta
checks=0
def check(value):
 global checks
 checks+=1;assert value,checks
for yaw in (0,.6,1.57079632679,-2.3):
 c,s=math.cos(yaw),math.sin(yaw)
 cols=((100*c,100*s,0),(0,0,100),(100*s,-100*c,0));base=(1394,-927,591)
 probes=[tuple(base[j]+.05*v[j] for j in range(3)) for v in cols];m=calibration(base,probes)
 for raw in ((.7,0,0),(-.3,.1,.5),(0,0,-.12)):
  world=tuple(sum(cols[i][j]*raw[i] for i in range(3)) for j in range(3));back=raw_delta(m,world)
  check(max(abs(a-b) for a,b in zip(raw,back))<1e-10)
for probes in (((0,0,0),)*3,((5,0,0),(5,0,0),(0,0,5)),((5,0,0),(0,7,0),(0,0,5)),((float('nan'),0,0),(0,5,0),(0,0,5))):
 try:calibration((0,0,0),probes)
 except ValueError:check(True)
 else:check(False)
result={'checks':checks,'failures':0,'game_access':False,'input_injection':False}
Path(__file__).with_name('mapping-test-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
