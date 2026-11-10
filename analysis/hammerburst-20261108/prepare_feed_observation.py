"""Bounded actual insertion outcome/early gate witness; no gameplay repair."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2];A=Path(__file__).resolve().parent
BASE=A/'part-frame-r1';TARGET=A/'feed-observe-r1'
def replace(text,old,new):
    assert text.count(old)==1,old[:100]
    return text.replace(old,new)
def main():
    assert not TARGET.exists(),'Preserve named candidate'
    for name in ('playground','vr-common','third-party','native-capture'):
        shutil.copytree(BASE/'source/tools'/name,TARGET/'source/tools'/name)
    helper='tools/playground/weapon_hammerburst_feed_observation.inl';shutil.copy2(ROOT/helper,TARGET/'source'/helper)
    rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before
    after=replace(after,'#include "weapon_hammerburst_restore_log.inl"',
        '#include "weapon_hammerburst_restore_log.inl"\n#include "weapon_hammerburst_feed_observation.inl"')
    after=replace(after,'bool move_magazine(hand::Frame const& f) noexcept {',
        'bool move_magazine(hand::Frame const& f) noexcept {\n    feed_move_gate=1;')
    after=replace(after,'    return visual::move_target(f,{position,gears_vr::ue_angles(gears_vr::rotation_matrix(orientation))},left.position);',
        '    feed_move_gate=2;\n    const bool moved=visual::move_target(f,{position,gears_vr::ue_angles(gears_vr::rotation_matrix(orientation))},left.position);\n    if(moved)feed_move_gate=0;return moved;')
    after=replace(after,'    P left{};if(!local_hand(f,left)){insertion.reset();return gears_weapon::FeedResult::rejected;}',
        '    P left{};if(!local_hand(f,left)){insertion.reset();observe_feed_rejection(f,1);return gears_weapon::FeedResult::rejected;}')
    after=replace(after,'    if(feed_item!=item||!feed_ns||f.controllers.sync_ns-feed_ns>feed.maximum_sample_gap_ns){',
        '    const auto sample_gap_ns=feed_ns?f.controllers.sync_ns-feed_ns:0;\n    const bool rearmed=feed_item!=item||!feed_ns||sample_gap_ns>feed.maximum_sample_gap_ns;\n    if(rearmed){')
    after=replace(after,'    if(f.controllers.sequence<=feed_sequence||f.controllers.sync_ns<=feed_ns)return gears_weapon::FeedResult::rejected;',
        '    if(f.controllers.sequence<=feed_sequence||f.controllers.sync_ns<=feed_ns){observe_feed_rejection(f,2);return gears_weapon::FeedResult::rejected;}')
    after=replace(after,'    return insertion.observe(sample,feed);',
        '    const auto result=insertion.observe(sample,feed);\n    observe_feed_result(f,item,result,sample_gap_ns,rearmed,depth,lateral,aligned);return result;')
    after=replace(after,'    if(!granted(f,decision,kind)||!move_magazine(f))return;',
        '    const bool part_granted=granted(f,decision,kind);\n    if(!part_granted||!move_magazine(f)){observe_move_rejection(f,decision,part_granted);return;}')
    (TARGET/'source'/rel).write_text(after)
    (ROOT/'docs/hammerburst-feed-observation.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
    cmake=TARGET/'source/tools/playground/CMakeLists.txt';cmake.write_text(cmake.read_text()+'\ntarget_compile_definitions(GearsPlayground PRIVATE GEARS_PLAYGROUND_HB_FEED_OBSERVE=1)\n')
    parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
    configure[configure.index('-S')+1]=str(TARGET/'source/tools/playground');configure[configure.index('-B')+1]=str(TARGET/'build')
    build[build.index('--build')+1]=str(TARGET/'build')
    for label,command in (('configure',configure),('build',build)):
        with (TARGET/(label+'.log')).open('wb') as log:subprocess.run(command,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
    dll=TARGET/'build/Release/GearsPlayground6DofUnified3.dll'
    proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,
        dll=str(dll),dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,
        native_rebuilt=False,xr_rebuilt=False,observation_only=True,shared_client_changed=False,
        source_clocks_or_geometry_limits_changed=False,maximum_outcome_rows=48,maximum_early_rows=28,
        scope='Existing HB insertion outcome, sample-gap rearm and move/early rejection witnesses; no source/metadata/clock read or engine dispatch added; all geometry/input/ownership/deadline rules unchanged.')
    (TARGET/'prepared.json').write_text(json.dumps(proof,indent=2))
    (TARGET/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((TARGET/'source'/n).read_bytes()).hexdigest() for n in (rel,helper,'tools/playground/CMakeLists.txt')},indent=2))
    print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','observation_only','gameplay_accepted')}))
if __name__=='__main__':main()
