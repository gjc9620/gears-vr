"""Build/test a new CoreAudio helper and emit a local hash provenance receipt.

Run in an x64 MSVC developer shell. Does not execute the CoreAudio helper or any UI.
No fixed binary hash: stage/launch verify the trusted build receipt plus inventory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

SOURCES = ('CMakeLists.txt', 'game_audio_guard.cpp', 'audio_guard_policy.h',
           'audio_guard_policy_test.cpp', 'build_helper.py')

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def build(source, build_dir, output, cmake, generator, ninja=None):
    source, build_dir, output = (Path(p).resolve() for p in (source, build_dir, output))
    if build_dir.exists(): raise FileExistsError('Use a new helper build directory')
    helper=output/'GearsTestAudioGuard.exe'; receipt=output/'audio-helper-provenance.json'
    if helper.exists() or receipt.exists(): raise FileExistsError('Helper/receipt already exists')
    source_hashes={name:sha(source/name) for name in SOURCES}
    configure=[str(cmake),'-S',str(source),'-B',str(build_dir),'-G',generator,'-DCMAKE_BUILD_TYPE=Release']
    if ninja:configure += ['-DCMAKE_MAKE_PROGRAM='+str(ninja)]
    commands=[configure,[str(cmake),'--build',str(build_dir),'--config','Release'],
              [str(cmake),'--build',str(build_dir),'--target','test','--config','Release']]
    # ctest executable beside explicitly selected cmake; no PATH lookup.
    commands[2]=[str(Path(cmake).with_name('ctest.exe')),'--test-dir',str(build_dir),'-C','Release','--output-on-failure']
    for command in commands:subprocess.run(command,check=True)
    if source_hashes!={name:sha(source/name) for name in SOURCES}:raise RuntimeError('Helper sources changed during build')
    binary=build_dir/'GearsTestAudioGuard.exe'
    if not binary.is_file():binary=build_dir/'Release/GearsTestAudioGuard.exe'
    digest=sha(binary)
    cache=(build_dir/'CMakeCache.txt').read_text(encoding='utf-8')
    toolchain=[line for line in cache.splitlines() if line.startswith(('CMAKE_CXX_COMPILER:',
        'CMAKE_GENERATOR:', 'CMAKE_MAKE_PROGRAM:', 'CMAKE_BUILD_TYPE:', 'CMAKE_GENERATOR_INSTANCE:'))]
    value=dict(schema=1,policy='manual_game_session_v1',helper=helper.name,helper_sha256=digest,
        source_sha256=source_hashes,commands=commands,toolchain=toolchain,
        pure_tests_passed=True,helper_executed=False,gui_started=False)
    output.mkdir(parents=True,exist_ok=True)
    with helper.open('xb') as stream:stream.write(binary.read_bytes())
    if sha(binary)!=digest or sha(helper)!=digest:raise RuntimeError('Helper changed during copy')
    with receipt.open('x',encoding='utf-8') as stream:json.dump(value,stream,indent=2);stream.write('\n')
    return value

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,required=True)
    parser.add_argument('--output-dir',type=Path,required=True)
    parser.add_argument('--cmake',type=Path,required=True)
    parser.add_argument('--generator',default='Ninja')
    parser.add_argument('--ninja',type=Path)
    args=parser.parse_args()
    print(json.dumps(build(Path(__file__).resolve().parent,args.build_dir,args.output_dir,
        args.cmake,args.generator,args.ninja)))
if __name__=='__main__':main()
