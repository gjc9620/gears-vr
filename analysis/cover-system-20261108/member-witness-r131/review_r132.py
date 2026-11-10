from pathlib import Path
import json,hashlib,ctypes,subprocess
from PIL import Image,ImageDraw
H=Path(__file__).resolve().parent;R=H.parents[2];out=H/'runtime-r132';out.mkdir(exist_ok=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
root=R/'analysis/instances/cover-1110-r132-cloak-member-witness';record=root/'cover-output/projection'
manifest=json.loads((record/'recording.json').read_text());frames=manifest['frames'];assert len(frames)>0 and not manifest['errors']
actions=list((root/'cover-output').glob('menu-firstperson-*/cover-boundary.json'));assert len(actions)==1;action=actions[0].parent
events=json.loads((action/'recording.json').read_text())['events'];phases=json.loads((action/'cover-boundary.json').read_text()).get('eye_observation',[])
frequency=ctypes.c_longlong();assert ctypes.WinDLL('kernel32').QueryPerformanceFrequency(ctypes.byref(frequency))
pages=[]
for begin in range(0,len(frames),20):
    sheet=Image.new('RGB',(1280,1020),(25,25,25));draw=ImageDraw.Draw(sheet)
    for k,f in enumerate(frames[begin:begin+20]):
        with Image.open(record/f['file']) as im:tile=im.convert('RGB').resize((320,180))
        x=k%4*320;y=k//4*204;sheet.paste(tile,(x,y));draw.text((x+4,y+181),str(begin+k)+' '+str(round(f['time'],3))+'s',fill='white')
    p=out/('overview-'+str(begin//20+1)+'.jpg');sheet.save(p,quality=92);pages.append(dict(file=str(p),sha256=sha(p)))
contacts=[]
for phase in phases:
    event=next(e for e in events if e['label']==phase['label']);duration=(phase['end_qpc']-phase['begin_qpc'])/frequency.value
    eligible=[f for f in frames if f['requested_wall_ns']>=event['wall_ns'] and f['observed_wall_ns']<=event['wall_ns']+int(duration*1e9)]
    # Bounded event interval only, not a nearest-frame or same-C0/GPU proof.
    assert eligible,(phase['label'],duration)
    f=eligible[len(eligible)//2];contacts.append(dict(label=phase['label'],file=str(record/f['file']),sha256=sha(record/f['file']),
        event_wall_ns=event['wall_ns'],duration_seconds=duration,eligible_frames=len(eligible),frame=f,same_C0_proven=False))
result=dict(frames=len(frames),recording_sha256=sha(record/'recording.json'),pages=pages,contacts=contacts,
    source='actual private simulator projection',desktop_capture=False,full_span_personally_reviewed=False,headset_accepted=False)
(out/'media-review.json').write_text(json.dumps(result,indent=2)+'\n')
assert not (record/'stereo-demo.mp4').exists()
cmd=['D:/ffmpeg/bin/ffmpeg.exe','-nostdin','-hide_banner','-y','-f','concat','-safe','0','-i',str(record/'frames.txt'),'-fps_mode','vfr',
    '-c:v','libx264','-threads','2','-preset','fast','-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(record/'stereo-demo.mp4')]
with (record/'encode.log').open('xb') as log:
    encode=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)
result['encode_exit']=encode.returncode;result['encoded_sha256']=sha(record/'stereo-demo.mp4') if not encode.returncode else None
(out/'media-review.json').write_text(json.dumps(result,indent=2)+'\n');assert encode.returncode==0
print(json.dumps(dict(frames=len(frames),pages=len(pages),contacts=contacts,encode_exit=encode.returncode)))
