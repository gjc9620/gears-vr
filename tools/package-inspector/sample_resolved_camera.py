"""Resolve afresh and sample camera + LastViewLocation, with focus telemetry."""
import argparse
import csv
import ctypes as c
from ctypes import wintypes as w
import json
import math
import pathlib
import struct
import time
from resolve_camera import Resolver


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--pid',type=int,required=True)
    parser.add_argument('--seconds',type=float,default=35)
    parser.add_argument('--wait-for-focus',type=float,default=120)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    user32=c.WinDLL('user32',use_last_error=True)
    user32.GetForegroundWindow.restype=w.HWND
    user32.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)]
    def focused():
        pid=w.DWORD()
        user32.GetWindowThreadProcessId(user32.GetForegroundWindow(),c.byref(pid))
        return pid.value==args.pid
    profile=json.loads(pathlib.Path(__file__).with_name('reloaded-profile.json').read_text())
    r=Resolver(args.pid,profile)
    try:
        resolved=r.resolve()
        cam=int(resolved['camera_address'],16)
        pc=int(resolved['controller_address'],16)
        local=int(resolved['local_player_address'],16)
        local_class=r.u64(local+profile['object_class'])
        last_offset=r.property(local_class,'LastViewLocation',True)['offset']
        def sample():
            if r.u64(pc+int(resolved['offsets']['PlayerCamera'],16))!=cam:
                raise RuntimeError('PlayerCamera changed; re-resolve before sampling')
            if r.u64(cam+int(resolved['offsets']['PCOwner'],16))!=pc:
                raise RuntimeError('Camera owner changed')
            stamp=struct.unpack('<f',r.read(int(resolved['timestamp_address'],16),4))[0]
            pose=struct.unpack('<3f3if',r.read(int(resolved['pov_address'],16),28))
            last=struct.unpack('<3f',r.read(local+last_offset,12))
            if not all(math.isfinite(v) for v in (stamp,*pose[:3],pose[-1],*last)):
                raise RuntimeError('Nonfinite camera/view data')
            return stamp,pose,last
        before=sample()[0]
        deadline=time.monotonic()+args.wait_for_focus
        print('READY: waiting for game focus AND advancing camera timestamp',flush=True)
        while True:
            current=sample()[0]
            if focused() and current!=before:
                break
            if time.monotonic()>=deadline:
                raise RuntimeError('No focused advancing gameplay observed before timeout')
            before=current
            time.sleep(.1)
        print('SAMPLING started',flush=True)
        rows=[]
        start=time.monotonic()
        while time.monotonic()-start<args.seconds:
            stamp,pose,last=sample()
            rows.append([time.monotonic()-start,int(focused()),stamp,*pose,*last])
            time.sleep(.02)
        args.output.parent.mkdir(parents=True,exist_ok=True)
        with args.output.open('w',newline='',encoding='utf-8') as stream:
            writer=csv.writer(stream)
            writer.writerow(['elapsed','game_focused','game_timestamp','x','y','z','pitch_ru','yaw_ru','roll_ru','fov','last_view_x','last_view_y','last_view_z'])
            writer.writerows(rows)
        active=[row for row in rows if row[1]]
        report=dict(pid=args.pid,seconds=args.seconds,samples=len(rows),focused_samples=len(active),
            focus_fraction=len(active)/len(rows),unique_focused_positions=len({tuple(row[3:6]) for row in active}),
            unique_focused_rotations=len({tuple(row[6:9]) for row in active}),
            ranges={key:[min(row[i] for row in active),max(row[i] for row in active)] for key,i in
                    [('timestamp',2),('x',3),('y',4),('z',5),('pitch',6),('yaw',7),('roll',8),('fov',9)]} if active else {},
            last_view_position_rms_error=math.sqrt(sum(sum((row[i]-row[i+7])**2 for i in range(3,6)) for row in active)/len(active)) if active else None,
            caveat='Read-only correlation test; asynchronous reads may straddle frames. Does not validate native function cadence or VR edits.')
        args.output.with_suffix('.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
        print(json.dumps(report),flush=True)
    finally:
        r.p.close()


if __name__=='__main__':main()
