"""Exercise the generated pre-input identity guard with explicit fake sessions."""
import ast
import copy
import json
from pathlib import Path
import tempfile
from prepare import HERE, load, sha


def main():
    proof = load(HERE / 'cover-1110-r105-source-stage/operator-prepared.json')
    path = Path(proof['operator'])
    if sha(path) != proof['operator_sha256']:
        raise RuntimeError('Generated action differs')
    tree = ast.parse(path.read_text(encoding='utf-8'))
    sources = [node.value.value for node in tree.body
               if isinstance(node, ast.Assign) and isinstance(node.value, ast.Constant)
               and isinstance(node.value.value, str) and any(isinstance(t, ast.Name) and t.id == 's' for t in node.targets)]
    menu = next(value for value in sources if 'def live():' in value and 'session_bound=' in value)
    function = next(node for node in ast.parse(menu).body if isinstance(node, ast.FunctionDef) and node.name == 'live')
    code = compile(ast.fix_missing_locations(ast.Module(body=[function], type_ignores=[])), str(path), 'exec')
    with tempfile.TemporaryDirectory(prefix='identity-check-', dir=HERE) as temporary:
        selected = Path(temporary)
        output = selected / 'cover-output'
        output.mkdir()
        identities = [dict(pid=i, creation_time=100+i, image=str(i)) for i in (1, 2, 3)]
        report = dict(session_id='owned', phase='running', game_identity=identities[0],
                      owner_identity=identities[1], xr_identity=identities[2])
        state = dict(instance_root=str(selected), session_report=str(selected/'session.json'),
                     game_identity=identities[0], owner_identity=identities[1])
        alive = {i['pid']:copy.deepcopy(i) for i in identities}
        class Control:
            def identity(self, pid):
                return alive.get(pid)
        scope = dict(json=json, Path=Path, OUT=output, state=copy.deepcopy(state),
                     game=copy.deepcopy(identities[0]), control=Control(),
                     session_bound=copy.deepcopy(report), identities_bound=copy.deepcopy(identities))
        exec(code, scope)
        def evaluate(s, r):
            (selected/'launcher-session.json').write_text(json.dumps(s), encoding='utf-8')
            (selected/'session.json').write_text(json.dumps(r), encoding='utf-8')
            return scope['live']()
        assert evaluate(state, report)
        checks = 1
        for field in ('instance_root', 'session_report'):
            changed = copy.deepcopy(state)
            changed[field] = 'different'
            assert not evaluate(changed, report)
            checks += 1
        for field in ('game_identity', 'owner_identity'):
            changed = copy.deepcopy(state)
            changed[field]['creation_time'] += 1
            assert not evaluate(changed, report)
            checks += 1
        for field in ('game_identity', 'owner_identity', 'xr_identity'):
            changed = copy.deepcopy(report)
            changed[field]['creation_time'] += 1
            assert not evaluate(state, changed)
            checks += 1
        for field, value in (('session_id', 'new'), ('phase', 'stopped')):
            changed = copy.deepcopy(report)
            changed[field] = value
            assert not evaluate(state, changed)
            checks += 1
        for identity in identities:
            alive[identity['pid']]['creation_time'] += 1
            assert not evaluate(state, report)
            alive[identity['pid']] = copy.deepcopy(identity)
            checks += 1
        # An unrelated extra process never changes this own-instance guard.
        alive[99] = dict(pid=99, creation_time=999, image='unrelated game')
        assert evaluate(state, report)
        checks += 1
    result = dict(checks=checks, passed=True, scope='actual generated live() guard; fake identity provider and synthetic reports only',
                  operator_sha256=proof['operator_sha256'], test_source_sha256=sha(__file__),
                  game_started=False, input_published=False, gameplay_accepted=False)
    (HERE/'identity-check.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
