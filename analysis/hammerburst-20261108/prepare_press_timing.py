"""Bounded first-press preparation timing on the exact feed-history candidate."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'feed-history-r1';P=A/'press-timing-r1'
def once(t,a,b):assert t.count(a)==1,a[:90];return t.replace(a,b)
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 helper='tools/playground/weapon_hammerburst_press_timing.inl';shutil.copy2(ROOT/helper,P/'source'/helper)
 rel='tools/playground/weapon_lancer_r2_native.inl';before=(BASE/'source'/rel).read_text();after=before
 signature='gears_weapon::OriginalR2Execution execute(gears_weapon::OriginalR2Action action,std::uint64_t controller,wh::Frame const* source,'
 after=once(after,signature,'#include "weapon_hammerburst_press_timing.inl"\n'+signature)
 after=once(after,'    lr::Game const* tick_game=nullptr) noexcept {\n    auto reject=',
  '    lr::Game const* tick_game=nullptr) noexcept {\n    HammerburstPressTiming hb_timing(action==gears_weapon::OriginalR2Action::press&&nativelancerfollow::bound_is_hammerburst,source);\n    auto reject=')
 after=once(after,'        lancermain_asset::Evidence asset{};','        hb_timing.mark(2);\n        lancermain_asset::Evidence asset{};')
 after=once(after,'        hb_press=observed_kind==lancermain_asset::HeldAsset::hammerburst;',
  '        hb_timing.mark(3);\n        hb_press=observed_kind==lancermain_asset::HeldAsset::hammerburst;')
 after=once(after,'        if(hb_press&&!hammerburst_burst_event::prepare_press(*source))return gears_weapon::OriginalR2Execution::not_ready;',
  '        if(hb_press&&!hammerburst_burst_event::prepare_press(*source))return gears_weapon::OriginalR2Execution::not_ready;\n        hb_timing.mark(4);')
 after=once(after,'    // All attachment and ButtonName reflection precedes the last freshness',
  '    hb_timing.mark(5);\n    // All attachment and ButtonName reflection precedes the last freshness')
 after=once(after,'        unified_weapon_fire::ReadinessEvidence evidence{};',
  '        hb_timing.mark(6);\n        unified_weapon_fire::ReadinessEvidence evidence{};')
 after=once(after,'    const bool owned_press=action==gears_weapon::OriginalR2Action::press;',
  '    hb_timing.mark(6);\n    const bool owned_press=action==gears_weapon::OriginalR2Action::press;')
 after=once(after,'    const bool invoked=call.run();','    const bool invoked=call.run();\n    hb_timing.invoked=invoked;hb_timing.mark(7);')
 # Removing ONLY inserted observer statements must reproduce the entire source.
 stripped=after.replace('#include "weapon_hammerburst_press_timing.inl"\n','')
 stripped='\n'.join(l for l in stripped.split('\n') if 'hb_timing.' not in l and 'HammerburstPressTiming hb_timing(' not in l)
 assert stripped==before,'Observer unexpectedly changed production source'
 (P/'source'/rel).write_text(after)
 (ROOT/'docs/hammerburst-press-timing.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),
  dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,
  source_clocks_or_geometry_limits_changed=False,production_statement_parity=True,observation_only=True,maximum_press_records=12,
  scope='Bounded HB first press entry/asset/contract/ButtonName/final-reserve/dispatch durations. Only observer clock reads; source timestamps, control, gates, dispatch and deadlines unchanged; includes unexercised feed-history repair.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2))
 (P/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((P/'source'/n).read_bytes()).hexdigest() for n in (rel,helper)},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','production_statement_parity')}))
if __name__=='__main__':main()
