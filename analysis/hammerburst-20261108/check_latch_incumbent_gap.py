"""Exact helper, original Client/Native-owner fixture; no game/engine actions."""
from pathlib import Path
import json,hashlib,subprocess
A=Path(__file__).resolve().parent;P=A/'latch-incumbent-gap-r1';BASE=A/'latch-observation-wait-r1'
def helper(p):
 s=(p/'source/tools/playground/weapon_hammerburst_runtime.inl').read_text();start=s.index('void unavailable_frame(');return s[start:s.index('\nbool tick(uptr pc)',start)]
old=helper(BASE).replace('void unavailable_frame(', 'void legacy_unavailable_frame(')
script=(A/'check_pose_gap_incumbent.py').read_text().replace("P=A/'pose-gap-incumbent-r1'","P=A/'latch-incumbent-gap-r1'")
script=script.replace('struct Counter {unsigned resets{};void reset(){++resets;}} stroke,insertion,extraction;', 'struct Counter {unsigned resets{};bool observed=true;bool active()const{return observed;}void reset(){++resets;observed=false;}} stroke,insertion,extraction;')
script=script.replace('@HELPER@', '@HELPER@\n@LEGACY@',1)
# Keep all existing new-mag-only cases, including the real original loss and UP.
extra=r'''
 for(auto latch_stage:{Stage::opening,Stage::closing}){
  for(unsigned reason=0;reason<=unsigned(FrameReadFailure::implicit_clear);++reason){
   reset();stage=latch_stage;released=false;held.kind=LeaseKind::magazine;
   const auto before=held;FrameReadObservation f{};f.failure=FrameReadFailure(reason);unavailable_frame(f);
   const bool keep=f.failure==FrameReadFailure::publication||f.failure==FrameReadFailure::stale_before;
   assert(keep?same(held,before):held.kind==LeaseKind::none);
   assert(stroke.resets==(keep?0u:1u));++cases;
  }
  for(unsigned bad=0;bad<7;++bad){
   reset();stage=latch_stage;released=false;held.kind=LeaseKind::magazine;
   if(bad==0)released=true;if(bad==1)stroke.observed=false;if(bad==2)++held.lifecycle;
   if(bad==3)held.kind=LeaseKind::ammo;if(bad==4)held={};
   if(bad==5)stage=Stage::old_magazine;if(bad==6)stage=Stage::recovering;
   FrameReadObservation f{};f.failure=FrameReadFailure::stale_before;unavailable_frame(f);
   assert(held.kind==LeaseKind::none&&stroke.resets==1);++cases;
  }
  for(bool repaired:{false,true}){
   Host h;HandClient c(h.adapter());h.connect(c);h.game.mount_known=false;
   h.step();h.next();h.step();h.game.request_magazine=true;h.next(true);h.step();
   Frame out{};GripGameState game{};Decision d{};{GT gt;assert(c.copy_frame_on_gt(out,game,d));}
   assert(d.new_grant&&d.lease.kind==LeaseKind::magazine);
   reset();stage=latch_stage;released=false;held=d.lease;opening.identity.lifecycle=d.lease.lifecycle;
   h.now+=100000001;FrameReadObservation f{};
   {GT gt;assert(!c.copy_frame_on_gt(out,game,d,&f));}
   assert(f.failure==FrameReadFailure::stale_before&&!out.key.frame);
   if(repaired)unavailable_frame(f);else legacy_unavailable_frame(f);
   h.game.request_magazine=false;h.game.object_incumbents[0]=held;
   h.frame.now_ns=h.now;h.frame.stereo.received_ns=h.now;h.next(true);h.step();
   {GT gt;assert(c.copy_frame_on_gt(out,game,d));}
   if(repaired){
    assert(d.status==DecisionStatus::retained&&!d.new_grant&&same(d.lease,held));
    assert(stroke.active()&&stroke.resets==0);
    h.next(false);h.step();{GT gt;assert(c.copy_frame_on_gt(out,game,d));}
    assert(d.status==DecisionStatus::released&&d.lease.kind==LeaseKind::none);
   }else assert(d.status==DecisionStatus::lease_lost&&d.lease.kind==LeaseKind::none);
   ++cases;
  }
 }
'''
needle=' std::printf("HB exact unavailable helper + actual Client/owner:'
assert script.count(needle)==1;script=script.replace(needle,extra+needle)
script=script.replace(".replace('@HELPER@',helper)", ".replace('@HELPER@',helper).replace('@LEGACY@',legacy)")
exec(compile(script,str(__file__),'exec'),{'__file__':str(__file__),'__name__':'__main__','legacy':old})
p=P/'pose-gap-test-result.json';v=json.loads(p.read_text());v.update(latch_opening_and_closing_checked=True,actual_legacy_helper_reproduced_loss=True,fresh_successor_retains_same_lease=True,real_up_releases=True,acquisition_and_old_magazine_not_extended=True,limits='Actual Client, sole-owner adapter, exact old/new unavailable helper. Reflected game, animator, geometric contact and full broker remain fixture/model bounds; not game acceptance.');p.write_text(json.dumps(v,indent=2))
print('Opening/closing incumbent cases and prior new-mag suite passed; no game started')
