"""Keep validated non-dispatched path observations; actual angle remains separate."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'render-age-pg-r1';P=A/'latch-observation-wait-r1'
def replace(s,a,b):assert s.count(a)==1,a;return s.replace(a,b)
def main():
 if P.exists():
  assert not (P/'build').exists() and not (P/'prepared.json').exists()
  for rel in ('tools/vr-common/weapon_hammerburst_latch.h','tools/playground/weapon_hammerburst_runtime.inl'):
   assert (P/'source'/rel).read_bytes()==(BASE/'source'/rel).read_bytes(),'Only resume the unchanged failed preparation'
 else:
  for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/vr-common/weapon_hammerburst_latch.h';before=(P/'source'/rel).read_text();s=before
 old='''        if(angle_<=.02)return HammerburstLatchMotion::closed;
        if(angle_>=geometry_.open_angle-.02)return HammerburstLatchMotion::open;
        return HammerburstLatchMotion::moving;'''
 s=replace(s,old,'        return observed_motion();')
 s=replace(s,'    double angle() const noexcept {return angle_;}', '''    double angle() const noexcept {return angle_;}
    // Pure result of already validated observations, never a mechanical write.
    HammerburstLatchMotion observed_motion() const noexcept {
        if(angle_<=.02)return HammerburstLatchMotion::closed;
        if(angle_>=geometry_.open_angle-.02)return HammerburstLatchMotion::open;
        return HammerburstLatchMotion::moving;
    }''')
 (P/'source'/rel).write_text(s)
 patches=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 rel='tools/playground/weapon_hammerburst_runtime.inl';before=(P/'source'/rel).read_text();s=before
 s=replace(s,'        const auto stroke_before=stroke;const auto history_before=latch_history::previous;\n','')
 s=replace(s,'''            return; // No seek, mechanical progress, completion or new grab.
        }
        if(moved==gears_weapon::HammerburstLatchMotion::rejected){''','''            if(std::abs(stroke.angle()-angle)<=1e-7)return;
            // A previous accepted segment reached this target but its seek
            // was never dispatched. This current stationary same-grab frame
            // may retry the normal fresh seek; no new motion is inferred.
            moved=stroke.observed_motion();
        }
        if(moved==gears_weapon::HammerburstLatchMotion::rejected){''')
 s=replace(s,'if(wait){stroke=stroke_before;latch_history::previous=history_before;return;}', '''if(wait){
                    // Keep ONLY the validated pure path/sample. `angle` and
                    // stage remain at their actual prior committed values.
                    // Next frame still needs the original grant/context,
                    // original history continuity and fresh owned animation seek.
                    return;
                }''')
 s=replace(s,'sync_ns=%lld no_dispatch=%u\\n','sync_ns=%lld no_dispatch=%u validated_path_kept=%u\\n')
 s=replace(s,'f.controllers.sync_ns,unsigned(wait));fflush(log_file);','f.controllers.sync_ns,unsigned(wait),unsigned(wait));fflush(log_file);')
 (P/'source'/rel).write_text(s);patches+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel));(R/'docs/hammerburst-latch-observation-wait.patch').write_text(patches)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll';proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Proven no-dispatch age-only seek WAIT retains validated pure path observation, not actual mechanical angle/stage. Next fresh same-grab stationary or moving sample uses original seek once. Original history capacity8 and gap/input/identity/mount guards unchanged. Pending setter/unknown failures still cleanup.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
