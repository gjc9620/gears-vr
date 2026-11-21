#include "weapon_reload_transaction.h"
using namespace gears_weapon;
struct Identity {
    unsigned world{}, weapon{}, generation{};
    constexpr bool valid() const {return world&&weapon&&generation;}
    constexpr bool operator==(Identity const& b) const {
        return world==b.world&&weapon==b.weapon&&generation==b.generation;
    }
};
constexpr Identity id{1,2,3};
using Inv=ReloadTxnInventory<Identity>;
using Tx=ReloadTransaction<Identity>;
using Req=ReloadTxnRequest<Identity>;
constexpr bool magazine_retains_old_and_commits_once() {
    Tx t;Inv before{id,10,60,17,100};Req q;
    if(!t.bind(before)||!t.begin(before,1,ReloadIntentSource::magazine_pull,7,
        ReloadTxnMode::magazine,60,100,10))return false;
    if(t.inventory().loaded!=17||t.inventory().reserve!=100||t.target()!=43||t.mechanical_ready())return false;
    if(t.reserve_item(11)||!t.stow_original(10)||!t.reserve_item(11))return false;
    if(t.reserved()!=43||t.available_reserve()!=57||t.inventory().reserve!=100)return false;
    if(!t.request_insertion(11)||!t.take_request(q)||t.take_request(q))return false;
    if(q.expected.loaded!=60||q.expected.reserve!=57)return false;
    if(!t.acknowledge({q.token,true,{id,11,60,60,57}})||t.acknowledge({q.token,true,{id,11,60,60,57}}))return false;
    if(t.inserted()!=43||t.inventory().loaded+t.inventory().reserve!=117)return false;
    if(!t.confirm_closed(150,0)||!t.take_request(q)||q.result!=ReloadTxnResult::ordinary)return false;
    if(!t.acknowledge({q.token,true,{id,11,60,60,57},ReloadTxnResult::ordinary}))return false;
    return t.mechanical_ready()&&t.result()==ReloadTxnResult::ordinary&&!t.confirm_closed(151,0);
}
constexpr bool old_magazine_is_inspection_not_refill() {
    Tx t;Inv i{id,1,12,5,20};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::magazine_pull,1,ReloadTxnMode::magazine,12,0,10))return false;
    if(!t.restore_original(10)||!t.confirm_closed(5,100)||!t.take_request(q))return false;
    if(q.call!=ReloadTxnCall::exit_without_fill||q.result!=ReloadTxnResult::inspection)return false;
    return t.acknowledge({q.token,true,i})&&t.inventory().loaded==5&&t.inventory().reserve==20&&
        t.result()==ReloadTxnResult::inspection;
}
constexpr bool full_or_no_reserve_inspection() {
    for(unsigned reserve: {0u,20u}) {
        Tx t;Inv i{id,1,12,12,reserve};Req q;
        if(!t.bind(i)||t.begin(i,1,ReloadIntentSource::magazine_pull,1,ReloadTxnMode::magazine,12,0,10))return false;
        if(!t.begin(i,1,ReloadIntentSource::magazine_pull,0,ReloadTxnMode::magazine,12,0,10))return false;
        if(!t.interrupt()||!t.resume(i,2,0)||t.stage()!=ReloadTxnStage::open)return false;
        if(!t.restore_original(10)||!t.confirm_closed(5)||t.take_request(q)||!t.mechanical_ready())return false;
    }
    Tx t;Inv i{id,1,12,5,0};
    return t.bind(i)&&t.begin(i,1,ReloadIntentSource::magazine_pull,0,ReloadTxnMode::magazine,12,0,10)&&
        !t.reserve_item(11)&&t.restore_original(10)&&t.confirm_closed(2);
}
constexpr bool partial_close_preserves_plan() {
    Tx t;Inv i{id,1,8,2,10};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::take_ammunition,1,ReloadTxnMode::round,3,10))return false;
    if(!t.reserve_item(1)||t.request_close()||!t.request_insertion(1)||!t.take_request(q)||t.request_close())return false;
    if(!t.acknowledge({q.token,true,{id,2,8,3,9}})||!t.request_close()||t.target()!=3)return false;
    if(!t.confirm_closed(11,100)||!t.take_request(q)||q.result!=ReloadTxnResult::ordinary)return false;
    return t.acknowledge({q.token,true,{id,2,8,3,9},ReloadTxnResult::ordinary})&&t.inserted()==1&&t.target()==3;
}
constexpr bool dispatched_cancel_retains_actual_commit() {
    Tx t;Inv i{id,1,8,2,10};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::take_ammunition,1,ReloadTxnMode::round,3,0)||
        !t.reserve_item(1)||!t.request_insertion(1)||!t.take_request(q)||!t.interrupt())return false;
    if(!t.pending()||t.reserved()!=1||t.resume(i,2,1))return false;
    if(!t.acknowledge({q.token,true,{id,2,8,3,9}})||t.stage()!=ReloadTxnStage::interrupted)return false;
    return t.inventory().loaded==3&&t.resume({id,2,8,3,9},2,1)&&t.inserted()==1&&t.target()==3;
}
constexpr bool undispatched_cancel_has_no_ammo_effect() {
    Tx t;Inv i{id,1,8,2,10};Req q;
    return t.bind(i)&&t.begin(i,1,ReloadIntentSource::take_ammunition,1,ReloadTxnMode::round,3,0)&&
        t.reserve_item(1)&&t.request_insertion(1)&&t.interrupt()&&!t.take_request(q)&&
        t.inventory().loaded==2&&t.inventory().reserve==10&&t.reserved()==0;
}
constexpr bool ambiguous_receipt_must_reconcile() {
    Tx t;Inv i{id,1,8,2,10};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::take_ammunition,1,ReloadTxnMode::round,3,0)||
        !t.reserve_item(1)||!t.request_insertion(1)||!t.take_request(q))return false;
    if(t.acknowledge({q.token,true,{id,2,8,4,8}})||!t.pending()||
        t.stage()!=ReloadTxnStage::reconciliation_required||t.take_request(q))return false;
    if(t.acknowledge({q.token,true,{{9,2,3},2,8,3,9}}))return false;
    if(t.acknowledge({q.token,true,{id,1,8,3,9}}))return false;
    return t.acknowledge({q.token,true,{id,2,8,3,9}})&&t.inserted()==1&&
        t.stage()==ReloadTxnStage::interrupted&&!t.reserve_item(2)&&t.resume({id,2,8,3,9},2,1);
}
constexpr bool rejection_releases_reservation_not_inventory() {
    Tx t;Inv i{id,1,8,2,10};Req q;
    return t.bind(i)&&t.begin(i,1,ReloadIntentSource::take_ammunition,1,ReloadTxnMode::round,3,0)&&
        t.reserve_item(1)&&t.request_insertion(1)&&t.take_request(q)&&t.acknowledge({q.token,false,i})&&
        t.reserved()==0&&t.available_reserve()==10&&!t.reserve_item(1)&&t.reserve_item(2);
}
constexpr bool perfect_requires_full_original_target_and_one_ack() {
    Tx t;Inv i{id,1,2,0,5};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::take_ammunition,1,ReloadTxnMode::round,2,0))return false;
    for(unsigned round=1;round<=2;++round){
        if(!t.reserve_item(round)||!t.request_insertion(round)||!t.take_request(q)||
           !t.acknowledge({q.token,true,{id,round+1,2,round,5-round}}))return false;
    }
    if(!t.confirm_closed(100,100)||!t.take_request(q)||q.result!=ReloadTxnResult::perfect)return false;
    if(!t.acknowledge({q.token,true,{id,3,2,2,3},ReloadTxnResult::perfect}))return false;
    return t.result()==ReloadTxnResult::perfect&&!t.acknowledge({q.token,true,{id,3,2,2,3},ReloadTxnResult::perfect});
}
static_assert(magazine_retains_old_and_commits_once());
static_assert(old_magazine_is_inspection_not_refill());
static_assert(full_or_no_reserve_inspection());
static_assert(partial_close_preserves_plan());
static_assert(dispatched_cancel_retains_actual_commit());
static_assert(undispatched_cancel_has_no_ammo_effect());
static_assert(ambiguous_receipt_must_reconcile());
static_assert(rejection_releases_reservation_not_inventory());
static_assert(perfect_requires_full_original_target_and_one_ack());
constexpr bool infinite_reserve_keeps_original_counter() {
    Tx t;Inv i{id,1,60,17,0,true};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::magazine_pull,1,
        ReloadTxnMode::magazine,60,100,1)||t.target()!=43||!t.stow_original(1)||
        !t.reserve_item(2)||t.reserved()!=43||t.available_reserve()!=0||
        !t.request_insertion(2)||!t.take_request(q))return false;
    if(q.expected.loaded!=60||q.expected.reserve!=0||!q.expected.infinite_reserve)return false;
    return t.acknowledge({q.token,true,{id,2,60,60,0,true}})&&
        t.confirm_closed(150)&&t.take_request(q)&&q.result==ReloadTxnResult::ordinary;
}
constexpr bool reserve_policy_change_requires_reconciliation() {
    Tx t;Inv i{id,1,60,17,80};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::magazine_pull,1,
        ReloadTxnMode::magazine,60,100,1)||!t.stow_original(1)||!t.reserve_item(2)||
        !t.request_insertion(2)||!t.take_request(q))return false;
    return !t.acknowledge({q.token,true,{id,2,60,60,37,true}})&&
        t.stage()==ReloadTxnStage::reconciliation_required&&t.pending();
}
static_assert(infinite_reserve_keeps_original_counter());
static_assert(reserve_policy_change_requires_reconciliation());
constexpr bool recovery_observation_never_redispatches() {
    Tx t;Inv i{id,1,8,2,10};Req dispatched{},observed{};
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::take_ammunition,1,ReloadTxnMode::round,1,0)||
       !t.reserve_item(1)||!t.request_insertion(1)||t.dispatched_request(observed)||
       !t.take_request(dispatched)||!t.require_reconciliation()||
       !t.dispatched_request(observed)||observed.token!=dispatched.token||t.take_request(observed))return false;
    // Partial reserve-only state remains pending. Exact final state can settle
    // the interrupted call without a second setter dispatch.
    if(t.acknowledge({dispatched.token,true,{id,2,8,2,9}})||!t.pending())return false;
    return t.acknowledge({dispatched.token,true,{id,3,8,3,9}})&&
        t.stage()==ReloadTxnStage::interrupted&&!t.dispatched_request(observed)&&
        !t.acknowledge({dispatched.token,true,{id,3,8,3,9}});
}
static_assert(recovery_observation_never_redispatches());
constexpr bool lost_replacement_returns_reservation_without_restarting_deadline() {
    Tx t;Inv i{id,1,60,57,360};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::magazine_pull,1,
        ReloadTxnMode::magazine,60,100,1)||!t.stow_original(1)||
        !t.reserve_item(2)||t.reserved()!=3||!t.release_item(2))return false;
    // No insertion was dispatched: loss returns only the reservation, not
    // original ammo. The next physical grab has a new item token.
    if(t.inventory().loaded!=57||t.inventory().reserve!=360||t.reserved()||
       t.feed_present()||t.reserve_item(2)||!t.reserve_item(3)||
       !t.request_insertion(3)||!t.take_request(q))return false;
    if(!t.acknowledge({q.token,true,{id,2,60,60,357}})||
       !t.confirm_closed(401,300)||!t.take_request(q))return false;
    return q.result==ReloadTxnResult::ordinary&&q.reload_serial==1;
}
static_assert(lost_replacement_returns_reservation_without_restarting_deadline());
constexpr bool suspended_hold_rebind_preserves_counts_and_deadline() {
    Tx t;Inv i{id,1,60,57,360};Req q;
    if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::magazine_pull,7,
        ReloadTxnMode::magazine,60,100,1)||!t.stow_original(1)||!t.interrupt())return false;
    const Identity next_id{1,2,4};Inv next{next_id,2,60,57,360};
    auto changed=next;changed.loaded=58;
    if(t.rebind_suspended(id,changed)||t.rebind_suspended(next_id,next)||
       !t.rebind_suspended(id,next)||t.resume(next,1,7)||t.resume(next,2,8)||
       !t.resume(next,2,7)||t.target()!=3||t.feed_present()||
       !t.reserve_item(2)||!t.request_insertion(2)||!t.take_request(q))return false;
    if(t.rebind_suspended(next_id,next)||q.reload_serial!=7||
       !(q.identity==next_id)||!t.acknowledge({q.token,true,{next_id,3,60,60,357}})||
       !t.confirm_closed(401,300)||!t.take_request(q))return false;
    return q.result==ReloadTxnResult::ordinary;
}
static_assert(suspended_hold_rebind_preserves_counts_and_deadline());
constexpr bool retained_original_rebind_preserves_item_ammo_and_inspection() {
    Tx t;Inv initial{id,1,60,56,360};Req q;
    if(!t.bind(initial)||!t.begin(initial,1,ReloadIntentSource::magazine_pull,7,
        ReloadTxnMode::magazine,60,100,11))return false;
    const Identity next_id{1,2,4};Inv next{next_id,2,60,56,360};
    auto changed=next;changed.reserve=359;
    if(t.rebind_retained_original(id,changed,11)||t.rebind_retained_original(next_id,next,11)||
       t.rebind_retained_original(id,next,12)||!t.rebind_retained_original(id,next,11)||
       t.stage()!=ReloadTxnStage::open||t.feed_present()||t.reserved()||t.pending()||
       t.reserve_item(12)||!t.restore_original(11)||!t.confirm_closed(5000,4000)||!t.take_request(q))return false;
    if(q.call!=ReloadTxnCall::exit_without_fill||q.result!=ReloadTxnResult::inspection||
       q.reload_serial!=7||!(q.identity==next_id))return false;
    return t.acknowledge({q.token,true,next})&&t.inventory().loaded==56&&t.inventory().reserve==360;
}
static_assert(retained_original_rebind_preserves_item_ammo_and_inspection());
constexpr bool retained_original_rebind_rejects_replacement_and_stowed_original() {
    Tx t;Inv initial{id,1,60,56,360};const Identity next_id{1,2,4};Inv next{next_id,2,60,56,360};
    if(!t.bind(initial)||!t.begin(initial,1,ReloadIntentSource::magazine_pull,7,
        ReloadTxnMode::magazine,60,100,11)||!t.stow_original(11))return false;
    if(t.rebind_retained_original(id,next,11)||!t.reserve_item(12)||
       t.rebind_retained_original(id,next,11)||!t.release_item(12)||!t.interrupt())return false;
    return !t.rebind_retained_original(id,next,11);
}
static_assert(retained_original_rebind_rejects_replacement_and_stowed_original());
constexpr bool zero_target_original_inspection_rebind_has_no_reload_reward() {
    for(unsigned i=0;i<2;++i){
        Tx t;Inv initial{id,1,60,i?20u:60u,i?0u:360u};Req q;
        if(!t.bind(initial)||!t.begin(initial,1,ReloadIntentSource::magazine_pull,0,
            ReloadTxnMode::magazine,60,100,11)||t.target()!=0)return false;
        const Identity next_id{1,2,4};auto next=initial;next.identity=next_id;next.revision=2;
        if(!t.rebind_retained_original(id,next,11)||t.reserve_item(12)||
           !t.restore_original(11)||!t.confirm_closed(5000,4000)||t.pending()||t.take_request(q)||
           t.stage()!=ReloadTxnStage::complete||t.result()!=ReloadTxnResult::inspection||
           !(t.inventory().identity==next_id)||t.inventory().loaded!=initial.loaded||
           t.inventory().reserve!=initial.reserve)return false;
    }
    return true;
}
static_assert(zero_target_original_inspection_rebind_has_no_reload_reward());
constexpr bool discarded_magazine_rounds_can_return_without_spare_or_setters() {
    for(unsigned remaining=1;remaining<=60;++remaining){
        Tx t;Inv i{id,1,60,remaining,0};Req q;
        if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::magazine_pull,0,
            ReloadTxnMode::magazine,60,10,11)||t.reserve_magazine_reusing_loaded(12)||
            !t.stow_original(11)||t.reserve_item(12)||
            !t.reserve_magazine_reusing_loaded(12)||t.reserved()||t.available_reserve()||
            t.held_magazine_rounds()!=remaining||t.reserve_magazine_reusing_loaded(13)||
            t.request_insertion(12)||t.seat_reused_rounds(13)||!t.release_item(12)||
            t.reserve_magazine_reusing_loaded(12)||!t.reserve_magazine_reusing_loaded(13)||
            !t.seat_reused_rounds(13)||t.take_request(q)||t.pending()||
            !t.confirm_closed(20,4000)||t.result()!=ReloadTxnResult::inspection||
            !t.mechanical_ready()||t.take_request(q)||t.inventory().loaded!=remaining||
            t.inventory().reserve||t.inventory().revision!=i.revision)return false;
    }
    return true;
}
static_assert(discarded_magazine_rounds_can_return_without_spare_or_setters());
constexpr bool total_zero_cannot_create_a_replacement_magazine() {
    Tx t;Inv i{id,1,60,0,0};Req q;
    return t.bind(i)&&t.begin(i,1,ReloadIntentSource::magazine_pull,0,
        ReloadTxnMode::magazine,60,10,11)&&t.stow_original(11)&&
        !t.reserve_magazine_reusing_loaded(12)&&!t.reserve_item(12)&&
        !t.held_magazine_rounds()&&!t.seat_reused_rounds(12)&&!t.pending()&&
        !t.take_request(q)&&t.inventory().loaded==0&&t.inventory().reserve==0;
}
static_assert(total_zero_cannot_create_a_replacement_magazine());
constexpr bool partially_filled_replacements_never_create_extra_rounds() {
    for(unsigned loaded=0;loaded<60;++loaded)for(unsigned reserve=1;reserve<=3;++reserve){
        Tx t;Inv i{id,1,60,loaded,reserve};Req q;
        const auto used=std::min(60-loaded,reserve);
        if(!t.bind(i)||!t.begin(i,1,ReloadIntentSource::magazine_pull,7,
            ReloadTxnMode::magazine,60,10,11)||!t.stow_original(11)||
            !t.reserve_magazine_reusing_loaded(12)||t.reserved()!=used||
            t.held_magazine_rounds()!=loaded+used||!t.request_insertion(12)||
            !t.take_request(q)||q.expected.loaded!=loaded+used||
            q.expected.reserve!=reserve-used||q.expected.loaded+q.expected.reserve!=loaded+reserve||
            !t.acknowledge({q.token,true,{id,2,60,loaded+used,reserve-used}}))return false;
    }
    return true;
}
static_assert(partially_filled_replacements_never_create_extra_rounds());
constexpr bool empty_supply_feedback_once_per_real_press_lifetime() {
    ReloadSupplyRejection f;
    return !f.reject(0,1)&&!f.reject(1,0)&&!f.rejected(1,1)&&f.reject(1,1)&&
        f.rejected(1,1)&&!f.reject(1,1)&&f.reject(1,2)&&!f.reject(1,2)&&
        !f.rejected(2,2)&&f.reject(2,2)&&!f.reject(2,2);
}
static_assert(empty_supply_feedback_once_per_real_press_lifetime());
// Workbench repeated-full-magazine report: the retail adapter's explicit
// zero-target transaction must reopen repeatedly without reset/prime, filling,
// a fake reload serial, a spare reservation or loss of the retained L rounds.
constexpr bool repeated_inspection_replacement_keeps_ammo_and_nonce_floors() {
    for(unsigned loaded:{2u,60u})for(unsigned spare:{0u,120u}){
        if(loaded!=60&&spare)continue; // A real deficit uses the existing refill path.
        Tx t;const Inv initial{id,1,60,loaded,spare};Req q;
        if(!t.bind(initial))return false;
        for(unsigned cycle=0;cycle<3;++cycle){
            const auto intent=std::uint64_t(cycle+1),old_item=std::uint64_t(10+cycle*3),replacement=old_item+1;
            const auto before=t.inventory();
            if(!t.begin(before,intent,ReloadIntentSource::magazine_pull,0,
                ReloadTxnMode::magazine,60,100+cycle*20,old_item)||
               !t.stow_original(old_item)||!t.reserve_magazine_reusing_loaded(replacement)||
               t.held_magazine_rounds()!=loaded||t.reserved()||
               !t.seat_reused_rounds(replacement)||!t.confirm_closed(110+cycle*20,4000)||
               !t.mechanical_ready()||t.result()!=ReloadTxnResult::inspection||t.take_request(q)||
               t.inventory().loaded!=loaded||t.inventory().reserve!=spare||
               t.inventory().revision!=initial.revision||
               t.begin(t.inventory(),intent,ReloadIntentSource::magazine_pull,0,
                   ReloadTxnMode::magazine,60,111+cycle*20,replacement+1))return false;
        }
    }
    return true;
}
static_assert(repeated_inspection_replacement_keeps_ammo_and_nonce_floors());
// Compile-time contract checks; this translation unit has no main or runtime.
