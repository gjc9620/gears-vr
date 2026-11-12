"""Read the existing app bridge in the owned Source1 session; never calibrates."""
from pathlib import Path
import ctypes as C,json,sys,subprocess,time
ROOT=Path('J:\\codexWorktree\\gearsVr');OUT=Path(__file__).resolve().parent
for folder in ('vr-launcher','native-capture'):sys.path.insert(0,str(ROOT/'tools'/folder))
import armspan_wire as w
from run_stereo_continuous import WindowsControl
control=WindowsControl()
OUT=Path('J:\\codexWorktree\\gearsVr\\analysis\\instances\\cover-1112-r272-cloak-body\\cover-output')
state=json.loads(Path('J:\\codexWorktree\\gearsVr\\analysis\\instances\\cover-1112-r272-cloak-body\\launcher-session.json').read_text())
assert Path(state['instance_root']).resolve()==Path('J:\\codexWorktree\\gearsVr\\analysis\\instances\\cover-1112-r272-cloak-body').resolve()
session=json.loads(Path(state['session_report']).read_text())
assert state['phase'] in ('session_owned','waiting_for_level') and session['phase']=='running', ('indexed/session phase',state.get('phase'),session.get('phase'))
assert state['session_id']==session['session_id']
assert state['owner_identity']==session['owner_identity']
assert state['game_identity']==session['game_identity']
assert state.get('vr_gamepad',{}).get('source')==1
game=state['game_identity'];publisher=session['xr_identity']
manifest=json.loads(Path('J:\\codexWorktree\\gearsVr\\analysis\\instances\\cover-1112-r272-cloak-body\\manifest.json').read_text())
expected=(ROOT/manifest['xr_build']/'OpenXRPoseProbe.exe').resolve()
assert Path(publisher['image']).resolve()==expected
for identity in (game,publisher,state['owner_identity']):
    assert control.identity(identity['pid'])==identity
reader=w.Reader(game,publisher,control.identity)
try:
    pid,start=game['pid'],game['creation_time']
    reader.stereo=w.Mapping(reader.api,f'Local\\GearsVR.Stereo.{pid}',f'Local\\GearsVR.StereoLock.{pid}',C.sizeof(w.StereoPacket))
    reader.input=w.Mapping(reader.api,f'Local\\GearsVR.ControllerInput.v4.{pid}.{start}',f'Local\\GearsVR.ControllerInputLock.v4.{pid}.{start}',C.sizeof(w.InputPacket))
    deadline=time.monotonic()+5
    while time.monotonic()<deadline:
        assert control.identity(pid)==game and control.identity(publisher['pid'])==publisher
        a,b=reader.stereo.read(),reader.input.read()
        if a is None or b is None:continue
        stereo,packet=w.StereoPacket.from_buffer_copy(a),w.InputPacket.from_buffer_copy(b)
        assert (stereo.magic,stereo.version,stereo.size,stereo.pid,stereo.frequency)==(0x47565331,4,C.sizeof(w.StereoPacket),pid,reader.frequency)
        assert (packet.magic,packet.version,packet.size,packet.pid,packet.game_start,packet.publisher_pid,packet.publisher_start,packet.frequency)==(0x47564334,4,C.sizeof(w.InputPacket),pid,start,publisher['pid'],publisher['creation_time'],reader.frequency)
        v=stereo.sample
        if not v.sequence or packet.latest_sequence-v.sequence not in range(8):continue
        f=packet.frames[v.sequence%8];now=reader.now()
        if (f.source,f.session,f.sequence,f.space_epoch,f.predicted_time)!=(1,v.session,v.sequence,v.space_epoch,v.predicted_time) or v.source!=1:continue
        if not (0<now-f.received_ns<100_000_000 and 0<now-v.received_ns<100_000_000):continue
        def pose(p):return {'xyz':w.xyz(p.position),'quat':[p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w]}
        result={'source':1,'simulation_not_physical_tracking':True,'game':game,'publisher':publisher,'sequence':f.sequence,'head':pose(v.head),
                'head_flags':v.head_flags,'view_flags':v.view_flags,
                'display_mode_hint':f.display_mode,'display_observed_ns':f.display_observed_ns,
                'input_sync_ns':f.sync_ns,'input_received_ns':f.received_ns,
                'eyes':[{'pose':pose(e.pose),'fov':{k:getattr(e.fov,k) for k in ['left','right','up','down']}} for e in v.eyes],
                'hands':[{'grip':pose(h.grip),'flags':h.grip_flags,'active':h.grip_active,'squeeze':h.squeeze,'trigger':h.trigger,'stick':[h.stick_x,h.stick_y],'stick_active':h.stick_active} for h in f.hands]}
        target=OUT/(sys.argv[1] if len(sys.argv)>1 else 'private-input.json');target.write_text(json.dumps(result,indent=2));print(json.dumps(result));break
    else:raise RuntimeError('No fresh exact retained Source1 ticket')
finally:reader.close()
