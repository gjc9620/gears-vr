from pathlib import Path
from decimal import Decimal,localcontext
import json,re,hashlib,struct,shutil,collections
h=Path(__file__).resolve().parent;r=h.parents[2];out=h/'runtime-r245';out.mkdir(exist_ok=True)
root=r/'analysis/instances/cover-1112-r245-cloak-asset-bounds';run=json.loads((root/'cover-output/run-result.json').read_text());s=run['terminal_session']
assert run['normal_cleanup_verified'] and not run['remaining_owned']
for source,name in [(root/'cover-output/run-result.json','run-result-original.json'),(root/'isolated-done.json','isolated-done-original.json'),(root/'launcher-session.json','launcher-index-original.json'),(Path(s['session_report']).with_suffix('.session-status.txt'),'native-session-status-original.txt')]:shutil.copyfile(source,out/name)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();raw=out/'native-session-status-original.txt';lines=raw.read_text().splitlines();fields=lambda line:dict(re.findall(r'(\w+)=([^\s]+)',line))
header=fields(next(x for x in lines if x.startswith('cover_shape_snapshot_requested=')))
derived=fields(next(x for x in lines if x.startswith('cover_shape_bounds_attempted=')))
report={k:run.get(k) for k in ['operator_exit','normal_cleanup_verified','remaining_owned','projection_frames','projection_errors','anchor_sources_unchanged','cover_reader_sources_unchanged','cloak_observations','foreground_observations']}
report.update(header=header,derived=derived,source_sha256=sha(raw),gameplay_accepted=False,headset_accepted=False)
if header['code']=='0' and derived['cover_shape_bounds_attempted']=='1' and derived['code']=='0':
    f32=lambda x:struct.unpack('<f',struct.pack('<f',float(x)))[0]
    raw_rows=[fields(x) for x in lines if x.startswith('cover_shape_snapshot_row=')];bounds=[fields(x) for x in lines if x.startswith('cover_shape_bounds_row=')]
    assert len(raw_rows)==len(bounds)==int(header['shapes'])
    rows=[]
    with localcontext() as ctx:
      ctx.prec=160;D=Decimal.from_float
      for row,bound in zip(raw_rows,bounds):
        key=list(map(int,row['cover_shape_snapshot_row'].split(',')));index,actor,member,kind,shape=key[:5]
        assert list(map(int,bound['cover_shape_bounds_row'].split(',')))==[index,actor,member,shape]
        m=list(map(f32,row['world'].split(',')));sizes=list(map(f32,row['sizes'].split(',')));lo=list(map(float,bound['lower'].split(',')));hi=list(map(float,bound['upper'].split(',')))
        md=list(map(D,m));sd=list(map(D,sizes));oracle=[]
        for axis in range(3):
          if kind==0:values=[md[9+axis]+sum(md[3*axis+k]*sd[k]*sign for k,sign in enumerate((x,y,z))) for x in (-1,1) for y in (-1,1) for z in (-1,1)]
          else:
            assert kind in (1,5)
            values=[md[9+axis]+sd[3]*side+(md[3*axis+1]*sd[4]*end if kind==1 else 0) for side in (-1,1) for end in (-1,1)]
          lower=min(values);upper=max(values);assert D(lo[axis])<=lower<=upper<=D(hi[axis]);oracle.append([str(lower),str(upper)])
        rows.append(dict(index=index,actor=actor,member=member,kind=kind,shape=shape,world=m,sizes=sizes,lower=lo,upper=hi,decimal160=oracle))
    assert header['original_started']==header['original_returned']=='0,0' and derived['derived_after_close']==derived['mathematical_only']=='1' and derived['query_permission']==derived['movement_permission']=='0'
    report.update(successful_read_and_bounds=True,counts=[int(header[k]) for k in ['bodies','actors','shapes']],kinds=dict(collections.Counter(str(x['kind']) for x in rows)),elapsed_ms=(int(header['end_ns'])-int(header['begin_ns']))/1e6,derivation_ms=(int(derived['end_ns'])-int(derived['begin_ns']))/1e6,independent_certificates=len(rows))
    (out/'actual-bounds-packet.json').write_text(json.dumps(dict(source_sha256=sha(raw),rows=rows),indent=2)+'\n')
else:report['successful_read_and_bounds']=False
(out/'closeout.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k not in ('header','derived','cloak_observations','foreground_observations')}))
