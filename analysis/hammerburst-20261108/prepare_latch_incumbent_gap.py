"""Preserve an already-observed latch grab across unavailable pose publication."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'latch-observation-wait-r1';P=A/'latch-incumbent-gap-r1'
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/playground/weapon_hammerburst_runtime.inl';before=(P/'source'/rel).read_text();s=before
 old='''    if(pose_gap&&stage==Stage::new_magazine&&new_item&&held.kind==hand::LeaseKind::ammo&&
       held.id==new_item&&hand::lease_valid(held,opening.identity.lifecycle)){'''
 new='''    const bool same_lifecycle=hand::lease_valid(held,opening.identity.lifecycle);
    const bool held_new_magazine=stage==Stage::new_magazine&&new_item&&held.kind==hand::LeaseKind::ammo&&held.id==new_item&&same_lifecycle;
    // Only an already observed actual latch grab, never acquisition/hover.
    // The next original prepare independently checks current input/token,
    // tracking, identities and mount before granting any fresh Frame/action.
    const bool held_latch=(stage==Stage::opening||stage==Stage::closing)&&
        !released&&stroke.active()&&held.kind==hand::LeaseKind::magazine&&same_lifecycle;
    if(pose_gap&&(held_new_magazine||held_latch)){'''
 assert s.count(old)==1;s=s.replace(old,new)
 old='''item=%llu action=0\\n",
                unsigned(failure.failure),failure.frame,failure.sequence,new_item);'''
 new='''item=%llu action=0 stage=%u latch=%u\\n",
                unsigned(failure.failure),failure.frame,failure.sequence,held.id,unsigned(stage),unsigned(held_latch));'''
 assert s.count(old)==1;s=s.replace(old,new)
 s=s.replace('// only this exact already-held new magazine for the next independent fresh','// only this exact held new magazine or observed latch for the next independent fresh')
 (P/'source'/rel).write_text(s);(R/'docs/hammerburst-latch-incumbent-gap.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll';proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,source_sha256={rel:hashlib.sha256((P/'source'/rel).read_bytes()).hexdigest()},scope='Only publication/stale_before keeps an already-observed active opening/closing magazine-latch lease at original lifecycle, not released. No acquisition/old-mag extension. Fresh Native prepare/commit still checks actual input/token/tracking/identity/mount; no Frame/action/time created, original stroke/history/source limits unchanged. Existing exact new-mag incumbent policy preserved.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
