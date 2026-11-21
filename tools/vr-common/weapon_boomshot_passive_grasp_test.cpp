#include "weapon_boomshot_passive_grasp.h"
#include <cassert>
#include <cstdio>
#include <limits>
namespace p=gears_boomshot_passive_grasp;
using P=p::Part;
int main(){unsigned checks{};
    p::Scope scope{};scope.key.frame=1;scope.key.lifecycle=4;scope.units=100;scope.root_scale=scope.hand_scale=1;
    const auto expect=[&](bool condition){assert(condition);++checks;};
    p::Selector selected;
    expect(selected.advance(scope,100,P::opening,.121,.121)==P::none);
    expect(selected.advance(scope,100,P::opening,.12,.12)==P::opening);
    ++scope.key.frame;expect(selected.advance(scope,101,P::opening,.20,.01)==P::opening);
    ++scope.key.frame;expect(selected.advance(scope,102,P::opening,.200001,.01)==P::support);
    ++scope.key.frame;expect(selected.advance(scope,103,P::opening,.001,.20)==P::support);
    ++scope.key.frame;expect(selected.advance(scope,104,P::opening,.001,.200001)==P::opening);
    ++scope.key.frame;expect(selected.advance(scope,105,P::closing,.19,.3)==P::none);
    ++scope.key.frame;expect(selected.advance(scope,106,P::closing,.12,.3)==P::closing);
    ++scope.key.frame;expect(selected.advance(scope,107,P::none,.01,.19)==P::none);
    ++scope.key.frame;expect(selected.advance(scope,108,P::none,.01,.12)==P::support);
    selected.lost();++scope.key.frame;expect(selected.advance(scope,109,P::none,1,.19)==P::none);
    const auto reset_boundary=[&](auto mutate){
        p::Selector choice;auto initial=scope;initial.key.frame=10;
        expect(choice.advance(initial,1000,P::opening,.01,.1)==P::opening);
        auto changed=initial;++changed.key.frame;mutate(changed);
        expect(choice.advance(changed,1001,P::opening,.18,.3)==P::none);
    };
    for(unsigned i=0;i<17;++i)reset_boundary([&](p::Scope& changed){++changed.boundaries[i];});
    for(unsigned i=0;i<6;++i)reset_boundary([&](p::Scope& changed){++changed.objects[i].name_number;});
    reset_boundary([](p::Scope& changed){++changed.key.game_start;});
    reset_boundary([](p::Scope& changed){++changed.key.native_started_qpc;});
    reset_boundary([](p::Scope& changed){++changed.key.registration_cookie;});
    reset_boundary([](p::Scope& changed){++changed.key.lifecycle;});
    reset_boundary([](p::Scope& changed){++changed.key.world_generation;});
    reset_boundary([](p::Scope& changed){++changed.key.game_pid;});
    reset_boundary([](p::Scope& changed){++changed.key.thread_id;});
    reset_boundary([](p::Scope& changed){++changed.units;});
    reset_boundary([](p::Scope& changed){changed.root_scale+=.01;});
    reset_boundary([](p::Scope& changed){changed.hand_scale+=.01;});
    reset_boundary([](p::Scope& changed){++changed.source;});
    reset_boundary([](p::Scope& changed){++changed.control;});
    for(double invalid:{-.01,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){
        selected.lost();++scope.key.frame;expect(selected.advance(scope,1000,P::opening,invalid,.3)==P::none);
        ++scope.key.frame;expect(selected.advance(scope,1001,P::opening,.3,invalid)==P::none);
    }
    selected.lost();scope.key.frame=100;
    expect(selected.advance(scope,1000,P::opening,.01,.1)==P::opening);
    // Duplicate observations cannot switch to a closer neighbour or refresh time.
    expect(selected.advance(scope,1000,P::opening,.19,.001)==P::opening);
    ++scope.key.frame;expect(selected.advance(scope,100001000,P::opening,.19,.3)==P::opening);
    ++scope.key.frame;expect(selected.advance(scope,200001001,P::opening,.19,.3)==P::none);
    expect(selected.advance(scope,200001002,P::opening,.01,.3)==P::opening);
    --scope.key.frame;expect(selected.advance(scope,200001003,P::opening,.18,.3)==P::none);
    ++scope.key.frame;expect(selected.advance(scope,3000,P::opening,.01,.3)==P::opening);
    ++scope.key.frame;expect(selected.advance(scope,2999,P::opening,.18,.3)==P::none);
    expect(selected.advance(scope,3001,static_cast<P>(99),.01,.01)==P::none);
    expect(selected.advance(scope,0,P::opening,.01,.01)==P::none);
    scope.key.frame=0;expect(selected.advance(scope,3002,P::opening,.01,.01)==P::none);
    std::printf("Boomshot passive cosmetic preference: %u checks passed; acquire/retain boundaries, neighbour jitter, phase loss, scope/continuity changes, gaps/replays/rewinds, invalid distances; no pose or action authority\n",checks);
}
