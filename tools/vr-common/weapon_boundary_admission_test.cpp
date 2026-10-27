#include "weapon_boundary_admission.h"
#include <cassert>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#ifdef NDEBUG
#error BoundaryAdmissionTest requires active assertions
#endif
namespace {
namespace boundary=gears_weapon_boundary;
namespace hand=gears_weapon_hand;
unsigned context{};std::atomic<unsigned> calls{};
hand::Reply revoke(boundary::Request const*,std::uint64_t* generation) noexcept {++calls;*generation=9;return hand::Reply::accepted;}
hand::Reply no_scope(boundary::Request const*,boundary::State*,boundary::Receipt*) noexcept {++calls;return hand::Reply::unavailable;}
boundary::End close_scope(boundary::State*,boundary::Receipt const*,boundary::Finish const*) noexcept {++calls;return boundary::End::closed;}
boundary::Binding binding(std::uint64_t cookie=33){return {cookie,22,&context,revoke,no_scope,close_scope};}
struct Signal {
    std::mutex mutex;std::condition_variable cv;bool set{};
    void arrive(){std::lock_guard<std::mutex> lock(mutex);set=true;cv.notify_one();}
    void wait(){std::unique_lock<std::mutex> lock(mutex);cv.wait(lock,[this]{return set;});}
};
}
int main(){
    unsigned cases{};
    {boundary::Admission channel;boundary::Admission::Entry entry{};
        assert(!channel.acquire(entry)&&!channel.view(entry));assert(channel.quiescent());++cases;}
    {boundary::Admission channel;assert(channel.connect(binding()));boundary::Admission::Entry entry{};
        assert(channel.acquire(entry));auto api=channel.view(entry);assert(api&&api->cookie==33);
        boundary::Request request{};boundary::State state{};boundary::Receipt receipt{};
        assert(api->open(&request,&state,&receipt)==hand::Reply::unavailable);
        for(auto value:state.storage)assert(value==std::byte{});
        assert(!receipt.flags&&!receipt.scope_id&&channel.in_flight()==1);
        channel.seal();assert(!channel.quiescent()&&!channel.detach());
        std::uint64_t generation{};assert(api->revoke(&request,&generation)==hand::Reply::accepted&&generation==9);
        assert(channel.release(entry)&&!channel.view(entry)&&channel.quiescent()&&channel.detach());++cases;}
    {boundary::Admission channel;assert(channel.connect(binding()));boundary::Admission::Entry first{},second{};
        assert(channel.acquire(first)&&channel.acquire(second)&&channel.in_flight()==2);
        assert(channel.release(first)&&channel.in_flight()==1);
        assert(channel.release(first)&&channel.in_flight()==1); // Cannot release the other entry.
        channel.seal();assert(!channel.detach());assert(channel.release(second)&&channel.detach());++cases;}
    {boundary::Admission channel;assert(channel.connect(binding()));boundary::Admission::Entry original{};
        assert(channel.acquire(original));auto copy=original;
        assert(!channel.view(copy)&&!channel.release(copy)&&channel.faulted());
        assert(channel.in_flight()==1&&channel.release(original));assert(!channel.quiescent()&&!channel.detach());++cases;}
    {boundary::Admission channel;assert(channel.connect(binding()));boundary::Admission::Entry old{};
        assert(channel.acquire(old)&&channel.release(old));channel.seal();assert(channel.detach());
        assert(channel.connect(binding(44)));boundary::Admission::Entry current{};
        assert(channel.acquire(current)&&channel.view(current)->cookie==44&&!channel.view(old));
        assert(channel.release(old)&&channel.in_flight()==1);channel.seal();assert(channel.release(current)&&channel.detach());++cases;}
    {boundary::Admission channel;assert(channel.connect(binding()));Signal admitted,finish;
        std::thread gt([&]{boundary::Admission::Entry entry{};assert(channel.acquire(entry));admitted.arrive();finish.wait();
            assert(channel.view(entry)&&channel.view(entry)->cookie==33);assert(channel.release(entry));});
        admitted.wait();channel.seal();assert(!channel.quiescent()&&!channel.detach());
        boundary::Admission::Entry rejected{};assert(!channel.acquire(rejected)&&!channel.view(rejected));
        finish.arrive();gt.join();assert(channel.quiescent()&&channel.detach());++cases;}
    {boundary::Admission channel;assert(channel.connect(binding()));Signal sealed;
        std::thread gt([&]{sealed.wait();boundary::Admission::Entry entry{};assert(!channel.acquire(entry));});
        channel.seal();assert(channel.detach());sealed.arrive();gt.join();++cases;}
    {boundary::Admission channel;auto missing=binding();missing.open=nullptr;
        assert(!channel.connect(missing)&&channel.quiescent());assert(channel.connect(binding()));
        assert(!channel.connect(binding(99)));channel.seal();assert(channel.detach());++cases;}
    std::printf("BoundaryAdmissionTest passed: %u cases; local lifetime/CAS and fake NoAuth exports only\n",cases);
}
