from pathlib import Path
import json,re,struct,decimal,hashlib
h=Path(__file__).resolve().parent/'runtime-r242';p=h/'actual-asset-packet.json';v=json.loads(p.read_text());raw=h/'native-session-status-original.txt';assert hashlib.sha256(raw.read_bytes()).hexdigest()==v['source_sha256']
f=lambda line:dict(re.findall(r'(\w+)=([^\s]+)',line));f32=lambda x:struct.unpack('<f',struct.pack('<f',float(x)))[0];lines=raw.read_text().splitlines()
header=f(next(x for x in lines if x.startswith('cover_shape_snapshot_requested=')));assert header==v['header'] and header['code']=='0' and header['session_stage']=='7' and header['units']=='physx' and header['movement_permission']=='0'
assert header['original_started']==header['original_returned']=='0,0'
raw_rows=[f(x) for x in lines if x.startswith('cover_shape_snapshot_row=')];raw_bodies=[f(x) for x in lines if x.startswith('cover_shape_snapshot_body_mapping=')]
assert len(raw_rows)==len(v['shapes'])==int(header['shapes']) and len(raw_bodies)==len(v['bodies'])==int(header['bodies'])
decimal.getcontext().prec=120;D=decimal.Decimal
for row,source in zip(v['shapes'],raw_rows):
 assert list(map(int,source['cover_shape_snapshot_row'].split(',')))==[row[x] for x in ['index','actor_index','member','kind','shape','actor','internal_shape','internal_actor']]
 for a,b in [('world','world'),('local_p','stored_p'),('local_q','stored_q'),('sizes','sizes')]:assert list(map(f32,source[a].split(',')))==row[b]
 assert list(map(float,source['observed_gram_error'].split(',')))==row['gram_error'] and int(source['flags'])==row['flags']
 m=list(map(D.from_float,row['world'][:9]));g=[sum(m[3*k+i]*m[3*k+j] for k in range(3))-(D(1) if i==j else D(0)) for i in range(3) for j in range(3)]
 norm=sum(x*x for x in g).sqrt();assert D.from_float(row['gram_error'][1])>=norm
 det=m[0]*(m[4]*m[8]-m[5]*m[7])-m[1]*(m[3]*m[8]-m[5]*m[6])+m[2]*(m[3]*m[7]-m[4]*m[6]);assert det>0 and row['gram_error'][1]<1
for body,source in zip(v['bodies'],raw_bodies):assert list(map(int,source['cover_shape_snapshot_body_mapping'].split(',')))==[body[x] for x in ['index','actor_index','presence','body','actor','table','scene','internal']]
actors={b['actor_index']:b for b in v['bodies'] if b['presence']==2};assert set(actors)==set(range(int(header['actors'])))
for row in v['shapes']:assert row['actor']==actors[row['actor_index']]['actor'] and row['internal_actor']==actors[row['actor_index']]['internal']
assert len({r['shape'] for r in v['shapes']})==len(v['shapes'])==len({r['internal_shape'] for r in v['shapes']})
for a in actors:
 members=[r['member'] for r in v['shapes'] if r['actor_index']==a];assert members==list(range(len(members)))
report=dict(verified=True,source_sha256=v['source_sha256'],packet_sha256=hashlib.sha256(p.read_bytes()).hexdigest(),bodies=len(v['bodies']),actors=len(actors),shapes=len(v['shapes']),independent_decimal_gram_certificates=len(v['shapes']),decoder='Every exported float round-trips binary32; matrix Gram error independently checked with Decimal120',scope='historical closed supplied asset only; no image alignment, whole self/world fence or live permission',game_started=False)
(h/'packet-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
