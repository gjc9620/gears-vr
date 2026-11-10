"""Exercise only the runner's conservative pre-birth refusal classification."""
from pathlib import Path
import importlib.util,json,sys
H=Path(__file__).resolve().parent;folder=H.parent/'isolated-r105'
sys.path.insert(0,str(folder))
spec=importlib.util.spec_from_file_location('cover_runner_compat',folder/'run.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
messages=(
    ('RuntimeError: Two automatic instances already own this 32GiB machine; old',True),
    ('runtime_receipts.ResourceUnavailable: Two automatic instances already own this 32GiB machine; waiting expired',True),
    ('runtime_receipts.ResourceUnavailable: Insufficient currently available memory for another game; waiting expired',True),
    ('RuntimeError: Insufficient currently available memory for another game;',False),
    ('OSError: registration failed after spawn',False),
)
count=0
for message,expected in messages:
    result=dict(start_exit=1,start_stdout='',start_stderr=message)
    assert m.prebirth_resource_refusal(result,False,False)==expected;count+=1
    if expected:
        for worker,session in ((True,False),(False,True)):
            assert not m.prebirth_resource_refusal(result,worker,session);count+=1
        assert not m.prebirth_resource_refusal(dict(result,start_stdout='unexpected'),False,False);count+=1
        assert not m.prebirth_resource_refusal(dict(result,start_exit=0),False,False);count+=1
import types
class ResourceUnavailable(RuntimeError):pass
def refused():raise ResourceUnavailable('No free worker slot')
def unexpected():raise RuntimeError('Bad process identity')
fixture=types.SimpleNamespace(ResourceUnavailable=ResourceUnavailable,require_memory=lambda:dict(active_instances=[]))
assert m.advisory_memory(fixture)==dict(active_instances=[]);count+=1
fixture.require_memory=refused
assert m.advisory_memory(fixture)==dict(admitted=False,advisory_refusal='No free worker slot',authoritative_gate='isolated_sim.first_admission');count+=1
fixture.require_memory=unexpected
try:m.advisory_memory(fixture);raise AssertionError('Unexpected failure swallowed')
except RuntimeError as error:assert str(error)=='Bad process identity';count+=1
record=dict(checks=count,failures=0,scope='No process, game, resource reservation, or startup invoked')
(H/'admission-compat-check.json').write_text(json.dumps(record,indent=2)+'\n')
print(json.dumps(record))
