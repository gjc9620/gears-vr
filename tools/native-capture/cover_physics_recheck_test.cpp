#define GEARS_PHYSICS_ROLES_TEST_MAIN PriorPhysicsRecheckMain
#include "cover_physics_roles_live_test.cpp"
#undef GEARS_PHYSICS_ROLES_TEST_MAIN
#include <thread>
#ifndef GEARS_PHYSICS_RECHECK
#define GEARS_PHYSICS_RECHECK 0
#endif
template<class Guard> static roles::Code repeat(Guard current,roles::Snapshot& output){
#if GEARS_PHYSICS_RECHECK
    return roles::recheck(actor,physics_expected,current,workspace,output);
#else
    auto next=output;const auto code=roles::sample(actor,physics_expected,current,workspace,next);
    if(code!=roles::Code::current)return code;
    if(!roles::same(next,output))return roles::Code::changed;output=next;return roles::Code::current;
#endif
}
int main(){
    if(PriorPhysicsRecheckMain())return 1;
    arena=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,saved_arena.size(),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!arena)return 99;
    for(unsigned count:{2u,16u,64u}){
        physics_setup();
        if(count!=2){for(unsigned n=0;n<count;++n){const uintptr_t body=arena+0xa0000+n*0x100;
            object(body,150+n,body_class,physics_instance,L"BoundedBody",n);
            put(arena+0x86000+n*8,body);put(body+0x60,int(n));put(body+0x68,fixture_mesh);put(body+0x88,arena+0xb0000+n*0x100);}
            put(physics_instance+0x6c,roles::pose::ArrayHeader{arena+0x86000,int(count),256});put(physics_instance+0x7c,int(count-1));}
        roles::Snapshot out{};C(observe(out));const auto first=out;const auto begin=reads;
        for(unsigned n=0;n<5;++n)C(repeat(guard,out)==roles::Code::current&&roles::same(first,out));
        printf("physics_recheck bodies=%u five_reads=%u\n",count,reads-begin);
    }
    for(unsigned pass:{1u,2u})for(unsigned bad=0;bad<18;++bad){
        physics_setup();roles::Snapshot out{};C(observe(out));const auto first=out;
        const auto occurrence=GEARS_PHYSICS_RECHECK?(pass==1?3u:6u):pass;
        const auto watched=body_instances[1]+0x90;
        if(bad==0)plan(watched,occurrence,props[7]+0x94,0x80u,4);
        if(bad==1)plan(watched,occurrence,props[6]+0xd0,fixture_weapon_base,8);
        if(bad==2)plan(watched,occurrence,body_class+0x60,fixture_weapon_base,8);
        if(bad==3)plan(watched,occurrence,props[8]+0x68,props[5],8);
        if(bad==4)plan(watched,occurrence,body_instances[0]+0x68,fixture_pc,8);
        if(bad==5)plan(watched,occurrence,body_instances[1]+0x68,fixture_pc,8);
        if(bad==6)plan(watched,occurrence,physics_instance+0x7c,0,4);
        if(bad==7)plan(watched,occurrence,physics_instance+0x6c,arena+0x86008,8);
        if(bad==8){plan(watched,occurrence,0,0,0);change_generation=true;}
        if(bad==9)plan(watched,occurrence,fixture_mesh+0x250,fixture_mesh,8);
        if(bad==10)plan(watched,occurrence,body_instances[1]+0x88,arena+0x8a000,8);
        if(bad==11)plan(watched,occurrence,body_instances[1]+0x90,arena+0x8a000,8);
        if(bad==12)plan(watched,occurrence,body_instances[1]+0x60,0,4);
        if(bad==13)plan(watched,occurrence,physics_instance+0x60,fixture_mesh,8);
        if(bad==14)plan(watched,occurrence,fixture_mesh+0x248,fixture_mesh,8);
        if(bad==15)plan(watched,occurrence,physics_instance+0x78,256,4);
        if(bad==16)plan(watched,occurrence,physics_instance+0x74,1,4);
        if(bad==17)plan(watched,occurrence,arena+0x86000,body_instances[1],8);
        const auto code=repeat(guard,out);
#if !GEARS_PHYSICS_RECHECK
        // Old final Proof never retained the dynamic fields. The baseline
        // accepts these after the second values pass; keep that real failure.
        if(pass==2&&bad!=11&&(bad==4||bad==5||bad==6||bad==7||bad>=9)){
            C(code==roles::Code::current&&roles::same(first,out));printf("baseline_unclosed_dynamic=%u\n",bad);
        }else
#endif
        C(code!=roles::Code::current&&roles::same(first,out));read_hook=nullptr;
    }
#if GEARS_PHYSICS_RECHECK
    physics_setup();roles::Snapshot out{};C(observe(out));const auto first=out;const auto begin=reads;
    C(repeat([](){return false;},out)==roles::Code::guard&&roles::same(first,out)&&reads==begin);
    C(repeat(guard,out)==roles::Code::changed&&!workspace.recheck_ready);
    C(observe(out));auto other=actor;other.scene_invocation++;
    C(roles::recheck(other,physics_expected,guard,workspace,out)==roles::Code::changed&&roles::same(first,out));
    C(observe(out));auto stale=physics_expected;stale.tick_epoch++;
    C(roles::recheck(actor,stale,guard,workspace,out)==roles::Code::changed&&roles::same(first,out));
    C(observe(out));unsigned closing{};
    C(repeat([&](){return ++closing<2;},out)==roles::Code::guard&&!workspace.recheck_ready&&roles::same(first,out));
    C(roles::recheck(actor,physics_expected,guard,workspace,workspace.first)==roles::Code::request);
    C(observe(out));h::mutex.lock();C(repeat([](){return true;},out)==roles::Code::metadata&&!workspace.recheck_ready);h::mutex.unlock();
    C(observe(out));roles::Code foreign{};
    std::thread worker([&](){foreign=repeat([](){return true;},out);});worker.join();
    C(foreign==roles::Code::metadata&&roles::same(first,out));
    C(observe(out));C(repeat([]()->bool{throw 7;},out)==roles::Code::exception&&!workspace.recheck_ready);
#endif
    C(native_calls==0&&guard_lock_free);C(VirtualFree(reinterpret_cast<void*>(arena),0,MEM_RELEASE)!=0);
    printf("CoverPhysicsRecheck checks=%u failures=%u actual reader and dynamic races; no game\n",checks,failures);
    return failures?1:0;
}
