"""Bounded stdio MCP discovery against the installed Operator proxy.

Discovery launches only our standalone render probe and never calls pose tools.
"""
import datetime
import json
import math
import os
import pathlib
import queue
import socket
import subprocess
import threading
import time

ROOT = pathlib.Path(__file__).resolve().parents[2]
LAYER = ROOT / 'tools/third-party/metavr/state/tools/meta-xr-operator/meta-xr-operator-standalone-public/windows'


class StdioMCP:
    def __init__(self, command, stderr, transcript, deadline):
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=stderr, text=True, encoding='utf-8')
        self.incoming = queue.Queue()
        self.transcript = transcript
        self.deadline = deadline
        self.serial = 0
        def read():
            try:
                for line in self.process.stdout:
                    self.incoming.put(line)
            finally:
                self.incoming.put(None)
        threading.Thread(target=read, daemon=True).start()

    def rpc(self, method, params, notification=False, timeout=None):
        call_deadline = min(self.deadline, time.monotonic()+timeout) if timeout else self.deadline
        request = {'jsonrpc': '2.0', 'method': method, 'params': params}
        if not notification:
            self.serial += 1
            request['id'] = self.serial
        self.transcript.write(json.dumps({'direction': 'send', 'message': request})+'\n')
        self.transcript.flush()
        self.process.stdin.write(json.dumps(request)+'\n')
        self.process.stdin.flush()
        if notification:
            return None
        while True:
            remaining = call_deadline-time.monotonic()
            if remaining <= 0:
                raise TimeoutError('MCP total deadline expired')
            try:
                raw = self.incoming.get(timeout=remaining)
            except queue.Empty as exc:
                raise TimeoutError('MCP response deadline expired') from exc
            if raw is None:
                raise RuntimeError('MCP proxy closed stdout')
            self.transcript.write(json.dumps({'direction':'receive','raw':raw.rstrip()})+'\n')
            self.transcript.flush()
            reply = json.loads(raw)
            if reply.get('id') != request['id']:
                continue
            if 'error' in reply:
                raise RuntimeError(reply['error'])
            return reply['result']

    def close(self):
        if self.process.stdin:
            try:
                self.process.stdin.close()
            except (BrokenPipeError, OSError):
                pass
        try:
            self.process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=2)


def assert_operator_port_free():
    """Call before launching the owned XR child; never contacts an MCP tool."""
    with socket.socket() as sock:
        sock.settimeout(.25)
        if sock.connect_ex(('127.0.0.1',8720)) == 0:
            raise RuntimeError('Port 8720 is already occupied; refusing operator motion')


def operator_child_env(existing_env):
    """Return a child-only copy. No registry or process-global changes."""
    result = dict(existing_env)
    for name in ('XrApiLayer_METAX_operator.json', 'XrApiLayer_METAX_operator.dll',
                 'meta-xr-operator-mcp-proxy.exe'):
        if not (LAYER/name).is_file():
            raise FileNotFoundError(LAYER/name)
    result['XR_API_LAYER_PATH'] = str(LAYER)
    result['XR_ENABLE_API_LAYERS'] = 'XR_APILAYER_METAX_operator'
    return result


def validate_pose(pose):
    """Validate the documented pose contract, never infer missing components."""
    if not isinstance(pose, dict):
        raise ValueError('Head pose is not a position/orientation object')
    answer = {}
    for key, count in (('position',3), ('orientation',4)):
        values = pose.get(key)
        if not isinstance(values, list) or len(values) != count:
            raise ValueError('Head pose lacks documented '+key+' array')
        if any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) for v in values):
            raise ValueError('Head pose has non-finite/non-numeric '+key)
        answer[key] = list(values)
    norm = math.sqrt(sum(v*v for v in answer['orientation']))
    if abs(norm-1)>0.01:
        raise ValueError('Head orientation is not a unit quaternion')
    return answer


def documented_pose_result(result):
    """Accept only the advertised object in standard MCP envelopes.

    The standalone validation observed {pose: {position, orientation}, flags}
    in content[0].text. Require all four tracking/validity flags on that shape.
    Unknown wrappers remain rejected; a runner can supply an evidence adapter.
    """
    candidates=[]
    if isinstance(result.get('structuredContent'),dict):
        candidates.append(result['structuredContent'])
    for block in result.get('content',[]):
        if block.get('type') == 'text':
            try:
                candidates.append(json.loads(block['text']))
            except (ValueError, KeyError):
                continue
    poses=[]
    for candidate in candidates:
        if isinstance(candidate,dict) and 'pose' in candidate:
            flags=candidate.get('flags',{})
            if any(flags.get(k) is not True for k in ('position_valid','orientation_valid',
                                                      'position_tracked','orientation_tracked')):
                raise ValueError('Observed head-pose wrapper does not report fully valid/tracked pose')
            poses.append(validate_pose(candidate['pose']))
            continue
        if isinstance(candidate,dict) and 'position' in candidate and 'orientation' in candidate:
            raise ValueError('Unflagged head-pose object cannot establish fully valid/tracked pose')
    if not poses or any(p!=poses[0] for p in poses[1:]):
        raise ValueError('Unobserved/ambiguous get_head_pose result shape; raw result retained for adaptation')
    return poses[0]


def quaternion_multiply(a,b):
    """Hamilton product, OpenXR xyzw order."""
    x,y,z,w=a;u,v,t,s=b
    return [w*u+x*s+y*t-z*v,w*v-x*t+y*s+z*u,w*t+x*v-y*u+z*s,w*s-x*u-y*v-z*t]


def motion_targets(initial,profile='yaw-x'):
    """Offsets from the initial pose in fixed LOCAL axes, without accumulated drift."""
    initial=validate_pose(initial)
    if profile not in ('yaw-x','sixdof'):raise ValueError('Unknown Operator profile')
    if profile=='sixdof':
        result=[]
        for label,axis,degrees in (('yaw',1,15),('pitch',0,10),('roll',2,10)):
            for angle in (degrees,-degrees):
                delta=[0.,0.,0.,math.cos(math.radians(angle)/2)]
                delta[axis]=math.sin(math.radians(angle)/2)
                q=quaternion_multiply(delta,initial['orientation'])
                norm=math.sqrt(sum(v*v for v in q))
                result.append({'label':label+'_%+d'%angle,'position':initial['position'][:],
                               'orientation':[v/norm for v in q]})
        for axis,offset in enumerate((.06,.04,.05)):
            for sign in (1,-1):
                position=initial['position'][:];position[axis]+=sign*offset
                result.append({'label':'local_'+'xyz'[axis]+'_%+.2fm'%(sign*offset),
                               'position':position,'orientation':initial['orientation'][:]})
        result.append({'label':'return_initial',**initial})
        return result
    x,y,z,w=initial['orientation']
    result=[]
    for degrees in (15,-15):
        s=math.sin(math.radians(degrees)/2)
        c=math.cos(math.radians(degrees)/2)
        quaternion=[c*x+s*z, c*y+s*w, c*z-s*x, c*w-s*y]
        length=math.sqrt(sum(v*v for v in quaternion))
        result.append({'label':'yaw_%+d'%degrees, 'position':initial['position'][:],
                       'orientation':[v/length for v in quaternion]})
    for offset in (.06,-.06):
        position=initial['position'][:]
        position[0]+=offset
        result.append({'label':'local_x_%+.2fm'%offset,'position':position,
                       'orientation':initial['orientation'][:]})
    result.append({'label':'return_initial',**initial})
    return result


class OperatorMotion:
    """Opt-in controller. Caller owns XR/native processes and readiness checks.

    Call connect_and_read_initial after tracking preflight, before native start.
    start_motion accepts a callback that checks actual paired-render status.
    finish raises on every unsuccessful motion pass; close always saves evidence.
    """
    def __init__(self, prefix, total_timeout=90, pose_adapter=None,profile='yaw-x'):
        if profile not in ('yaw-x','sixdof'):raise ValueError('Unknown Operator profile')
        self.profile=profile
        self.motion_timeout=65 if profile=='sixdof' else 23
        self.prefix=pathlib.Path(prefix)
        self.deadline=time.monotonic()+total_timeout
        self.adapter=pose_adapter or documented_pose_result
        self.client=None
        self.worker=None
        self.stop=threading.Event()
        self.error=None
        self.initial=None
        self.files=[]
        self.lock=threading.Lock()
        perf_clock=time.get_clock_info('perf_counter')
        self.report={'synthetic_motion':True,'live_tracking_verified':False,
                     'base_space':'local','operator_profile':profile,'motion_timeout_seconds':self.motion_timeout,
                     'actions':[],'success':False,
                     'initial_return_shape_observed':False,
                     'timing_domain':'perf_counter_ns',
                     'perf_counter_clock':{'implementation':perf_clock.implementation,'monotonic':perf_clock.monotonic,
                                           'adjustable':perf_clock.adjustable,'resolution_seconds':perf_clock.resolution},
                     'legacy_monotonic_usage':'Deadline scheduling only; not interchangeable with native QPC timestamps'}

    def _path(self, suffix):
        return pathlib.Path(str(self.prefix)+suffix)

    def _save(self):
        with self.lock:
            self._path('.operator.json').write_text(json.dumps(self.report,indent=2),encoding='utf-8')

    def _call(self, name, arguments, timeout=4,label=None):
        record={'tool':name,'arguments':arguments,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
                'started_monotonic':time.monotonic(),'started_perf_counter_ns':time.perf_counter_ns()}
        if label is not None:record['motion_label']=label
        self.report['actions'].append(record)
        try:
            result=self.client.rpc('tools/call',{'name':name,'arguments':arguments},timeout=timeout)
            record['raw_result']=result
            if result.get('isError') or any(b.get('isError') or
                (b.get('type')=='text' and b.get('text','').lstrip().startswith('Error:'))
                for b in result.get('content',[])):
                raise RuntimeError('Operator tool reported failure: '+name)
            if name in ('openxr_set_head_pose','openxr_release_input_devices'):
                payloads=[]
                for block in result.get('content',[]):
                    if block.get('type')=='text':
                        try:
                            payloads.append(json.loads(block.get('text','')))
                        except ValueError:
                            pass
                if not payloads or any(not isinstance(p,dict) or p.get('success') is not True for p in payloads):
                    raise RuntimeError('Operator response lacks observed success:true contract: '+name)
            return result
        except Exception as exc:
            record['error']=repr(exc)
            raise
        finally:
            record['finished_monotonic']=time.monotonic()
            record['finished_perf_counter_ns']=time.perf_counter_ns()
            record['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
            self._save()

    def connect_and_read_initial(self):
        try:
            self.prefix.parent.mkdir(parents=True,exist_ok=True)
            err=self._path('.operator.proxy.stderr.txt').open('wb')
            transcript=self._path('.operator.mcp.jsonl').open('w',encoding='utf-8')
            self.files=[err,transcript]
            self.client=StdioMCP([str(LAYER/'meta-xr-operator-mcp-proxy.exe')],err,transcript,self.deadline)
            self.report['proxy_pid']=self.client.process.pid
            self.report['initialize']=self.client.rpc('initialize',{'protocolVersion':'2024-11-05',
                'capabilities':{},'clientInfo':{'name':'GearsVR Bounded Synthetic Motion','version':'1'}},timeout=10)
            self.client.rpc('notifications/initialized',{},notification=True)
            listing=self.client.rpc('tools/list',{},timeout=10)
            tools={t['name']:t for t in listing.get('tools',[])}
            required={'openxr_get_head_pose':{'base_space','reason','sentiment'},
                      'openxr_set_head_pose':{'position','orientation','base_space','duration_seconds','reason','sentiment'},
                      'openxr_release_input_devices':{'reason','sentiment'}}
            self.report['schemas']={name:tools.get(name) for name in required}
            for name,fields in required.items():
                if name not in tools or not fields <= tools[name].get('inputSchema',{}).get('properties',{}).keys():
                    raise ValueError('Live Operator schema does not support required fields: '+name)
            props=tools['openxr_set_head_pose']['inputSchema']['properties']
            for name,length in (('position',3),('orientation',4)):
                if props[name].get('type')!='array' or props[name].get('minItems')!=length or props[name].get('maxItems')!=length:
                    raise ValueError('Unexpected pose array schema: '+name)
            for name,kind in (('base_space','string'),('duration_seconds','number')):
                if props[name].get('type')!=kind:
                    raise ValueError('Unexpected schema: '+name)
            result=self._call('openxr_get_head_pose',{'base_space':'local',
                'reason':'Read baseline before native synthetic motion test','sentiment':'neutral'},timeout=8)
            self.initial=validate_pose(self.adapter(result))
            self.report['initial_pose']=self.initial
            self.report['initial_return_shape_observed']=True
            self._save()
            return self.initial
        except Exception as exc:
            self.error=exc
            self.report['error']=repr(exc)
            self._save()
            raise

    def start_motion(self, paired_render_ready, wait_timeout=20):
        if self.initial is None or self.worker is not None:
            raise RuntimeError('Read initial pose once before starting motion')
        def work():
            try:
                wait_deadline=min(self.deadline,time.monotonic()+wait_timeout)
                while not paired_render_ready():
                    if self.stop.wait(.1):
                        raise RuntimeError('Motion cancelled before paired render')
                    if time.monotonic()>=wait_deadline:
                        raise TimeoutError('Native paired-render readiness deadline expired')
                self.report['paired_render_ready_monotonic']=time.monotonic()
                motion_deadline=min(self.deadline,time.monotonic()+self.motion_timeout)
                for target in motion_targets(self.initial,self.profile):
                    if self.stop.is_set() or not paired_render_ready():
                        raise RuntimeError('Native paired-render window ended during synthetic motion')
                    remaining=motion_deadline-time.monotonic()
                    if remaining<3.6:
                        raise TimeoutError('Synthetic motion cannot complete within profile deadline')
                    step_started=time.monotonic()
                    args={k:target[k] for k in ('position','orientation')}
                    args.update(base_space='local',duration_seconds=3.5,
                                reason='Bounded native paired-render validation: '+target['label'],sentiment='neutral')
                    self._call('openxr_set_head_pose',args,timeout=min(4,remaining-3.5),label=target['label'])
                    if self.stop.wait(max(0,3.5-(time.monotonic()-step_started))):
                        raise RuntimeError('Synthetic motion cancelled')
                if not paired_render_ready():
                    raise RuntimeError('Native paired-render window ended before motion completed')
                self.report['motion_completed_monotonic']=time.monotonic()
                self.report['motion_elapsed_seconds']=self.report['motion_completed_monotonic']-self.report['paired_render_ready_monotonic']
            except Exception as exc:
                self.error=exc
                self.report['error']=repr(exc)
            finally:
                try:
                    if any(a['tool']=='openxr_set_head_pose' for a in self.report['actions']):
                        self._call('openxr_release_input_devices',{
                            'reason':'Release synthetic input overrides after bounded validation',
                            'sentiment':'neutral'},timeout=4)
                        self.report['override_released']=True
                    self._verify_restore()
                    if self.error is None:
                        self.report['success']=True
                except Exception as exc:
                    self.report['cleanup_error']=repr(exc)
                    self.report['success']=False
                    if self.error is None:
                        self.error=exc
                        self.report['error']=repr(exc)
                self._save()
        self.worker=threading.Thread(target=work,name='bounded-operator-motion',daemon=True)
        self.worker.start()

    def _verify_restore(self):
        restored_raw=self._call('openxr_get_head_pose',{'base_space':'local',
            'reason':'Verify tracked baseline restored after synthetic motion','sentiment':'neutral'},
            timeout=4)
        restored=validate_pose(self.adapter(restored_raw))
        self.report['restored_pose']=restored
        position_error=max(abs(a-b) for a,b in zip(restored['position'],self.initial['position']))
        orientation_dot=abs(sum(a*b for a,b in zip(restored['orientation'],self.initial['orientation'])))
        self.report['restore_position_error_m']=position_error
        self.report['restore_orientation_abs_dot']=orientation_dot
        if position_error>.001 or orientation_dot<.99999:
            raise RuntimeError('Initial head pose was not restored within tolerance')
        if 'paired_render_ready_monotonic' in self.report and time.monotonic()-self.report['paired_render_ready_monotonic']>self.motion_timeout:
            raise TimeoutError('Motion/restore verification exceeded profile deadline')

    def finish(self, timeout=25):
        if self.worker is None:
            raise RuntimeError('Motion was never started')
        self.worker.join(timeout)
        if self.worker.is_alive():
            self.stop.set()
            raise TimeoutError('Operator motion finish deadline expired')
        if self.error:
            raise RuntimeError('Operator motion failed; see evidence') from self.error
        if not self.report['success']:
            raise RuntimeError('Operator motion did not complete')
        return self.report

    def close(self):
        self.stop.set()
        cleanup_error=None
        try:
            if self.worker:
                self.worker.join(timeout=5)
            if self.client:
                self.client.close()
                self.report['proxy_exit_code']=self.client.process.returncode
            if self.worker and self.worker.is_alive():
                self.worker.join(timeout=1)
                self.report['cleanup_worker_still_alive']=self.worker.is_alive()
                if self.worker.is_alive():
                    raise TimeoutError('Worker exceeded cleanup deadline')
        except Exception as exc:
            cleanup_error=exc
            self.report['success']=False
            self.report['cleanup_error']=repr(exc)
        finally:
            self._save()
            for file in self.files:
                file.close()
        if cleanup_error:
            raise RuntimeError('Operator cleanup failed; see evidence') from cleanup_error


def discover():
    with socket.socket() as sock:
        sock.settimeout(.25)
        if sock.connect_ex(('127.0.0.1',8720)) == 0:
            raise RuntimeError('Port 8720 is already in use; refusing another XR discovery')
    stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    stem=ROOT / 'analysis/openxr' / ('operator-discovery-'+stamp)
    env=os.environ.copy()
    env['XR_API_LAYER_PATH']=str(LAYER)
    env['XR_ENABLE_API_LAYERS']='XR_APILAYER_METAX_operator'
    command=[str(ROOT/'tools/openxr-probe/build-stereogpu/OpenXRPoseProbe.exe'),
             '25',str(stem.with_suffix('.csv')),'--render']
    report={'probe_command':command,'layer':str(LAYER),'pose_calls_sent':0}
    probe=None
    client=None
    deadline=time.monotonic()+40
    with stem.with_suffix('.stdout.txt').open('wb') as out, stem.with_suffix('.stderr.txt').open('wb') as err, \
         stem.with_suffix('.proxy.stderr.txt').open('wb') as proxy_err, stem.with_suffix('.mcp.jsonl').open('w',encoding='utf-8') as transcript:
        try:
            probe=subprocess.Popen(command,env=env,stdout=out,stderr=err)
            report['probe_pid']=probe.pid
            ready_deadline=min(deadline,time.monotonic()+12)
            while time.monotonic()<ready_deadline:
                if probe.poll() is not None:
                    raise RuntimeError('Probe exited before Operator port became available')
                with socket.socket() as sock:
                    sock.settimeout(.2)
                    if sock.connect_ex(('127.0.0.1',8720))==0:
                        break
                time.sleep(.2)
            else:
                raise TimeoutError('Operator port not available within 12 seconds')
            client=StdioMCP([str(LAYER/'meta-xr-operator-mcp-proxy.exe')],proxy_err,transcript,
                            min(deadline,time.monotonic()+15))
            report['proxy_pid']=client.process.pid
            report['initialize']=client.rpc('initialize',{'protocolVersion':'2024-11-05',
                'capabilities':{},'clientInfo':{'name':'GearsVR Operator Discovery','version':'1'}})
            client.rpc('notifications/initialized',{},notification=True)
            result=client.rpc('tools/list',{})
            tools=result.get('tools',[])
            report['tool_count']=len(tools)
            report['related_tools']=[t for t in tools if 'head' in t.get('name','').lower()]
            report['next_cursor']=result.get('nextCursor')
        except Exception as exc:
            report['error']=repr(exc)
        finally:
            if client:
                client.close()
                report['proxy_exit_code']=client.process.returncode
            if probe:
                try:
                    probe.wait(timeout=max(.1,deadline-time.monotonic()))
                except subprocess.TimeoutExpired:
                    report['probe_deadline_terminated']=True
                    probe.terminate()
                    try:
                        probe.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        probe.kill()
                        probe.wait(timeout=3)
                report['probe_exit_code']=probe.returncode
    report['artifact_prefix']=str(stem)
    stem.with_suffix('.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    print(json.dumps(report,indent=2))
    return 1 if 'error' in report else 0


if __name__=='__main__':
    raise SystemExit(discover())
