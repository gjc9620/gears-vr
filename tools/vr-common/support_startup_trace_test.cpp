#include "support_startup_trace.h"
#ifdef NDEBUG
#error SupportStartupTraceTest requires assertions
#endif
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

std::string read(std::filesystem::path const& p){std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int wmain(){
    wchar_t temp[MAX_PATH]{};assert(GetTempPathW(MAX_PATH,temp));
    const auto dir=std::filesystem::path(temp)/(L"gears-startup-trace-"+std::to_wstring(GetCurrentProcessId()));
    assert(std::filesystem::create_directory(dir));
    const auto path=dir/(L"support-startup-"+std::to_wstring(GetCurrentProcessId())+L".jsonl");
    {
        gears_startup::Trace trace;trace.initialize(dir.c_str());
        SetLastError(1234);errno=EDOM;trace.record("blocked_query","enter");
        assert(GetLastError()==1234&&errno==EDOM);
        // The entry is independently readable while the simulated operation
        // has not returned and no session-status file exists.
        auto data=read(path);assert(data.find("blocked_query")!=std::string::npos);
        assert(data.find("\"phase\":\"enter\"")!=std::string::npos);
        assert(data.find("\"phase\":\"exit\"")==std::string::npos);
        trace.record("status_replace","exit",false,0,ERROR_SHARING_VIOLATION);
        assert(read(path).find("\"winerror\":32")!=std::string::npos);
        unsigned count{};const auto result=trace.call("query",[&]{++count;return 42;});assert(result==42&&count==1);
        trace.call("void_query",[&]{++count;});assert(count==2);
        // A genuinely contended mutex leaves an independently readable
        // substep before acquisition. Releasing OUR fixture lock allows exit;
        // the returned guard must retain ownership through its original scope.
        std::mutex fixture;fixture.lock();std::atomic<bool> acquired{};
        std::thread waiter([&]{auto guard=trace.acquire("owned_gpu_frame_lock",fixture);
            acquired=true;assert(guard.owns_lock());});
        const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(3);
        do{data=read(path);if(data.find("owned_gpu_frame_lock")!=std::string::npos)break;
           std::this_thread::yield();}while(std::chrono::steady_clock::now()<until);
        assert(data.find("\"stage\":\"owned_gpu_frame_lock\",\"phase\":\"enter\"")!=std::string::npos);
        assert(data.find("\"stage\":\"owned_gpu_frame_lock\",\"phase\":\"exit\"")==std::string::npos);
        assert(!acquired.load());fixture.unlock();waiter.join();assert(acquired.load());
        assert(read(path).find("\"stage\":\"owned_gpu_frame_lock\",\"phase\":\"exit\"")!=std::string::npos);
        assert(fixture.try_lock());fixture.unlock();
        for(unsigned i=0;i<gears_startup::Trace::capacity+50;++i)trace.record("loop","enter");
        data=read(path);assert(static_cast<unsigned>(std::count(data.begin(),data.end(),'\n'))==gears_startup::Trace::capacity);
        assert(data.find("\"stage\":\"trace_budget\",\"phase\":\"complete\"")!=std::string::npos);
        assert(data.size()<768*gears_startup::Trace::capacity);
    }
    assert(std::filesystem::remove(path));assert(std::filesystem::remove(dir));
    std::puts("SupportStartupTraceTest passed: pending entry readable, error fidelity, unchanged call results, bounded termination");
}
