"""One frozen HB ordinary-empty-reload attempt; --prepare-only never stages or consumes admission."""
import os
import sys

# This deliberately precedes argument, ticket, source and dependency validation.
# The parent is created offline. Never unlink/recreate a consumed attempt.
_CLAIM = 'C:\\Users\\11\\.codex\\worktrees\\6dof-single\\gearsVr\\analysis\\hammerburst-20261108\\standalone-r4\\claims\\empty-acquisition-contract.json'
_PREPARE = sys.argv[1:] == ['--prepare-only']
def consume_attempt():
    _fd = os.open(_CLAIM, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    try:
        os.write(_fd, ('{"state":"consumed_unverified","pid":%d}\n' % os.getpid()).encode('ascii'))
        os.fsync(_fd)
    finally:
        os.close(_fd)
# END_EXCLUSIVE_BOOTSTRAP
from pathlib import Path
import hashlib,json,os,socket,subprocess,sys,time,importlib.util
ROOT=Path(__file__).resolve().parents[2];E=ROOT/'analysis/hammerburst-20261108';O=ROOT/'analysis/solo-m1-20261028'
PREFIX='standalone-r4'

def load(p):return json.loads(Path(p).read_text(encoding='utf-8-sig'))

def sha(p):
 with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def clean_environment(environment):
 return {key:value for key,value in environment.items() if not key.upper().startswith(('GEARS','PYTHON','XR','OPENXR'))}

def bind_attempt(identity,proof_reference,binding):
 path=Path(_CLAIM);before=load(path)
 if before!=dict(state='consumed_unverified',pid=os.getpid()):raise RuntimeError('Exclusive attempt owner changed')
 linked=dict(before,state='consumed_baseline_verified',identity=identity,baseline_evidence=proof_reference,predecessor_binding=binding)
 with path.open('r+',encoding='utf-8') as stream:
  if json.load(stream)!=before:raise RuntimeError('Exclusive claim raced')
  stream.seek(0);json.dump(linked,stream,indent=2);stream.truncate();stream.flush();os.fsync(stream.fileno())
 if load(path)!=linked:raise RuntimeError('Attempt binding did not persist')
 return linked

def verify_attempt(linked):
 if load(_CLAIM)!=linked or sha(linked['baseline_evidence']['path'])!=linked['baseline_evidence']['sha256']:
  raise RuntimeError('Restoration responsibility changed')

def prepare():
 ready_path=E/'standalone-r4-ready.json';ready=load(ready_path);ready['_actual_sha256']=sha(ready_path);plan_path=Path(ready['plan']);assert sha(plan_path)==ready['plan_sha256'];plan=load(plan_path)
 assert socket.gethostname().casefold()==plan['host']['computer_name'].casefold()
 assert ROOT.resolve()==Path(plan['host']['worktree']).resolve() and plan['host']['codex_host_id']=='local'
 assert sha(__file__)==plan['entry_sha256']==ready['entry_sha256']
 target=Path(plan['candidate']);assert target.parent==O and target.name=='hammerburst-prepared-20261110-empty-acquisition-contract'
 assert sha(target/'prepared.json')==ready['prepared_sha256']==plan['prepared_sha256'];record=load(target/'prepared.json')
 for rel,digest in record['source_sha256'].items():assert sha(target/'source'/rel)==digest,rel
 for rel,digest in record['native_source_sha256'].items():assert sha(target/'native-source'/rel)==digest,rel
 for key,b in record['builds'].items():assert sha(b['saved'])==b['sha256'],key
 assert sha(plan['runtime_inputs_receipt'])==plan['runtime_inputs_sha256'];inputs=load(plan['runtime_inputs_receipt'])
 for path,digest in inputs['sha256'].items():assert sha(path)==digest,path
 boundary=load(plan['runtime_python_boundary']['path']);assert sha(plan['runtime_python_boundary']['path'])==plan['runtime_python_boundary']['sha256']
 for rel,digest in boundary['files'].items():assert sha(ROOT/rel)==sha(Path(boundary['archive'])/rel)==digest,rel
 for path,digest in plan['entry_dependencies'].items():assert sha(path)==digest,path
 host_path=Path(plan['host_dependencies']['path']);assert sha(host_path)==plan['host_dependencies']['sha256'];host_deps=load(host_path)
 for path,digest in host_deps['files'].items():assert sha(path)==digest,path
 for name,expected in host_deps['module_origins'].items():
  module=importlib.import_module(name);assert Path(module.__spec__.origin).resolve()==Path(expected['path']).resolve();assert sha(module.__spec__.origin)==expected['sha256']
 import PIL,numpy
 assert PIL.__version__==host_deps['versions']['Pillow'] and numpy.__version__==host_deps['versions']['numpy']
 assert sha(plan['selected_game_path'])==plan['selected_game_sha256']
 assert Path(sys.executable).resolve()==Path(plan['review_interpreter']['path']).resolve() and sha(sys.executable)==plan['review_interpreter']['sha256']
 return ready,plan,record

def verify_prebirth_failure(done,worker,session):
 if done.get('session_report') or not isinstance(done.get('launcher_exit_code'),int) or done['launcher_exit_code']==0:
  raise RuntimeError('Not an explicit failed startup without a runtime report')
 if done.get('remaining_owned_game','unknown') is not None or done.get('launcher_pid')!=worker.get('launcher_pid'):
  raise RuntimeError('Startup ownership or game absence is unproven')
 owner=session.get('owner_identity')
 if not isinstance(owner,dict) or owner.get('pid')!=worker.get('launcher_pid') or not owner.get('creation_time') or not owner.get('image'):
  raise RuntimeError('Missing exact startup launcher identity')
 if session.get('phase')!='incomplete' or session.get('game_identity') or not session.get('error') or session.get('reservation_failure_cleanup',{}).get('status')!='no_birth_receipt':
  raise RuntimeError('Game-free reservation failure is unproven; retain manual cleanup duty')
 return dict(owner_identity=owner,startup_error=session['error'],game_born=False,launcher_exit=done['launcher_exit_code'])

def main():
 assert Path.cwd().resolve()==ROOT
 if not (sys.flags.optimize==0 and sys.flags.dont_write_bytecode and sys.flags.ignore_environment and sys.flags.no_user_site and sys.flags.utf8_mode and not sys.flags.isolated):raise RuntimeError('Exact -E -s -B -X utf8 without -O/-I required')
 assert _PREPARE or sys.argv[1:]==['--run']
 cleaned=clean_environment(os.environ);os.environ.clear();os.environ.update(cleaned)
 ready,plan,record=prepare()
 os.environ.update(plan['environment'])
 if _PREPARE:
  print(json.dumps(dict(prepared=Path(plan['candidate']).name,host=socket.gethostname(),static_passed=True,staged=False,started=False,shared_runtime_queried=False,once_claimed=False)));return 0
 sys.path.insert(0,str(O));from retain_test_media import idle
 assert idle(),'Shared runtime occupied; no baseline or stage'
 consume_attempt()
 identity=dict(prepared=Path(plan['candidate']).name,prepared_sha256=ready['prepared_sha256'],plan_sha256=ready['plan_sha256'],ready_sha256=ready['_actual_sha256'],entrypoint_sha256=sha(__file__))
 store=E/'standalone-r4'/('prestage-'+str(time.time_ns()));store.mkdir()
 proof=dict(identity=identity,before_stage=True,files={})
 for i,item in enumerate(plan['baseline_files']):
  file=Path(item['path']);assert not file.is_symlink()
  exists=file.exists();raw=file.read_bytes() if exists else None;saved=store/(str(i)+'.raw') if exists else None
  if exists:
   with saved.open('xb') as out:out.write(raw);out.flush();os.fsync(out.fileno())
  proof['files'][str(file)]=dict(exists=exists,sha256=hashlib.sha256(raw).hexdigest() if exists else None,saved=str(saved) if exists else None)
 proof_path=store/'baseline.json'
 with proof_path.open('x',encoding='utf-8') as out:json.dump(proof,out,indent=2);out.flush();os.fsync(out.fileno())
 window=dict(baseline_evidence=dict(path=str(proof_path),sha256=sha(proof_path)))
 predecessor_binding=dict(policy='owner-independent-startup-coordination-only',no_predecessor_required=True)
 queue_path=E/'standalone-r4/queue.json';queue=dict(candidate=Path(plan['candidate']).name,state='independent_owned_not_staged',window_owned=True)
 specs={x['path']:x for x in plan['baseline_files']};assert set(proof['files'])==set(specs) and len(specs)==10
 baseline={'files':[]}
 for path,item in proof['files'].items():
  file=Path(path);assert not file.is_symlink() and type(item['exists']) is bool
  row=dict(path=path,exists=item['exists'],sha256=item['sha256'],public_read_only=specs[path]['public_read_only'])
  if item['exists']:
   saved=Path(item['saved']);assert saved.resolve().is_relative_to(E.resolve());assert sha(file)==sha(saved)==item['sha256'];row['backup']=str(saved)
  else:assert not file.exists()
  baseline['files'].append(row)
 linked_attempt=bind_attempt(identity,window['baseline_evidence'],predecessor_binding)
 helper=Path(plan['selection_helper']);spec=importlib.util.spec_from_file_location('hb_existing_selection_helper',helper);selection=importlib.util.module_from_spec(spec);spec.loader.exec_module(selection)
 selection.validate(dict(game_path=plan['selected_game_path'],game_sha256=plan['selected_game_sha256'],settings_path=plan['selected_settings_path']),ROOT)
 sys.path.insert(0,str(O));from retain_test_media import idle
 assert idle(),'Runtime active: no claim/stage'
 assert idle(),'Runtime appeared: no stage'
 verify_attempt(linked_attempt)
 stage=subprocess.run([sys.executable,'-E','-s','-B','-X','utf8',str(O/'stage_lancer_private.py'),queue['candidate']],cwd=ROOT,capture_output=True,text=True)
 (E/(PREFIX+'-stage.log')).write_text(stage.stdout+stage.stderr,encoding='utf-8');assert stage.returncode==0,stage.stderr
 queue.update(state='running_owned',staged=True,started_ns=time.time_ns(),baseline=window['baseline_evidence']['path']);queue_path.write_text(json.dumps(queue,indent=2))
 instance=ROOT/'analysis/instances/solo-m1-1028d';config=instance/'config.json';config.write_bytes(selection.config_bytes(config.read_bytes(),dict(game_path=plan['selected_game_path'],game_sha256=plan['selected_game_sha256'],settings_path=plan['selected_settings_path']),ROOT))
 env=dict(os.environ)
 for name in plan['clear_environment']:env.pop(name,None)
 env.update(plan['environment'])
 with (E/(PREFIX+'-run.log')).open('w',encoding='utf-8') as log:
  launched_wall=time.time();deadline=time.monotonic()+plan['maximum_seconds'];process=subprocess.Popen([sys.executable,'-E','-s','-B','-X','utf8',str(O/'run_hammerburst_check.py')],cwd=ROOT,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)
  while process.poll() is None and time.monotonic()<deadline:time.sleep(.25)
  deadline_stop=False
  if process.poll() is None:
   sys.path.insert(0,str(ROOT/'tools/native-capture'));from run_stereo_continuous import WindowsControl
   worker_path=O/'worker.json';worker=load(worker_path);session=load(instance/'launcher-session.json');owner=session.get('owner_identity')
   assert worker_path.stat().st_mtime>=launched_wall and owner and owner['pid']==worker['launcher_pid'] and WindowsControl().identity(owner['pid'])==owner
   assert load(instance/'manifest.json')['playground_sha256']==record['builds']['playground']['sha256']
   (O/'stop-basic').write_text('Owned HB230535360second boundary; existing normal stop requested',encoding='utf-8');deadline_stop=True
  result=process.wait(timeout=120)
 done_path=O/'done.json';done=load(done_path);worker=load(O/'worker.json');assert done_path.stat().st_mtime>=launched_wall
 if not done.get('session_report'):
  failed_path=instance/'launcher-session.json';assert failed_path.stat().st_mtime>=launched_wall
  failure=verify_prebirth_failure(done,worker,load(failed_path))
  assert idle(),'Startup failure not idle: no restoration'
  verify_attempt(linked_attempt);checks=selection.restore(baseline,ROOT)
  receipt=dict(failure,exit_code=result or done['launcher_exit_code'],at_ns=time.time_ns(),startup_prebirth_verified=True,baseline_checks=checks,normal_game_cleanup_not_applicable=True,paired_review_encode_not_applicable=True,game_success_not_inferred=True,review_retention_handoff_still_required=True)
  (E/(PREFIX+'-run-exit.json')).write_text(json.dumps(receipt,indent=2))
  queue.update(state='startup_failed_restored_released',window_owned=False,runner_exit=receipt['exit_code'],startup_error=failure['startup_error']);queue_path.write_text(json.dumps(queue,indent=2))
  print(json.dumps(receipt));return receipt['exit_code']
 report_path=Path(done['session_report']);report=load(report_path);assert report_path.stat().st_mtime>=launched_wall
 assert report['owner_identity']['pid']==worker['launcher_pid']
 assert Path(report['game_identity']['image']).resolve()==Path(plan['selected_game_path']).resolve()
 assert report['diagnostics_artifacts']['native_sha256']==record['builds']['native']['sha256'] and report['diagnostics_artifacts']['xr_sha256']==record['builds']['sim_xr']['sha256']
 assert report['native_cleanup_verified'] and report['entry_restore_verified'] and report['vr_gamepad_cleanup']['restored'] and report['unified_weapons_cleanup']['drained']
 assert report['xr_exit_code']==0 and done['launcher_exit_code']==0 and done['remaining_owned_game'] is None
 assert idle(),'Runtime not idle: no restoration'
 verify_attempt(linked_attempt);checks=selection.restore(baseline,ROOT)
 receipt=dict(exit_code=result,at_ns=time.time_ns(),deadline_stop=deadline_stop,game=report['game_identity'],session_report=str(report_path),normal_cleanup_verified=True,baseline_checks=checks,paired_review_encode_retention_still_required=True,game_success_not_inferred=True)
 (E/(PREFIX+'-run-exit.json')).write_text(json.dumps(receipt,indent=2));queue.update(state='runtime_released_offline_review_pending',window_owned=False,runner_exit=result,runtime_report=str(report_path),game=report['game_identity']);queue_path.write_text(json.dumps(queue,indent=2));print(json.dumps(receipt));return result

if __name__=='__main__':
 if _PREPARE:raise SystemExit(main())
 sys.path.insert(0,str(ROOT/'tools/debug-launch'))
 from runtime_stage_guard import RuntimeGuard
 with RuntimeGuard('solo-m1-1028d'):
  raise SystemExit(main())
