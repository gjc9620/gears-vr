"""Prepare a timing-only delta against the exact last tested HB source.

No shared source mutation, launcher, game, input or binary replacement.
"""
from pathlib import Path
import difflib
import hashlib
import json
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
AREA = Path(__file__).resolve().parent
BASE = ROOT / 'analysis/solo-m1-20261028/hammerburst-prepared-20261110-return-observation-r1'
REL = Path('tools/playground/weapon_lancer_r2_native.inl')


def prepare():
    target = AREA / 'input-admission-observation-r1'
    if (target / 'prepared.json').exists():
        raise RuntimeError('Preserve existing preparation/build evidence; do not overwrite or rebuild this named snapshot')
    original = (BASE / 'source' / REL).read_text(encoding='utf-8')
    source = original
    old = '    const auto result=input.advance(current?&frame:nullptr,current,game,weaponhand::clock_ns(nullptr),'
    new = '''    // Preserve the exact admission clock already sampled here. The earlier
    // `now` predates aim.advance and cannot describe this input admission.
    const auto input_admission_ns=weaponhand::clock_ns(nullptr);
    const auto result=input.advance(current?&frame:nullptr,current,game,input_admission_ns,'''
    assert source.count(old) == 1
    source = source.replace(old, new)
    old = ' continuity_native_selected=%u\\n",'
    new = (' continuity_native_selected=%u input_admission_ns=%lld input_admission_delay_ns=%lld'
           ' input_frame_now_ns=%lld input_stereo_received_ns=%lld input_controller_received_ns=%lld'
           ' input_frame_flags=%u input_stereo_status=%u input_frame_shape=%u input_frame_active=%u\\n",')
    assert source.count(old) == 1
    source = source.replace(old, new)
    old = 'unsigned(continuity_failure),unsigned(continuity_native_selected));'
    new = '''unsigned(continuity_failure),unsigned(continuity_native_selected),
            input_admission_ns,input_admission_ns-now,frame.now_ns,frame.stereo.received_ns,frame.controllers.received_ns,
            frame.flags,frame.stereo_status,unsigned(wh::frame_shape_valid(frame)),unsigned(wh::frame_active(frame,input_admission_ns)));'''
    assert source.count(old) == 1
    source = source.replace(old, new)
    target.mkdir(exist_ok=True)
    (target / REL.name).write_text(source, encoding='utf-8')
    patch = ''.join(difflib.unified_diff(original.splitlines(True), source.splitlines(True),
                                        fromfile='a/' + REL.as_posix(), tofile='b/' + REL.as_posix()))
    destination = ROOT / 'docs/hammerburst-input-admission-clock.patch'
    destination.write_text(patch, encoding='utf-8')
    receipt = dict(base=str(BASE), source=str(BASE / 'source' / REL),
                   original_sha256=hashlib.sha256(original.encode()).hexdigest(),
                   proposed_sha256=hashlib.sha256(source.encode()).hexdigest(),
                   patch=str(destination), patch_sha256=hashlib.sha256(patch.encode()).hexdigest(),
                   existing_clock_reused=True, extra_clock_reads=0, guards_changed=False,
                   shared_source_changed=False, full_playground_compiled=False, runtime_exercised=False)
    (target / 'prepared.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    if '--build' in sys.argv:
        private = target / 'source'
        if private.exists():
            raise RuntimeError('Preserve this private source/build; inspect existing result rather than recreate it')
        for name in ('playground', 'vr-common', 'third-party', 'native-capture'):
            shutil.copytree(BASE / 'source/tools' / name, private / 'tools' / name)
        (private / REL).write_text(source, encoding='utf-8')
        flags = []
        for line in (BASE / 'CMakeCache-playground.txt').read_text().splitlines():
            if re.match(r'^GEARS_PLAYGROUND_[A-Z0-9_]+:(BOOL|STRING)=', line):
                key, value = line.split('=', 1)
                flags.append('-D' + key + '=' + value)
        cmake = 'E:/CMake/bin/cmake.exe'
        configure = [cmake, '-S', str(private / 'tools/playground'), '-B', str(target / 'build'),
                     '-G', 'Visual Studio 18 2026', '-A', 'x64', *flags]
        build = [cmake, '--build', str(target / 'build'), '--config', 'Release', '--target', 'GearsPlayground', '--parallel', '2']
        with (target / 'configure.log').open('wb') as log:
            subprocess.run(configure, check=True, stdout=log, stderr=subprocess.STDOUT,
                           creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        with (target / 'build.log').open('wb') as log:
            subprocess.run(build, check=True, stdout=log, stderr=subprocess.STDOUT,
                           creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
        dll = target / 'build/Release/GearsPlayground6DofUnified3.dll'
        changed = []
        for name in ('playground', 'vr-common', 'third-party', 'native-capture'):
            for file in (private / 'tools' / name).rglob('*'):
                if file.is_file() and file.read_bytes() != (BASE / 'source' / file.relative_to(private)).read_bytes():
                    changed.append(file.relative_to(private).as_posix())
        assert changed == [REL.as_posix()], changed
        receipt.update(full_playground_compiled=True, dll=str(dll),
                       dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(), changed_source_files=changed,
                       configure_command=configure, build_command=build)
        (target / 'prepared.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    prepare()
