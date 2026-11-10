"""Derive an independent entry; never changes the sealed r80 observation."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
OLD = ROOT/'analysis/solo-m1-20261028'
assert not (HERE/'runtime/claim.json').exists(), 'Never regenerate a consumed observation entry'
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
old_path = OLD/'run_cover_eye_r80.py'
assert sha(old_path) == '6709dc5681294dd34c2a26c2e5fa549838be0be0238d4f519e980ed80f91239e'
source = old_path.read_text(encoding='utf-8')


def replace_once(before, after):
    global source
    assert source.count(before) == 1, before
    source = source.replace(before, after)


begin = source.index('if not _PREPARE:\n    window = json.loads(TICKET')
end = source.index("deps_path = TARGET / 'runtime/dependencies.json'", begin)
source = source[:begin] + source[end:]
replace_once('    current = R / relative',
             "    current = (TARGET/'driver-source'/relative if relative == 'analysis/solo-m1-20261028/run_v63_startup_repro.py' else R/relative)")
replace_once("s = (O/'run_v63_startup_repro.py').read_text(encoding='utf-8')",
             "s = (TARGET/'driver-source/analysis/solo-m1-20261028/run_v63_startup_repro.py').read_text(encoding='utf-8')")
replace_once("assert _PREPARE or sys.argv[1:] == ['--run', '--window-ticket', str(TICKET)]",
             "assert _PREPARE or sys.argv[1:] == ['--run']")
claim_line = next(line for line in source.splitlines() if line.startswith('_CLAIM = '))
replace_once(claim_line, '_CLAIM = ' + repr(str(HERE/'runtime/claim.json')))
replace_once("from retain_test_media import idle", "from cover_observation_runtime import idle")
replace_once("'Another owner has a live game/XR/recorder/encode; consumed attempt cannot start'",
             "'Another owner has a live game/XR/recorder; consumed attempt cannot start'")
replace_once("receipt.update(claim=_CLAIM, ticket=str(TICKET), baseline_evidence=window['baseline_evidence'],\n               final_handoff_evidence=window['handoff_evidence'])",
             "receipt.update(claim=_CLAIM, entry_policy='independent actual-start occupancy; no predecessor or FINAL',\n               independent_entry=str(OUT_RUN/'run_cover_eye_independent.py'))")
replace_once("str(O/'stage_lancer_private.py'), TARGET.name", "str(OUT_RUN/'stage.py'), TARGET.name")
replace_once("compile(s, str(O/'run_v63_startup_repro.py'), 'exec')\nif _PREPARE:",
             "assert s.count('from retain_test_media import idle') == 1\ns = s.replace('from retain_test_media import idle', 'from cover_observation_runtime import idle')\ncompile(s, str(O/'run_v63_startup_repro.py'), 'exec')\nif _PREPARE:")
source = source.replace("TARGET/'runtime/", "OUT_RUN/'runtime/")
begin = source.index('finally:\n\n\n    if idle():')
source = source[:begin] + '''finally:
    # Stop/restore is not conditional on successful geometry decoding. Preserve
    # raw evidence now; the independent parent restores the full baseline before
    # any optional numeric or image review.
    if idle():
        config.write_bytes(before)
        receipt['restored'] = config.read_bytes() == before
        if trace_source.exists():
            raw = trace_source.read_bytes()
            saved = OUT_RUN/'runtime/authored-eye-geometry.bin'
            with saved.open('xb') as stream:
                stream.write(raw)
                stream.flush()
                os.fsync(stream.fileno())
            assert sha(saved) == sha(trace_source)
            receipt['eye_geometry'] = dict(path=str(saved), sha256=sha(saved), bytes=len(raw),
                decoded=False, visual_accepted=False, physical_headset_accepted=False)
            receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
            trace_source.unlink()
    receipt['runtime_drivers_still_match'] = all(sha(TARGET/'driver-source'/name if name == 'analysis/solo-m1-20261028/run_v63_startup_repro.py' else R/name) == digest for name,digest in plan['runtime_driver_sha256'].items())
    receipt['complete_baseline_restoration_required'] = True
    receipt_path.write_text(json.dumps(receipt, indent=2), encoding='utf-8')
'''
bootstrap = ('# Derived r80 observation; original __file__ binds original static inputs.\n'
             '# Actual derived bytes and independent parent are bound by r96 ready/plan.\n'
             'from pathlib import Path\nimport sys\n'
             '__file__ = '+repr(str(old_path))+'\n'
             'OUT_RUN = Path('+repr(str(HERE))+')\n'
             'sys.path.insert(0, str(OUT_RUN))\n'
             'sys.path.insert(0, '+repr(str(ROOT/'tools/game-playtest'))+')\n'
             'sys.path.insert(0, '+repr(str(OLD))+')\n')
inner = bootstrap + source
compile(inner, str(HERE/'inner.py'), 'exec')
(HERE/'inner.py').write_text(inner, encoding='utf-8')
stage_path = OLD/'stage_lancer_private.py'
stage = stage_path.read_text(encoding='utf-8')
assert stage.count('from retain_test_media import idle') == 1
stage = stage.replace('from retain_test_media import idle',
                      'from cover_observation_runtime import idle\nimport cover_launch_preflight\ncover_launch_preflight.check(__import__("os").getpid())')
stage_bootstrap = bootstrap.replace(repr(str(old_path)), repr(str(stage_path)))
compile(stage_bootstrap+stage, str(HERE/'stage.py'), 'exec')
(HERE/'stage.py').write_text(stage_bootstrap+stage, encoding='utf-8')
(HERE/'runtime').mkdir(exist_ok=True)
original_plan = json.loads((OLD/'cover-eye-r80-plan.json').read_text(encoding='utf-8'))
prepared = OLD/original_plan['prepared']
originals = dict(prepared=prepared/'prepared.json', plan=OLD/'cover-eye-r80-plan.json',
                 ready=prepared/'runtime-ready.json', entry=old_path,
                 dependencies=prepared/'runtime/dependencies.json')
sources = [HERE/name for name in ('run_cover_eye_independent.py', 'cover_observation_runtime.py',
                                  'inner.py', 'stage.py', 'prepare_entry.py')]
sources += [ROOT/'tools/game-playtest'/name for name in ('cover_launch_preflight.py', 'cover_observation_baseline.py')]
sources += [ROOT/'tools/debug-launch/runtime_stage_guard.py']
plan = dict(interpreter=original_plan['invocation']['interpreter'],
            interpreter_sha256=original_plan['invocation']['interpreter_sha256'],
            original_five={k:dict(path=str(p),sha256=sha(p)) for k,p in originals.items()},
            source_sha256={str(p.relative_to(ROOT)):sha(p) for p in sources},
            game_path=original_plan['selected_game']['path'],
            build_sha256=original_plan['build_sha256'],
            public_read_only=str(Path('J:/gearsVr/candidates/6dof-unified/candidate-state.json').resolve()),
            policy='Independent module; coordinate only actual launch. Existing launcher mutex, real occupancy, own fresh baseline and immediate restore.',
            material_change='Replace obsolete predecessor ticket admission only; same observation Native, phase sequence and source protections.',
            startup_source='Sealed r80 driver-source copy, not the independently updated shared run_v63_startup_repro.py',
            query_grab_or_weapon_actions_added=False, original_r80_claim_consumed=False,
            complete_goal=False, gameplay_accepted=False)
(HERE/'plan.json').write_text(json.dumps(plan,indent=2),encoding='utf-8')
(HERE/'ready.json').write_text(json.dumps(dict(plan_sha256=sha(HERE/'plan.json'),
    entry_sha256=sha(HERE/'run_cover_eye_independent.py'), staged=False, started=False),indent=2),encoding='utf-8')
print(json.dumps(dict(entry_sha256=sha(HERE/'run_cover_eye_independent.py'),
                     plan_sha256=sha(HERE/'plan.json'), ready_sha256=sha(HERE/'ready.json'),
                     original_five=plan['original_five'], staged=False, started=False)))
