"""Execute the actual replacement on strict success and all refusal kinds."""
from pathlib import Path
import ast
import hashlib
import json
from instrument import instrument, NEEDLE

H = Path(__file__).resolve().parent
R = H.parents[2]
source = (R/'analysis/solo-m1-20261028/cover_boundary_chain.py').read_text()
changed = instrument(source)
ast.parse(changed)
assert changed.count('q.finish()') == source.count('q.finish()')
try:
    instrument(changed)
except ValueError:
    pass
else:
    raise AssertionError('Double instrumentation accepted')

# The same replacement executes here, including its bare re-raise and durable
# save. The query supplies only the existing closing record; any new read fails.
prefix = "def run(q, report, save):\n    try:\n        for address in [123]:\n"
snippet = prefix + NEEDLE + "\n                pass\n    finally:\n        report['finally_ran'] = True\n"
env = dict(slots={'rows': []}, composed=[])
exec(compile(instrument(snippet), 'actual-candidate-wrapper', 'exec'), env)

class Query:
    def __init__(self, detail=None, error=None):
        self.closing_rejection, self.error, self.calls = detail, error, 0
    def finish(self):
        self.calls += 1
        if self.error:
            raise self.error
    def read(self, *args):
        raise AssertionError('Diagnostic added a read')

rows = []
for kind in ('identity', 'metadata', 'registry', 'unknown', 'success'):
    detail = None if kind in ('unknown', 'success') else dict(kind=kind, address=123, before={'value': 1}, after={'value': 2})
    error = None if kind == 'success' else RuntimeError('original strict refusal')
    query = Query(detail, error)
    report, saved = {}, []
    def save():
        saved.append(json.loads(json.dumps(report)))
    try:
        env['run'](query, report, save)
    except RuntimeError as caught:
        assert caught is error
    else:
        assert error is None
    assert query.calls == 1 and report['finally_ran']
    if error:
        assert len(saved) == 1
        observed = saved[0]['candidate_closing_rejection']
        assert observed['difference'] == detail and observed['difference_missing_is_unknown'] == (detail is None)
        assert 'finally_ran' not in saved[0]
    else:
        assert not saved and 'candidate_closing_rejection' not in report
    rows.append(dict(case=kind, passed=True, strict_refusal_preserved=bool(error), finish_calls=query.calls))

result = dict(cases=rows, source_sha256=hashlib.sha256((R/'analysis/solo-m1-20261028/cover_boundary_chain.py').read_bytes()).hexdigest(),
              instrument_sha256=hashlib.sha256((H/'instrument.py').read_bytes()).hexdigest(),
              original_finish_calls=source.count('q.finish()'), transformed_finish_calls=changed.count('q.finish()'),
              added_game_reads=False, added_input=False, added_retry=False, game_started=False)
(H/'results.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result))
