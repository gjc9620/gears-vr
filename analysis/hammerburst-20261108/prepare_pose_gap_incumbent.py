"""Keep only the exact existing HB new-magazine incumbent across pose expiry."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1]
BASE=A/'endpoint-baseline-r1';P=A/'pose-gap-incumbent-r1'
HELPER='''// A missing/expired render publication is not a physical release. Preserve
// only this exact already-held new magazine for the next independent fresh
// Native prepare. No Frame, action, clock, grant or motion is produced here.
// Existing insertion/history continuity checks still govern the next sample.
void unavailable_frame(hand::FrameReadObservation const& failure) noexcept {
    const bool pose_gap=failure.failure==hand::FrameReadFailure::publication||
        failure.failure==hand::FrameReadFailure::stale_before;
    if(pose_gap&&stage==Stage::new_magazine&&new_item&&held.kind==hand::LeaseKind::ammo&&
       held.id==new_item&&hand::lease_valid(held,opening.identity.lifecycle)){
        static unsigned rows{};
        if(log_file&&rows<8){++rows;
            fwprintf(log_file,L"hammerburst_pose_gap_incumbent failure=%u frame=%llu sequence=%llu item=%llu action=0\\n",
                unsigned(failure.failure),failure.frame,failure.sequence,new_item);fflush(log_file);}
        return;
    }
    held={};stroke.reset();insertion.reset();extraction.reset();released=false;
}
'''
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before
 needle='bool tick(uptr pc) noexcept {';assert after.count(needle)==1;after=after.replace(needle,HELPER+needle)
 old='''    hand::Frame f{};hand::GripGameState game{};hand::Decision decision{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,decision)||f.controller.address!=pc||'''
 new='''    hand::Frame f{};hand::GripGameState game{};hand::Decision decision{};hand::FrameReadObservation frame_read{};
    if(!weaponhand::client.copy_frame_on_gt(f,game,decision,&frame_read)||f.controller.address!=pc||'''
 assert after.count(old)==1;after=after.replace(old,new)
 old='''        if(stage!=Stage::idle){held={};stroke.reset();insertion.reset();extraction.reset();released=false;
            if(lancerreload_runtime::weapon_retired(opening.identity.weapon))cleanup(__LINE__);return true;}'''
 new='''        if(stage!=Stage::idle){unavailable_frame(frame_read);
            if(lancerreload_runtime::weapon_retired(opening.identity.weapon))cleanup(__LINE__);return true;}'''
 assert after.count(old)==1;after=after.replace(old,new)
 (P/'source'/rel).write_text(after)
 (R/'docs/hammerburst-pose-gap-incumbent.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Exact held HB new-magazine logical incumbent only across publication/stale_before read failure. No stale Frame/action grant, clock extension, actor/input relaxation or changed motion continuity. Other failures and stages retain original reset.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({rel:hashlib.sha256((P/'source'/rel).read_bytes()).hexdigest()},indent=2));print(json.dumps(proof))
if __name__=='__main__':main()
