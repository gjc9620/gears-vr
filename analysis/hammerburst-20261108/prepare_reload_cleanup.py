"""Compile the bounded HB stop/completion repair; no game or staging."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess

ROOT=Path(__file__).resolve().parents[2]
AREA=Path(__file__).resolve().parent
BASE=AREA/'reload-eligibility-r1'
TARGET=AREA/'reload-cleanup-r1'
REL=Path('tools/playground/weapon_hammerburst_runtime.inl')
HELPER=Path('tools/playground/weapon_hammerburst_reload_cleanup.inl')
TEST=Path('tools/playground/weapon_hammerburst_reload_cleanup_test.cpp')


def main():
    if TARGET.exists():raise RuntimeError('Retain existing source/build evidence; do not recreate this snapshot')
    original=(BASE/'source'/REL).read_text(encoding='utf-8')
    source=original.replace('#include "weapon_hammerburst_reload_begin.inl"',
                            '#include "weapon_hammerburst_reload_begin.inl"\n#include "weapon_hammerburst_reload_cleanup.inl"')
    start=source.index('    if(transaction.dispatched_request(pending_request)){')
    end=source.index('    reset();return true;',start)
    source=source[:start]+'    if(!cleanup_completion::settle(transaction,observer,opening,reward))return false;\n'+source[end:]
    block='''    if(lancerreload_runtime::stop_requested.load(std::memory_order_acquire)||switch_requested){
        if(stage!=Stage::idle){cleanup(__LINE__);return true;}return false;
    }
'''
    assert source.count(block)==1
    source=source.replace(block,'')
    anchor='    if(stage==Stage::recovering){cleanup(__LINE__);return true;}\n'
    assert source.count(anchor)==1
    source=source.replace(anchor,anchor+block)
    # Root has other preserved deltas. Prove this exact proposed function is
    # present there without copying those unrelated changes into the candidate.
    current=(ROOT/REL).read_text()
    assert 'cleanup_completion::settle(transaction,observer,opening,reward)' in current
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    (TARGET/'source'/REL).write_text(source,encoding='utf-8')
    for relative in (HELPER,TEST):shutil.copy2(ROOT/relative,TARGET/'source'/relative)
    patch=''.join(difflib.unified_diff(original.splitlines(True),source.splitlines(True),
                                     fromfile='a/'+REL.as_posix(),tofile='b/'+REL.as_posix()))
    (ROOT/'docs/hammerburst-reload-cleanup.patch').write_text(patch,encoding='utf-8')
    base=json.loads((BASE/'prepared.json').read_text())
    configure=list(base['configure_command']);configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build=list(base['build_command']);build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:
            subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    test_command=(ROOT/'analysis/hammerburst-20261108/build-reload-cleanup-check.cmd').read_text()
    test_command=test_command.replace(str(ROOT/TEST).replace('/','\\'),str(TARGET/'source'/TEST).replace('/','\\'))
    # Path string spelling on Windows is stable after Path conversion above.
    assert str(TARGET/'source'/TEST).replace('/','\\') in test_command
    cmd=TARGET/'check-frozen.cmd';cmd.write_text(test_command)
    with (TARGET/'test.log').open('wb') as log:
        subprocess.run(['cmd','/c',str(cmd)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    changed=[]
    for name in ('playground','vr-common','third-party','native-capture'):
        for file in (TARGET/'source/tools'/name).rglob('*'):
            if not file.is_file():continue
            relative=file.relative_to(TARGET/'source');old=BASE/'source'/relative
            if not old.exists() or file.read_bytes()!=old.read_bytes():changed.append(relative.as_posix())
    assert sorted(changed)==sorted(x.as_posix() for x in (REL,HELPER,TEST)),changed
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),changed_source_files=changed,full_playground_compiled=True,
               frozen_cleanup_tests_passed=True,runtime_exercised=False,shared_instance_changed=False,
               dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),
               original_runtime_sha256=hashlib.sha256(original.encode()).hexdigest(),
               updated_runtime_sha256=hashlib.sha256(source.encode()).hexdigest(),
               configure_command=configure,build_command=build)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','frozen_cleanup_tests_passed','dll_sha256','changed_source_files')},indent=2))


if __name__=='__main__':main()
