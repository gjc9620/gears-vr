"""Observe original history rejection data and retain bounded first-debt costs."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'tick-cost-tail-r1';P=A/'history-rejection-r1'
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 shutil.copy2(A/'hb_history_rejection.h',P/'source/tools/playground/hb_history_rejection.h');patch=''
 rel='tools/playground/playground.cpp';before=(P/'source'/rel).read_text();s=before.replace('#include "hb_tick_cost_tail.h"','#include "hb_tick_cost_tail.h"\n#include "hb_history_rejection.h"');assert s!=before;(P/'source'/rel).write_text(s);patch+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 rel='tools/playground/weapon_hammerburst_latch_history.inl';before=(P/'source'/rel).read_text()
 old='    if(!relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment)||segment.count<3)return false;'
 new='''    const bool built=relative::build(previous.raw,sampled.raw,history,control,sampled.mount,sampled.size,segment);
    if(!built||segment.count<3){
        hb_history_rejection::capture(log_file,f.key.frame,f.controllers.sequence,control,built,segment.count,
            previous.raw,sampled.raw,history,sampled.mount,sampled.size);
        return false;
    }'''
 assert before.count(old)==1;s=before.replace(old,new);(P/'source'/rel).write_text(s);patch+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 rel='tools/playground/hb_tick_cost_tail.h';before=(P/'source'/rel).read_text();s=before
 s=s.replace('inline std::uint64_t total{};','inline std::uint64_t total{};\ninline std::array<Row,64> first_rows{};\ninline std::array<std::uint64_t,64> first_ordinals{};\ninline std::uint64_t first_seen{};\ninline unsigned first_kept{};')
 old='    rows[total%rows.size()]=row;++total;';new='''    rows[total%rows.size()]=row;++total;
    if(row.first_debt&&row.event&&row.captured>0){
        if(first_seen!=UINT64_MAX)++first_seen;
        if(first_kept<first_rows.size()){first_rows[first_kept]=row;first_ordinals[first_kept]=total;++first_kept;}
    }''';assert s.count(old)==1;s=s.replace(old,new)
 begin=s.index('    for(auto n=total-count;n<total;++n)');end=s.index('    std::fflush(file);',begin)
 old=s[begin:end]
 new=old.replace('for(auto n=total-count;n<total;++n){const auto& r=rows[n%rows.size()];','for(unsigned series=0;series<2;++series){\n      const auto begin=series?std::uint64_t(0):total-count;const auto end=series?std::uint64_t(first_kept):total;\n      for(auto n=begin;n<end;++n){const auto& r=series?first_rows[n]:rows[n%rows.size()];\n        const auto ordinal=series?first_ordinals[n]:n+1;')
 new=new.replace('L"hammerburst_tick_cost ordinal=', 'L"%ls ordinal=').replace('            n+1,r.begin,','            series?L"hammerburst_first_tick_cost":L"hammerburst_tick_cost",ordinal,r.begin,')
 new+='    }\n    std::fwprintf(file,L"hammerburst_first_tick_summary seen=%llu retained=%u capacity=64 truncated=%u action=0\\n",first_seen,first_kept,unsigned(first_seen>first_kept));\n'
 s=s[:begin]+new+s[end:];(P/'source'/rel).write_text(s);patch+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel));(R/'docs/hammerburst-history-rejection.patch').write_text(patch)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll';proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Passive post-rejection original raw histories (max4, no requery/clock), original build invoked once and unchanged result. Separate first64 pending-first cost observations alongside128 tail on same GT; no source/input/dispatch/timer/motion predicate changes.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
