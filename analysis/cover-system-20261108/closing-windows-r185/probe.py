import hashlib,json,struct
from cover_state import CoverQuery,slot_snapshot
from test_cover_state import MemoryFixture
rows=[]
for count in (3,16,32):
 f=MemoryFixture();template=f.read(f.DATA,48)
 for n in range(count):f.put(f.DATA+n*48,template)
 f.put(f.value_address(f.LINK,'Slots'),struct.pack('<Qii',f.DATA,count,count))
 q=CoverQuery(f.observer);link=q.obj(f.LINK);decoded=slot_snapshot(q,link,list(range(count)))
 before=sum(f.read_counts.values());witnesses=len(q.values);q.finish();closing=sum(f.read_counts.values())-before
 rows.append(dict(slots=count,capture_reads=before,closing_reads=closing,witnesses=witnesses,rows_sha256=hashlib.sha256(json.dumps(decoded['rows'],sort_keys=True).encode()).hexdigest()))
print(json.dumps(rows))
