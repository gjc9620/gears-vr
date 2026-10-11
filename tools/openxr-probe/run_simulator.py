"""Run a bounded Meta XR Simulator diagnostic without changing the system runtime.

The separately installed simulator stays under tools/third-party on J:. This
wrapper does not download binaries, edit registry settings or simulate live
human tracking. Batch configuration is written beside the original config so
the simulator's relative tracking paths keep resolving correctly.
"""
import argparse
import json
import os
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
DEFAULT = ROOT / 'tools/third-party/meta-xr-simulator/PFiles/MetaXRSimulator/v207.0'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--simulator-dir', type=pathlib.Path, default=DEFAULT)
    parser.add_argument('--seconds', type=int, default=12, choices=range(3, 61))
    parser.add_argument('--gpu-game', action='store_true')
    parser.add_argument('--gpu-snapshot', action='store_true')
    args = parser.parse_args()
    if args.gpu_snapshot and (not args.gpu_game or args.seconds < 12):
        parser.error('Snapshot requires --gpu-game and at least 12 seconds')
    directory = args.simulator_dir.resolve()
    manifest = directory / 'meta_openxr_simulator.json'
    original = directory / 'config/sim_core_configuration.json'
    for file in (manifest, original, directory / 'SIMULATOR.dll'):
        if not file.is_file():
            parser.error(f'Missing separately installed simulator file: {file}')
    config = json.loads(original.read_text(encoding='utf-8'))
    config['use_batch_mode'] = True
    folder = ROOT / 'analysis/openxr'
    folder.mkdir(parents=True, exist_ok=True)
    config['log_location'] = (folder / 'meta-xrsim.log').as_posix()
    target = original.with_name('gears_automation.json')
    target.write_text(json.dumps(config, indent=2), encoding='utf-8')
    env = os.environ.copy()
    env['META_XRSIM_CONFIG_JSON'] = str(target)
    command = [sys.executable, str(pathlib.Path(__file__).with_name('run_pose.py')),
               '--seconds', str(args.seconds), '--runtime', str(manifest)]
    command.append('--gpu-game' if args.gpu_game else '--render')
    if args.gpu_snapshot:
        command.append('--gpu-snapshot')
    # run_pose applies its own child deadline and writes bounded reports.
    return subprocess.run(command, env=env, timeout=args.seconds + 35).returncode


if __name__ == '__main__':
    sys.exit(main())
