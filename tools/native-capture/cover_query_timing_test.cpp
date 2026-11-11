#define GEARS_QUERY_SESSION_TEST_MAIN TimingPriorMain
#include "cover_physics_query_live_test.cpp"
#undef GEARS_QUERY_SESSION_TEST_MAIN
namespace timing_test=gears_cover_query_timing;
static std::int64_t now_ns=100;
static unsigned clock_calls{};
static std::int64_t clock_value()noexcept{++clock_calls;now_ns+=10;return now_ns;}
static std::int64_t backwards()noexcept{return --now_ns;}
int main(){
    if(TimingPriorMain())return 1;
    timing_test::Report report{},nested{};unsigned invoked{};
    C(timing_test::measure(timing_test::Stage::engine,[&](){++invoked;return 17;})==17&&invoked==1&&!clock_calls);
    {
        timing_test::Scope disabled(false,report,clock_value);
        C(timing_test::measure(timing_test::Stage::engine,[](){return 18;})==18&&!clock_calls);
    }
    {
        timing_test::Scope outer(true,report,clock_value);
        int value=7;int& reference=timing_test::measure(timing_test::Stage::physics_initial,[&]()->int&{return value;});
        C(&reference==&value&&report.bins[0].calls==1&&report.bins[0].completed==1&&report.bins[0].total_ns==10);
        {
            timing_test::Scope nested_scope(true,nested,clock_value);
            C(timing_test::measure(timing_test::Stage::collider_initial,[](){return 19;})==19);
        }
        C(nested.bins[1].calls==1&&!report.bins[1].calls);
        try{timing_test::measure(timing_test::Stage::physics_repeat,[]()->int{throw 23;});C(false);}catch(int n){C(n==23);}
        C(report.bins[3].calls==1&&report.bins[3].completed==1&&report.bins[3].total_ns==10);
        const auto prior=clock_calls;timing_test::measure(timing_test::Stage::count,[](){});C(clock_calls==prior);
    }
    C(timing_test::active==nullptr);
    const auto prior=clock_calls;timing_test::measure(timing_test::Stage::engine,[](){});C(clock_calls==prior);
    {timing_test::Scope bad(true,nested,backwards);timing_test::measure(timing_test::Stage::engine,[](){});}
    C(nested.invalid==1&&nested.bins[6].calls==1&&!nested.bins[6].completed);
    auto& bin=report.bins[7];bin.total_ns=~std::uint64_t{};
    {timing_test::Scope overflow(true,report,clock_value);timing_test::measure(timing_test::Stage::closing,[](){});}
    C(report.overflow==1&&bin.total_ns==~std::uint64_t{});
    // Exercise actual production Session with its existing memory fixtures.
    // Original dispatch remains the existing Host seam, never game execution.
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    reset();auto owner=std::make_unique<HostSession>();C(owner->start(true,true));
    q::Overlap plain{},observed{};
    const auto baseline=owner->overlap(actor,physics_expected,test_box,source_read,guard,plain);
    timing_test::Report query{};
    {timing_test::Scope scope(true,query,clock_value);
        C(owner->overlap(actor,physics_expected,test_box,source_read,guard,observed)==baseline&&observed.raw_hit==plain.raw_hit);}
    C(query.bins[0].calls==1&&query.bins[3].calls>0&&query.bins[6].calls==1&&query.bins[7].calls==1);
    C(query.bins[6].total_ns>=query.bins[3].total_ns&&!query.invalid&&!query.overflow);
    for(auto const& b:query.bins)C(b.calls==b.completed);
    timing_test::Report refusal_timing{};observed.raw_hit=false;
    {timing_test::Scope scope(true,refusal_timing,clock_value);
        C(owner->overlap(actor,physics_expected,test_box,source_read,[](){return false;},observed)==q::Code::source&&!observed.raw_hit);}
    C(refusal_timing.bins[0].calls==1&&!refusal_timing.bins[6].calls&&!timing_test::active);
    owner->request_drain();C(owner->poll_stop());
    C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverQueryTiming checks=%u failures=%u observer only; nested times overlap; no original call\n",checks,failures);
    return failures?1:0;
}
