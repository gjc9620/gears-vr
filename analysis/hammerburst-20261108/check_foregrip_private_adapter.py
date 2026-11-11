from pathlib import Path
import ast,hashlib,json,shutil
from unittest.mock import patch
import hammerburst_isolated_operator as op
from run_isolated_empty import reload_test_mode
A=Path(__file__).resolve().parent;P=A/'foregrip-adapter-tests';P.mkdir(exist_ok=True)
for flags in ((),('--hands',),('--weapon',)):assert op.reader_arguments('read_private_skeleton.py',('sample.json',*flags))==flags
for name,args in [('unknown.py',('x.json',)),('read_private_skeleton.py',('../x.json',)),('read_private_skeleton.py',('x.json','--hands','--weapon')),('read_private_skeleton.py',('x.json','--write')),('read_aim_metrics.py',('x.json','--hands')),('read_private_skeleton.py',('x.json','--hands','--hands'))]:
 try:op.reader_arguments(name,args)
 except ValueError:pass
 else:raise AssertionError((name,args))
binding=object.__new__(op.Binding);binding.name='hb-offline-foregrip';binding.instance=P/'owned-instance';binding.instance.mkdir(exist_ok=True);binding.output=binding.instance/'actions';binding.state=lambda:{}
with patch.object(op.subprocess,'run') as run:
 binding.helper('read_private_skeleton.py','sample.json','--hands')
 assert run.call_args.args[0][-1]=='--reader-flag=--hands' and run.call_count==1
authored=('tools/vr-common/weapon_hammerburst_foregrip.h','tools/vr-common/hand_finger_pose.h')
for rel in authored:
 target=P/'source'/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(A/'early-released-refire-r1/source'/rel,target)
source_hashes={rel:hashlib.sha256((P/'source'/rel).read_bytes()).hexdigest() for rel in authored}
proof=dict(candidate_root=str(P),dll_sha256='offline-test-identity',source_sha256=source_hashes)
manifest=dict(playground_sha256='offline-test-identity')
def write_proof():
 (binding.instance/'hb-source-binding.json').write_text(json.dumps(proof));(binding.instance/'manifest.json').write_text(json.dumps(manifest))
write_proof();assert binding.source_text(authored[0])==(P/'source'/authored[0]).read_bytes().decode('utf-8-sig')
for mutation in ('dll','hash','path','unlisted'):
 old=json.loads(json.dumps(proof));old_manifest=dict(manifest)
 if mutation=='dll':manifest['playground_sha256']='another'
 if mutation=='hash':proof['source_sha256'][authored[0]]='wrong'
 if mutation=='path':proof['candidate_root']=str(A.parent)
 write_proof()
 try:binding.source_text('tools/vr-common/not-allowed.h' if mutation=='unlisted' else authored[0])
 except (ValueError,RuntimeError):pass
 else:raise AssertionError(mutation)
 proof=old;manifest=old_manifest;write_proof()
sources=op.check_sources();s=op.reader_source('read_private_skeleton.py')
assert 'private-instance.json' not in s and 'Weap_AssaultRifle' not in s and 'rn==3' in s
assert "expected=['b_HB_Root','b_HB_MagLatch','b_HB_Mag']" in s
assert 'count==(3 if weapon_mode else 115)' in s and 'r.read(rp,9*68)' not in s and 'r.read(rp,3*68)' in s
assert 'r.identity(address)==identity' in s and "control.identity(game['pid'])==game" in s
assert reload_test_mode([])['empty_only']=='1' and reload_test_mode(['--foregrip-four-state'])['empty_only']=='0'
try:reload_test_mode(['--foregrip-four-state','--focused-loaded-replacement'])
except AssertionError:pass
else:raise AssertionError('mixed scope')
support=op.support_source();assert "g.BOUND.source_text('tools/vr-common/weapon_hammerburst_foregrip.h')" in support
assert "neutral_socket_no_weapon_steering_degrees" in support and 'neutral_wrist_follows_actual_weapon' in support and 'release_returns_to_neutral_wrist' in support
proof_result=dict(passed=True,operator_sha256=hashlib.sha256(Path(op.__file__).read_bytes()).hexdigest(),compiled_sources=sources,reader_flag_and_output_negatives=6,pinned_source_negatives=4,skeleton_exact_HB_class_and3parts=True,body115_and_identity_brackets_preserved=True,default_full_reload_unchanged=True,game_started=False,limits='Adapter compilation and guarded binding/reader-scope tests only. Actual four-state mesh/wrist output still needs game evidence.')
(A/'foregrip-private-adapter-checks.json').write_text(json.dumps(proof_result,indent=2));print(json.dumps(proof_result))
