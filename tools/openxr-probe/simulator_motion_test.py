"""Drive synthetic head motion and capture the simulator compositor unattended.

Targets only our separately launched pose probe via its local diagnostic API
layer. Synthetic poses deliberately remain labelled as simulation evidence.
"""
import base64
import argparse
import datetime
import json
import math
import pathlib
import subprocess
import sys
import time
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[2]


class Operator:
    def __init__(self):
        self.headers = {'Content-Type': 'application/json', 'Accept': 'application/json, text/event-stream'}
        self.serial = 0
        result = self.rpc('initialize', {'protocolVersion': '2024-11-05', 'capabilities': {},
                                       'clientInfo': {'name': 'GearsVR Simulator Diagnostic', 'version': '1'}})
        if result.get('serverInfo', {}).get('name') != 'AgenticXR_MCP_Server':
            raise RuntimeError('Unexpected local diagnostic server')
        self.rpc('notifications/initialized', {}, notification=True)

    def rpc(self, method, params, notification=False):
        data = {'jsonrpc': '2.0', 'method': method, 'params': params}
        if not notification:
            self.serial += 1
            data['id'] = self.serial
        request = urllib.request.Request('http://localhost:8720/mcp', data=json.dumps(data).encode(), headers=self.headers)
        with urllib.request.urlopen(request, timeout=8) as response:
            session = response.headers.get('Mcp-Session-Id')
            if session: self.headers['Mcp-Session-Id'] = session
            raw = response.read()
        if not raw: return {}
        result = json.loads(raw)
        if 'error' in result: raise RuntimeError(result['error'])
        return result.get('result', {})

    def call(self, name, **arguments):
        result = self.rpc('tools/call', {'name': name, 'arguments': dict(arguments,
            reason='Verify bounded synthetic simulator head motion and display', sentiment='neutral')})
        if result.get('isError') or any(block.get('isError') or block.get('text','').startswith('Error:') for block in result.get('content', [])): raise RuntimeError(result)
        return result

    def close(self):
        request = urllib.request.Request('http://localhost:8720/mcp', headers=self.headers, method='DELETE')
        with urllib.request.urlopen(request, timeout=5) as response: response.read()


def texts(result):
    return '\n'.join(block.get('text', '') for block in result.get('content', []) if block.get('type') == 'text')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gpu-game',action='store_true',help='Use a freshly started game GPU channel instead of test patterns')
    args=parser.parse_args()
    folder = ROOT / 'analysis/openxr'
    folder.mkdir(parents=True, exist_ok=True)
    stem = folder / ('sim-motion-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    # Refuse to alter an already running XR app. Port ownership belongs to the
    # probe launched below; verify the advertised application name as well.
    import socket
    with socket.socket() as sock:
        sock.settimeout(.2)
        if sock.connect_ex(('localhost', 8720)) == 0:
            raise RuntimeError('Another diagnostic XR server is already running')
    command = [sys.executable, str(pathlib.Path(__file__).with_name('run_simulator.py')),
               '--seconds', '30', '--operator']
    if args.gpu_game: command.append('--gpu-game')
    report = {'synthetic_motion': True, 'live_tracking_verified': False, 'gpu_game':args.gpu_game, 'actions': []}
    with stem.with_suffix('.stdout.txt').open('wb') as out, stem.with_suffix('.stderr.txt').open('wb') as err:
        child = subprocess.Popen(command, stdout=out, stderr=err)
        try:
            deadline = time.monotonic() + 12
            while True:
                try: client = Operator(); break
                except (OSError, RuntimeError):
                    if child.poll() is not None or time.monotonic() >= deadline: raise
                    time.sleep(.2)
            instance = client.call('openxr_get_instance_info')
            if 'GearsVR Pose Probe' not in texts(instance):
                raise RuntimeError('Local XR server does not identify our pose probe')
            report['instance'] = instance
            deadline = time.monotonic() + 8
            while True:
                try:
                    pose = json.loads(texts(client.call('openxr_get_head_pose', base_space='local')))
                    if pose.get('flags', {}).get('position_valid'): break
                except (RuntimeError, ValueError): pass
                if time.monotonic() >= deadline: raise RuntimeError('Probe session not ready')
                time.sleep(.2)
            for x, y, z, yaw in ((.1, .05, -.1, 10), (-.1, -.05, .1, -10), (0, 0, 0, 0)):
                angle = math.radians(yaw) / 2
                result = client.call('openxr_set_head_pose', base_space='local',
                    position=[x, y, z], orientation=[0, math.sin(angle), 0, math.cos(angle)], duration_seconds=1)
                report['actions'].append({'requested_position': [x, y, z], 'requested_yaw': yaw, 'result': result})
                time.sleep(1.5)
                report['actions'][-1]['observed'] = client.call('openxr_get_head_pose', base_space='local')
            for eye in ('left', 'right'):
                result = client.call('openxr_capture_composited_image', eye=eye)
                saved = False
                for block in result.get('content', []):
                    if block.get('type') == 'image':
                        extension = '.png' if block.get('mimeType') == 'image/png' else '.jpg'
                        path = pathlib.Path(str(stem) + '-' + eye + extension)
                        path.write_bytes(base64.b64decode(block['data'], validate=True))
                        report[eye + '_capture'] = str(path); saved = True
                if not saved: raise RuntimeError('Compositor did not return an image')
            client.close()
            report['probe_exit'] = child.wait(timeout=45)
            probe = json.loads(stem.with_suffix('.stdout.txt').read_text(encoding='utf-8'))
            report['probe_report'] = probe
            ranges = probe['pose_summary']['head_ranges']
            report['motion_in_csv'] = all(ranges[key][0] < -.04 and ranges[key][1] > .04 for key in ('head_x', 'head_y', 'head_z')) and ranges['qy'][0] < -.08 and ranges['qy'][1] > .08 and probe['pose_summary']['unique_head_poses'] > 20
            if args.gpu_game and not probe.get('gpu_game_summary',{}).get('gpu_game_copies'):raise RuntimeError('No game GPU image copies verified')
            if report['probe_exit'] or not report['motion_in_csv']:
                raise RuntimeError('Synthetic motion or normal probe exit not verified')
        except Exception as error:
            report['error'] = str(error)
            # Allow the bounded probe to perform normal OpenXR cleanup first.
            try: report['probe_exit'] = child.wait(timeout=45)
            except subprocess.TimeoutExpired: child.terminate(); child.wait(timeout=5)
    stem.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps({'report': str(stem.with_suffix('.json')), 'passed': 'error' not in report,
                      'error': report.get('error'), 'motion_in_csv': report.get('motion_in_csv')}))
    return int('error' in report)


if __name__ == '__main__': sys.exit(main())
