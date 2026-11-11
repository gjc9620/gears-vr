from pathlib import Path
import hashlib,json,re,shutil,subprocess,sys
H=Path(__file__).resolve().parent;R=H.parents[2];base=H.parent/'link-path-proof-r183/native-source';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
manifest=json.loads((base.parent/'native-source-manifest.json').read_text())['files']
for p,d in manifest.items():assert sha(base/p)==d
out=H/'checks-v4';out.mkdir()
sys.path.insert(0,str(R/'tools/native-capture'));from build_verified import vc_environment
env=vc_environment('E:/vs/VC/Auxiliary/Build/vcvars64.bat');rows=[]
for variant in ('before','after'):
 source=out/variant;shutil.copytree(base,source);target=source/'tools/native-capture'
 for p in (R/'tools/native-capture').glob('*_test.cpp'):shutil.copyfile(p,target/p.name)
 if variant=='after':
  for name in ('cover_physics_query_live.inl','cover_query_guard_observation.h','cover_query_timing.h'):shutil.copyfile(R/'tools/native-capture'/name,target/name)
 prefix=out/(variant+'-link')
 cmd=['E:/vs/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64/cl.exe','/nologo','/std:c++17','/O2','/W4','/WX','/EHsc','/UNDEBUG','/MT','/I'+str(source/'tools/third-party/minhook/include'),str(target/'cover_query_owner_bracket_test.cpp'),'/Fo:'+str(prefix)+'.obj','/Fe:'+str(prefix)+'.exe','/sourceDependencies',str(prefix)+'-dependencies.json']
 b=subprocess.run(cmd,env=env,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-build.log').write_bytes(b.stdout+b.stderr);row=dict(variant=variant,build_exit=b.returncode)
 if not b.returncode:
  v=subprocess.run([str(prefix)+'.exe'],capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);Path(str(prefix)+'-test.log').write_bytes(v.stdout+v.stderr);row.update(test_exit=v.returncode,output=v.stdout.decode('utf-8','replace'))
  deps=json.loads(Path(str(prefix)+'-dependencies.json').read_text(encoding='utf-8-sig'))['Data']['Includes'];row['outside_worktree_dependencies']=[p for p in deps if Path(p).resolve().is_relative_to(R) and not Path(p).resolve().is_relative_to(source)];assert not row['outside_worktree_dependencies']
 rows.append(row);print(json.dumps(row),flush=True)
(out/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
assert all(r['build_exit']==r.get('test_exit')==0 for r in rows)
counts={r['variant']:{k:int(v) for k,v in re.findall(r'owner_bracket (full_calls|reads)=(\d+)',r['output'])} for r in rows}
# full_calls and reads share one output line; capture both explicitly.
for r in rows:
 match=re.search(r'owner_bracket full_calls=(\d+) reads=(\d+)',r['output']);assert match
 counts[r['variant']]=dict(full_calls=int(match[1]),reads=int(match[2]))
comparison={k:dict(before=v,after=counts['after'][k],reduction=1-counts['after'][k]/v) for k,v in counts['before'].items()}
assert all(r['after']<r['before'] for r in comparison.values());(out/'comparison.json').write_text(json.dumps(comparison,indent=2)+'\n');print(json.dumps(comparison))
