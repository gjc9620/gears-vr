"""Transfer this application's own files and convert captured finger poses for the existing Mod editor."""
from __future__ import annotations
import argparse
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys

HERE = Path(__file__).resolve().parent
DEVICE_ROOT = '/sdcard/Android/data/com.gearsvr.workbench/files'
SIGNATURE = '3b3c978cb7b6df3f3be48f60ac33daaf45dd8f30816a18a392fc906bdbc75193'


def run(adb, serial, *args):
    result = subprocess.run([str(adb), '-s', serial, *map(str, args)], capture_output=True,
                            text=True, encoding='utf-8', errors='replace', timeout=90)
    if result.returncode:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip() or 'Device command failed')
    return result.stdout


def device(adb, serial):
    if serial:
        if run(adb, serial, 'get-state').strip() != 'device':
            raise RuntimeError('Selected Quest is not connected')
        return serial
    result = subprocess.run([str(adb), 'devices'], capture_output=True, text=True, timeout=15)
    devices = [line.split()[0] for line in result.stdout.splitlines()[1:] if line.split()[1:] == ['device']]
    if len(devices) != 1:
        raise RuntimeError('Connect one authorized Quest, or select its serial explicitly')
    return devices[0]


def multiply(a, b):
    x, y, z, w = a
    X, Y, Z, W = b
    return [w*X+x*W+y*Z-z*Y, w*Y-x*Z+y*W+z*X, w*Z+x*Y-y*X+z*W, w*W-x*X-y*Y-z*Z]


def normalized(q):
    if len(q) != 4 or not all(math.isfinite(x) for x in q):
        raise ValueError('Invalid quaternion')
    n = math.sqrt(sum(x*x for x in q))
    if abs(n-1) > .01:
        raise ValueError('Quaternion is not normalized')
    return [x/n for x in q]


def angles(q):
    x, y, z, w = normalized(q)
    return [math.degrees(math.atan2(2*(w*x+y*z), 1-2*(x*x+y*y))),
            math.degrees(math.asin(max(-1., min(1., 2*(w*y-z*x))))),
            math.degrees(math.atan2(2*(w*z+x*y), 1-2*(y*y+z*z)))]


def correction(degrees):
    x, y, z = [math.radians(x)/2 for x in degrees]
    sx, sy, sz, cx, cy, cz = math.sin(x), math.sin(y), math.sin(z), math.cos(x), math.cos(y), math.cos(z)
    return [sx*cy*cz-cx*sy*sz, cx*sy*cz+sx*cy*sz, cx*cy*sz-sx*sy*cz, cx*cy*cz+sx*sy*sz]


def baselines(root):
    """Read numeric tables from the actual Mod, never a second hand-authored copy."""
    headers = root/'tools/vr-common'
    sources = {'main_grip': ('weapon_lancer_main_hand.h', 'rest'),
               'support': ('weapon_lancer_natural_hand.h', 'support_closed'),
               'magazine': ('weapon_lancer_part_hand.h', 'magazine'),
               'charging': ('weapon_lancer_part_hand.h', 'loading'),
               'starter': ('weapon_lancer_part_hand.h', 'starter')}
    model = root/'tools/quest-workbench/assets/Lancer.glb'
    identity = 14695981039346656037
    for byte in model.read_bytes():
        identity = ((identity ^ byte)*1099511628211) & ((1 << 64)-1)
    out = {'schema': 1, 'rig_signature': SIGNATURE, 'model_content_fnv64': str(identity),
           'parts': {}, 'source_sha256': {}}
    for part, (filename, symbol) in sources.items():
        path = headers/filename
        text = path.read_text('utf-8-sig')
        match = re.search(r'\b'+symbol+r'(?:\[15\])?\s*(?:=\s*)?\{(.*?)\};', text, re.S)
        if not match:
            raise ValueError('Cannot find current pose table: '+symbol)
        rows = re.findall(r'\{\s*([-+\d.eE]+)\s*,\s*([-+\d.eE]+)\s*,\s*([-+\d.eE]+)\s*,\s*([-+\d.eE]+)\s*\}', match[1])
        if len(rows) != 15:
            raise ValueError('Expected fifteen original pose quaternions: '+symbol)
        out['parts'][part] = [normalized(list(map(float, row))) for row in rows]
        out['source_sha256'][filename] = hashlib.sha256(path.read_bytes()).hexdigest()
    return out


def convert(project, baseline, destination):
    if project.get('schema') != 1 or project.get('kind') != 'quest_weapon_workbench' or project.get('rig_signature') != SIGNATURE:
        raise ValueError('Unsupported workbench project or rig')
    if project.get('model') != 'Lancer.glb' or baseline.get('rig_signature') != SIGNATURE or project.get('model_content_fnv64') != baseline.get('model_content_fnv64'):
        raise ValueError('This Mod adapter is for the original Lancer/Marcus rig only')
    if project.get('rig_joint_basis') != 'ue_wrist_reflect_x':
        raise ValueError('Project uses an older or unknown joint basis; recapture with the corrected model')
    # Preserve complete wrist/volume/raw recording data in project.json. The existing
    # V2 Mod channel admits bounded finger deltas; it does not import volume edits.
    if destination.exists() and any(destination.iterdir()):
        raise ValueError('Choose a fresh output folder; previous conversions are preserved')
    destination.mkdir(parents=True, exist_ok=True)
    written, rejected = [], []
    for part in project['parts']:
        key = part['id']
        if key not in baseline['parts']:
            continue
        for hand in part['hands']:
            side = hand['side']
            if side != ('right' if key == 'main_grip' else 'left'):
                continue
            try:
                if not hand['retargeted'] or len(hand['local_fingers_xyzw']) != 15:
                    raise ValueError('Pose has not been calibrated/retargeted')
                joints = []
                for i, local in enumerate(hand['local_fingers_xyzw']):
                    target = multiply(normalized(local), correction(hand['finger_correction_degrees'][i]))
                    # Undo the wrist-local reflection before applying raw Unreal reference tables.
                    target = [target[0], -target[1], -target[2], target[3]]
                    b = baseline['parts'][key][i]
                    delta = normalized(multiply([-b[0], -b[1], -b[2], b[3]], target))
                    if abs(delta[3]) < math.cos(math.pi/4)-1e-8:
                        raise ValueError(f'Joint {i+1} exceeds the current Mod editor 90-degree correction limit')
                    joints.append(angles(delta))
                profile = dict(schema=2, weapon='Lancer', side=side, part=key,
                               geometry=SIGNATURE, space='main_grip_forward_right_up_metres',
                               rotation_order='Rz_Ry_Rx_degrees', palm_metres=[0., 0., 0.],
                               palm_degrees=[0., 0., 0.], finger_degrees=joints)
                path = destination/(key+'-'+side+'-fingers-only.json')
                path.write_text(json.dumps(profile, ensure_ascii=False, indent=2), 'utf-8')
                written.append(str(path))
            except (ValueError, KeyError, TypeError) as error:
                rejected.append(dict(part=key, side=side, reason=str(error)))
    report = dict(files=written, rejected=rejected, scope='finger-only adapter to existing Mod V2',
                  wrist_and_volume_imported=False,
                  complete_project_preserved=True, target_baseline=baseline.get('source_sha256'))
    (destination/'conversion-review.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), 'utf-8')
    return report


def convert_capture_library(source, baseline, destination):
    """Keep each capture's identity, converting only its selected part/hand."""
    destination.mkdir(parents=True, exist_ok=True)
    report = []
    for path in sorted(source.glob('*.json')):
        try:
            if not re.fullmatch(r'\d+-\d+', path.stem):
                raise ValueError('Invalid capture filename')
            project = json.loads(path.read_text('utf-8'))
            meta = project['capture']
            if meta['id'] != path.stem or meta['side'] not in (0, 1, 2):
                raise ValueError('Invalid capture identity')
            if type(meta['part']) is not int or not 0 <= meta['part'] < len(project['parts']):
                raise ValueError('Invalid capture part')
            if project.get('model') != 'Lancer.glb':
                report.append(dict(capture_id=path.stem, model=project.get('model'), complete_project_preserved=True,
                                   mod_conversion='not_available_for_this_weapon'))
                continue
            part = dict(project['parts'][meta['part']])
            part['hands'] = [h for h in part['hands'] if meta['side'] == 2 or h['side'] == ('right' if meta['side'] else 'left')]
            selected = dict(project, parts=[part])
            result = convert(selected, baseline, destination/path.stem)
            report.append(dict(capture_id=path.stem, name=meta.get('name'), **result))
        except (ValueError, KeyError, TypeError, IndexError) as error:
            report.append(dict(capture_id=path.stem, error=str(error)))
    (destination/'capture-library-review.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), 'utf-8')
    return report


def take_summary(path):
    with path.open('rb') as f:
        header = f.read(16)
        if len(header) != 16 or header[:8] not in (b'QWBTAKE1',b'QWBTAKE2'):
            raise ValueError('Unsupported take file')
        count, stride = struct.unpack_from('<II', header, 8)
        if count > 21600 or stride != (2184 if header[:8]==b'QWBTAKE2' else 2168) or path.stat().st_size != 16+count*stride:
            raise ValueError('Invalid recording dimensions')
        first, last, inferred, lost, epoch = None, None, [0, 0], [0, 0], None
        for _ in range(count):
            raw = f.read(stride)
            timestamp, space = struct.unpack_from('<qQ', raw)
            if timestamp <= 0 or last is not None and (timestamp <= last or epoch != space):
                raise ValueError('Recording time/space discontinuity')
            first = timestamp if first is None else first
            last, epoch = timestamp, space
            for side in range(2):
                base = 72+1048*side
                active, = struct.unpack_from('<I', raw, base)
                if active not in (0, 1):
                    raise ValueError('Invalid active state')
                flags = [struct.unpack_from('<Q', raw, base+8+40*i+32)[0] for i in range(26)]
                if not active or any(x & 3 != 3 for x in flags):
                    lost[side] += 1
                elif any(x & 12 != 12 for x in flags):
                    inferred[side] += 1
    return dict(format=header[:8].decode(), frames=count, seconds=0 if not count else (last-first)/1e9,
                inferred_frames_left_right=inferred, lost_frames_left_right=lost,
                raw_sample_timing_preserved=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adb', type=Path, default=Path('D:/adb/platform-tools/adb.exe'))
    parser.add_argument('--serial')
    sub = parser.add_subparsers(dest='command', required=True)
    pull = sub.add_parser('pull')
    pull.add_argument('--output', type=Path, required=True)
    upload = sub.add_parser('import-glb')
    upload.add_argument('model', type=Path)
    export = sub.add_parser('convert')
    export.add_argument('project', type=Path)
    export.add_argument('--baselines', type=Path, default=HERE/'mod-baselines.json')
    export.add_argument('--output', type=Path, required=True)
    base = sub.add_parser('baselines')
    base.add_argument('--root', type=Path, default=HERE.parents[1])
    base.add_argument('--output', type=Path, default=HERE/'mod-baselines.json')
    args = parser.parse_args()
    if args.command == 'baselines':
        value = baselines(args.root)
        args.output.write_text(json.dumps(value, indent=2), 'utf-8')
        print(args.output)
    elif args.command == 'convert':
        print(json.dumps(convert(json.loads(args.project.read_text('utf-8-sig')),
                                 json.loads(args.baselines.read_text('utf-8-sig')), args.output), ensure_ascii=False))
    elif args.command == 'pull':
        serial = device(args.adb, args.serial)
        destination = args.output/datetime.now().strftime('%Y%m%d-%H%M%S')
        destination.mkdir(parents=True, exist_ok=False)
        print(run(args.adb, serial, 'pull', DEVICE_ROOT+'/.', destination))
        for take in destination.glob('*.take'):
            take.with_suffix('.review.json').write_text(json.dumps(take_summary(take), indent=2), 'utf-8')
        project = destination/'project.json'
        if project.exists():
            content = json.loads(project.read_text('utf-8'))
            if content.get('model') == 'Lancer.glb':
                convert(content, json.loads((HERE/'mod-baselines.json').read_text('utf-8')), destination/'mod-fingers')
        if (destination/'captures').is_dir():
            convert_capture_library(destination/'captures', json.loads((HERE/'mod-baselines.json').read_text('utf-8')), destination/'mod-captures')
        print('已导出：'+str(destination))
    elif args.command == 'import-glb':
        reserved={'lancer'}
        if (HERE/'weapon-catalog.json').exists():
            reserved.update(Path(w['model']).stem.casefold() for w in json.loads((HERE/'weapon-catalog.json').read_text('utf-8'))['weapons'])
        if args.model.stem.casefold() in reserved:
            raise ValueError('This name is reserved for a packaged original. Rename your custom model before importing it.')
        raw = args.model.read_bytes()
        if len(raw) < 20 or len(raw) > 64*1024*1024 or raw[:4] != b'glTF' or struct.unpack_from('<I', raw, 4)[0] != 2 or struct.unpack_from('<I', raw, 8)[0] != len(raw):
            raise ValueError('Choose a self-contained GLB below64MiB')
        size, kind = struct.unpack_from('<II', raw, 12)
        if kind != 0x4E4F534A or size > len(raw)-20:
            raise ValueError('GLB JSON chunk is invalid')
        content = json.loads(raw[20:20+size])
        if len(content.get('buffers', [])) != 1 or content['buffers'][0].get('uri'):
            raise ValueError('Export the model as a self-contained GLB with embedded buffers')
        serial = device(args.adb, args.serial)
        print(run(args.adb, serial, 'push', args.model, DEVICE_ROOT+'/models/'+args.model.stem+'.glb'))
        print('在工作台“武器库”翻到下一页，选择导入模型即可载入；自定义机械逻辑需另行配置。')


if __name__ == '__main__':
    try: main()
    except Exception as error:
        print('未完成：'+str(error), file=sys.stderr)
        raise SystemExit(1)
