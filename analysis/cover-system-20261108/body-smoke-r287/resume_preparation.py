"""Resume only the terminated stdlib bootstrap preparation; never starts a game."""
from pathlib import Path
import ast,contextlib,hashlib,importlib.util,io,json,sys
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent
HERE=ROOT/'analysis/cover-system-20261108/isolated-r105';NAME='cover-1112-r287-l2-grip';selected=ROOT/'analysis/instances'/NAME;output=selected/'cover-output'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def replace(s,a,b):
 assert s.count(a)==1,a;return s.replace(a,b)
def write(p,s):
 with p.open('x',encoding='utf-8') as f:f.write(s)
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
assert not (HERE/NAME/'operator-prepared.json').exists() and not (FOLDER/'run.py').exists()
source=(FOLDER/'prepare_operator_body.py').read_text(encoding='utf-8')
source=replace(source,"    with path.open('x', encoding='utf-8') as stream:\n        stream.write(wrapper)","    assert path.read_text(encoding='utf-8')==wrapper, 'Existing wrapper must be identical for preparation resume'")
source=replace(source,"    with adapter.open('x', encoding='utf-8') as stream:\n        stream.write(adapter_source.read_text(encoding='utf-8'))","    assert adapter.read_text(encoding='utf-8')==adapter_source.read_text(encoding='utf-8'), 'Existing adapter must be identical'")
generator=FOLDER/'resume_operator_v2.py';write(generator,source);ast.parse(source)
sys.path.insert(0,str(HERE));op=module('cover_operator_bootstrap_resume',generator)
original=op.write_new
def write_same(path,value):
 path=Path(path)
 if path.exists():
  assert path.read_text(encoding='utf-8')==value,(str(path),'prepared content changed')
  return
 return original(path,value)
op.write_new=write_same;sys.argv=[str(generator),'--name',NAME];log=io.StringIO()
with contextlib.redirect_stdout(log):op.main()
write(FOLDER/'prepare-operator-resume.log',log.getvalue())
tool=output/'cover_isolated_tool.py';runner=(HERE/'run.py').read_text(encoding='utf-8')
runner=replace(runner,'from prepare import ROOT, HERE, OLD, sha, load, write_new','sys.path.insert(0,'+repr(str(HERE))+')\nfrom prepare import ROOT, HERE, OLD, sha, load, write_new')
runner=replace(runner,"TOOLS = ROOT / 'tools/multi-instance/isolated_sim.py'",'TOOLS = Path('+repr(str(tool))+')')
runner=replace(runner,"choices=('sphere','path','shape'), default='sphere'","choices=('none',), default='none'")
run_path=FOLDER/'run.py';write(run_path,runner);ast.parse(runner)
native=ROOT/'analysis/cover-system-20261108/grip-observation-r284/native-build-v2/GearsCapture.dll';assert sha(native)=='c4055b2136e42319e9ed9a1d28ae93155bcc542edaa0c9b6c02eff7f3a036e4d'
info=dict(name=NAME,native_sha256=sha(native),entry_sha256=sha(output/'cover_body_entry.py'),tool_sha256=sha(tool),runner_sha256=sha(run_path),operator_template_sha256=sha(FOLDER/'operator-template.py'),prepared_only=True,game_started=False,physics_enabled=False,resumed_after_bootstrap_timeout=True,preparation_scope='Original stdlib child timeout was terminal; repeat startup passed in same cwd; generated sources must match existing bytes semantically; no guards removed')
write(FOLDER/'prepared.json',json.dumps(info,indent=2)+'\n');print(json.dumps(info))
