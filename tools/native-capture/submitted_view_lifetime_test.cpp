#include "submitted_view_lifetime.h"
#include <atomic>
#include <thread>
#include <cstdio>
int main() {
    gears_submitted::Lifetime life;
    if(life.begin(0) || !life.begin(1) || life.begin(1) || !life.begin(2))return 1;
    if(life.close() || life.begin(3) || life.retire(99) || life.retire(1))return 2;
    if(!life.retire(2) || life.close() || life.retire(2) || life.pending())return 3;
    gears_submitted::Lifetime pool;
    for(unsigned i=1;i<=64;++i)if(!pool.begin(i))return 4;
    if(pool.begin(65) || pool.retire(1) || !pool.begin(65))return 5;
    if(pool.close())return 6;
    std::atomic<unsigned> claims{};
    auto retire=[&](unsigned first,unsigned last){for(unsigned i=first;i<=last;++i)if(pool.retire(i))++claims;};
    std::thread a(retire,2,33),b(retire,34,65);a.join();b.join();
    if(claims!=1 || pool.pending() || pool.close())return 7;
    gears_submitted::Lifetime reused;
    if(!reused.begin(42) || reused.retire(42) || !reused.begin(42) || reused.close() || !reused.retire(42))return 8;
    printf("Submitted lifetime: bounded pool, duplicate/reused roots, concurrent retire and one release claim passed\n");
    return 0;
}
