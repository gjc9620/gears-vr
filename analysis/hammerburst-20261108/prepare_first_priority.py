"""Prioritize an already captured HB first event over optional L2 work.

Keep the entire original R2 Input::advance, after_tick and delivery sequence.
Only the existing L2 block changes position for an owned, pending HB first.
"""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1]
BASE=A/'pickup-baseline-r1';P=A/'first-priority-r1'
REL='tools/playground/weapon_lancer_r2_native.inl'
def transform(before):
 start=before.index('    const auto now=weaponhand::clock_ns(nullptr);\n#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)',before.index('const bool hb_pending_before='))
 end=before.index('    const auto previous_sync=',start)
 old=before[start:end]
 assert old.count('if(aim_result==gears_weapon::OriginalL2Status::abort_required)')==1
 body=old[old.index('    // The Native arbiter'):old.rindex('#endif')]
 body=body.replace('now,\n        [&](gears_weapon::OriginalL2Action', 'aim_now,\n        [&](gears_weapon::OriginalL2Action')
 assert 'aim_now,' in body
 body=body.replace('mode.store(fault,std::memory_order_release);return;}', 'mode.store(fault,std::memory_order_release);return false;}')
 body=body.replace('    const bool part_input_reserved=', '    part_input_reserved=')
 body=body.replace('    const auto aim_result=aim.advance(', '    aim_result=aim.advance(')
 body=body.replace('    const bool aim_allowed=', '    aim_allowed=')
 block='''    const auto now=weaponhand::clock_ns(nullptr);
#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    // Only an ALREADY owned/captured HB first may postpone L2 work. R2's
    // complete current-frame/continuity/UP checks still precede delivery.
    // No new edge, source clock, event lifetime or firing permit is created.
    const bool hb_first_before_aim=have_source&&input.owns_press()&&
        hammerburst_burst_event::targets(frame.weapon.address)&&lancerrefire::has_initial_debt();
    bool part_input_reserved{},aim_allowed{};
    gears_weapon::OriginalL2Status aim_result{};
    const auto advance_aim=[&](std::int64_t aim_now) noexcept -> bool {
'''+body+'''        return true;
    };
    if(!hb_first_before_aim&&!advance_aim(now))return;
#endif
'''
 after=before[:start]+block+before[end:]
 anchor='    const auto action_finished=weaponhand::clock_ns(nullptr);'
 assert after.count(anchor)==1
 tail='''#if defined(GEARS_PLAYGROUND_NATIVE_LANCER_L2_TRIAL)
    if(hb_first_before_aim&&!advance_aim(weaponhand::clock_ns(nullptr)))return;
#endif
'''
 after=after.replace(anchor,tail+anchor)
 # Undo only the scheduling transformation: every other production byte must match.
 assert after.replace(block,old).replace(tail,'')==before
 return after,block,body,tail
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):
  shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 before=(BASE/'source'/REL).read_text();after,block,body,tail=transform(before)
 (P/'source'/REL).write_text(after)
 (P/'scheduling-block.json').write_text(json.dumps(dict(block=block,body=body,tail=tail),indent=2))
 (ROOT/'docs/hammerburst-first-priority.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+REL,tofile='b/'+REL)))
 finish()
def finish():
 assert P.exists() and not (P/'prepared.json').exists()
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,source_clocks_or_geometry_limits_changed=False,full_input_checks_before_delivery=True,scope='Owned existing HB first event: postpone existing L2 aim block until after unchanged R2 Input::advance/after_tick/deliver_ready; original20ms/100ms/250ms limits and all production statements retained. Other weapons/new presses keep original order. Deferred L2 uses its actual later admission clock.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2))
 (P/'source-verification.json').write_text(json.dumps({REL:hashlib.sha256((P/'source'/REL).read_bytes()).hexdigest()},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','full_input_checks_before_delivery')}))
if __name__=='__main__':main()
