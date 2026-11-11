"""Wait only after a proven pre-dispatch expiry; restore pure trajectory state."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'extraction-observe-r1';P=A/'latch-seek-gap-r1'
HELPER='''bool expired_before_latch_dispatch(hand::Frame const& f,std::int64_t begin,std::int64_t end) noexcept {
    if(!anim::ownership.active||!anim::ownership.stopped||anim::ownership.seek_pending||
       anim::ownership.serial!=animation_serial()||!(anim::ownership.inventory==opening.identity)||
       begin<=0||end<begin||end<f.now_ns)return false;
    const auto expired=[&](std::int64_t stamp) noexcept {return stamp>0&&end>=stamp&&end-stamp>100000000;};
    return expired(f.controllers.received_ns)||expired(f.stereo.received_ns);
}
'''
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before
 needle='void step(hand::Frame const& f,hand::Decision const& decision) noexcept {'
 assert after.count(needle)==1;after=after.replace(needle,HELPER+needle)
 needle='        const auto previous_ms=stroke.last_observation_ms();'
 assert after.count(needle)==1;after=after.replace(needle,'        const auto stroke_before=stroke;const auto history_before=latch_history::previous;\n'+needle)
 old='''            last_latch_seek_elapsed_ns=weaponhand::clock_ns(nullptr)-began;
            if(!sought){cleanup(__LINE__);return;}'''
 new='''            const auto ended=weaponhand::clock_ns(nullptr);last_latch_seek_elapsed_ns=ended-began;
            if(!sought){
                const bool wait=expired_before_latch_dispatch(f,began,ended);
                static unsigned rows{};
                if(log_file&&rows<16){++rows;
                    fwprintf(log_file,L"hammerburst_latch_seek_unavailable frame=%llu sequence=%llu stage=%u wait=%u seek_pending=%u previous_angle=%.9g wanted_angle=%.9g begin_ns=%lld end_ns=%lld sync_ns=%lld no_dispatch=%u\\n",
                        f.key.frame,f.controllers.sequence,unsigned(stage),unsigned(wait),unsigned(anim::ownership.seek_pending),
                        angle,stroke.angle(),began,ended,f.controllers.sync_ns,unsigned(wait));fflush(log_file);}
                if(wait){stroke=stroke_before;latch_history::previous=history_before;return;}
                cleanup(__LINE__);return;
            }'''
 assert after.count(old)==1;after=after.replace(old,new);(P/'source'/rel).write_text(after)
 (R/'docs/hammerburst-latch-seek-gap.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Only same active/stopped animation inventory+serial, no dispatched SetPosition, original source age>100ms and monotonic clock may restore pre-observation pure stroke/history and wait. No write on stale data, prior setter/event replay, grant/clock renewal or changed motion limits. Other seek failures retain cleanup.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({rel:hashlib.sha256((P/'source'/rel).read_bytes()).hexdigest()},indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
