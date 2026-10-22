#include "equipment_switch_mailbox.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
#include <future>
#include <initializer_list>
#include <limits>
#include <thread>
using gears_weapon::EquipmentSwitchMailbox;

namespace {
void stow_then_unholster_does_not_publish_the_previous_result(){
    EquipmentSwitchMailbox requests;
    requests.observe(10);assert(requests.status()==10);
    assert(requests.enqueue(3)&&requests.pending()&&requests.status()==0);
    assert(requests.take()==3&&requests.pending()&&requests.status()==0);
    requests.observe(11);
    assert(requests.pending()&&requests.status()==0);
    assert(requests.complete()&&!requests.pending()&&requests.status()==11);
}

void only_the_completed_consumer_releases_busy(){
    EquipmentSwitchMailbox requests;
    assert(!requests.complete()&&requests.take()==EquipmentSwitchMailbox::no_slot);
    assert(requests.enqueue(1));
    requests.observe(6); // An unrelated/asynchronous observation is not completion.
    assert(!requests.complete()&&requests.pending()&&requests.status()==0);
    assert(!requests.enqueue(2));
    assert(requests.take()==1);
    assert(requests.take()==EquipmentSwitchMailbox::no_slot&&!requests.enqueue(2));
    std::thread observer([&]{requests.observe(7);});observer.join();
    assert(requests.pending()&&requests.status()==0&&!requests.enqueue(2));
    assert(requests.complete()&&requests.status()==7&&!requests.pending());
    assert(!requests.complete());
    requests.observe(6);assert(requests.status()==6); // Later original completion.
    assert(requests.enqueue(2)&&requests.take()==2);
    assert(requests.complete()&&requests.status()==0); // No observation cannot reuse 6.
}

void invalid_slots_never_change_the_request_or_result(){
    EquipmentSwitchMailbox requests;requests.observe(10);
    for(const auto slot:{4u,5u,std::numeric_limits<unsigned>::max()}){
        assert(!requests.enqueue(slot)&&!requests.pending()&&requests.status()==10);
    }
    assert(requests.enqueue(0));
    assert(!requests.enqueue(4)&&requests.take()==0);
    assert(!requests.enqueue(3));requests.observe(11);
    assert(requests.complete()&&requests.status()==11);
}

void published_slot_and_completion_survive_cross_thread_polling(){
    EquipmentSwitchMailbox requests;requests.observe(10);
    assert(requests.enqueue(3));
    std::promise<void> observed,finish;
    auto observed_done=observed.get_future();auto finish_now=finish.get_future();
    std::thread consumer([&]{
        assert(requests.take()==3);
        requests.observe(11);observed.set_value();
        finish_now.wait();assert(requests.complete());
    });
    observed_done.wait();
    // The original action/readback has reported 11, but its callback has not
    // finished. A polling owner must neither see old 10 nor enqueue again.
    assert(requests.pending()&&requests.status()==0&&!requests.enqueue(0));
    assert(requests.take()==EquipmentSwitchMailbox::no_slot);
    finish.set_value();consumer.join();
    assert(!requests.pending()&&requests.status()==11);
}

void competing_producers_publish_exactly_one_slot(){
    EquipmentSwitchMailbox requests;
    std::promise<void> start;auto ready=start.get_future().share();
    bool first{},second{};
    std::thread a([&]{ready.wait();first=requests.enqueue(1);});
    std::thread b([&]{ready.wait();second=requests.enqueue(2);});
    start.set_value();a.join();b.join();
    assert(first!=second&&requests.pending()&&requests.status()==0);
    assert(requests.take()==(first?1u:2u));
    requests.observe(6);assert(requests.complete()&&requests.status()==6);
}

void reset_after_callbacks_drain_starts_a_new_lifecycle(){
    EquipmentSwitchMailbox requests;
    assert(requests.enqueue(2)&&requests.take()==2);requests.observe(12);
    // No callback is running here. Teardown may discard an unfinished request.
    requests.reset();
    assert(!requests.pending()&&requests.status()==0);
    assert(requests.take()==EquipmentSwitchMailbox::no_slot&&!requests.complete());
    assert(requests.enqueue(0)&&requests.take()==0);requests.observe(11);
    assert(requests.complete()&&requests.status()==11);
    requests.reset();assert(!requests.pending()&&requests.status()==0);
}
}

int main(){
    stow_then_unholster_does_not_publish_the_previous_result();
    only_the_completed_consumer_releases_busy();
    invalid_slots_never_change_the_request_or_result();
    published_slot_and_completion_survive_cross_thread_polling();
    competing_producers_publish_exactly_one_slot();
    reset_after_callbacks_drain_starts_a_new_lifecycle();
    std::puts("equipment switch mailbox keeps queued and processing requests pending until completion");
}
