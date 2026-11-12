from pathlib import Path
from decimal import Decimal, localcontext
import hashlib,json
h=Path(__file__).resolve().parent;r=h.parents[2]
packet=r/'analysis/cover-system-20261108/asset-scope-runtime-r242/runtime-r242/actual-asset-packet.json'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
build=json.loads((h/'build-result.json').read_text());assert build['packet_sha256']==sha(packet)
assert all(x['build_exit']==0 and x['test_exit']==0 for x in build['cases'].values())
rows=json.loads(packet.read_text())['shapes'];lines=(h/'packet-test.log').read_text().splitlines();assert len(rows)==len(lines)==36
D=lambda x:Decimal.from_float(float(x))
certificates=[]
with localcontext() as ctx:
    ctx.prec=160
    for row,line in zip(rows,lines):
        fields=line.split();assert int(fields[0])==row['index'];lo=list(map(D,fields[1:4]));hi=list(map(D,fields[4:7]))
        m=list(map(D,row['world']));sizes=list(map(D,row['sizes']));kind=row['kind'];oracle=[]
        for axis in range(3):
            if kind==0:
                values=[m[9+axis]+sum(m[3*axis+k]*sizes[k]*s for k,s in enumerate((x,y,z))) for x in (-1,1) for y in (-1,1) for z in (-1,1)]
            elif kind in (1,5):
                values=[m[9+axis]+sizes[3]*side+(m[3*axis+1]*sizes[4]*end if kind==1 else 0) for side in (-1,1) for end in (-1,1)]
            else:raise AssertionError(kind)
            lower=min(values);upper=max(values);assert lo[axis]<=lower<=upper<=hi[axis]
            # Minimal mathematical AABB, independently enumerated in Decimal.
            oracle.append({'minimum':str(lower),'maximum':str(upper),'lower_slack':str(lower-lo[axis]),'upper_slack':str(hi[axis]-upper)})
        certificates.append({'row':row['index'],'kind':kind,'lower':fields[1:4],'upper':fields[4:7],'decimal160':oracle})
result={'verified':True,'rows':36,'packet_sha256':sha(packet),'replay_output_sha256':sha(h/'packet-test.log'),'precision':160,'certificates':certificates,
 'scope':'Enclosure of mathematical original observed box corners / capsule segment plus unscaled radius / sphere radius. Not a PhysX narrow-phase numerical certificate, authored assignment, full-self set, scene-query fence or movement authority.'}
(h/'packet-certificates.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='certificates'}))
