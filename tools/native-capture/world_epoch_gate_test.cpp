#include "world_epoch_gate.h"
#include <atomic>
#include <thread>
#include <cstdio>

int main(){
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
    std::puts("World epoch gate: nonwaiting admission, engine/render progress and generation tokens passed");
}
