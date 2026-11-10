from pathlib import Path
import json, os, subprocess, threading, time, traceback
from PIL import Image

class ProjectionRecording:
    """Record the private simulator's actual stereo projection, without desktop capture."""
    def __init__(self, folder, output, identity_check, layer='projection', capture_enabled=True):
        assert layer in ('projection','all')
        self.layer=layer
        self.capture_enabled=capture_enabled
        self.folder, self.output, self.identity_check = Path(folder), Path(output), identity_check
        self.output.mkdir(exist_ok=False)
        self.stop_event = threading.Event()
        self.frames, self.events, self.errors = [], [], []
        self.capture_stage = {}; self.capture_stages = []
        self.thread = threading.Thread(target=self.capture)
        self.started = time.perf_counter()
        self.started_wall_ns = time.time_ns()
    def event(self, label, **data):
        self.events.append(dict(time=time.perf_counter()-self.started,wall_ns=time.time_ns(),label=label,**data))
    def start(self):
        if self.capture_enabled:self.thread.start()
        else:self.event('independent_WGC_recording',synchronous_screenshot_requests=False)
    def stage(self, label):
        self.capture_stage = dict(label=label,wall_ns=time.time_ns(),perf=time.perf_counter(),frames=len(self.frames))
        self.capture_stages.append(self.capture_stage)
        if len(self.capture_stages)>64:del self.capture_stages[:-64]
    def capture(self):
        try:
            while not self.stop_event.is_set():
                self.stage('identity')
                assert self.identity_check(), 'Owned game identity changed'
                request=self.folder/'screenshot_request.json'
                assert not request.exists(), 'Another capture owns screenshot request'
                stamp=time.time_ns(); requested=time.perf_counter()
                tmp=request.with_suffix('.record-tmp')
                self.stage('request')
                tmp.write_text(json.dumps({'eye':'both','layer':self.layer}))
                os.replace(tmp,request)
                deadline=time.perf_counter()+5
                self.stage('await_file')
                while True:
                    if self.stop_event.is_set():return
                    source=self.folder/'screenshot.bmp'
                    if source.exists() and source.stat().st_mtime_ns>=stamp and not request.exists():
                        try:
                            self.stage('image_open')
                            with Image.open(source) as im:
                                self.stage('image_load'); im.load()
                                self.stage('image_convert'); im=im.convert('RGB')
                            break
                        except (OSError,ValueError): pass
                    if time.perf_counter()>deadline: raise RuntimeError('No complete fresh projection frame')
                    time.sleep(.005)
                observed=time.perf_counter()
                name=f'frame-{len(self.frames):05d}.jpg'
                self.stage('image_save')
                im.save(self.output/name,quality=94,subsampling=0)
                self.stage('append_frame')
                self.frames.append(dict(file=name,time=observed-self.started,requested=requested-self.started,
                                        observed_wall_ns=time.time_ns(),requested_wall_ns=stamp,
                                        width=im.width,height=im.height))
                self.stop_event.wait(max(0,.10-(time.perf_counter()-requested)))
        except BaseException:
            self.errors.append(traceback.format_exc())
    def stop(self):
        self.stop_event.set()
        if self.capture_enabled:
            self.thread.join(timeout=7)
            assert not self.thread.is_alive(),'Capture thread did not stop'
        manifest=dict(source=('actual private simulator stereo '+('projection' if self.layer=='projection' else 'composed frame')) if self.capture_enabled else 'Event timeline for independent game/simulator HWND recordings',layer=self.layer,desktop_capture=False,audio=False,
                      synchronous_projection_capture=self.capture_enabled,
                      physical_headset_acceptance=False,started_wall_ns=self.started_wall_ns,
                      clock='Python perf_counter; wall_ns brackets for independent window recording alignment',
                      frames=self.frames,events=self.events,errors=self.errors)
        (self.output/'recording.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
        assert not self.errors,self.errors
        if not self.capture_enabled:return
        assert len(self.frames)>1,'No video frames'
        lines=['ffconcat version 1.0']
        for i,frame in enumerate(self.frames):
            duration=self.frames[i+1]['time']-frame['time'] if i+1<len(self.frames) else .1
            lines += [f"file '{frame['file']}'",f'duration {duration:.6f}']
        lines.append(f"file '{self.frames[-1]['file']}'")
        (self.output/'frames.txt').write_text('\n'.join(lines)+'\n')

def encode(path):
    path=Path(path)
    with (path/'encode.log').open('w') as log:
        subprocess.run(['D:/ffmpeg/bin/ffmpeg.exe','-nostdin','-hide_banner','-y','-f','concat','-safe','0',
                        '-i',str(path/'frames.txt'),'-fps_mode','vfr','-c:v','libx264','-preset','fast',
                        '-crf','18','-pix_fmt','yuv420p','-movflags','+faststart',str(path/'stereo-demo.mp4')],
                       stdout=log,stderr=subprocess.STDOUT,check=True,creationflags=subprocess.CREATE_NO_WINDOW)

if __name__=='__main__':
    import sys
    encode(sys.argv[1])
