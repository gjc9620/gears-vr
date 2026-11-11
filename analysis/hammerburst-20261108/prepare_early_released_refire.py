from pathlib import Path
import json,hashlib,shutil,difflib,subprocess
A=Path(__file__).resolve().parent;R=A.parents[1];BASE=A/'history-origin-r1';P=A/'early-released-refire-r1'
HELPER='''// Consume only an EXISTING original continuation after a previously
// certified real UP. New presses/first events remain in the post-Input path.
void pre_original_released_refire(hand::Frame const& f) noexcept {
#if defined(GEARS_PLAYGROUND_HB_STALE_DEBT_WAIT) && GEARS_PLAYGROUND_HB_STALE_DEBT_WAIT
    if(!released_lifetime.epoch||tx::first_debt||!tx::firing_debt||tx::cancel_requested||
       vrfire::input.owns_press()||!owns_pending()||event.kind!=common::policy::EventKind::refire||
       tx::completion.state()!=tx::policy::State::committed||!tx::completion.count()||tx::completion.count()>=6||
       f.controllers.hands[1].trigger_active!=1||f.controllers.hands[1].trigger>.35f||
       !targets(f.weapon.address)||!weaponhand::client.existing_action_lifetime_on_gt(released_lifetime))return;
    const auto serial=event.serial;const auto captured=event.captured;const auto before=tx::completion.count();
    // Original checks, original event/capture and original once-only claim.
    // No Input advance, event capture, clock renewal or alternate dispatcher.
    service(f.controller.address);deliver(f);
    static unsigned rows{};
    if(log_file&&rows<12){++rows;
        fwprintf(log_file,L"hammerburst_pre_original_refire frame=%llu sequence=%llu event=%llu captured_ns=%lld before_count=%u after_count=%u pending_after=%u once_claim=1\\n",
            f.key.frame,f.controllers.sequence,serial,captured,before,tx::completion.count(),unsigned(owns_pending()));fflush(log_file);
    }
#else
    (void)f;
#endif
}
'''
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 patch=''
 def edit(rel,fn):
  nonlocal patch
  p=P/'source'/rel;before=p.read_text();after=fn(before);assert before!=after;p.write_text(after);patch+=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 def event(s):
  assert s.endswith('}\n}\n');return s[:-2]+HELPER+'}\n'
 edit('tools/playground/weapon_hammerburst_burst_event.inl',event)
 edit('tools/playground/weapon_hammerburst_burst_runtime.h',lambda s:s.replace('void deliver(gears_weapon_hand::Frame const&) noexcept;','void deliver(gears_weapon_hand::Frame const&) noexcept;\nvoid pre_original_released_refire(gears_weapon_hand::Frame const&) noexcept;'))
 def caller(s):
  old='''    if(unified_weapon_fire::ready_for(frame,20000000))return;
    nativelancerfollow::tick(frame,nativelancerfollow::Phase::prepare_only);
    if(!unified_weapon_fire::prepare_current(frame,game))unified_weapon_fire::clear_source();'''
  new='''    if(!unified_weapon_fire::ready_for(frame,20000000)){
        nativelancerfollow::tick(frame,nativelancerfollow::Phase::prepare_only);
        if(!unified_weapon_fire::prepare_current(frame,game))unified_weapon_fire::clear_source();
    }
#if defined(GEARS_PLAYGROUND_HB_FINITE_BURST) && GEARS_PLAYGROUND_HB_FINITE_BURST
    hammerburst_burst_event::pre_original_released_refire(frame);
#endif'''
  assert s.count(old)==1;return s.replace(old,new)
 edit('tools/playground/weapon_lancer_r2_native.inl',caller)
 (R/'docs/hammerburst-early-released-refire.patch').write_text(patch)
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command']);configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,scope='Existing pre-original held-muzzle stage may service/deliver only already captured HB refire with prior real-UP lifetime, no owned input/first/cancel/new event. Original service/delivery/claim, source100ms/muzzle20ms/event250ms/counter6 unchanged; no new input or timer.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));print(proof['dll_sha256'])
if __name__=='__main__':main()
