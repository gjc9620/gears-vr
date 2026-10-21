#include "ngx_source_activity.h"
#include <cassert>
#include <cstdio>
int main(){
    gears_ngx_source::Activity s;assert(s.eligible());
    s.accepted_pair(10);s.consumed(10,true);assert(s.eligible());
    s.consumed(11,false);assert(!s.eligible()&&s.gap_frames==1);
    s.consumed(12,false);s.accepted_pair(10);assert(!s.eligible());
    // Seeing a pair in the new epoch is insufficient until live admission has
    // separately certified its SDK returns, copies and transport.
    s.hold(13);s.consumed(13,true);assert(!s.eligible());
    s.accepted_pair(12);assert(!s.eligible()&&s.recoveries==0);
    s.accepted_pair(14);assert(s.eligible()&&s.recoveries==1);
    s.consumed(11,false);assert(s.eligible()); // Older completion cannot roll back current use.
    s.hold(20);s.accepted_pair(19);assert(!s.eligible());
    s.accepted_pair(20);assert(s.eligible()&&s.recoveries==2);
    s.hold(0);s.consumed(0,false);s.accepted_pair(0);assert(s.eligible());
    puts("NgxSourceActivityTest passed: cached inactive feature, candidate hold, stale receipt and accepted-pair recovery");
}
