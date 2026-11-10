"""Freeze and compile only the established-hold republication repair."""
from pathlib import Path
import difflib
import hashlib
import json
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[2]
AREA=Path(__file__).resolve().parent
BASE=AREA/'reload-cleanup-r1'
TARGET=AREA/'held-republish-r1'


def main():
    if TARGET.exists():raise RuntimeError('Preserve existing named snapshot')
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    changed=['weapon_equipment_runtime.h','weapon_hammerburst_equipment_fixture.inl',
             'weapon_hammerburst_held_republish_test.cpp']
    for name in changed:shutil.copy2(ROOT/'tools/playground'/name,TARGET/'source/tools/playground'/name)
    rel=Path('tools/playground/weapon_equipment_runtime.h')
    before=(BASE/'source'/rel).read_text();after=(TARGET/'source'/rel).read_text()
    patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),
        fromfile='a/'+rel.as_posix(),tofile='b/'+rel.as_posix()))
    (ROOT/'docs/hammerburst-held-republish.patch').write_text(patch)
    commands=['@echo off','call "E:\\vs\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul',
              'if errorlevel 1 exit /b %errorlevel%']
    for stem in ('weapon_equipment_runtime_test','weapon_hammerburst_held_republish_test'):
        source=TARGET/'source/tools/playground'/(stem+'.cpp')
        exe=TARGET/(stem+'.exe');obj=TARGET/(stem+'.obj')
        commands += [f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc "{source}" /Fo"{obj}" /Fe"{exe}"',
                     'if errorlevel 1 exit /b %errorlevel%',f'"{exe}"','if errorlevel 1 exit /b %errorlevel%']
    commands+=['exit /b 0']
    check=TARGET/'check-frozen.cmd';check.write_text('\n'.join(commands)+'\n')
    with (TARGET/'test.log').open('wb') as log:
        subprocess.run(['cmd','/c',str(check)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    flags=json.loads((AREA/'reload-eligibility-r1/prepared.json').read_text())
    configure=list(flags['configure_command']);configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build=list(flags['build_command']);build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:
            subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,frozen_equipment_tests_passed=True,
        runtime_exercised=False,parking_restoration_enabled=False,shared_instance_changed=False,
        native_rebuilt=False,xr_rebuilt=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),
        changed_source_files=['tools/playground/'+name for name in changed],
        configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({name:hashlib.sha256((TARGET/'source/tools/playground'/name).read_bytes()).hexdigest() for name in changed},indent=2))
    print((TARGET/'test.log').read_text())
    print(json.dumps(dict(full_playground_compiled=True,dll_sha256=proof['dll_sha256'])))


if __name__=='__main__':main()
