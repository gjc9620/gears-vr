"""Bounded passive HB first-debt phase costs on the original GT only."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'extraction-history-r1';P=A/'tick-cost-tail-r1'
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 shutil.copy2(A/'hb_tick_cost_tail.h',P/'source/tools/playground/hb_tick_cost_tail.h')
 rel='tools/playground/playground.cpp';before=(P/'source'/rel).read_text();s=before
 needle='#include <array>';assert s.count(needle)==1;s=s.replace(needle,needle+'\n#include "hb_tick_cost_tail.h"')
 needle='        static unsigned equipment_work_logs{},equipment_work_observed{};'
 insert='''        if(vrfire::mode.load(std::memory_order_acquire)==vrfire::running&&nativelancerfollow::bound_is_hammerburst){
            // Existing GT timestamps and state scalars only. The final passive
            // clock is not input/source time and never participates in gates.
            hb_tick_cost_tail::record({native_tick_work_begin,native_tick_original_begin,native_tick_original_end,
                equipment_work_begin,equipment_tick_end,weaponhand::clock_ns(nullptr),
                vrfire::input.last_sync_ns(),vrfire::input.last_observed_ns(),hammerburst_burst_event::event.captured,
                hammerburst_burst_event::event.serial,unsigned(hammerburst_burst_runtime::first_debt),
                unsigned(hammerburst_burst_runtime::cancel_requested),hammerburst_burst_runtime::completion.count(),
                unsigned(vrfire::input.owns_press()),unsigned(weaponhand::client.armed()),
                unsigned(fire_ticked_early),unsigned(reload_ticked_early),unsigned(pre_reload_ticked)});
        }
'''
 assert s.count(needle)==1;s=s.replace(needle,insert+needle);(P/'source'/rel).write_text(s)
 patch=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 rel='tools/playground/weapon_lancer_r2_native.inl';before=(P/'source'/rel).read_text();s=before
 needle='        else mode.store(stopped,std::memory_order_release);'
 assert s.count(needle)==1;s=s.replace(needle,'        else {hb_tick_cost_tail::dump(log_file);mode.store(stopped,std::memory_order_release);}')
 (P/'source'/rel).write_text(s);patch+=''.join(difflib.unified_diff(before.splitlines(True),s.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel));(R/'docs/hammerburst-tick-cost-tail.patch').write_text(patch)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll';proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Passive128-row HB-only existing GT tail: original phase clocks/scalars plus final observation clock, no gate use; dump on same GT before stopped publication after actual debt drain. No worker ring access or new hook, engine/source/input call, writer or action-policy change. Existing extraction-history candidate preserved.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
