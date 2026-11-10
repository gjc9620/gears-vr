"""Consume only current HB reload before same-tick visual mesh work."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'press-timing-r1';P=A/'reload-priority-r1'
BLOCK='''    // HB's existing action consumer must precede the visual mesh lease.
    // This only selects call order, never grants a Frame/press/lease.
    if(!reload_ticked_early&&hammerburst_runtime::priority_frame(frame)&&
       (frame.left_grip_down||hammerburst_runtime::blocks_fire(frame.weapon))){
        const bool before_hb=hammerburst_runtime::blocks_fire(frame.weapon);
        const bool consumed_hb=hammerburst_runtime::tick(pc);
        // Even a rejected attempt is not retried after visual work this Frame.
        reload_ticked_early=true;
        static unsigned hb_early_rows{};
        if(log_file&&hb_early_rows++<12){
            fwprintf(log_file,L"hammerburst_early_reload frame=%llu sequence=%llu sync_ns=%lld before_active=%u after_active=%u consumed=%u once=1\\n",
                frame.key.frame,frame.controllers.sequence,frame.controllers.sync_ns,unsigned(before_hb),unsigned(hammerburst_runtime::blocks_fire(frame.weapon)),unsigned(consumed_hb));fflush(log_file);
        }
    }
'''
DECL='namespace hammerburst_runtime {bool priority_frame(gears_weapon_hand::Frame const&) noexcept;}\n'
PREDICATE='''bool priority_frame(hand::Frame const& f) noexcept {
    return nativelancerfollow::bound_is_hammerburst&&nativelancerfollow::bound_key_known&&
        hand::same(nativelancerfollow::bound_key.weapon,f.weapon)&&
        nativelancerfollow::bound_key.hold_generation==vrequip::model.hold_generation();
}
'''
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 rel='tools/playground/weapon_equipment_backend.inl';before=(BASE/'source'/rel).read_text()
 anchor='    nativelancerfollow::tick(frame);\n    const auto timing_follow=clock_ns(nullptr);'
 assert before.count(anchor)==1
 after=before.replace(anchor,BLOCK+anchor).replace('namespace nativeequipment {',DECL+'namespace nativeequipment {',1)
 assert after.replace(BLOCK,'').replace(DECL,'')==before
 (P/'source'/rel).write_text(after)
 runtime='tools/playground/weapon_hammerburst_runtime.inl'
 t=(BASE/'source'/runtime).read_text();assert t.count('bool blocks_fire(')==1
 (P/'source'/runtime).write_text(t.replace('bool blocks_fire(',PREDICATE+'bool blocks_fire(',1))
 (ROOT/'docs/hammerburst-reload-priority.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 finish()
def finish():
 assert P.exists() and not (P/'prepared.json').exists(),'Only finish an unbuilt private candidate'
 rel='tools/playground/weapon_equipment_backend.inl'
 before=(BASE/'source'/rel).read_text();after=(P/'source'/rel).read_text()
 assert after.replace(BLOCK,'').replace(DECL,'')==before
 runtime='tools/playground/weapon_hammerburst_runtime.inl'
 t=(P/'source'/runtime).read_text();original=(BASE/'source'/runtime).read_text()
 assert t.replace(PREDICATE,'')==original
 (ROOT/'docs/hammerburst-reload-priority.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))+
  ''.join(difflib.unified_diff(original.splitlines(True),t.splitlines(True),fromfile='a/'+runtime,tofile='b/'+runtime)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),
  dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,
  source_clocks_or_geometry_limits_changed=False,shared_client_changed=False,
  scope='Only exact bound HB current reload consumed once before follow tick/apply, after equipment/fire. Original current/grant/identity/expiry/stock checks unchanged; no failed action retry. Other weapons retain prior order. Includes unexercised feed history.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2))
 (P/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((P/'source'/n).read_bytes()).hexdigest() for n in (rel,runtime)},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','gameplay_accepted')}))
if __name__=='__main__':main()
