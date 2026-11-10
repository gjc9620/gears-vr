"""Build only the HB opening-qualification repair over the last private source.

Preserves the clock observation delta; never stages a running instance.
"""
from pathlib import Path
import difflib
import hashlib
import json
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[2]
AREA=Path(__file__).resolve().parent
BASE=AREA/'input-admission-observation-r1'
TARGET=AREA/'reload-eligibility-r1'
REL=Path('tools/playground/weapon_hammerburst_runtime.inl')
HELPER=Path('tools/playground/weapon_hammerburst_reload_begin.inl')


def main():
    if TARGET.exists():raise RuntimeError('Preserve this named snapshot; inspect its existing result')
    original=(BASE/'source'/REL).read_text(encoding='utf-8')
    source=original
    old='lancerreload_state::RewardReceipt reward;'
    assert source.count(old)==1
    source=source.replace(old,old+'\n#include "weapon_hammerburst_reload_begin.inl"')
    old='        serial=0;\n        const auto result=needs_reload?lancerreload_state::begin(f,serial):lancerreload_state::BeginResult::acquired;'
    assert source.count(old)==1
    source=source.replace(old,'        const auto result=reload_begin::acquire(f,before,serial,reward);')
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    (TARGET/'source'/REL).write_text(source,encoding='utf-8')
    shutil.copy2(ROOT/HELPER,TARGET/'source'/HELPER)
    patch=''.join(difflib.unified_diff(original.splitlines(True),source.splitlines(True),
                                     fromfile='a/'+REL.as_posix(),tofile='b/'+REL.as_posix()))
    (ROOT/'docs/hammerburst-reload-eligibility.patch').write_text(patch,encoding='utf-8')
    base=json.loads((BASE/'prepared.json').read_text())
    configure=list(base['configure_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground')
    configure[configure.index('-B')+1]=str(TARGET/'build')
    build=list(base['build_command']);build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:
            subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,
                           creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    changed=[]
    for name in ('playground','vr-common','third-party','native-capture'):
        for file in (TARGET/'source/tools'/name).rglob('*'):
            if not file.is_file():continue
            relative=file.relative_to(TARGET/'source');old=BASE/'source'/relative
            if not old.exists() or file.read_bytes()!=old.read_bytes():changed.append(relative.as_posix())
    assert sorted(changed)==sorted([REL.as_posix(),HELPER.as_posix()]),changed
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),changed_source_files=changed,full_playground_compiled=True,
               runtime_exercised=False,shared_instance_changed=False,public_changed=False,
               native_rebuilt=False,xr_rebuilt=False,dll=str(dll),
               dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),
               original_runtime_sha256=hashlib.sha256(original.encode()).hexdigest(),
               updated_runtime_sha256=hashlib.sha256(source.encode()).hexdigest(),
               helper_sha256=hashlib.sha256((ROOT/HELPER).read_bytes()).hexdigest(),
               configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2),encoding='utf-8')
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','runtime_exercised','dll_sha256','changed_source_files')},indent=2))


if __name__=='__main__':main()
