"""Revalidate the exact pending refire after a successful actual-UP handoff."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'first-priority-r1';P=A/'release-handoff-r1'
def once(t,a,b):assert t.count(a)==1,a[:100];return t.replace(a,b)
def changes(rel,before):
 if rel.endswith('burst_runtime.h'):
  return once(before,'void after_tick(gears_weapon_hand::Frame const* current_source) noexcept;', 'void after_tick(gears_weapon_hand::Frame const* current_source) noexcept;\nvoid after_release(gears_weapon_hand::Frame const&) noexcept;')
 if rel.endswith('r2_native.inl'):
  return once(before,'    hammerburst_burst_event::after_tick(current?&frame:nullptr);','''    // Only a real owned->released transition may revalidate its exact already
    // pending HB refire. Full Input gates have completed; no new event/edge.
    if(current&&r2_owned_before&&!input.owns_press())hammerburst_burst_event::after_release(frame);
    hammerburst_burst_event::after_tick(current?&frame:nullptr);''')
 after=once(before,'std::uint64_t event_serial{};','''std::uint64_t event_serial{};
struct ReleasedPending {Pending pending{};std::uint64_t frame{};std::int64_t sync{};} released_pending;
void clear_release_handoff() noexcept {released_pending={};}''')
 after=once(after,'    event={};validated=false;\n}','    event={};validated=false;clear_release_handoff();\n}')
 after=once(after,'bool release_completed(hand::Frame const* f) noexcept {\n','bool release_completed(hand::Frame const* f) noexcept {\n    clear_release_handoff();\n')
 after=once(after,'gate(16,tx::need_source(*f))){remember_released_source(*f);return true;}','''gate(16,tx::need_source(*f))){
        remember_released_source(*f);
        if(owns_pending()&&event.kind==common::policy::EventKind::refire)
            released_pending={event,f->key.frame,f->controllers.sync_ns};
        return true;
    }''')
 helper='''// A successful release deliberately revokes the prior service validation.
// Consume this ONE same-frame handoff after Input::advance has finished. It
// supplies no Frame/source/permit; the original service and delivery recheck
// registration, original body/state, event expiry, current muzzle and reserve.
void after_release(hand::Frame const& f) noexcept {
    const auto handoff=released_pending;clear_release_handoff();
    if(!handoff.pending.serial||handoff.frame!=f.key.frame||handoff.sync!=f.controllers.sync_ns||
       vrfire::input.owns_press()||tx::first_debt||!tx::firing_debt||tx::cancel_requested||
       tx::completion.state()!=tx::policy::State::committed||!owns_pending()||
       event.serial!=handoff.pending.serial||event.captured!=handoff.pending.captured||
       event.kind!=common::policy::EventKind::refire||!hand::same(event.function,handoff.pending.function)||
       !right_tracked(f)||f.controllers.hands[1].trigger_active!=1||f.controllers.hands[1].trigger>.35f)return;
    const auto current=tx::cohort(f);
    if(!tx::policy::same(current,tx::pressed)||current.presses!=tx::pressed.presses||
       tx::pressed.releases==(std::numeric_limits<std::uint32_t>::max)()||
       current.releases!=tx::pressed.releases+1)return;
    service(f.controller.address);
    static unsigned rows{};
    if(log_file&&rows<8){
        fwprintf(log_file,L"hammerburst_release_handoff frame=%llu event=%llu captured_ns=%lld observed_ns=%lld validated=%u service_stage=%u pending_same=%u no_dispatch=1\\n",
            f.key.frame,handoff.pending.serial,handoff.pending.captured,weaponhand::clock_ns(nullptr),unsigned(validated),service_stage,
            unsigned(event.serial==handoff.pending.serial&&event.captured==handoff.pending.captured));
        fflush(log_file);++rows;
    }
}
'''
 return once(after,'void deliver(hand::Frame const& f) noexcept {',helper+'void deliver(hand::Frame const& f) noexcept {')
RELS=('tools/playground/weapon_hammerburst_burst_event.inl','tools/playground/weapon_hammerburst_burst_runtime.h','tools/playground/weapon_lancer_r2_native.inl')
def main():
 assert not P.exists(),'Preserve candidate identity'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 patch=[]
 for rel in RELS:
  before=(BASE/'source'/rel).read_text();after=changes(rel,before);(P/'source'/rel).write_text(after)
  patch.extend(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel))
 (ROOT/'docs/hammerburst-release-handoff.patch').write_text(''.join(patch));finish()
def finish():
 assert P.exists() and not (P/'prepared.json').exists()
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,source_clocks_or_geometry_limits_changed=False,scope='Successful real-UP release records exact existing refire serial/captured/frame/sync. Once Input has relinquished original R2, same-frame handoff re-runs full original service, then original after_tick/delivery. No new event/time/source/permit, no dispatch from handoff. Full100ms/20ms/250ms checks retained; pending original times logged.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2));(P/'source-verification.json').write_text(json.dumps({r:hashlib.sha256((P/'source'/r).read_bytes()).hexdigest() for r in RELS},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256')}))
if __name__=='__main__':main()
