from pathlib import Path
import json,hashlib,subprocess
from PIL import Image,ImageDraw
H=Path(__file__).resolve().parent;R=H.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
records=[]
for revision in ('r119','r120'):
    root=R/('analysis/instances/cover-1110-'+revision+'-cloak-source-stage/cover-output/projection')
    manifest=json.loads((root/'recording.json').read_text());frames=manifest['frames']
    assert not manifest['errors'] and len(frames)>1
    out=H/('review-'+revision);out.mkdir(exist_ok=True);pages=[]
    for begin in range(0,len(frames),20):
        sheet=Image.new('RGB',(1280,1020),(25,25,25));draw=ImageDraw.Draw(sheet)
        for k,f in enumerate(frames[begin:begin+20]):
            with Image.open(root/f['file']) as im:tile=im.convert('RGB').resize((320,180))
            x=(k%4)*320;y=(k//4)*204;sheet.paste(tile,(x,y));draw.text((x+4,y+181),str(begin+k)+' '+str(round(f['time'],3))+'s',fill='white')
        name='overview-'+str(begin//20+1)+'.jpg';sheet.save(out/name,quality=92);pages.append(dict(file=str(out/name),sha256=sha(out/name)))
    command=['D:/ffmpeg/bin/ffmpeg.exe','-nostdin','-hide_banner','-y','-f','concat','-safe','0','-i',str(root/'frames.txt'),
        '-fps_mode','vfr','-c:v','libx264','-threads','2','-preset','fast','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(root/'stereo-demo.mp4')]
    assert not (root/'stereo-demo.mp4').exists(),'Do not overwrite an existing encode'
    with (root/'encode.log').open('xb') as log:
        status=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)
    record=dict(revision=revision,frames=len(frames),recording_sha256=sha(root/'recording.json'),pages=pages,
        frames_sha256={f['file']:sha(root/f['file']) for f in frames},encode_exit=status.returncode,
        encoded_sha256=sha(root/'stereo-demo.mp4') if status.returncode==0 else None,
        full_span_personally_reviewed=False,grip_fit_accepted=False,cover_gameplay_accepted=False)
    (out/'media.json').write_text(json.dumps(record,indent=2)+'\n');records.append(record)
    assert status.returncode==0,status.returncode
print(json.dumps([dict(revision=r['revision'],frames=r['frames'],encode_exit=r['encode_exit']) for r in records]))
