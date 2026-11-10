"""Actual local recorder with synthetic BMP producer; never game/XR input."""
from pathlib import Path
import ast,hashlib,json,os,tempfile,threading,time
from PIL import Image
from cover_projection_recording import ProjectionRecording
H=Path(__file__).resolve().parent
tree=ast.parse((H/'run.py').read_text());node=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='original_stereo_submitted')
ns={};exec(compile(ast.fix_missing_locations(ast.Module(body=[node],type_ignores=[])),'actual-stereo-ready','exec'),ns)
ready=ns['original_stereo_submitted'];checks=0
for timing,expected in [(None,False),({},False),({'timing_is_runner_observation':True,'xr_monotonic_ns':1},False),
    ({'timing_is_runner_observation':False,'xr_monotonic_ns':0},False),
    ({'timing_is_runner_observation':False,'xr_monotonic_ns':True},False),
    ({'timing_is_runner_observation':False,'xr_monotonic_ns':123},True)]:
    assert ready({'startup_timings':{'first_stereo_submission':timing}})==expected;checks+=1
with tempfile.TemporaryDirectory(prefix='projection-check-',dir=H) as temp:
    root=Path(temp);folder=root/'sim';folder.mkdir();done=threading.Event();errors=[]
    def produce():
        try:
            while not done.is_set():
                request=folder/'screenshot_request.json'
                if request.exists():
                    assert json.loads(request.read_text())==dict(eye='both',layer='projection')
                    im=Image.new('RGB',(64,32),(21,42,63));tmp=folder/'fresh.bmp'
                    im.save(tmp,format='BMP');os.replace(tmp,folder/'screenshot.bmp');request.unlink()
                done.wait(.005)
        except BaseException as e:errors.append(repr(e))
    producer=threading.Thread(target=produce);producer.start()
    rec=ProjectionRecording(folder,root/'frames',lambda:True)
    try:
        rec.start();deadline=time.monotonic()+3
        while len(rec.frames)<3 and not rec.errors and time.monotonic()<deadline:time.sleep(.01)
        rec.stop();assert len(rec.frames)>=3 and not rec.errors and not rec.thread.is_alive();checks+=1
        stages={s['label'] for s in rec.capture_stages}
        assert {'identity','request','await_file','image_open','image_load','image_convert','image_save','append_frame'}<=stages;checks+=1
        manifest=json.loads((rec.output/'recording.json').read_text())
        assert not manifest['desktop_capture'] and not manifest['physical_headset_acceptance'];checks+=1
        assert all(f['width']==64 and f['height']==32 for f in manifest['frames']);checks+=1
    finally:
        done.set();producer.join(2);assert not producer.is_alive() and not errors
    empty=root/'empty-sim';empty.mkdir();waiting=ProjectionRecording(empty,root/'waiting',lambda:True);waiting.start()
    deadline=time.monotonic()+1
    while waiting.capture_stage.get('label')!='await_file' and time.monotonic()<deadline:time.sleep(.005)
    assert waiting.capture_stage['label']=='await_file';checks+=1
    try:waiting.stop();raise AssertionError('zero frames incorrectly passed')
    except AssertionError as e:assert str(e)=='No video frames',str(e)
    assert not waiting.thread.is_alive() and not waiting.errors;checks+=1
record=dict(checks=checks,passed=True,scope='actual recorder and readiness function; synthetic producer only',game_started=False,
    recorder_sha256=hashlib.sha256((H/'cover_projection_recording.py').read_bytes()).hexdigest(),
    original_recorder_sha256=hashlib.sha256((H.parents[2]/'analysis/solo-m1-20261028/record_projection.py').read_bytes()).hexdigest(),
    runner_sha256=hashlib.sha256((H/'run.py').read_bytes()).hexdigest())
(H/'projection-check.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
