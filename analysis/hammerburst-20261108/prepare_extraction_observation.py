"""Observe original extraction once, retaining every original gate and result."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'pose-gap-incumbent-r1';P=A/'extraction-observe-r1'
HELPER='''void observe_original_extraction(hand::Frame const& f,std::uint64_t item,
    gears_weapon::FeedSample const& sample,std::int64_t gap,double depth,double lateral,double aligned) noexcept {
    const bool accepted=extraction.observe(sample,feed); // Exact original call, once.
    static unsigned rows{};
    if(log_file&&rows<32){++rows;
        fwprintf(log_file,L"hammerburst_extraction_observed frame=%llu sequence=%llu item=%llu accepted=%u complete=%u gap_ns=%lld depth=%.9g lateral=%.9g aligned=%.9g sync_ns=%lld\\n",
            f.key.frame,f.controllers.sequence,item,unsigned(accepted),unsigned(extraction.complete()),gap,
            depth,lateral,aligned,f.controllers.sync_ns);fflush(log_file);
    }
}
'''
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before
 needle='gears_weapon::FeedResult insertion_step(hand::Frame const& f,std::uint64_t item) noexcept {'
 assert after.count(needle)==1;after=after.replace(needle,HELPER+needle)
 old='if(stage==Stage::old_magazine&&!extraction.complete())extraction.observe(sample,feed);'
 new='if(stage==Stage::old_magazine&&!extraction.complete())observe_original_extraction(f,item,sample,sample_gap_ns,depth,lateral,aligned);'
 assert after.count(old)==1;after=after.replace(old,new);(P/'source'/rel).write_text(after)
 (R/'docs/hammerburst-extraction-observation.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,observation_only=True,scope='32 bounded original extraction observe results; same conditional and exact original call once. No geometry/gap/identity/input/transaction changes. Includes unexercised new-mag pose-gap incumbent repair.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({rel:hashlib.sha256((P/'source'/rel).read_bytes()).hexdigest()},indent=2));print(dll)
if __name__=='__main__':main()
