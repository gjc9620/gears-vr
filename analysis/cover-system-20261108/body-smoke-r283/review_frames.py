from pathlib import Path
from PIL import Image,ImageDraw
import json,hashlib
r=Path(__file__).resolve().parents[3];o=Path(__file__).resolve().parent
b=r/'analysis/instances/cover-1112-r283-pending-observation/cover-output/projection'
pages=o/'visual-review';pages.mkdir(exist_ok=False)
v=json.loads((b/'recording.json').read_text());frames=v['frames'];index=[]
for start in range(0,len(frames),48):
 canvas=Image.new('RGB',(1440,1240),(28,28,28));draw=ImageDraw.Draw(canvas)
 for n,item in enumerate(frames[start:start+48]):
  p=b/item['file'];im=Image.open(p).convert('RGB');im.thumbnail((240,135));x=n%6*240;y=n//6*155
  canvas.paste(im,(x,y+20));draw.text((x+3,y+3),f"{start+n}  {item['time']:.2f}s",fill='white')
  index.append({**item,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'source':str(p)})
 canvas.save(pages/f'overview-{start//48+1}.jpg',quality=90)
(o/'visual-frame-index.json').write_text(json.dumps(dict(frames=index,source='actual submitted projection; SBS; no desktop capture',pages=(len(frames)+47)//48),indent=2)+'\n')
print(json.dumps(dict(frames=len(frames),pages=(len(frames)+47)//48)))
