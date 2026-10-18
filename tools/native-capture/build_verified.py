"""Fresh MSVC/Ninja build with probed showIncludes and fail-closed header-deps proof.

No source changes, game launches or existing build reuse. Run with explicit build
options; the receipt proves parsed dependencies, not runtime correctness.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]

def vc_environment(vcvars,run=subprocess.run):
    vcvars=Path(vcvars).resolve()
    if not vcvars.is_file():raise FileNotFoundError(vcvars)
    if any(char in str(vcvars) for char in '\r\n"%&|<>^!'):raise ValueError('Unsafe vcvars path')
    # /u makes the builtin set output UTF16; never print or persist its contents.
    result=run('"'+os.environ.get('COMSPEC','cmd.exe')+'" /d /u /s /c "call "'+str(vcvars)+'" >nul && set"',stdout=subprocess.PIPE,stderr=subprocess.PIPE,check=True)
    env={}
    for line in result.stdout.decode('utf-16-le').splitlines():
        if '='in line and not line.startswith('='):
            key,value=line.split('=',1);env[key]=value
    if not any(key.casefold()=='path'for key in env):raise RuntimeError('vcvars environment was not captured')
    # Apply AFTER vcvars, but installed localized-only CL may ignore1033.
    # The raw compiler probe below is authoritative, not this preference.
    env['VSLANG']='1033';env['PYTHONUTF8']='1'
    return env

def probe_prefix(build, env):
    tool=next((value for key,value in env.items()if key.casefold()=='vctoolsinstalldir'),None)
    if tool is None:raise RuntimeError('Missing VCToolsInstallDir after vcvars')
    compiler=Path(tool)/'bin/Hostx64/x64/cl.exe'
    folder=Path(build)/'showincludes-probe';folder.mkdir()
    header=folder/'verified_header.inl';header.write_text('constexpr int verified_value=1;\n')
    source=folder/'verified.cpp';source.write_text('#include "verified_header.inl"\nint proof(){return verified_value;}\n')
    command=[str(compiler),'/nologo','/showIncludes','/c',str(source),'/Fo'+str(folder/'verified.obj')]
    result=subprocess.run(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (folder/'compiler.raw.log').write_bytes(result.stdout)
    if result.returncode:raise RuntimeError('Compiler prefix probe failed; see raw log')
    path=str(header).encode('utf-8')
    lines=[line for line in result.stdout.splitlines()if path.lower()in line.lower()]
    if len(lines)!=1:raise RuntimeError('No unique compiler showIncludes header path')
    raw=lines[0][:lines[0].lower().index(path.lower())]
    prefix=raw.decode('utf-8')  # Refuse an unknown byte encoding; no guessed mojibake.
    if not prefix or '\r'in prefix or '\n'in prefix or ']==]'in prefix:raise RuntimeError('Invalid compiler prefix')
    hook=Path(build)/'verified-msvc-prefix.cmake'
    # CMake's compiler auto-detection can overwrite -D language prefix variables.
    # This project-end hook executes after that detection, before Ninja generation.
    hook.write_text('set(CMAKE_CL_SHOWINCLUDES_PREFIX [==['+prefix+']==])\n',encoding='utf-8')
    return prefix,compiler,hook,command

def verified_prefix(build, expected, compiler):
    build=Path(build)
    rules=(build/'CMakeFiles/rules.ninja').read_text(encoding='utf-8')
    match=re.search(r'^msvc_deps_prefix = (.*)$',rules,re.M)
    if match is None or match.group(1)!=expected:
        raise RuntimeError('Ninja prefix differs from actual compiler bytes; refuse build')
    cache=(build/'CMakeCache.txt').read_text(encoding='utf-8')
    for language in ('C','CXX'):
        match=re.search(r'^CMAKE_'+language+r'_COMPILER:FILEPATH=(.*)$',cache,re.M)
        if match and Path(match.group(1).strip()).resolve()!=compiler.resolve():
            raise RuntimeError('CMake compiler differs from prefix probe compiler')
    return expected

def dependency_proof(text,required):
    count=re.search(r'#deps (\d+)',text)
    if count is None or int(count.group(1))==0:raise RuntimeError('Ninja recorded zero/missing dependencies')
    paths={line.strip().replace('\\','/').casefold()for line in text.splitlines()[1:]if line.strip()}
    missing=[name for name in required if not any(path.endswith('/'+name.replace('\\','/').casefold())for path in paths)]
    if missing:raise RuntimeError('Required header not tracked: '+', '.join(missing))
    return dict(count=int(count.group(1)),required=list(required))

def build_verified(source,build,cmake,ninja,vcvars,defines,targets,obj,required):
    source,build,cmake,ninja=map(lambda p:Path(p).resolve(),(source,build,cmake,ninja))
    if build.exists():raise FileExistsError('Use a NEW build directory; stale prefix/deps caches are refused')
    if any(not re.fullmatch(r'[A-Za-z_][A-Za-z0-9_]*=.*',value)for value in defines):raise ValueError('Expected NAME=VALUE defines')
    if any(not re.fullmatch(r'[A-Za-z0-9_.+-]+',value)for value in targets):raise ValueError('Invalid target')
    if Path(obj).is_absolute()or '..'in Path(obj).parts:raise ValueError('Contained object path required')
    if any(value.startswith('CMAKE_PROJECT_INCLUDE=')for value in defines):raise ValueError('Existing project include override refused')
    env=vc_environment(vcvars)
    build.mkdir(parents=True)
    prefix,compiler,hook,probe_command=probe_prefix(build,env)
    commands=[probe_command]
    def run(command,label):
        commands.append(list(map(str,command)))
        result=subprocess.run(command,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        # Preserve raw bytes; UTF8 decoding is only for readable logs, not Ninja's parser.
        (build/(label+'.raw.log')).write_bytes(result.stdout)
        text=result.stdout.decode('utf-8',errors='replace')
        (build/(label+'.log')).write_text(text,encoding='utf-8')
        if result.returncode:raise RuntimeError(label+' failed; see '+str(build/(label+'.log')))
        return text
    configure=[str(cmake),'-S',str(source),'-B',str(build),'-G','Ninja',
        '-DCMAKE_BUILD_TYPE=Release','-DCMAKE_MAKE_PROGRAM='+str(ninja),'-DCMAKE_PROJECT_INCLUDE='+str(hook)]+['-D'+value for value in defines]
    run(configure,'configure')
    prefix=verified_prefix(build,prefix,compiler)
    run([str(cmake),'--build',str(build),'--parallel','2','--target',*targets],'build')
    text=run([str(ninja),'-C',str(build),'-t','deps',obj],'dependencies')
    receipt=dict(schema=1,VSLANG='1033',vcvars_applied_before_language=True,showincludes_prefix=prefix,
        prefix_utf8_hex=prefix.encode('utf-8').hex(),compiler=str(compiler),project_include_hook=str(hook),
        source=str(source),build=str(build),object=obj,dependencies=dependency_proof(text,required),commands=commands,
        rules_sha256=hashlib.sha256((build/'CMakeFiles/rules.ninja').read_bytes()).hexdigest(),
        limits='Dependency parsing verified only; no game/runtime acceptance or source coherence claim.')
    (build/'verified-build.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    return receipt

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir',type=Path,default=ROOT/'tools/native-capture')
    parser.add_argument('--build-dir',type=Path,required=True)
    parser.add_argument('--cmake',type=Path,default=Path('E:/CMake/bin/cmake.exe'))
    parser.add_argument('--ninja',type=Path,default=Path('E:/vs/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe'))
    parser.add_argument('--vcvars',type=Path,default=Path('E:/vs/VC/Auxiliary/Build/vcvars64.bat'))
    parser.add_argument('--define',action='append',default=[])
    parser.add_argument('--target',action='append',default=[])
    parser.add_argument('--object',default='CMakeFiles/GearsCapture.dir/capture.cpp.obj')
    parser.add_argument('--dependency',action='append',default=[])
    args=parser.parse_args()
    print(json.dumps(build_verified(args.source_dir,args.build_dir,args.cmake,args.ninja,args.vcvars,args.define,
        args.target or ['GearsCapture'],args.object,args.dependency or ['ngx_stereo_host_live.inl','ngx_control_capture.inl'])))
if __name__=='__main__':main()
