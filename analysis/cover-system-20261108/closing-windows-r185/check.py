from pathlib import Path
import hashlib,json,shutil,subprocess,sys
H=Path(__file__).resolve().parent;R=H.parents[2];sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();rows=[]
for variant in ('before','after'):
 d=H/variant;d.mkdir()
 for p in (R/'tools/game-playtest').glob('*.py'):shutil.copyfile(p,d/p.name)
 shutil.copyfile(H/'probe.py',d/'probe.py')
 if variant=='before':
  for name in ('cover_state.py','test_cover_state.py'):
   original=subprocess.run(['git','show','HEAD:tools/game-playtest/'+name],cwd=R,capture_output=True,check=True).stdout;(d/name).write_bytes(original)
 tests=subprocess.run([sys.executable,'-E','-s','-B','-X','utf8','-m','unittest','test_cover_state'],cwd=d,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(H/(variant+'-test.log')).write_bytes(tests.stdout+tests.stderr)
 run=subprocess.run([sys.executable,'-E','-s','-B','-X','utf8','probe.py'],cwd=d,capture_output=True,creationflags=subprocess.CREATE_NO_WINDOW);(H/(variant+'-probe.log')).write_bytes(run.stdout+run.stderr)
 row=dict(variant=variant,test_exit=tests.returncode,probe_exit=run.returncode,source_sha256=sha(d/'cover_state.py'),test_output=(tests.stdout+tests.stderr).decode('utf-8','replace'),probe_output=run.stdout.decode('utf-8','replace'))
 if not run.returncode:row['counts']=json.loads(run.stdout)
 rows.append(row);print(json.dumps(row),flush=True)
(H/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
assert all(r['test_exit']==r['probe_exit']==0 for r in rows)
comparison=[]
for b,a in zip(rows[0]['counts'],rows[1]['counts']):
 assert a['slots']==b['slots'] and a['rows_sha256']==b['rows_sha256'] and a['witnesses']==b['witnesses'] and a['capture_reads']==b['capture_reads']
 assert a['closing_reads']<b['closing_reads'];comparison.append(dict(slots=a['slots'],before=b['closing_reads'],after=a['closing_reads'],reduction=1-a['closing_reads']/b['closing_reads']))
(H/'comparison.json').write_text(json.dumps(comparison,indent=2)+'\n');print(json.dumps(comparison))
