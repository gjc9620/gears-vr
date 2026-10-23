#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "world_control_wait.h"
#include <cstdio>
#include <stdexcept>
#include <thread>
using namespace gears_world;
static void require(bool value,char const* message){if(!value)throw std::runtime_error(message);}
struct Event {
    HANDLE value{};
    explicit Event(bool manual=false):value(CreateEventW(nullptr,manual,FALSE,nullptr)){require(value!=nullptr,"create test event");}
    ~Event(){if(value)CloseHandle(value);}
    Event(Event const&)=delete;Event& operator=(Event const&)=delete;
    void signal(){require(SetEvent(value)!=0,"signal test event");}
};
int main(){try{
    Event stop(true),prepare,commit;ControlWake wake;
    require(wake.wait(stop.value,prepare.value,commit.value,0)==ControlWakeResult::timeout,"idle timeout");
    for(unsigned i=0;i<1000;++i){
        prepare.signal();
        require(wake.wait(stop.value,prepare.value,commit.value,0)==ControlWakeResult::prepare,"prepare wake");
        require(WaitForSingleObject(prepare.value,0)==WAIT_TIMEOUT,"auto-reset not consumed");
        require(wake.take_prepare(prepare.value)&&!wake.take_prepare(prepare.value),"prepare receipt lost or duplicated");
        commit.signal();
        require(wake.wait(stop.value,prepare.value,commit.value,0)==ControlWakeResult::commit,"commit wake");
        require(wake.take_commit(commit.value)&&!wake.take_commit(commit.value),"commit receipt lost or duplicated");
    }
    prepare.signal();commit.signal();
    require(wake.wait(stop.value,prepare.value,commit.value,0)==ControlWakeResult::prepare,"simultaneous prepare wake");
    require(wake.take_commit(commit.value)&&wake.take_prepare(prepare.value),"commit-first poll lost simultaneous signals");
    require(!wake.take_commit(commit.value)&&!wake.take_prepare(prepare.value),"simultaneous duplicated");
    // A second signal arriving after the first was consumed remains a distinct
    // receipt; it must not silently disappear into the first latched receipt.
    prepare.signal();wake.wait(stop.value,prepare.value,commit.value,0);prepare.signal();
    require(wake.wait(stop.value,prepare.value,commit.value,0)==ControlWakeResult::timeout,"unpolled receipt consumed second signal");
    require(wake.take_prepare(prepare.value)&&wake.take_prepare(prepare.value)&&!wake.take_prepare(prepare.value),"subsequent signal lost");
    prepare.signal();commit.signal();stop.signal();
    require(wake.wait(stop.value,prepare.value,commit.value,0)==ControlWakeResult::stop,"stop priority");
    require(WaitForSingleObject(stop.value,0)==WAIT_OBJECT_0,"manual stop reset");
    require(wake.take_prepare(prepare.value)&&wake.take_commit(commit.value),"stop consumed control receipts");
    ResetEvent(stop.value);
    std::thread sender([&](){prepare.signal();});
    const auto threaded=wake.wait(stop.value,prepare.value,commit.value,1000);sender.join();
    require(threaded==ControlWakeResult::prepare&&wake.take_prepare(prepare.value),"threaded wake receipt");
    require(wake.prepare_wakes()==1003&&wake.commit_wakes()==1000&&wake.stop_wakes()==1,"wake diagnostics");
    ControlCadence cadence;
    require(cadence.interval(0,0,false)==20,"idle polling changed");
    require(cadence.interval(1,1000,false)==1&&cadence.interval(1,1249,false)==1,"active burst");
    require(cadence.interval(1,1250,false)==20&&cadence.interval(1,2000,false)==20,"same request extends burst");
    require(cadence.interval(2,2000,false)==1&&cadence.interval(0,2100,false)==1,"first image tail lost");
    require(cadence.interval(0,2250,false)==20,"tail unbounded");
    require(cadence.interval(3,3000,false)==1&&cadence.interval(3,2999,false)==20,"backward clock");
    require(cadence.interval(3,3001,true)==20&&cadence.interval(0,3002,false)==20,"cleanup fast spin");
    ControlWake missing;require(missing.wait(nullptr,nullptr,nullptr,0)==ControlWakeResult::timeout&&!missing.failed(),"optional absent handles");
    HANDLE closed=CreateEventW(nullptr,FALSE,FALSE,nullptr);require(closed!=nullptr,"create failed-wait fixture");CloseHandle(closed);
    ControlWake failed;require(failed.wait(stop.value,closed,nullptr,0)==ControlWakeResult::failed&&failed.failed(),"failed wait accepted");
    puts("WorldControlWaitTest passed: 1000 Prepare/Commit rounds, exact receipts, stop priority, bounded burst/tail and cleanup cadence");return 0;
}catch(std::exception const& error){fprintf(stderr,"WorldControlWaitTest failed: %s\n",error.what());return 1;}}
