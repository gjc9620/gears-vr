"""HB actual producer-motion ingestion, preserving strict current authority."""
from pathlib import Path
import difflib,hashlib,json,shutil,subprocess
A=Path(__file__).resolve().parent;ROOT=A.parents[1];BASE=A/'feed-observe-r1';P=A/'feed-history-r1'
def replace(t,a,b):assert t.count(a)==1,a[:90];return t.replace(a,b)
def main():
 assert not P.exists(),'Preserve named candidate'
 for n in ('playground','vr-common','native-capture','third-party'):shutil.copytree(BASE/'source/tools'/n,P/'source/tools'/n)
 helpers=['tools/vr-common/weapon_hammerburst_feed_history.h','tools/playground/weapon_hammerburst_feed_history.inl']
 for n in helpers:shutil.copy2(ROOT/n,P/'source'/n)
 rel='tools/playground/weapon_hammerburst_runtime.inl';before=(BASE/'source'/rel).read_text();after=before
 after=replace(after,'// One GT consumer,','#include "../vr-common/weapon_hammerburst_feed_history.h"\n// One GT consumer,')
 after=replace(after,'Stage stage{};','Stage stage{};\nnamespace feed_history {void clear() noexcept;}')
 after=replace(after,'    feed_item=feed_sequence=0;feed_ns=0;','    feed_item=feed_sequence=0;feed_ns=0;feed_history::clear();')
 after=replace(after,'#include "weapon_hammerburst_latch_history.inl"','#include "weapon_hammerburst_latch_history.inl"\n#include "weapon_hammerburst_feed_history.inl"')
 start=after.index('gears_weapon::FeedResult insertion_step(');end=after.index('\nvoid step(',start)
 old=after[start:end];body=old[old.index('    const auto position='):]
 body=replace(body,'    gears_weapon::FeedSample sample{f.weapon.address,item,f.world.address,f.stereo.space_epoch,f.controllers.sequence,\n        f.controllers.sync_ns,{{0,0,0},{0,0,0,1}},left,f.left_tracked!=0,true};\n','')
 body=replace(body,'    const auto result=insertion.observe(sample,feed);','    if(!used_history)result=insertion.observe(sample,feed);\n    if(stage==Stage::new_magazine)feed_history::remember(sample);else feed_history::clear();')
 head='''gears_weapon::FeedResult insertion_step(hand::Frame const& f,std::uint64_t item) noexcept {
    P left{};if(!local_hand(f,left)){insertion.reset();feed_history::clear();observe_feed_rejection(f,1);return gears_weapon::FeedResult::rejected;}
    const auto sample_gap_ns=feed_ns?f.controllers.sync_ns-feed_ns:0;
    const bool first_item_sample=feed_item!=item||!feed_ns;
    if(first_item_sample){
        if(!insertion.arm(f.weapon.address,item,f.world.address,f.stereo.space_epoch))return gears_weapon::FeedResult::rejected;
        feed_sequence=0;
    }
    if(f.controllers.sequence<=feed_sequence||f.controllers.sync_ns<=feed_ns){observe_feed_rejection(f,2);return gears_weapon::FeedResult::rejected;}
    gears_weapon::FeedSample sample{f.weapon.address,item,f.world.address,f.stereo.space_epoch,f.controllers.sequence,
        f.controllers.sync_ns,{{0,0,0},{0,0,0,1}},left,f.left_tracked!=0,true};
    if(stage==Stage::new_magazine)feed_history::capture(f,left);
    gears_weapon::FeedResult result=gears_weapon::FeedResult::rejected;
    const bool gap=sample_gap_ns>feed.maximum_sample_gap_ns;
    const bool used_history=!first_item_sample&&gap&&feed_history::advance(f,sample,result);
    const bool rearmed=first_item_sample||(gap&&!used_history);
    if(!first_item_sample&&gap&&!used_history){
        if(!insertion.arm(f.weapon.address,item,f.world.address,f.stereo.space_epoch))return gears_weapon::FeedResult::rejected;
    }
    if(stage==Stage::new_magazine&&gap&&log_file){static unsigned rows{};if(rows++<16){
        fwprintf(log_file,L"hammerburst_feed_history gate=%u used=%u points=%u frame=%llu sequence=%llu gap_ns=%lld result=%u\\n",
            feed_history::gate,unsigned(used_history),feed_history::point_count,f.key.frame,f.controllers.sequence,sample_gap_ns,unsigned(result));fflush(log_file);
    }}
    feed_item=item;feed_sequence=f.controllers.sequence;feed_ns=f.controllers.sync_ns;
'''
 after=after[:start]+head+body+after[end:];(P/'source'/rel).write_text(after)
 (ROOT/'docs/hammerburst-feed-history.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 finish()
def finish():
 assert P.exists() and not (P/'prepared.json').exists(),'Only finish an unbuilt private candidate'
 rel='tools/playground/weapon_hammerburst_runtime.inl'
 helpers=['tools/vr-common/weapon_hammerburst_feed_history.h','tools/playground/weapon_hammerburst_feed_history.inl']
 before=(BASE/'source'/rel).read_text();after=(P/'source'/rel).read_text()
 (ROOT/'docs/hammerburst-feed-history.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+rel,tofile='b/'+rel)))
 parent=json.loads((BASE/'prepared.json').read_text());configure=list(parent['configure_command']);build=list(parent['build_command'])
 configure[configure.index('-S')+1]=str(P/'source/tools/playground');configure[configure.index('-B')+1]=str(P/'build');build[build.index('--build')+1]=str(P/'build')
 for label,cmd in (('configure',configure),('build',build)):
  with (P/(label+'.log')).open('wb') as log:subprocess.run(cmd,check=True,stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
 dll=P/'build/Release/GearsPlayground6DofUnified3.dll'
 proof=dict(base=str(BASE),full_playground_compiled=True,runtime_exercised=False,gameplay_accepted=False,dll=str(dll),
  dll_sha256=hashlib.sha256(dll.read_bytes()).hexdigest(),configure_command=configure,build_command=build,native_rebuilt=False,xr_rebuilt=False,
  source_clocks_or_geometry_limits_changed=False,shared_client_changed=False,
  scope='HB insertion original producer ring, exact current pose/source/held context; atomic geometry ingestion, strict fallback rearm. No new Frame/edge/grant/timestamp or stock dispatch.')
 (P/'prepared.json').write_text(json.dumps(proof,indent=2))
 (P/'source-verification.json').write_text(json.dumps({n:hashlib.sha256((P/'source'/n).read_bytes()).hexdigest() for n in [rel]+helpers},indent=2))
 print(json.dumps({k:proof[k] for k in ('full_playground_compiled','dll_sha256','gameplay_accepted')}))
if __name__=='__main__':main()
