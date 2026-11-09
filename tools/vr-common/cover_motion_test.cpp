#include "cover_motion.h"
#include <cstdio>
namespace m=gears_cover_motion;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
template<class T> auto bytes(T const& x){std::array<unsigned char,sizeof(T)> b{};std::memcpy(b.data(),&x,sizeof x);return b;}
// Explicit Host seam: completed writes are to this fixture's owned position,
// not game memory or physics. It exercises the production ledger, NOT a second
// implementation of its ordering rules or a Native certification backend.
struct Host final:m::Backend {
    m::Ledger ledger;
    m::Binding binding{{1,1,1,1,1,1,1},71,72};
    m::Hand hand{};m::Offset selected{.1,.5},body{.1,.2};
    m::Commit published{},forced{};
    m::Observation observation{m::Observation::committed};
    m::Admission admission{m::Admission::current};
    bool force{},reenter_observe{},reenter_admit{};
    std::uint64_t counter{10};std::int64_t now{1000000000};
    Host(){
        hand.sample={10,now,1,1,{.2,1.2,-.4},true,true,true,true};hand.grab=7;hand.edge=8;hand.axis=gears_vr::CoverEdgeKind::top;
        published.binding=binding;published.event=counter;published.version=1;
        published.body_publication=100;published.occupancy=101;published.from=published.requested=published.position=body;
    }
    m::Observation observe(std::uint64_t after,m::Commit& out) noexcept override {
        if(reenter_observe){reenter_observe=false;ledger.cancel(*this);}
        if(observation!=m::Observation::committed)return observation;
        if(force){out=forced;return observation;}
        if(after==published.event)return m::Observation::unchanged;
        out=published;return observation;
    }
    m::Admission admit(m::Intent const& i,std::int64_t stamp) noexcept override {
        if(reenter_admit){reenter_admit=false;ledger.cancel(*this);}
        if(admission!=m::Admission::current)return admission;
        return stamp==now&&m::detail::same(i.binding,binding)&&m::detail::same(i.hand,hand)&&
            m::detail::same(i.requested,selected)&&m::detail::same(i.from,body)?m::Admission::current:m::Admission::rejected;
    }
    m::Code propose(m::Intent& out){return ledger.propose(*this,binding,hand,selected,now,out);}
    bool finish(m::Intent const& intent,m::Offset actual,m::Completion completion=m::Completion::accepted){
        if(!ledger.current(intent))return false;
        m::Commit next{};next.binding=intent.binding;next.event=++counter;next.previous_event=intent.from_event;
        next.version=intent.from_version+1;next.from_version=intent.from_version;next.intent=intent.id;
        next.body_publication=published.body_publication+1;next.occupancy=published.occupancy+1;
        next.completion=completion;next.from=intent.from;next.requested=intent.requested;next.position=actual;next.hand=intent.hand;
        body=actual;published=next;return true; // owned Host body write precedes publication
    }
    void advance(){++hand.sample.sequence;now+=10000000;hand.sample.observed_ns=now;}
};
static bool at(m::Ledger const& l,double right,double up){auto c=l.committed();return c.position.right==right&&c.position.up==up;}

static void wait_cancel_and_commit_order(){
    Host before;m::Intent a{};C(before.propose(a)==m::Code::pending);
    C(before.ledger.pending()&&at(before.ledger,.1,.2)&&before.ledger.current(a));
    C(before.ledger.wait(before,a.id)==m::Code::waiting&&before.ledger.current(a));
    C(before.ledger.cancel(before)==m::Code::cancelled&&!before.ledger.pending());
    C(!before.finish(a,before.selected)&&at(before.ledger,.1,.2));

    Host after;m::Intent b{};C(after.propose(b)==m::Code::pending);
    C(after.finish(b,after.selected));C(at(after.ledger,.1,.2)); // input has not polled the actual completion yet
    C(after.ledger.cancel(after)==m::Code::cancelled);
    C(at(after.ledger,.1,.5)&&after.ledger.committed().version==2&&!after.ledger.pending());
    C(after.ledger.wait(after,b.id)==m::Code::ready);
    // A rejected new input still synchronizes the already completed body first.
    Host invalid;m::Intent c{},untouched{};C(invalid.propose(c)==m::Code::pending);C(invalid.finish(c,invalid.selected));
    invalid.advance();invalid.hand.sample.position_tracked=false;auto saved=bytes(untouched);
    C(invalid.propose(untouched)==m::Code::rejected&&bytes(untouched)==saved);
    C(at(invalid.ledger,.1,.5)&&!invalid.ledger.pending());
}
static void replacement_and_old_ack(){
    Host f;m::Intent first{},newer{};C(f.propose(first)==m::Code::pending);
    f.advance();f.selected.up=.6;C(f.propose(newer)==m::Code::pending&&newer.id>first.id);
    C(!f.ledger.current(first)&&!f.finish(first,{.1,.5}));
    C(f.finish(newer,f.selected)&&f.ledger.synchronize(f)==m::Code::ready);
    const auto older=f.ledger.committed();
    f.advance();f.selected.up=.4;C(f.propose(newer)==m::Code::pending);C(f.finish(newer,f.selected));
    C(f.ledger.synchronize(f)==m::Code::ready);const auto current=f.ledger.committed();
    f.force=true;f.forced=older;
    C(f.ledger.synchronize(f)==m::Code::stale&&m::detail::same(current,f.ledger.committed())&&!f.ledger.recovery_required());
    f.forced=current;C(f.ledger.synchronize(f)==m::Code::ready);
    f.forced.position.up+=.01;
    C(f.ledger.synchronize(f)==m::Code::recovery_required&&at(f.ledger,.1,.4));
}
static void clips_and_orthogonal_retention(){
    for(auto axis:{gears_vr::CoverEdgeKind::left,gears_vr::CoverEdgeKind::right,gears_vr::CoverEdgeKind::top}){
        Host f;f.hand.axis=axis;f.selected=axis==gears_vr::CoverEdgeKind::top?m::Offset{.1,.5}:m::Offset{-.5,.2};
        m::Intent i{};C(f.propose(i)==m::Code::pending);
        const auto actual=axis==gears_vr::CoverEdgeKind::top?m::Offset{.1,.3}:m::Offset{-.2,.2};
        C(f.finish(i,actual,m::Completion::clipped));C(f.ledger.synchronize(f)==m::Code::ready);
        const auto receipt=f.ledger.committed();
        C(m::detail::same(receipt.position,actual)&&m::detail::same(receipt.hand,i.hand)&&receipt.completion==m::Completion::clipped);
        // The consumer gets the exact clipped hand sample to rebase at the
        // actual position. It must not rebase at the unexecuted request.
        C(receipt.position.right!=receipt.requested.right||receipt.position.up!=receipt.requested.up);
    }
}
static void malformed_body_completions_preserve_debt(){
    for(unsigned bad=0;bad<25;++bad){
        Host f;m::Intent intent{};C(f.propose(intent)==m::Code::pending);const auto old=f.ledger.committed();
        C(f.finish(intent,f.selected));f.force=true;f.forced=f.published;auto& c=f.forced;
        switch(bad){
        case 0:++c.binding.context;break;case 1:++c.binding.basis;break;case 2:++c.binding.epochs.world;break;
        case 3:++c.binding.epochs.reference;break;case 4:++c.binding.epochs.slot;break;case 5:++c.previous_event;break;
        case 6:++c.from_version;break;case 7:++c.version;break;case 8:++c.intent;break;case 9:c.body_publication=0;break;
        case 10:c.occupancy=0;break;case 11:c.from.up+=.01;break;case 12:c.requested.up+=.01;break;
        case 13:c.position.up+=.01;break;case 14:++c.hand.sample.sequence;break;case 15:++c.hand.sample.observed_ns;break;
        case 16:++c.hand.grab;break;case 17:++c.hand.edge;break;case 18:c.hand.axis=gears_vr::CoverEdgeKind::left;break;
        case 19:c.completion=m::Completion::baseline;break;case 20:c.completion=m::Completion::clipped;c.position.up=.6;break;
        case 21:c.position.up=NAN;break;case 22:c.body_publication=old.body_publication;break;case 23:c.completion=static_cast<m::Completion>(99);break;
        case 24:c.hand.limit_clipped=!c.hand.limit_clipped;break;
        }
        C(f.ledger.synchronize(f)==m::Code::recovery_required);
        C(m::detail::same(f.ledger.committed(),old)&&!f.ledger.pending()&&f.ledger.recovery_required());
        f.force=false;C(f.ledger.synchronize(f)==m::Code::recovery_required); // no baseline/occupancy escape from real write debt
    }
}
static void actual_cancel_race_and_unavailable_backend(){
    Host f;m::Intent old{};C(f.propose(old)==m::Code::pending);
    C(f.ledger.cancel(f)==m::Code::cancelled);
    // A deliberately faulty Host publishes a write after cancellation. Do
    // not silently call it cancelled/no-write or accept its new position.
    f.published.event++;f.published.previous_event=old.from_event;f.published.version=2;
    f.published.from_version=1;f.published.intent=old.id;f.published.completion=m::Completion::accepted;
    C(f.ledger.synchronize(f)==m::Code::recovery_required&&at(f.ledger,.1,.2));
    Host uncertain;m::Intent i{};C(uncertain.propose(i)==m::Code::pending);
    uncertain.observation=m::Observation::uncertain;C(uncertain.ledger.cancel(uncertain)==m::Code::recovery_required);
    C(at(uncertain.ledger,.1,.2)&&!uncertain.ledger.pending());
    Host missing;C(missing.propose(i)==m::Code::pending);missing.observation=m::Observation::unavailable;
    C(missing.ledger.cancel(missing)==m::Code::recovery_required&&at(missing.ledger,.1,.2));
    Host lost;C(lost.propose(i)==m::Code::pending);lost.observation=m::Observation::unavailable;
    C(lost.propose(i)==m::Code::recovery_required&&!lost.ledger.pending()&&at(lost.ledger,.1,.2));
    m::UnavailableBackend unavailable;m::Ledger closed;m::Intent out{};const auto saved=bytes(out);
    C(closed.propose(unavailable,f.binding,f.hand,f.selected,f.now,out)==m::Code::unavailable);
    C(!closed.bound()&&!closed.pending()&&bytes(out)==saved);
    Host pinned,foreign;C(pinned.ledger.synchronize(pinned)==m::Code::ready);
    C(pinned.ledger.wait(pinned,0)==m::Code::stale);
    C(pinned.ledger.synchronize(foreign)==m::Code::recovery_required&&at(pinned.ledger,.1,.2));
    C(m::detail::next_sequence(0)==1&&m::detail::next_sequence(UINT64_MAX-1)==UINT64_MAX&&m::detail::next_sequence(UINT64_MAX)==0);
}
static void source_replay_and_backend_reentry(){
    Host repeat;m::Intent a{},b{};C(repeat.propose(a)==m::Code::pending);
    C(repeat.propose(b)==m::Code::pending&&a.id==b.id);
    repeat.advance();repeat.selected.up=.6;C(repeat.propose(b)==m::Code::pending);
    --repeat.hand.sample.sequence;repeat.hand.sample.observed_ns-=10000000;
    C(repeat.propose(a)==m::Code::stale&&repeat.ledger.current(b));
    Host changed;C(changed.propose(a)==m::Code::pending);changed.hand.sample.left_position_metres.x+=.01;
    C(changed.propose(b)==m::Code::rejected&&!changed.ledger.pending());
    Host clock;C(clock.propose(a)==m::Code::pending);--clock.now;
    C(clock.propose(b)==m::Code::rejected&&!clock.ledger.pending());
    for(unsigned where=0;where<2;++where){
        Host reentrant;reentrant.reenter_observe=where==0;reentrant.reenter_admit=where==1;
        C(reentrant.propose(a)==m::Code::recovery_required&&reentrant.ledger.recovery_required()&&!reentrant.ledger.pending());
    }
    for(unsigned bad=0;bad<11;++bad){
        Host invalid;
        switch(bad){
        case 0:invalid.hand.sample.position_tracked=false;break;case 1:invalid.hand.sample.lgrip_down=false;break;
        case 2:invalid.hand.sample.observed_ns=invalid.now+1;break;case 3:invalid.hand.sample.observed_ns-=100000001;break;
        case 4:invalid.hand.sample.sequence=0;break;case 5:invalid.hand.grab=0;break;case 6:invalid.hand.edge=0;break;
        case 7:invalid.hand.sample.left_position_metres.x=NAN;break;case 8:++invalid.hand.sample.space_epoch;break;
        case 9:invalid.selected.right+=.1;break;case 10:invalid.admission=m::Admission::unavailable;break;
        }
        const auto old=bytes(b);C(invalid.propose(b)!=m::Code::pending);
        C(bytes(b)==old&&!invalid.ledger.pending()&&at(invalid.ledger,.1,.2));
    }
}
int main(){
    wait_cancel_and_commit_order();replacement_and_old_ack();clips_and_orthogonal_retention();
    malformed_body_completions_preserve_debt();actual_cancel_race_and_unavailable_backend();source_replay_and_backend_reentry();
    std::printf("cover_motion: %u checks, %u failures; production ledger with explicit owned Host publication; Game=0 physics=0\n",checks,failures);
    return failures?1:0;
}
