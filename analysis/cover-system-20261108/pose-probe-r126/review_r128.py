"""Recover an ordinal gallery only; disk-full destroyed the timing manifest."""
from pathlib import Path
import hashlib,json
from PIL import Image,ImageDraw
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r128'
root=R/'analysis/instances/cover-1110-r128-cloak-registry';record=root/'cover-output/projection'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert (record/'recording.json').stat().st_size==0
files=sorted(record.glob('frame-*.jpg'));rows=[];pages=[]
for begin in range(0,len(files),20):
    sheet=Image.new('RGB',(1280,1020),(25,25,25));draw=ImageDraw.Draw(sheet)
    for k,p in enumerate(files[begin:begin+20]):
        x=k%4*320;y=k//4*204;row=dict(file=str(p),sha256=sha(p),bytes=p.stat().st_size)
        try:
            with Image.open(p) as im:tile=im.convert('RGB').resize((320,180))
            sheet.paste(tile,(x,y));row['decoded']=True
        except Exception as e:
            row.update(decoded=False,error=repr(e));draw.text((x+4,y+60),'CORRUPT ORIGINAL FILE',fill='red')
        rows.append(row);draw.text((x+4,y+181),p.name+' ordinal only',fill='white')
    p=out/('overview-'+str(begin//20+1)+'.jpg');sheet.save(p,quality=92);pages.append(dict(file=str(p),sha256=sha(p)))
result=dict(files=len(rows),decoded=sum(r['decoded'] for r in rows),rows=rows,pages=pages,contacts=[],
    timing_manifest_lost=True,video_encoded=False,frame_times_reconstructed=False,
    source='actual private projection files; ordinal gallery, no reliable timing',
    full_span_personally_reviewed=False,headset_accepted=False)
(out/'media-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(dict(files=len(rows),decoded=result['decoded'],pages=len(pages))))
