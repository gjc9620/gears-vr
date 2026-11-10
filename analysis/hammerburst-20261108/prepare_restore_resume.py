"""Freeze the HB restore/continuation chain without unrelated shared edits."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[2];AREA=Path(__file__).resolve().parent
BASE=AREA/'restore-display-r1';TARGET=AREA/'restore-resume-r3'
def change(text,old,new):
    assert text.count(old)==1,old[:100]
    return text.replace(old,new)
def main():
    repairing='--repair-existing' in sys.argv
    if TARGET.exists() and (not repairing or (TARGET/'prepared.json').exists()):raise RuntimeError('Preserve named snapshot')
    for name in ('playground','vr-common','third-party','native-capture'):
        if not (TARGET/'source/tools'/name).exists():shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before;root=(ROOT/rel).read_text()
    after=change(after,'bool restoration_failed{};','bool restoration_failed{};\nunsigned restore_resume_step{};\nstd::uint64_t restore_resume_intent{};\nbool resume_grant_ready{};\nbool restore_native_uncertain{};')
    after=change(after,'    restore_display_step=0;restoration_failed=false;','    restore_display_step=0;restoration_failed=false;\n    restore_resume_step=0;restore_resume_intent=0;resume_grant_ready=restore_native_uncertain=false;')
    def section(text,start,end):
        a=text.index(start);b=text.index(end,a);return text[a:b]
    for start,end in (('void propose(','bool animation_owned('),('bool animation_owned(','std::uint64_t animation_serial()')):
        after=change(after,section(before,start,end),section(root,start,end))
    after=change(after,'#include "weapon_hammerburst_restore_display.inl"',section(root,'#include "weapon_hammerburst_restore_display.inl"','#include "weapon_hammerburst_latch_history.inl"').rstrip())
    after=change(after,'        released=true;held={};stroke.reset();','        released=true;held={};stroke.reset();resume_grant_ready=false;')
    after=change(after,'        if(!released||!granted(f,decision,hand::LeaseKind::ammo,true))return;','        const bool resumed_grant=resume_grant_ready&&granted(f,decision,hand::LeaseKind::ammo)&&hand::same(held,decision.lease);\n        if(!resumed_grant&&(!released||!granted(f,decision,hand::LeaseKind::ammo,true)))return;\n        resume_grant_ready=false;')
    after=change(after,'            const auto window=lancerreload_state::closure_time(f,serial,tick)?lancerreload_state::lease.perfect_end_ms:0;','            std::int64_t accepted_begin{},window{};\n            if(!closure_clock(f,tick,accepted_begin,window))window=0;')
    after=change(after,'serial,tick-lancerreload_state::lease.accepted_begin_ms,window,','serial,tick-accepted_begin,window,')
    after=change(after,'            if(restore_control::display(resumed)==restore_control::DisplayResult::recovery_required){','            const bool failed=restore_display_ready?\n                restore_control::resume(resumed,resumed_decision)==restore_control::ResumeResult::recovery_required:\n                restore_control::display(resumed)==restore_control::DisplayResult::recovery_required;\n            if(failed){')
    after=change(after,'        }\n        // A restored picture is not permission to reuse an old hand action.','        }else released=false;\n        // A restored picture is not permission to reuse an old hand action.')
    after=change(after,'    inv::Request pending_request{};','    if(restore_native_uncertain)return false; // Unknown original dispatch cannot be acknowledged as cleaned.\n    inv::Request pending_request{};')
    after=change(after,'    if(stage!=Stage::idle&&serial){','    if(stage!=Stage::idle&&serial&&!native_exited){')
    needle='        inv::Inventory counts{};\n        if(completion.result=='
    after=change(after,needle,'        if(anim::ownership.active){\n            if(!anim::ownership.visual_only||!(anim::ownership.inventory==opening.identity)||!restore_animation())return true;\n        }\n'+needle)
    (TARGET/'source'/rel).write_text(after)
    patch=''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
    names=['weapon_hammerburst_restore_resume.inl','weapon_hammerburst_restore_resume_test.cpp',
        'weapon_hammerburst_restore_clock.inl','weapon_hammerburst_restore_display_fixture.inl',
        'weapon_hammerburst_restore_display_test.cpp','weapon_hammerburst_restore_fixture.inl',
        'weapon_hammerburst_restore_rebind.inl','weapon_hammerburst_reload_park.inl','weapon_hammerburst_reload_park_test.cpp']
    for name in names:
        relative='tools/playground/'+name;new=(ROOT/relative).read_text();old=BASE/'source'/relative
        if old.exists():patch+=''.join(difflib.unified_diff(old.read_text().splitlines(True),new.splitlines(True),fromfile='a/'+relative,tofile='b/'+relative))
        shutil.copy2(ROOT/relative,TARGET/'source'/relative)
    state_rel='tools/playground/weapon_lancer_reload_state.inl'
    old_state=(BASE/'source'/state_rel).read_text();new_state=old_state
    new_state=change(new_state,'BeginResult begin(hand::Frame const& source,std::uint64_t& serial,Lease const* suspended=nullptr) noexcept {\n    serial=0;',
        'BeginResult begin(hand::Frame const& source,std::uint64_t& serial,Lease const* suspended=nullptr,bool* start_dispatched=nullptr) noexcept {\n    serial=0;\n    if(start_dispatched)*start_dispatched=false;')
    new_state=change(new_state,'    if(call_begin<=0||!start.run())return BeginResult::rejected;',
        '    if(call_begin<=0)return BeginResult::rejected;\n    if(start_dispatched)*start_dispatched=true;\n    if(!start.run())return BeginResult::rejected;')
    (TARGET/'source'/state_rel).write_text(new_state)
    patch+=''.join(difflib.unified_diff(old_state.splitlines(True),new_state.splitlines(True),fromfile='a/'+state_rel,tofile='b/'+state_rel))
    (ROOT/'docs/hammerburst-restore-resume.patch').write_text(patch)
    for stem in ('restore-resume','restore-display','restore-rebind','restore-admission','park-control'):
        command=(AREA/('build-'+stem+'-check.cmd')).read_text().replace(str(ROOT/'tools/playground'),str(TARGET/'source/tools/playground'))
        path=TARGET/(stem+'-check.cmd');path.write_text(command)
        with (TARGET/(stem+'-test.log')).open('wb') as log:subprocess.run(['cmd','/c',str(path)],check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
        print((TARGET/(stem+'-test.log')).read_text(),flush=True)
    flags=json.loads((BASE/'prepared.json').read_text());configure=list(flags['configure_command']);build=list(flags['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+('-repair' if repairing else '')+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll';changed=[rel,state_rel]+['tools/playground/'+n for n in names]
    proof=dict(base=str(BASE),full_playground_compiled=True,frozen_restore_chain_tests_passed=True,
        restoration_display_integrated=True,fresh_intent_resume_integrated=True,retained_game_clock_integrated=True,
        runtime_ready=False,runtime_exercised=False,selected_by_runtime_entry=False,shared_default_enabled=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),changed_source_files=changed,
        configure_command=configure,build_command=build,first_build_failed=repairing,
        first_build_failure='Frozen state API predates begin_for_part; used its existing suspended begin with optional pre-dispatch receipt; retained failed build log' if repairing else None)
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in changed},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','fresh_intent_resume_integrated','dll_sha256')}),flush=True)
if __name__=='__main__':main()
