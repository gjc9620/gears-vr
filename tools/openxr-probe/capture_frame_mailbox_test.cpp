#include "capture_frame_mailbox.h"
#include "menu_capture_epoch.h"
#include <atomic>
#include <cassert>
#include <cstdio>
#include <memory>
#include <thread>

struct FakeFrame {
    std::shared_ptr<std::atomic<unsigned>> closes;
    FakeFrame(std::nullptr_t=nullptr){}
    explicit FakeFrame(std::shared_ptr<std::atomic<unsigned>> count):closes(std::move(count)){}
    explicit operator bool()const{return bool(closes);}
    void Close(){++*closes;}
};

int main(){
    using Mailbox=CaptureFrameMailbox<FakeFrame>;
    auto first=std::make_shared<std::atomic<unsigned>>(0),second=std::make_shared<std::atomic<unsigned>>(0);
    Mailbox box;box.publish(FakeFrame(first));box.publish(FakeFrame(second));
    assert(*first==1&&*second==0);auto taken=box.take();assert(taken&&!box.take());
    box.retire();assert(*second==0);taken.Close();assert(*second==1);

    // Simulate an already scheduled callback arriving after the binding is retired.
    auto old=std::make_shared<Mailbox>(),next=std::make_shared<Mailbox>();
    auto late=std::make_shared<std::atomic<unsigned>>(0),fresh=std::make_shared<std::atomic<unsigned>>(0);
    std::atomic<bool> released=false;
    std::thread callback([old,late,&released]{while(!released.load())std::this_thread::yield();old->publish(FakeFrame(late));});
    old->retire();next->publish(FakeFrame(fresh));released=true;callback.join();
    assert(*late==1&&!old->take()&&*fresh==0);
    next->mark_closed();assert(next->closed()&&*fresh==1&&!next->take());
    next->publish(FakeFrame(fresh));assert(*fresh==2&&!next->take());

    // A new binding with the same dimensions still needs a frame after its epoch.
    MenuCaptureEpoch epoch;epoch.begin(100);assert(epoch.accept(101,102));
    epoch.begin(200);assert(!epoch.fresh(201)&&!epoch.accept(199,201));
    assert(epoch.accept(201,202));
    std::puts("capture mailbox: retirement, late callback, closed item and new epoch passed");
}
