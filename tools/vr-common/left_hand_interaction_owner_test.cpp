#include "left_hand_interaction_owner.h"
#include "cover_interaction.h"
#include <cstdio>
#include <set>
#include <tuple>

using namespace gears_vr;
static unsigned checks=0;
#define CHECK(condition) do{++checks;if(!(condition)){std::printf("Failed line %d: %s\n",__LINE__,#condition);throw __LINE__;}}while(0)
static bool close(double a,double b){return std::abs(a-b)<1e-9;}
static bool same(LeftHandInteractionDecision const& a,LeftHandInteractionDecision const& b){
    return a.lease.kind==b.lease.kind&&a.lease.lifecycle==b.lease.lifecycle&&a.lease.id==b.lease.id&&
        a.status==b.status&&a.press_token==b.press_token&&a.new_grant==b.new_grant&&a.release_required==b.release_required;
}
static LeftHandInteractionFrame neutral(){
    LeftHandInteractionFrame f{};f.lifecycle=1;f.frame=1;f.context_valid=true;f.tracked=true;return f;
}
static void next(LeftHandInteractionFrame& f){++f.frame;}
static void press(LeftHandInteractionFrame& f){next(f);f.grip_down=true;++f.press_token;}
static void release(LeftHandInteractionFrame& f){next(f);f.grip_down=false;f.incumbents={};}
static bool empty(LeftHandInteractionDecision const& d){return d.lease.kind==LeftHandInteraction::none&&!d.new_grant;}

static void single_press_cover_priority_and_idempotence(){
    LeftHandInteractionOwner owner;auto f=neutral();auto ready=owner.update(f);
    CHECK(ready.status==LeftHandInteractionStatus::released&&!ready.release_required&&empty(ready));
    press(f);f.requests={42,true,true,true};auto cover=owner.update(f);
    CHECK(cover.status==LeftHandInteractionStatus::granted&&cover.new_grant&&!cover.release_required);
    CHECK(cover.lease.kind==LeftHandInteraction::cover&&cover.lease.lifecycle==1&&cover.lease.id==42);
    std::set<std::tuple<std::uint64_t,std::uint64_t,LeftHandInteraction,std::uint64_t>> committed;
    for(unsigned i=0;i<100;++i){
        auto repeated=owner.update(f);CHECK(same(repeated,cover));
        if(repeated.new_grant)committed.emplace(repeated.lease.lifecycle,repeated.press_token,repeated.lease.kind,repeated.lease.id);
    }
    CHECK(committed.size()==1); // A consumer commits the stable operation key once.
    next(f);f.incumbents[0]=cover.lease;f.requests.cover_request_id=999;
    auto held=owner.update(f);CHECK(held.status==LeftHandInteractionStatus::retained&&!held.new_grant&&held.lease.id==42);
    release(f);CHECK(empty(owner.update(f)));press(f);
    CHECK(owner.update(f).lease.id==999); // A distinct released/re-pressed token permits another grant.

    LeftHandInteractionOwner boot;auto down=neutral();down.grip_down=true;down.press_token=8;
    CHECK(boot.update(down).status==LeftHandInteractionStatus::needs_release);
    next(down);down.requests.cover_request_id=55;CHECK(empty(boot.update(down)));
    release(down);boot.update(down);press(down);CHECK(boot.update(down).lease.kind==LeftHandInteraction::cover);
}

static void existing_leases_win_and_conflicts_never_guess(){
    for(LeftHandInteraction kind:{LeftHandInteraction::support,LeftHandInteraction::ammo,
        LeftHandInteraction::magazine,LeftHandInteraction::cover,LeftHandInteraction::other}){
        LeftHandInteractionOwner owner;auto f=neutral();f.grip_down=true;f.press_token=7;
        f.incumbents[0]={kind,1,81};f.requests={999,true,true,true};
        auto retained=owner.update(f);
        CHECK(retained.status==LeftHandInteractionStatus::retained&&!retained.new_grant&&retained.lease.kind==kind&&retained.lease.id==81);
        next(f);CHECK(owner.update(f).lease.kind==kind);
        release(f);CHECK(empty(owner.update(f)));press(f);CHECK(owner.update(f).lease.kind==LeftHandInteraction::cover);
    }
    for(bool conflicting:{false,true}){
        LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);f.requests.support=true;
        auto support=owner.update(f);CHECK(support.lease.kind==LeftHandInteraction::support);
        next(f);f.requests={50,false,false,true};
        if(conflicting)f.incumbents[0]={LeftHandInteraction::magazine,1,99};
        auto invalid=owner.update(f);
        CHECK(empty(invalid)&&invalid.release_required);
        CHECK(invalid.status==(conflicting?LeftHandInteractionStatus::incumbent_conflict:LeftHandInteractionStatus::lease_lost));
        next(f);f.incumbents[0]=support.lease;CHECK(empty(owner.update(f))); // Restoring a lease after failure cannot resume it.
    }
    LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);f.requests.cover_request_id=9;
    f.incumbents={LeftHandInteractionLease{LeftHandInteraction::support,1,10},
                  LeftHandInteractionLease{LeftHandInteraction::magazine,1,11}};
    auto conflict=owner.update(f);
    CHECK(conflict.status==LeftHandInteractionStatus::incumbent_conflict&&empty(conflict)&&conflict.release_required);
}

static void weapon_candidates_and_consumed_empty_press(){
    for(unsigned candidate=0;candidate<3;++candidate){
        LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);
        f.requests.support=candidate==0;f.requests.ammo=candidate==1;f.requests.magazine=candidate==2;
        auto d=owner.update(f);const auto expected=candidate==0?LeftHandInteraction::support:
            candidate==1?LeftHandInteraction::ammo:LeftHandInteraction::magazine;
        CHECK(d.new_grant&&d.lease.kind==expected&&d.lease.id==f.press_token);
    }
    for(unsigned mask:{3u,5u,6u,7u}){
        LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);
        f.requests.support=(mask&1)!=0;f.requests.ammo=(mask&2)!=0;f.requests.magazine=(mask&4)!=0;
        auto d=owner.update(f);CHECK(d.status==LeftHandInteractionStatus::candidate_conflict&&empty(d));
    }
    LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);
    CHECK(owner.update(f).status==LeftHandInteractionStatus::no_candidate);
    next(f);f.requests.cover_request_id=43;CHECK(owner.update(f).status==LeftHandInteractionStatus::needs_release);
    release(f);owner.update(f);press(f);CHECK(owner.update(f).lease.kind==LeftHandInteraction::cover);
}

static void interruptions_replay_and_lifecycle(){
    for(bool tracking:{false,true}){
        LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);f.requests.magazine=true;
        auto grant=owner.update(f);next(f);f.incumbents[0]=grant.lease;
        if(tracking)f.tracked=false;else f.context_valid=false;
        auto lost=owner.update(f);CHECK(lost.status==LeftHandInteractionStatus::inactive&&empty(lost)&&lost.release_required);
        next(f);f.tracked=true;f.context_valid=true;CHECK(empty(owner.update(f)));
        release(f);CHECK(!owner.update(f).release_required);press(f);CHECK(owner.update(f).lease.kind==LeftHandInteraction::magazine);
    }
    for(unsigned mutation=0;mutation<4;++mutation){
        LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);f.requests.cover_request_id=12;
        auto granted=owner.update(f);auto original=f;
        if(mutation==0)++f.requests.cover_request_id;
        if(mutation==1)f.grip_down=false;
        if(mutation==2)f.incumbents[0]=granted.lease; // Confirmation belongs to the kernel, not a changed owner frame.
        if(mutation==3)f.tracked=false;
        auto rejected=owner.update(f);CHECK(rejected.status==LeftHandInteractionStatus::replayed&&empty(rejected));
        f=original;CHECK(empty(owner.update(f)));release(f);CHECK(!owner.update(f).release_required);
    }
    LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);press(f);f.requests.support=true;
    auto granted=owner.update(f);auto old=f;old.frame=1;
    CHECK(owner.update(old).status==LeftHandInteractionStatus::replayed);CHECK(empty(owner.update(f)));
    release(f);owner.update(f);press(f);owner.update(f);
    f.lifecycle=2;f.frame=1;f.incumbents[0]={LeftHandInteraction::support,2,90};
    auto boundary=owner.update(f);CHECK(boundary.status==LeftHandInteractionStatus::lifecycle_changed&&empty(boundary));
    next(f);CHECK(empty(owner.update(f)));release(f);owner.update(f);press(f);
    CHECK(owner.update(f).new_grant);
    auto prior=f;prior.lifecycle=1;prior.frame=999;
    CHECK(owner.update(prior).status==LeftHandInteractionStatus::replayed);
    CHECK(granted.lease.id==1);

    LeftHandInteractionOwner tokens;auto t=neutral();tokens.update(t);press(t);t.requests.ammo=true;
    auto ammo=tokens.update(t);next(t);t.incumbents[0]=ammo.lease;t.press_token+=2;
    CHECK(tokens.update(t).status==LeftHandInteractionStatus::ambiguous);
    release(t);tokens.update(t);press(t);tokens.update(t);release(t);tokens.update(t);
    next(t);t.grip_down=true;CHECK(tokens.update(t).status==LeftHandInteractionStatus::replayed); // Reused token.

    for(unsigned invalid=0;invalid<6;++invalid){
        LeftHandInteractionOwner bad;auto value=neutral();
        if(invalid==0)value.frame=0;
        if(invalid==1)value.lifecycle=0;
        if(invalid==2)value.grip_down=true;
        if(invalid==3)value.incumbents[0]={LeftHandInteraction::support,2,1};
        if(invalid==4)value.incumbents[0]={LeftHandInteraction::support,1,0};
        if(invalid==5)value.incumbents[0]={static_cast<LeftHandInteraction>(99),1,1};
        auto d=bad.update(value);CHECK(d.status==LeftHandInteractionStatus::invalid&&empty(d));
    }
}

struct CoverIntegration {
    LeftHandInteractionOwner owner;
    LeftHandInteractionFrame input=neutral();
    CoverInteraction kernel;
    CoverObservation cover;
    CoverHandSample hand;
    std::int64_t now=1000000000;
    CoverIntegration(){
        cover.sequence=1;cover.observed_ns=now;cover.epochs={1,1,1,1,1,1,1};
        cover.cover_valid=true;cover.interaction_enabled=true;cover.range={-.6,.5,0,.7};
        cover.edge={17,CoverEdgeKind::left,true,true};
        hand.sequence=1;hand.observed_ns=now;hand.session_epoch=1;hand.space_epoch=1;
        hand.input_active=true;hand.position_valid=true;hand.position_tracked=true;
    }
    void advance(){
        ++input.frame;++cover.sequence;++hand.sequence;now+=10000000;cover.observed_ns=hand.observed_ns=now;
        hand.lgrip_down=input.grip_down;hand.position_tracked=input.tracked;
    }
    CoverResult apply(LeftHandInteractionDecision const& decision){
        CoverOwnership grant{};
        if(decision.lease.kind==LeftHandInteraction::cover)grant={CoverLeftHandOwner::cover,decision.lease.id};
        else if(decision.lease.kind!=LeftHandInteraction::none)grant.owner=CoverLeftHandOwner::other;
        return kernel.update(cover,hand,grant,now);
    }
};

static void cover_two_stage_with_weapon_competition(){
    CoverIntegration f;auto released=f.owner.update(f.input);CHECK(f.apply(released).phase==CoverPhase::attached);
    f.input.grip_down=true;++f.input.press_token;f.advance();
    auto request=f.kernel.update(f.cover,f.hand,{},f.now);CHECK(request.request_left_hand&&!request.owns_left_hand);
    f.input.requests={request.request_id,true,true,true};
    auto decision=f.owner.update(f.input);CHECK(decision.lease.kind==LeftHandInteraction::cover&&decision.new_grant);
    auto confirmed=f.apply(decision);CHECK(confirmed.owns_left_hand&&close(confirmed.offset.right,0));
    CHECK(!f.input.incumbents[0].id); // The same frozen owner input has not been rewritten for confirmation.
    CHECK(same(f.owner.update(f.input),decision));CHECK(f.apply(decision).owns_left_hand);
    const bool support=decision.lease.kind==LeftHandInteraction::support;
    const bool ammo=decision.lease.kind==LeftHandInteraction::ammo;
    const bool magazine=decision.lease.kind==LeftHandInteraction::magazine;
    CHECK(!support&&!ammo&&!magazine);
    f.input.incumbents[0]=decision.lease;f.hand.left_position_metres.x=.2;f.advance();
    auto held=f.owner.update(f.input);auto dragged=f.apply(held);
    CHECK(held.status==LeftHandInteractionStatus::retained&&dragged.owns_left_hand&&close(dragged.offset.right,-.2));
    f.input.tracked=false;f.advance();auto lost=f.owner.update(f.input);auto stopped=f.apply(lost);
    CHECK(empty(lost)&&!stopped.owns_left_hand&&close(stopped.offset.right,-.2));
    f.input.tracked=true;f.advance();auto recovered=f.owner.update(f.input);
    CHECK(empty(recovered)&&!f.apply(recovered).owns_left_hand); // Still-held old lease does not resume.
    f.input.grip_down=false;f.advance();auto release_decision=f.owner.update(f.input);auto offset=f.apply(release_decision);
    CHECK(empty(release_decision)&&close(offset.offset.right,-.2));
    f.input.incumbents={};f.input.grip_down=true;++f.input.press_token;f.advance();
    request=f.kernel.update(f.cover,f.hand,{},f.now);CHECK(request.request_left_hand);
    f.input.requests.cover_request_id=request.request_id;
    auto regrab=f.owner.update(f.input);CHECK(f.apply(regrab).owns_left_hand&&close(f.apply(regrab).offset.right,-.2));
    f.input.incumbents[0]=regrab.lease;f.hand.left_position_metres.x=.1;f.advance();
    CHECK(close(f.apply(f.owner.update(f.input)).offset.right,-.1));

    for(LeftHandInteraction incumbent:{LeftHandInteraction::support,LeftHandInteraction::magazine}){
        CoverIntegration occupied;occupied.owner.update(occupied.input);occupied.kernel.update(occupied.cover,occupied.hand,{},occupied.now);
        occupied.input.grip_down=true;++occupied.input.press_token;occupied.input.incumbents[0]={incumbent,1,70};occupied.advance();
        auto candidate=occupied.kernel.update(occupied.cover,occupied.hand,{},occupied.now);CHECK(candidate.request_left_hand);
        occupied.input.requests={candidate.request_id,true,false,true};
        auto existing=occupied.owner.update(occupied.input);auto denied=occupied.apply(existing);
        CHECK(existing.lease.kind==incumbent&&!existing.new_grant&&!denied.owns_left_hand&&close(denied.offset.right,0));
    }
}

static void bounded_magazine_observation_handoff(){
    const auto begin=[](LeftHandInteractionOwner& owner){
        auto f=neutral();f.now_ns=1000000000;owner.update(f);press(f);f.now_ns+=1000000;
        f.requests.magazine_observation_pending=true;
        const auto waiting=owner.update(f);
        CHECK(empty(waiting)&&!waiting.release_required&&waiting.status==LeftHandInteractionStatus::no_candidate);
        CHECK(same(waiting,owner.update(f)));return f;
    };
    {
        LeftHandInteractionOwner owner;auto f=begin(owner);
        next(f);f.now_ns+=50000000;f.requests.magazine_observation_pending=false;f.requests.magazine=true;
        const auto acquired=owner.update(f);CHECK(acquired.new_grant&&acquired.lease.kind==LeftHandInteraction::magazine);
        CHECK(same(acquired,owner.update(f)));
        next(f);++f.now_ns;f.incumbents[0]=acquired.lease;
        CHECK(owner.update(f).status==LeftHandInteractionStatus::retained);
        release(f);++f.now_ns;CHECK(!owner.update(f).release_required);
    }
    for(unsigned mode=0;mode<11;++mode){
        LeftHandInteractionOwner owner;auto f=begin(owner);next(f);
        f.now_ns+=50000000;f.requests.magazine_observation_pending=false;f.requests.magazine=true;
        if(mode==0)f.now_ns+=50000001;
        if(mode==1)f.now_ns-=50000000;
        if(mode==2)f.now_ns-=50000001;
        if(mode==3)f.tracked=false;
        if(mode==4)f.context_valid=false;
        if(mode==5)++f.lifecycle;
        if(mode==6)++f.press_token;
        if(mode==7){f.requests.magazine=false;f.requests.ammo=true;}
        if(mode==8){f.requests.magazine=false;f.requests.support=true;f.requests.magazine_observation_pending=true;}
        if(mode==9){f.requests.magazine=false;f.requests.cover_request_id=42;}
        if(mode==10)f.incumbents[0]={LeftHandInteraction::ammo,f.lifecycle,88};
        const auto denied=owner.update(f);CHECK(empty(denied)&&denied.release_required);
        f.tracked=f.context_valid=true;f.requests={};f.requests.magazine=true;f.incumbents={};next(f);++f.now_ns;
        CHECK(empty(owner.update(f)));
    }
    {
        LeftHandInteractionOwner owner;auto f=begin(owner);next(f);f.now_ns+=50000000;
        f.requests.magazine_observation_pending=false;f.requests.support=true;
        const auto granted=owner.update(f);CHECK(granted.new_grant&&!granted.release_required&&
            granted.lease.kind==LeftHandInteraction::support&&granted.press_token==f.press_token);
        f.incumbents[0]=granted.lease;f.requests={};next(f);++f.now_ns;
        CHECK(owner.update(f).status==LeftHandInteractionStatus::retained);
    }
    {
        LeftHandInteractionOwner owner;auto f=begin(owner);next(f);f.now_ns+=30000000;
        CHECK(!owner.update(f).release_required);
        next(f);f.now_ns+=70000000;f.requests.magazine_observation_pending=false;f.requests.magazine=true;
        CHECK(owner.update(f).new_grant);
    }
    {
        LeftHandInteractionOwner owner;auto f=begin(owner);next(f);++f.now_ns;
        f.requests.magazine_observation_pending=false;CHECK(owner.update(f).release_required);
        next(f);++f.now_ns;f.requests.magazine=true;CHECK(empty(owner.update(f)));
    }
    {
        LeftHandInteractionOwner owner;auto f=begin(owner);auto mutated=f;mutated.requests.magazine_observation_pending=false;
        CHECK(owner.update(mutated).status==LeftHandInteractionStatus::replayed);
        next(f);++f.now_ns;f.requests.magazine=true;CHECK(empty(owner.update(f)));
    }
    {
        LeftHandInteractionOwner owner;auto f=begin(owner);release(f);++f.now_ns;
        CHECK(!owner.update(f).release_required);
        press(f);++f.now_ns;f.requests.magazine_observation_pending=false;f.requests.magazine=true;
        CHECK(owner.update(f).new_grant);
    }
}

int main(){
    try{
        // A rejected held press keeps the original cause for observation;
        // reading it must not rearm or change the required physical release.
        {LeftHandInteractionOwner owner;auto f=neutral();owner.update(f);
            press(f);CHECK(owner.update(f).status==LeftHandInteractionStatus::no_candidate);
            CHECK(owner.blocking_status()==LeftHandInteractionStatus::no_candidate);
            next(f);f.requests.magazine=true;
            CHECK(owner.update(f).status==LeftHandInteractionStatus::needs_release);
            CHECK(owner.blocking_status()==LeftHandInteractionStatus::no_candidate);
            release(f);CHECK(owner.update(f).status==LeftHandInteractionStatus::released);
            CHECK(owner.blocking_status()==LeftHandInteractionStatus::released);
            press(f);CHECK(owner.update(f).new_grant);}
        single_press_cover_priority_and_idempotence();existing_leases_win_and_conflicts_never_guess();
        weapon_candidates_and_consumed_empty_press();interruptions_replay_and_lifecycle();
        cover_two_stage_with_weapon_competition();bounded_magazine_observation_handoff();
    }catch(int){return 1;}
    std::printf("Shared left-hand owner CPU PASS: 6 groups, %u checks; priority, incumbents, single press, frozen frames, bounded magazine observation, lifecycle/tracking rearm and cover integration.\n",checks);
    return 0;
}
