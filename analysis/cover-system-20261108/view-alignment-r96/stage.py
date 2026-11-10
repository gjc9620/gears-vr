# Derived r80 observation; original __file__ binds original static inputs.
# Actual derived bytes and independent parent are bound by r96 ready/plan.
from pathlib import Path
import sys
__file__ = 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\solo-m1-20261028\\stage_lancer_private.py'
OUT_RUN = Path('C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\cover-system-20261108\\view-alignment-r96')
sys.path.insert(0, str(OUT_RUN))
sys.path.insert(0, 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\tools\\game-playtest')
sys.path.insert(0, 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\solo-m1-20261028')
"""Stage an exact prepared batch into the idle existing private instance only."""
from pathlib import Path
import hashlib,json,shutil,sys,time
from cover_observation_runtime import idle
import cover_launch_preflight
cover_launch_preflight.check(__import__("os").getpid())
O=Path(__file__).resolve().parent;R=O.parents[1]
assert idle(),'A game/XR/recorder is active; do not stage'
f=(O/sys.argv[1]).resolve()
assert f.parent==O and f.name==sys.argv[1]
# Review an already delivered freeze without changing the current development
# checkpoint or silently staging newer unfinished work.
p=json.loads((f/'prepared.json').read_text())
assert Path(p['source_archive']).resolve()==f
# Retention belongs after NORMAL completed recording/encoding, once per run.
# Repeating its full media walk here delayed every source stage by minutes
# even after the previous run's cleanup. Never delete current authoring media
# merely to stage a binary; the existing end-of-run helper remains mandatory.
print(json.dumps(dict(media_retention='Reuse existing post-run helper after recording/encode; no redundant pre-stage sweep')))
manifest=R/'analysis/instances/solo-m1-1028d/manifest.json';m=json.loads(manifest.read_text())
a=O/('prior-private-text-'+time.strftime('%Y%m%d-%H%M%S'));a.mkdir()
for name in ('worker.json','done.json','launcher.log','stop-basic'):
 old=O/name
 if old.exists():assert old.is_file();shutil.copy2(old,a/name)
shutil.copy2(manifest,a/'before-private-manifest.json')
assert idle(),'A game started while archiving; no stage'
# Reuse run_vr_only_hotfix's CREATE_NEW numerical-receipt handoff. New Native
# exports must not fail against an older run's files or be read as that run.
# Move only explicit evidence files, preserving bytes and the prior report.
native=Path(m['native_build']).resolve();assert native.is_relative_to(R.resolve())
numerical=a/'prior-numerical-receipts';saved=[]
for name in ('body-hands-commits.bin','body-held-outcomes.bin','body-c0-budget.bin',
             'body-c0-first-failure.bin','body-c0-first-commit-failure.bin',
             'body-metadata-rejects.bin','body-tick-commits.bin','body-mount-source-commits.bin',
             'body-arm-alpha.bin','body-ready-wrist.bin','head-palette-observations.csv',
             'body-source-rejections.csv','continuous-hand-plans.bin','control-node-phases.bin',
             'cover-pose-phases.bin','fresh-pose-producer.bin'):
 old=native/name
 if not old.exists():continue
 assert old.is_file() and not old.is_symlink() and old.resolve().parent==native
 numerical.mkdir(exist_ok=True);target=numerical/name
 assert target.resolve().is_relative_to(a.resolve()) and not target.exists()
 before=hashlib.sha256(old.read_bytes()).hexdigest();size=old.stat().st_size
 old.rename(target)
 assert hashlib.sha256(target.read_bytes()).hexdigest()==before
 saved.append(dict(name=name,bytes=size,sha256=before))
(a/'numerical-receipts.json').write_text(json.dumps(dict(prior=saved,source=str(native),
    prior_owner_receipt='done.json' if (a/'done.json').exists() else None),indent=2),encoding='utf-8')
assert idle(),'A game started during evidence handoff; no binary stage'
stages=[('native',Path(m['native_build']),m['native_dll'],'native_sha256'),
        ('playground',R/m['playground_build'],m['playground_dll'],'playground_sha256')]
if 'sim_xr' in p['builds']:
 stages.append(('sim_xr',R/m['xr_build'],'OpenXRPoseProbe.exe','xr_sha256'))
for key,folder,name,manifest_hash in stages:
 source=Path(p['builds'][key]['saved']);target=(folder/name).resolve();sha=p['builds'][key]['sha256']
 assert target.is_relative_to(R) and hashlib.sha256(source.read_bytes()).hexdigest()==sha
 if not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest()!=sha:shutil.copy2(source,target)
 assert hashlib.sha256(target.read_bytes()).hexdigest()==sha
 m[manifest_hash]=sha
m['candidate_validation']='Private '+f.name+': focused current Lancer visual repair; simulation and headset acceptance separate'
m['private_main_grip_ab']=p.get('private_diagnostics',{}).get('main_grip_ab',False)
if 'tools/playground/weapon_lancer_interaction_ranges.inl' in p['source_sha256']:
 m['lancer_interaction_ranges']=True
manifest.write_text(json.dumps(m,indent=2),encoding='utf-8')
shutil.copy2(manifest,a/'next-private-manifest.json')
stop=O/'stop-basic'
if stop.exists():assert stop.is_file() and stop.resolve().parent==O;stop.unlink()
print(json.dumps(dict(staged=str(f),text_backup=str(a),public_changed=False)))
