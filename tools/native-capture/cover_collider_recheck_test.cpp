#define GEARS_COLLIDER_ROLES_TEST_MAIN RecheckPriorMain
#include "cover_collider_roles_live_test.cpp"
#undef GEARS_COLLIDER_ROLES_TEST_MAIN
#ifndef GEARS_COLLIDER_RECHECK
#define GEARS_COLLIDER_RECHECK 0
#endif
template<class Guard> static collider::Code repeat(Guard current,collider::Snapshot& output){
#if GEARS_COLLIDER_RECHECK
    return collider::recheck(actor,current,collider_work,output);
#else
    // Existing SourceGuard also compares every fresh sample with its initial
    // snapshot. Keep that actual caller contract in the baseline comparison.
    auto candidate=output;const auto code=collider::sample(actor,current,collider_work,candidate);
    if(code!=collider::Code::current)return code;
    if(!collider::same(output,candidate))return collider::Code::changed;
    output=candidate;return collider::Code::current;
#endif
}
int main(){
    if(RecheckPriorMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    for(unsigned presence=0;presence<3;++presence){
        collider_setup();if(!presence)put(cylinder+0x1a0,uintptr_t{});if(presence==1)put(single_body+0x88,uintptr_t{});
        collider::Snapshot out{};C(capture(out)==collider::Code::current);const auto first=out;const auto begin=reads;
        for(unsigned n=0;n<5;++n)C(repeat(guard,out)==collider::Code::current&&collider::same(out,first));
        printf("collider_recheck presence=%u five_reads=%u\n",presence,reads-begin);
    }
    for(unsigned pass:{1u,2u})for(unsigned bad=0;bad<8;++bad){
        collider_setup();collider::Snapshot out{};C(capture(out)==collider::Code::current);const auto first=out;
        const auto occurrence=GEARS_COLLIDER_RECHECK?(pass==1?2u:5u):pass;
        if(bad==0)plan(single_body+0x88,occurrence,cylinder_prop+0x94,0x1f8u,4);
        if(bad==1)plan(single_body+0x88,occurrence,instance_prop+0xd0,fixture_weapon_base,8);
        if(bad==2)plan(single_body+0x88,occurrence,cylinder_class+0x60,fixture_weapon_base,8);
        if(bad==3)plan(single_body+0x88,occurrence,props[8]+0x68,props[5],8);
        if(bad==4)plan(single_body+0x88,occurrence,props[7]+0x94,0x80u,4);
        if(bad==5)plan(single_body+0x88,occurrence,owner_prop+80,fixture_component_property,8);
        if(bad==6){plan(single_body+0x88,occurrence,0,0,0);change_generation=true;}
        if(bad==7)plan(single_body+0x88,occurrence,single_body+0x68,fixture_mesh,8);
        const auto observed=repeat(guard,out);
#if !GEARS_COLLIDER_RECHECK
        if(bad==7&&pass==2){C(observed==collider::Code::current&&collider::same(out,first));printf("baseline_unclosed_owner_race=1\n");}
        else
#endif
        C(observed!=collider::Code::current&&collider::same(out,first));read_hook=nullptr;
    }
    for(bool empty:{false,true}){
        collider_setup();if(empty)put(cylinder+0x1a0,uintptr_t{});
        collider::Snapshot out{};C(capture(out)==collider::Code::current);const auto first=out;
        put(cylinder+0x1a0,empty?single_body:uintptr_t{});
        C(repeat(guard,out)!=collider::Code::current&&collider::same(out,first));
    }
#if GEARS_COLLIDER_RECHECK
    collider_setup();collider::Snapshot out{};C(capture(out)==collider::Code::current);const auto first=out;
    const auto prior=reads;C(repeat([](){return false;},out)==collider::Code::guard&&reads==prior&&collider::same(out,first));
    C(repeat(guard,out)==collider::Code::changed&&collider::same(out,first)); // Failure cannot revive the initial certificate.
    C(capture(out)==collider::Code::current&&repeat(guard,out)==collider::Code::current);
    auto changed=actor;changed.scene_invocation++;
    C(collider::recheck(changed,guard,collider_work,out)==collider::Code::changed&&collider::same(out,first));
    C(capture(out)==collider::Code::current);unsigned ends{};
    C(repeat([&](){return ++ends<2;},out)==collider::Code::guard&&collider::same(out,first));
    C(!collider_work.recheck_ready);
    C(collider::recheck(actor,guard,collider_work,collider_work.first)==collider::Code::request);
    C(capture(out)==collider::Code::current);h::mutex.lock();
    C(repeat([](){return true;},out)==collider::Code::metadata&&collider::same(out,first));h::mutex.unlock();
    C(!collider_work.recheck_ready);
    C(capture(out)==collider::Code::current);collider::Code foreign{};
    std::thread worker([&](){foreign=repeat([](){return true;},out);});worker.join();
    C(foreign==collider::Code::metadata&&collider::same(out,first));
#endif
    C(native_calls==0&&guard_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverColliderRecheck checks=%u failures=%u existing fixtures, actual readers; no game\n",checks,failures);
    return failures?1:0;
}
