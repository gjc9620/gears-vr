"""Use only authenticated original producer points for old-mag extraction."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'latch-incumbent-gap-r1';P=A/'extraction-history-r1'
def replace(s,a,b):assert s.count(a)==1,a;return s.replace(a,b)
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 patches=''
 rel='tools/vr-common/weapon_hammerburst_magazine.h';before=(P/'source'/rel).read_text();s=replace(before,'    bool complete() const noexcept {return complete_;}', '    bool complete() const noexcept {return complete_;}\n    FeedSample const* previous_sample() const noexcept {return started_?&previous_:nullptr;}');(P/'source'/rel).write_text(s)
 patches+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 rel='tools/vr-common/weapon_hammerburst_extraction_history.h';shutil.copy2(A/'extraction_history_policy.h',P/'source'/rel)
 extra='''bool advance_extraction(hand::Frame const& f,gears_weapon::FeedSample const& current) noexcept {
    gate=4;point_count=0;
    if(stage!=Stage::old_magazine||prior.item!=current.item||
       !latch_history::same_context(previous,sampled))return false;
    unsigned control{};
    if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))control=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))control=gears_vr::control_trigger;
    else return false;
    gate=5;relative::Segment segment{};
    if(!relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment)||segment.count<3)return false;
    std::array<gears_weapon::FeedSample,9> points{};points[0]=prior;gate=6;
    for(unsigned i=1;i<segment.count;++i){const auto sequence=segment.points[i].sequence;
        input::TrackedInput const* raw=nullptr;
        for(unsigned n=0;n<history.count;++n)if(history.frames[n].input.sequence==sequence)raw=&history.frames[n];
        P left{};if(!raw||!project_pose(*raw,sampled,left))return false;
        points[i]=current;points[i].sequence=sequence;points[i].monotonic_ns=raw->input.sync_ns;points[i].left_grip=left;
    }
    gate=7;if(!gears_hammerburst_feed_history::same_sample(points[segment.count-1],current))return false;
    points[segment.count-1]=current;
    if(!gears_hammerburst_extraction_history::advance(extraction,prior,current,points.data(),segment.count,feed))return false;
    point_count=segment.count;gate=0;return true;
}
'''
 rel='tools/playground/weapon_hammerburst_feed_history.inl';before=(P/'source'/rel).read_text();s=replace(before,'void remember(gears_weapon::FeedSample const& sample)',extra+'void remember(gears_weapon::FeedSample const& sample)');(P/'source'/rel).write_text(s);patches+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 rel='tools/playground/weapon_hammerburst_runtime.inl';before=(P/'source'/rel).read_text();s=replace(before,'#include "../vr-common/weapon_hammerburst_feed_history.h"','#include "../vr-common/weapon_hammerburst_feed_history.h"\n#include "../vr-common/weapon_hammerburst_extraction_history.h"')
 s=replace(s,'    const bool accepted=extraction.observe(sample,feed); // Exact original call, once.', '''    const bool history_used=gap>feed.maximum_sample_gap_ns&&feed_history::advance_extraction(f,sample);
    const bool accepted=history_used||extraction.observe(sample,feed); // Original fallback exactly once.
    if(gap>feed.maximum_sample_gap_ns&&log_file){static unsigned history_rows{};if(history_rows++<16){
        fwprintf(log_file,L"hammerburst_extraction_history frame=%llu sequence=%llu previous_sequence=%llu used=%u gate=%u points=%u gap_ns=%lld complete=%u action=0\\n",
            f.key.frame,f.controllers.sequence,feed_history::prior.sequence,unsigned(history_used),feed_history::gate,
            feed_history::point_count,gap,unsigned(extraction.complete()));fflush(log_file);}}''')
 s=replace(s,'    if(stage==Stage::new_magazine)feed_history::capture(f,left);','    if(stage==Stage::new_magazine||stage==Stage::old_magazine)feed_history::capture(f,left);')
 s=replace(s,'    if(stage==Stage::new_magazine)feed_history::remember(sample);else feed_history::clear();','    if(stage==Stage::new_magazine||stage==Stage::old_magazine)feed_history::remember(sample);else feed_history::clear();')
 (P/'source'/rel).write_text(s);patches+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel));(R/'docs/hammerburst-extraction-history.patch').write_text(patches)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll';proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Old extraction only: exact existing producer history/mapping/context/control checks, contiguous original samples, per-point original100ms and original extraction predicates. Trial copy commits only after current endpoint matches; missing/malformed history uses original direct rejection. No interpolation/Frame/grant/clock/gap change, no old-mag incumbent extension; insertion implementation unchanged.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
