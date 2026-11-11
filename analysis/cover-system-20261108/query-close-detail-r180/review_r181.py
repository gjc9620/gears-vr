"""Review only the actually captured span, retaining the recording failure."""
from pathlib import Path
HERE=Path(__file__).resolve().parent
source=(HERE.parent/'guard-witness-r137/review_r138.py').read_text(encoding='utf-8')
assert source.count('runtime-r138')==1
source=source.replace('runtime-r138','runtime-r181')
assert source.count('cover-1111-r138-cloak-guard')==1
source=source.replace('cover-1111-r138-cloak-guard','cover-1111-r181-cloak-close')
assert source.count("assert len(frames)>0 and not manifest['errors']")==1
source=source.replace("assert len(frames)>0 and not manifest['errors']",'assert len(frames)>0')
assert source.count('contacts=[]')==1
source=source.replace('contacts=[]','contacts=[];missing=[]')
assert source.count("    assert eligible,(phase['label'],duration)")==1
source=source.replace("    assert eligible,(phase['label'],duration)","    if not eligible:\n        missing.append(dict(label=phase['label'],duration_seconds=duration,reason='No complete captured frame inside this actual event interval'));continue")
assert source.count('full_span_personally_reviewed=False,headset_accepted=False)')==1
source=source.replace('full_span_personally_reviewed=False,headset_accepted=False)',"full_span_personally_reviewed=False,headset_accepted=False,recording_errors=manifest['errors'],captured_span_complete=not manifest['errors'] and not missing,missing_contacts=missing)")
assert source.count("str(record/'frames.txt')")==1
source=source.replace("str(record/'frames.txt')",'str(concat)')
assert source.count("str(record/'stereo-demo.mp4')")==1
source=source.replace("str(record/'stereo-demo.mp4')","str(out/'captured-span.mp4')")
assert source.count("sha(record/'stereo-demo.mp4')")==1
source=source.replace("sha(record/'stereo-demo.mp4')","sha(out/'captured-span.mp4')")
assert source.count('contacts=contacts,encode_exit=encode.returncode)))')==1
source=source.replace('contacts=contacts,encode_exit=encode.returncode)))','contacts=len(contacts),encode_exit=encode.returncode)))')
prefix="""
concat=out/'captured-span.ffconcat'
lines=['ffconcat version 1.0']
for i,frame in enumerate(frames):
    filename=(record/frame['file']).as_posix();assert "'" not in filename
    duration=frames[i+1]['time']-frame['time'] if i+1<len(frames) else .1
    lines += [f"file '{filename}'",f'duration {duration:.6f}']
lines.append(f"file '{(record/frames[-1]['file']).as_posix()}'")
concat.write_text('\\n'.join(lines)+'\\n')
"""
assert source.count("cmd=['D:/ffmpeg/bin/ffmpeg.exe'")==1
source=source.replace("cmd=['D:/ffmpeg/bin/ffmpeg.exe'",prefix+"cmd=['D:/ffmpeg/bin/ffmpeg.exe'")
source=source.replace("captured_span_complete=not manifest['errors'] and not missing", "captured_span_complete=not manifest['errors'] and not missing and len(phases)==5")
exec(compile(source,str(__file__),'exec'),globals())



