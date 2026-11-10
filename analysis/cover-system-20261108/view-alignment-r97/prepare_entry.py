"""A distinct cover approach repair; preserve the consumed r96 attempt."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
PREVIOUS = HERE.parent/'view-alignment-r96'
OLD = ROOT/'analysis/solo-m1-20261028'
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert not (HERE/'runtime/claim.json').exists(), 'Never regenerate a consumed attempt'
assert sha(PREVIOUS/'run_cover_eye_independent.py') == 'b09fc5103bfbef00f92472a7a30a86cadab4517b9919e2d6e9b4e9ce3e60ec44'
operator_path = OLD/'run_cover_eye_observation_operator.py'
assert sha(operator_path) == 'd909f3981bb2537fc0cb80cc6d6e8d47a14fd96c9aea5a52e61d307397cb1a25'
operator = operator_path.read_text(encoding='utf-8')
needle = 'O = Path(__file__).resolve().parent'
assert operator.count(needle) == 1
operator = operator.replace(needle, "O = Path(__file__).resolve().parents[3] / 'analysis/solo-m1-20261028'\nsys.path.insert(0, str(O))")
needle = "m = types.ModuleType('cover_boundary_chain')"
assert operator.count(needle) == 1
injection = '''needle = "        ctx.navigate(approach, arrival_distance=35.)"
assert s.count(needle) == 1
s = s.replace(needle, "        from cover_ordinary_approach import navigate as cover_navigate\\n        cover_navigate(ctx, approach, arrival_distance=35.)")
'''
operator = operator.replace(needle, injection + needle)
compile(operator, str(HERE/'run_cover_eye_approach_operator.py'), 'exec')
(HERE/'run_cover_eye_approach_operator.py').write_text(operator, encoding='utf-8')

entry = (PREVIOUS/'run_cover_eye_independent.py').read_text(encoding='utf-8')
needle = '    gate.check(os.getpid())\n    claim = HERE/'
assert entry.count(needle) == 1
recovery_check = '''    # This is the specifically outstanding GN restoration, not a future
    # predecessor ticket. Do not infer its completion from an idle game/mutex.
    recovery = plan['outstanding_restoration']
    if sha(recovery['baseline']) != recovery['baseline_sha256'] or sha(recovery['claim']) != recovery['claim_sha256']:
        raise RuntimeError('Outstanding original-restoration identity changed')
    original = load(recovery['baseline'])
    resolution = load(recovery['receipt'])
    if (resolution.get('identity') != original['identity']
            or resolution.get('claim_sha256') != recovery['claim_sha256']
            or resolution.get('restored') is not True
            or resolution.get('released_after_restore') is not True
            or resolution.get('public_written') is not False
            or resolution.get('checks') != {row['label']: True for row in original['files']}):
        raise RuntimeError('Known outstanding original restoration is not complete')
'''
entry = entry.replace(needle, recovery_check + needle)
action_reader = '''def action_result(inner, started_wall):
    # Cleanup success is independent of action success. In 56276 the outer
    # child returned 0 while the actual operator returned 1 before cover entry.
    result = dict(observation_sequence_completed=False, gameplay_accepted=False)
    try:
        game = inner['capture_identity']
        capture = Path(inner['capture_identity_receipt'])
        prefix = 'menu-firstperson-' + str(game['pid']) + '-interrupted-cover-'
        if capture.parent.parent != OLD or not capture.parent.name.startswith(prefix):
            raise ValueError('Capture identity path differs')
        stamp = capture.parent.name[len(prefix):]
        if len(stamp) != 15 or stamp[8] != '-' or not stamp.replace('-', '').isdigit():
            raise ValueError('Capture stamp differs')
        actions_path = OLD/('lancer-startup-actions-' + stamp + '.json')
        if actions_path.stat().st_mtime < started_wall:
            raise ValueError('Operator result predates launch')
        actions = load(actions_path)
        result.update(operator_exit=actions.get('exit_code'), operator_error=actions.get('error'),
                      actions_path=str(actions_path), actions_sha256=sha(actions_path))
        current = load(OLD/'menu-firstperson-current.json')
        if current['game'] != game:
            raise ValueError('Action result game differs')
        result['action_error'] = current.get('error')
        directory = Path(current['directory'])
        if directory.parent != OLD or not directory.name.startswith('menu-firstperson-' + str(game['pid']) + '-'):
            raise ValueError('Action directory differs')
        boundary_path = directory/'cover-boundary.json'
        boundary = load(boundary_path)
        phases = boundary.get('eye_observation', [])
        result.update(boundary_path=str(boundary_path), boundary_sha256=sha(boundary_path),
                      observation_phases=len(phases))
        result['observation_sequence_completed'] = (
            type(actions.get('exit_code')) is int and actions['exit_code'] == 0
            and not actions.get('error') and not current.get('error')
            and current.get('checks', {}).get('cover_eye_observation_sequence_completed') is True
            and boundary.get('game') == game and boundary.get('passed') is True
            and len(phases) == 5
            and all(type(p.get('begin_qpc')) is int and type(p.get('end_qpc')) is int
                    and 0 < p['begin_qpc'] <= p['end_qpc'] for p in phases))
    except (OSError, ValueError, KeyError, TypeError) as error:
        result['action_evidence_error'] = repr(error)
    return result


'''
assert entry.count('def main():') == 1
entry = entry.replace('def main():', action_reader + 'def main():')
needle = "        outcome['runtime_released'] = True"
assert entry.count(needle) == 1
entry = entry.replace(needle, needle + "\n        outcome['action'] = action_result(inner, started_wall)")
needle = '    return result\n\n\nif __name__'
assert entry.count(needle) == 1
entry = entry.replace(needle, "    return result if result else (0 if outcome.get('action', {}).get('observation_sequence_completed') else 1)\n\n\nif __name__")
(HERE/'run_cover_eye_independent.py').write_text(entry, encoding='utf-8')
(HERE/'cover_observation_runtime.py').write_bytes((PREVIOUS/'cover_observation_runtime.py').read_bytes())

# Reuse the original reviewed derivation, relocated to this distinct attempt.
# It still certifies old r80 sources/builds, not new current shared changes.
builder = (PREVIOUS/'prepare_entry.py').read_text(encoding='utf-8')
exec(compile(builder, str(HERE/'prepare_entry.py'), 'exec'), {'__file__': str(HERE/'prepare_entry.py')})
inner_path = HERE/'inner.py'
inner = inner_path.read_text(encoding='utf-8')
needle = 's = s.replace(needle, "str(o/\'run_cover_eye_observation_operator.py\')")'
assert inner.count(needle) == 1
inner = inner.replace(needle, 's = s.replace(needle, ' + repr(repr(str(HERE/'run_cover_eye_approach_operator.py'))) + ')')
compile(inner, str(inner_path), 'exec')
inner_path.write_text(inner, encoding='utf-8')
plan_path = HERE/'plan.json'
plan = json.loads(plan_path.read_text(encoding='utf-8'))
for p in (inner_path, HERE/'run_cover_eye_approach_operator.py',
          ROOT/'tools/game-playtest/cover_ordinary_approach.py'):
    plan['source_sha256'][str(p.relative_to(ROOT))] = sha(p)
plan['material_change'] = 'Cover-specific ordinary approach keeps responding .62 amplitude and brakes with .04s pulses instead of .22 amplitude; arrival35, stall3, step42 and original input/identity protections retained. Actual operator failure is reported independently of normal cleanup.'
gn = ROOT/'analysis/gnasher-development-20261109'
plan['outstanding_restoration'] = dict(
    baseline=str(gn/'independent-sg35-r2-owned-run/baseline.json'),
    baseline_sha256='e224c804dc51506b2b81dd3cf1c23a97e8b29db6c4ea85080d5907a382f2f2e1',
    claim=str(gn/'claims/equipment-controls-prepared-20261110-gnasher-sg35-readiness-r2.json'),
    claim_sha256='8f91eb419cc5a218b6fb356b01f6323253c58278136dc7827153e79c3aa02db7',
    receipt=str(gn/'independent-sg35-r2-owned-run/recovery-restoration.json'))
plan_path.write_text(json.dumps(plan, indent=2), encoding='utf-8')
(HERE/'ready.json').write_text(json.dumps(dict(plan_sha256=sha(plan_path),
    entry_sha256=sha(HERE/'run_cover_eye_independent.py'), staged=False, started=False), indent=2), encoding='utf-8')
print(json.dumps(dict(final_entry=sha(HERE/'run_cover_eye_independent.py'),
    final_plan=sha(plan_path), final_ready=sha(HERE/'ready.json'),
    old_r96_consumed_preserved=True, new_attempt_consumed=False, staged=False, started=False)))
