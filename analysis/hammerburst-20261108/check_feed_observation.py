"""Compare exact before/after insertion functions using the real frozen kernel.

Only local-hand/Frame and extraction are synthetic. No game execution.
"""
from pathlib import Path
import hashlib,json,subprocess
A=Path(__file__).resolve().parent;P=A/'feed-observe-r1'
def function(candidate):
    text=(A/candidate/'source/tools/playground/weapon_hammerburst_runtime.inl').read_text()
    start=text.index('gears_weapon::FeedResult insertion_step(')
    return text[start:text.index('\nvoid step(',start)]
fixture=r'''
using P=gears_weapon::Pose;using V=gears_weapon::Vector;
enum class Stage {old_magazine=4,new_magazine=6};Stage stage=Stage::new_magazine;
FILE* log_file{};P pose{{0,0,.06},{0,0,0,1}};bool local_valid=true;
unsigned feed_logs{};bool last_feed_position_known{};V last_feed_position{};
std::uint64_t feed_item{},feed_sequence{};std::int64_t feed_ns{};
gears_weapon::FeedPortMount feed;
struct CountingStroke {
 gears_weapon::FeedInsertionStroke real;unsigned arms{},observes{},resets{};
 bool arm(std::uint64_t a,std::uint64_t b,std::uint64_t c,std::uint64_t d){++arms;return real.arm(a,b,c,d);}
 void reset(){++resets;real.reset();}
 auto observe(gears_weapon::FeedSample const& s,gears_weapon::FeedPortMount const& m){++observes;return real.observe(s,m);}
} insertion;
struct Extraction {unsigned calls{};bool complete(){return false;}void observe(gears_weapon::FeedSample const&,gears_weapon::FeedPortMount const&){++calls;}} extraction;
bool local_hand(hand::Frame const&,P& p){p=pose;return local_valid;}
V add(V a,V b){return gears_weapon::sum(a,b);}V sub(V a,V b){return gears_weapon::difference(a,b);}
V scale(V a,double b){return gears_weapon::scaled(a,b);}double length(V a){return gears_weapon::length(a);}
'''
source=r'''
#include "weapon_reload_geometry.h"
#include <cassert>
#include <cstdio>
#include <cwchar>
namespace hand {
struct Frame {struct{std::uint64_t frame{};}key;struct{std::uint64_t sequence{};std::int64_t sync_ns{};}controllers;
 struct{std::uint64_t address{1};}weapon,world;struct{std::uint64_t space_epoch{1};}stereo;
 unsigned left_tracked{1};std::uint64_t press_token{1};std::int64_t now_ns{};};
struct Decision{unsigned status{};struct{unsigned kind{};std::uint64_t id{};}lease;};
}
namespace before {
'''+fixture+function('part-frame-r1')+'\n}\nnamespace after {\n'+fixture+r'''
#define GEARS_PLAYGROUND_HB_FEED_OBSERVE 1
#include "weapon_hammerburst_feed_observation.inl"
'''+function('feed-observe-r1')+r'''
}
int main(){
 unsigned cases{},samples{};
 for(unsigned scenario=0;scenario<7;++scenario){
  before::insertion={};after::insertion={};before::feed_item=after::feed_item=0;
  before::feed_ns=after::feed_ns=0;before::feed_sequence=after::feed_sequence=0;
  before::stage=before::Stage::new_magazine;after::stage=after::Stage::new_magazine;
  if(scenario==6){before::stage=before::Stage::old_magazine;after::stage=after::Stage::old_magazine;}
  before::extraction.calls=after::extraction.calls=0;
  before::feed_logs=after::feed_logs=0;before::last_feed_position_known=after::last_feed_position_known=false;
  after::feed_observation={};assert(tmpfile_s(&after::log_file)==0&&after::log_file);
  hand::Frame f;f.controllers.sync_ns=1000000000;
  gears_weapon::FeedResult last{};
  for(unsigned i=0;i<12;++i){
   ++f.key.frame;++f.controllers.sequence;f.controllers.sync_ns+=20000000;
   const double depth=.06-.005*i;
   before::pose=after::pose={{scenario==2?.05:0,0,depth},{0,0,0,1}};
   before::local_valid=after::local_valid=!(scenario==3&&i==5);
   if(scenario==1&&i==5)f.controllers.sync_ns+=110000000;
   if(scenario==4&&i==5){--f.controllers.sequence;f.controllers.sync_ns-=20000000;}
   if(scenario==5&&i==5){before::pose.position.z=after::pose.position.z=.2;}
   const auto b=before::insertion_step(f,2),a=after::insertion_step(f,2);last=a;
   assert(a==b);assert(before::insertion.arms==after::insertion.arms);
   assert(before::insertion.observes==after::insertion.observes);
   assert(before::insertion.resets==after::insertion.resets);
   assert(before::feed_ns==after::feed_ns&&before::feed_sequence==after::feed_sequence);
   assert(before::extraction.calls==after::extraction.calls);++samples;
  }
  // Duplicate source is rejected without erasing an otherwise valid stroke.
  if(scenario==0||scenario==4||scenario==6)assert(last==gears_weapon::FeedResult::inserted);
  else assert(last!=gears_weapon::FeedResult::inserted);
  if(scenario==1)assert(after::insertion.arms==2);
  std::fclose(after::log_file);after::log_file=nullptr;++cases;
 }
 // Travel region can exhaust its allowance without starving seating observations.
 after::feed_observation={};after::stage=after::Stage::new_magazine;
 assert(tmpfile_s(&after::log_file)==0&&after::log_file);hand::Frame f;
 for(unsigned i=0;i<80;++i)after::observe_feed_result(f,2,gears_weapon::FeedResult::rejected,1,true,.05,.001,1);
 assert(after::feed_observation.rows[0]==24&&after::feed_observation.rows[1]==0);
 for(unsigned i=0;i<80;++i)after::observe_feed_result(f,2,gears_weapon::FeedResult::rejected,1,true,.005,.001,1);
 assert(after::feed_observation.rows[0]==24&&after::feed_observation.rows[1]==24);
 std::fclose(after::log_file);++cases;
 std::printf("HB exact insertion before/after parity: %u scenarios, %u paired samples; real FeedInsertionStroke, synthetic Frame/local hand; no game\n",cases,samples);
}
'''
target=P/'source/tools/playground/hammerburst_feed_observation_test.cpp';target.write_text(source)
cmd=P/'feed-observation-test.cmd';exe=P/'feed-observation-test.exe'
cmd.write_text('@echo off\ncall "E:/vs/VC/Auxiliary/Build/vcvars64.bat" >nul\nif errorlevel 1 exit /b %errorlevel%\n'+
 f'cl /nologo /O2 /std:c++17 /W4 /WX /UNDEBUG /EHsc /I"{P / "source/tools/vr-common"}" "{target}" /Fo"{P / "feed-observation-test.obj"}" /Fe"{exe}"\n'+
 f'if errorlevel 1 exit /b %errorlevel%\n"{exe}"\nexit /b %errorlevel%\n')
with (P/'feed-observation-test.log').open('wb') as log:
 result=subprocess.run(['cmd','/c',str(cmd)],stdout=log,stderr=subprocess.STDOUT,creationflags=subprocess.CREATE_NO_WINDOW)
print((P/'feed-observation-test.log').read_text(errors='replace'));result.check_returncode()
(P/'feed-observation-test-result.json').write_text(json.dumps(dict(passed=True,scenarios=8,paired_samples=84,
 real_feed_kernel=True,exact_extracted_functions=True,game_execution=False,
 source_sha256=hashlib.sha256(target.read_bytes()).hexdigest()),indent=2))
