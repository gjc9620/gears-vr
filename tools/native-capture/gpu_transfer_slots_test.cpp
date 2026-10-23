#include "../vr-common/gpu_transfer_slots.h"
#include <cstdio>
#include <cstdlib>
using namespace gears_gpu_slots;
static unsigned checks{};
static void check(bool condition) { ++checks; if (!condition) std::abort(); }
int main() {
    std::array<FenceState, count> slots{};
    check(choose(slots, 0) == 0 && choose(slots, 1) == 1 && drained(slots));
    slots[0] = {1, 0, 0, true, false};
    check(choose(slots, 0) == 1 && !drained(slots));
    slots[1] = {2, 2, 0, false, false};
    check(choose(slots, 0) == none);
    slots[1].done = 2; // ACK for slot 1 cannot release slot 0.
    check(choose(slots, 0) == 1 && !reusable(slots[0]));
    slots[0].ready = 1;
    check(producer_complete(slots[0]) && !reusable(slots[0]));
    slots[0].in_flight = false;
    check(!reusable(slots[0]));
    slots[0].done = 1;
    check(drained(slots));
    slots[0].quarantined = true; // Execute followed by failed Signal.
    check(!producer_complete(slots[0]) && !reusable(slots[0]) && choose(slots, 0) == 1);
    slots[0].quarantined = false;
    slots[0].ready = removed;
    check(!drained(slots));
    slots[0].ready = 1; slots[0].done = removed;
    check(!drained(slots));
    check(next({8, 7}, 6) == 1 && next({8, 7}, 7) == 0 && next({8, 7}, 8) == none);
    check(next({0, 1}, 0) == 1 && next({removed, 1}, 1) == none);
    // Adversarial two-load interleaving: load old slot 0, producer publishes
    // seq 3 in slot 0 and seq 4 in slot 1, then load slot 1. Do not acquire 4.
    check(next({2, 4}, 2) == none && next({3, 4}, 2) == 0 && next({3, 4}, 3) == 1);
    check(next({removed, 0}, removed - 1) == none);
    Lease current{7, 12, 0};
    check(same(current, current));
    check(!same(current, {6, 12, 0}) && !same(current, {7, 11, 0}) && !same(current, {7, 12, 1}));
    check(!valid({0, 1, 0}) && !valid({1, 0, 0}) && !valid({1, 1, none}) && !valid({1, removed, 0}));
    // Exhaust all independent ready/done/in-flight combinations for two leases.
    for (unsigned mask = 0; mask < 64; ++mask) {
        for (unsigned i = 0; i < count; ++i) {
            const unsigned bits = (mask >> (3 * i)) & 7;
            slots[i] = {i + 1, bits & 1 ? i + 1u : 0u, bits & 2 ? i + 1u : 0u, (bits & 4) != 0, false};
        }
        const bool a = (mask & 7) == 3, b = ((mask >> 3) & 7) == 3;
        check(reusable(slots[0]) == a && reusable(slots[1]) == b);
        check(choose(slots, 0) == (a ? 0u : b ? 1u : none));
        check(choose(slots, 1) == (b ? 1u : a ? 0u : none));
        check(drained(slots) == (a && b));
    }
    check(consumer_ready(3,2,0,true)==ConsumerReady::defer);
    check(consumer_ready(3,3,0,true)==ConsumerReady::proceed);
    check(consumer_ready(3,2,0,false)==ConsumerReady::proceed);
    check(consumer_ready(3,removed,0,true)==ConsumerReady::device_removed);
    check(consumer_ready(3,3,removed,true)==ConsumerReady::device_removed);
    check(consumer_ready(3,removed,removed,false)==ConsumerReady::device_removed);
    std::printf("{\"two_slot_policy_checks\":%u}\n", checks);
}
