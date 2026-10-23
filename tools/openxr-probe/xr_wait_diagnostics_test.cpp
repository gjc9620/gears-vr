#include "xr_wait_diagnostics.h"
#include <cassert>
#include <string>
using namespace xr_wait_diagnostics;
struct InjectedClock {
    int* calls;std::int64_t begin,end;
    std::int64_t operator()() const {return (*calls)++==0?begin:end;}
};
static FILE* temporary(){FILE* file{};assert(tmpfile_s(&file)==0&&file);return file;}
static std::string contents(FILE* file){rewind(file);std::string text;char buffer[256];while(auto count=fread(buffer,1,sizeof(buffer),file))text.append(buffer,count);return text;}
int main(){
    int clocks=0,calls=0,contexts=0,classifications=0;
    Recorder<true,InjectedClock> enabled({&clocks,100,11112100});
    const auto result=enabled.measure(Operation::wait_eye0,[&]{assert(clocks==1);++calls;return -7;},
        [&]{assert(clocks==2);++contexts;return Context::focused_render;},
        [&](int value){++classifications;assert(value==-7);return Outcome::error;});
    assert(result==-7&&clocks==2&&calls==1&&contexts==1&&classifications==1);
    const auto& error=enabled.values[unsigned(Operation::wait_eye0)][unsigned(Context::focused_render)];
    assert(error.count==1&&error.errors==1&&error.total_ns==11112000&&error.buckets[6]==1);
    int timeout_clocks=0;
    Recorder<true,InjectedClock> timeout({&timeout_clocks,0,1000000000});
    assert(timeout.measure(Operation::wait_eye1,[]{return 1;},[]{return Context::not_rendering;},
        [](int value){return value==1?Outcome::timeout:Outcome::success;})==1);
    assert(timeout.values[2][2].timeouts==1&&timeout.values[2][2].buckets[9]==1&&timeout_clocks==2);
    Recorder<false,InjectedClock> disabled({&clocks,1,2});
    assert(disabled.measure(Operation::wait_frame,[&]{++calls;return 42;},
        [&]{++contexts;return Context::unknown;},[&](int){++classifications;return Outcome::error;})==42);
    assert(clocks==2&&calls==2&&contexts==1&&classifications==1);
    assert(disabled.values[0][0].count==0);
    assert(context(5,true)==Context::focused_render&&context(4,true)==Context::other_render);
    assert(context(5,false)==Context::not_rendering&&context(4,false)==Context::not_rendering);
    for(unsigned i=0;i<bounds_ns.size();++i){
        Statistics s;s.observe(0,std::int64_t(bounds_ns[i]),Outcome::success);assert(s.buckets[i]==1);
        s.observe(0,std::int64_t(bounds_ns[i]+1),Outcome::timeout);assert(s.buckets[i+1]==1&&s.timeouts==1);
    }
    Statistics backwards;backwards.observe(2,1,Outcome::error);backwards.observe(-1,1,Outcome::timeout);
    assert(backwards.count==2&&backwards.invalid_clock==2&&backwards.errors==1&&backwards.timeouts==1&&backwards.total_ns==0);
    Statistics overflow;
    for(unsigned i=0;i<3;++i)overflow.observe(0,std::numeric_limits<std::int64_t>::max(),Outcome::success);
    assert(overflow.total_ns==std::numeric_limits<std::uint64_t>::max());
    overflow.count=overflow.errors=overflow.buckets[0]=std::numeric_limits<std::uint64_t>::max();
    overflow.observe(0,0,Outcome::error);
    assert(overflow.count==std::numeric_limits<std::uint64_t>::max()&&overflow.errors==overflow.count&&overflow.buckets[0]==overflow.count);
    // Exercise the production steady clock as well as deterministic injection.
    Recorder<true> real;
    real.measure(Operation::end_frame,[]{return 0;},[]{return Context::not_rendering;},[](int){return Outcome::success;});
    assert(real.values[4][2].count==1&&real.values[4][2].invalid_clock==0);
    auto* file=temporary();enabled.dump(file);const auto first=contents(file);enabled.dump(file);
    assert(!first.empty()&&contents(file)==first&&first.find("xrWaitSwapchainImage_eye0")!=std::string::npos);
    assert(first.find("focused_should_render")!=std::string::npos&&first.find("\"errors\":1")!=std::string::npos);fclose(file);
    file=temporary();timeout.dump(file);assert(contents(file).find("should_render_false")!=std::string::npos);fclose(file);
    Recorder<true> unused;file=temporary();unused.dump(file);assert(contents(file).empty());fclose(file);
    file=temporary();disabled.dump(file);assert(contents(file).empty());fclose(file);
    assert(enabled.total(Operation::wait_eye0).count==1&&enabled.total(Operation::wait_eye0).ns==11112000);
    enabled.values[unsigned(Operation::wait_eye0)][unsigned(Context::unknown)].observe(1,11,Outcome::success);
    assert(enabled.total(Operation::wait_eye0).count==2&&enabled.total(Operation::wait_eye0).ns==11112010);
    assert(disabled.total(Operation::wait_frame).count==0&&disabled.total(Operation::wait_frame).ns==0);
    puts("XR wait diagnostics checks passed");
}
