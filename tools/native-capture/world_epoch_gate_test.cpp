#include "world_epoch_gate.h"
#include <atomic>
#include <thread>
#include <cstdio>
#include <array>

// The integration site, not the header, owns this thread-local instance.
thread_local gears_world::ThreadState recursive_thread;
static bool recursive_admission_tests(){
    using namespace gears_world;
    EpochGate gate,other;EpochGate::CloseToken token{};
    {
        RecursiveAdmission outer(recursive_thread,gate);
        if(!outer||outer.epoch()!=1||recursive_thread.depth!=1)return false;
        {
            RecursiveAdmission nested(recursive_thread,gate);
            if(!nested||nested.epoch()!=outer.epoch()||recursive_thread.depth!=2||gate.try_close(token))return false;
            {
                RecursiveAdmission deeper(recursive_thread,gate);
                if(!deeper||recursive_thread.depth!=3||gate.try_close(token))return false;
            }
            if(recursive_thread.depth!=2)return false;
        }
        if(recursive_thread.depth!=1||gate.try_close(token))return false;
        std::atomic<bool> thread_ok{};auto* main_state=&recursive_thread;
        std::thread worker([&]{
            if(&recursive_thread==main_state||recursive_thread.depth)return;
            {
                RecursiveAdmission independent(recursive_thread,gate);
                if(!independent||recursive_thread.depth!=1)return;
                RecursiveAdmission nested(recursive_thread,gate);
                if(!nested||recursive_thread.depth!=2)return;
            }
            thread_ok=recursive_thread.depth==0&&!recursive_thread.admitted&&!recursive_thread.epoch&&!recursive_thread.gate;
        });
        worker.join();
        if(!thread_ok||recursive_thread.depth!=1||gate.try_close(token))return false;
        {
            RecursiveAdmission foreign(recursive_thread,other);
            if(foreign||foreign.epoch()||recursive_thread.depth!=1||recursive_thread.gate!=&gate||!recursive_thread.admitted)return false;
            EpochGate::CloseToken other_token{};
            if(!other.try_close(other_token)||!other.open(other_token))return false;
            RecursiveAdmission same(recursive_thread,gate);
            if(!same||recursive_thread.depth!=2)return false;
        }
        if(recursive_thread.depth!=1)return false;
    }
    if(recursive_thread.depth||recursive_thread.admitted||recursive_thread.epoch||recursive_thread.gate||!gate.try_close(token))return false;
    {
        RecursiveAdmission refused(recursive_thread,gate);
        if(refused||refused.epoch()||recursive_thread.depth!=1||recursive_thread.admitted)return false;
        if(!gate.open(token))return false;
        {
            RecursiveAdmission nested(recursive_thread,gate);
            if(nested||nested.epoch()||recursive_thread.depth!=2)return false;
            // Reusing an outer rejection must not acquire a reader after open.
            if(!gate.try_close(token)||!gate.open(token))return false;
            RecursiveAdmission foreign(recursive_thread,other);
            if(foreign||recursive_thread.depth!=2||recursive_thread.gate!=&gate||recursive_thread.admitted)return false;
        }
    }
    {
        RecursiveAdmission next(recursive_thread,gate);
        if(!next||next.epoch()!=3||recursive_thread.depth!=1)return false;
    }
    ThreadState bounded(2);
    {
        RecursiveAdmission outer(bounded,gate);
        if(!outer)return false;
        {
            RecursiveAdmission inner(bounded,gate);if(!inner||bounded.depth!=2)return false;
            RecursiveAdmission overflow(bounded,gate);
            if(overflow||overflow.epoch()||bounded.depth!=2||!bounded.admitted||gate.try_close(token))return false;
        }
        if(bounded.depth!=1||gate.try_close(token))return false;
    }
    if(bounded.depth||bounded.admitted||bounded.gate||!gate.try_close(token)||!gate.open(token))return false;
    ThreadState disabled(0);
    {
        RecursiveAdmission denied(disabled,gate);
        if(denied||disabled.depth||disabled.gate||!gate.try_close(token)||!gate.open(token))return false;
    }
    return true;
}

int main(){
    {
        gears_world::EpochGate open_gate;std::atomic<unsigned> missed{};
        std::array<std::thread,8> readers;
        for(auto& thread:readers)thread=std::thread([&]{for(unsigned i=0;i<10000;++i){
            auto lease=open_gate.try_enter();if(!lease)++missed;}});
        for(auto& thread:readers)thread.join();
        if(missed)return 90;
    }
    gears_world::EpochGate gate,other;
    gears_world::EpochGate::CloseToken first{},second{};
    {
        auto reader=gate.try_enter();if(!reader||reader.epoch()!=1)return 1;
        if(gate.try_close(first))return 2;
        auto moved=std::move(reader);if(reader||!moved)return 3;
    }
    if(!gate.try_close(first)||gate.try_enter()||other.open(first))return 4;
    auto stale=first;
    gears_world::EpochGate::CloseToken rejected{};
    if(gate.try_close(rejected)||rejected.owner)return 9;
    if(!gate.open(first)||gate.open(first)||!gate.try_close(second)||gate.open(stale))return 5;
    if(second.epoch!=2||!gate.open(second))return 6;
    // Model an engine thread holding admission while waiting for its render
    // thread. Writer attempts cannot queue behind it and obstruct the render
    // thread's independent admission. Bounded spins make regression fail.
    std::atomic<bool> engine_entered{},render_done{},writer_done{},failed{};
    std::atomic<unsigned> close_attempts{};
    std::thread engine([&]{
        auto lease=gate.try_enter();if(!lease){failed=true;return;}
        engine_entered=true;
        for(unsigned i=0;i<1000000&&!writer_done;++i)std::this_thread::yield();
        if(!writer_done)failed=true;
    });
    std::thread writer([&]{
        while(!engine_entered&&!failed)std::this_thread::yield();
        while(!render_done&&!failed){
            gears_world::EpochGate::CloseToken token;
            if(gate.try_close(token)){failed=true;gate.open(token);}
            ++close_attempts;
        }
        writer_done=true;
    });
    std::thread render([&]{
        while(!close_attempts&&!failed)std::this_thread::yield();
        auto lease=gate.try_enter();if(!lease)failed=true;
        render_done=true;
    });
    engine.join();writer.join();render.join();
    if(failed||!close_attempts||!gate.try_close(second)||gate.try_enter()||!gate.open(second))return 7;
    // Writer and readers race: admitted readers never overlap a closed gate.
    std::atomic<bool> writer_inside{};
    std::thread reads([&]{for(unsigned i=0;i<20000;++i){auto lease=gate.try_enter();if(lease&&writer_inside)failed=true;}});
    for(unsigned i=0;i<20000;++i){gears_world::EpochGate::CloseToken token;
        if(gate.try_close(token)){writer_inside=true;if(gate.try_enter())failed=true;writer_inside=false;if(!gate.open(token))failed=true;}}
    reads.join();if(failed)return 8;
    if(!recursive_admission_tests())return 10;
    std::puts("World epoch gate: nonwaiting admission, engine/render progress and generation tokens passed");
    std::puts("Recursive admission: outer lease, inherited rejection, independent TLS, depth and foreign gate guards passed");
}
