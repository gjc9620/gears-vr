"""Collect creation bindings and independently verify restored hook entries."""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import pathlib
import time
from start_capture import modules, pefile
from runtime_probe import Process


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--build-dir', type=pathlib.Path, default=pathlib.Path(__file__).with_name('build-binding'))
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--timeout', type=float, default=60)
    args = parser.parse_args()
    source = args.build_dir / f'binding-{args.pid}.jsonl'
    deadline = time.monotonic() + args.timeout
    while True:
        raw = source.read_text() if source.exists() else ''
        lines = raw.splitlines()
        if lines and '"event":"stopped"' in lines[-1]:
            break
        if time.monotonic() > deadline:
            raise RuntimeError('Observer stop not confirmed')
        time.sleep(.5)
    records = [json.loads(line) for line in lines]
    if not records[0].get('ok') or records[0].get('pid') != args.pid:
        raise RuntimeError('Observer startup did not validate')
    bindings = [item for item in records if item['event'] == 'binding']
    presents = [item for item in records if item['event'] == 'present']
    entries = [item for item in records if item['event'] == 'entry']
    report = dict(pid=args.pid, stopped=records[-1], bindings=bindings, presents=presents, entry_checks=[], confirmed=[])
    report['readback'] = next((item for item in records if item['event'] == 'readback'), None)
    report['resources'] = next((item for item in records if item['event'] == 'resources'), None)
    report['gpu_share'] = next((item for item in records if item['event'] == 'gpu_share'), None)
    process = Process(args.pid)
    try:
        loaded = modules(process.handle)
        allowed = []
        for name, (base, path) in loaded.items():
            if name != 'dxgi.dll':
                continue
            pe = pefile.PE(str(path), fast_load=True)
            allowed.append((base, base + pe.OPTIONAL_HEADER.SizeOfImage))
            pe.close()
        for entry in entries:
            target = int(entry['target'], 16)
            expected = bytes.fromhex(entry['before'])
            if len(expected) != 32 or not any(low <= target and target + 32 <= high for low, high in allowed):
                raise RuntimeError('Entry outside allowed DXGI module or wrong byte count')
            report['entry_checks'].append(dict(target=entry['target'], matches=process.read(target, 32) == expected))
        user = c.WinDLL('user32', use_last_error=True)
        user.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
        for binding in bindings:
            owner = w.DWORD()
            user.GetWindowThreadProcessId(int(binding['window'], 16), c.byref(owner))
            matching = [item for item in presents if item['chain'] == binding['chain'] and item['window'] == binding['window']]
            if owner.value == args.pid and binding['queue_type'] == 0 and binding['queue'] != '0x0' and binding['device'] != '0x0' and matching:
                report['confirmed'].append(dict(binding=binding, observed_present_calls=sum(item['count'] for item in matching)))
    finally:
        process.close()
    report['entries_restored'] = len(entries) == 4 and all(item['matches'] for item in report['entry_checks'])
    report['binding_confirmed'] = bool(report['confirmed'])
    report['limitation'] = ('Creation and Present canonical COM identity match proves observed queue binding. '
        + ('GPU shared frames were published; consumer and headset evidence must be checked separately. '
           if report['gpu_share'] else 'One GPU back-buffer copy was saved; this does not establish continuous capture, stereo, gameplay content or HDR color accuracy. '
           if report['readback'] and report['readback']['saved'] else 'No saved GPU back-buffer copy is confirmed. ')
        + 'Addresses are specific to this process.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.with_suffix('.jsonl').write_text(raw)
    if report['readback'] and report['readback']['saved']:
        import shutil
        image = args.output.with_suffix('.ppm')
        shutil.copyfile(args.build_dir / f'gpu-{args.pid}.ppm', image)
        report['image'] = str(image.resolve())
    args.output.write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))
    if (not report['entries_restored'] or not report['stopped']['restored']
        or not report['binding_confirmed']
        or (report['readback'] is not None and not report['readback']['saved'])
        or (report['gpu_share'] is not None and (report['gpu_share']['state']!=2 or not report['gpu_share']['published'] or report['gpu_share']['hr']!='0x00000000'))
        or (report['resources'] is not None and not report['resources']['released'])):
        raise SystemExit(1)


if __name__ == '__main__':
    main()
