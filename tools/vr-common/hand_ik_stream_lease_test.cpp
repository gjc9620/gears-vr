#include "hand_ik_stream_lease.h"
#include <cassert>
#include <cstdio>
#include <limits>
#if defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif
using namespace gears_hand_ik_stream;
namespace {
constexpr float weight=.125f;
Target target(unsigned seed){
    Target result{};
    for(unsigned i=0;i<result.size();++i)result[i]=static_cast<unsigned char>(seed+i*17);
    return result;
}
std::array<Target,4> targets(){return {{target(51),target(72),target(93),target(114)}};}
struct Backend {
    enum class Mode {normal,fail_before,fail_after,no_change,partial};
    std::array<Value,4> values{};
    std::array<Value,4> original{};
    int target_call{},strength_call{},read_call{},mutations{};
    unsigned read_mask{},target_mask{},strength_mask{},first_mutation_reads{};
    int fail_target{-1},fail_strength{-1},fail_read{-1};
    Mode target_mode{Mode::normal},strength_mode{Mode::normal};
    Backend(){
        for(unsigned i=0;i<values.size();++i)values[i].target=target(i*4+1);
        original=values;
    }
    bool read(unsigned i,Value& value){
        read_mask|=1u<<i;
        if(read_call++==fail_read)return false;
        value=values[i];return true;
    }
    bool write_target(unsigned i,Target const& expected,Target const& wanted){
        target_mask|=1u<<i;
        const auto mode=target_call++==fail_target?target_mode:Mode::normal;
        if(!exact(values[i].target,expected)||mode==Mode::fail_before)return false;
        if(mode!=Mode::no_change){
            if(!mutations)first_mutation_reads=read_mask;
            values[i].target=wanted;++mutations;
        }
        return mode!=Mode::fail_after;
    }
    bool set_strength(unsigned i,float value){
        strength_mask|=1u<<i;
        const auto mode=strength_call++==fail_strength?strength_mode:Mode::normal;
        if(mode==Mode::fail_before)return false;
        if(mode!=Mode::no_change&&!mutations)first_mutation_reads=read_mask;
        if(mode==Mode::partial){values[i].strength[0]=value;++mutations;return false;}
        if(mode!=Mode::no_change){values[i].strength={{value,value,0.f}};++mutations;}
        return mode!=Mode::fail_after;
    }
    bool is_original(unsigned i) const {
        return exact(values[i].target,original[i].target)&&
            exact(values[i].strength,original[i].strength);
    }
    bool all_original() const {
        for(unsigned i=0;i<values.size();++i)if(!is_original(i))return false;
        return true;
    }
};
void success_and_reset(){
    Backend backend;Lease lease;
    assert(!lease.dirty()&&!lease.faulted()&&lease.reset());
    assert(lease.acquire(targets(),weight,backend));
    assert(lease.dirty()&&!lease.faulted()&&!lease.reset());
    for(unsigned i=0;i<4;++i){
        assert(exact(backend.values[i].target,targets()[i]));
        assert(exact(backend.values[i].strength,Strength{{weight,weight,0.f}}));
    }
    assert(lease.restore(backend)&&backend.all_original());
    const auto writes=backend.mutations;
    assert(lease.restore(backend)&&backend.mutations==writes);
    assert(!lease.dirty()&&!lease.faulted()&&lease.reset());
    // Equal original/wanted target still owns and restores strength correctly.
    auto wanted=targets();wanted[2]=backend.original[2].target;
    assert(lease.acquire(wanted,weight,backend)&&lease.restore(backend));
    assert(backend.all_original());
}
void preflight_has_no_writes(){
    for(unsigned i=0;i<4;++i){
        Backend backend;Lease lease;
        backend.values[i].strength[1]=std::numeric_limits<float>::denorm_min();
        assert(!lease.acquire(targets(),weight,backend));
        assert(lease.faulted()&&!lease.dirty()&&backend.mutations==0);
        assert(!lease.acquire(targets(),weight,backend)&&backend.mutations==0);
        assert(lease.reset()&&!lease.faulted());
    }
    for(int i=0;i<4;++i){
        Backend backend;Lease lease;backend.fail_read=i;
        assert(!lease.acquire(targets(),weight,backend)&&backend.mutations==0);
        assert(lease.faulted()&&!lease.dirty());
    }
    for(float invalid:{0.f,-1.f,1.00001f,std::numeric_limits<float>::infinity(),
                       std::numeric_limits<float>::quiet_NaN()}){
        Backend backend;Lease lease;
        assert(!lease.acquire(targets(),invalid,backend)&&backend.mutations==0);
    }
    Backend backend;Lease lease;backend.values[3].strength[2]=-0.f;
    assert(!lease.acquire(targets(),weight,backend)&&backend.mutations==0);
}
void selected_channels_are_independent(){
    for(const unsigned selected:{3u,12u}){
        Backend backend;Lease lease;
        // An original or foreign owner may drive the other hand. Its values
        // need not satisfy this lease's +0 precondition and must stay unread.
        for(unsigned i=0;i<4;++i)if(!(selected&(1u<<i))){
            backend.values[i].strength={{1.f,std::numeric_limits<float>::quiet_NaN(),-0.f}};
            backend.values[i].target=target(201+i);
        }
        backend.original=backend.values;
        assert(lease.acquire(selected,targets(),weight,backend));
        assert(backend.first_mutation_reads==selected);
        assert(backend.read_mask==selected&&backend.target_mask==selected&&backend.strength_mask==selected);
        assert(lease.restore(backend)&&backend.all_original());
        assert(backend.read_mask==selected&&backend.target_mask==selected&&backend.strength_mask==selected);
        assert(!lease.dirty()&&!lease.faulted());
    }
    for(const unsigned selected:{3u,12u}){
        Backend backend;Lease lease;
        backend.fail_target=1;backend.target_mode=Backend::Mode::fail_after;
        assert(!lease.acquire(selected,targets(),weight,backend));
        assert(lease.faulted()&&!lease.dirty()&&backend.all_original());
        assert(backend.first_mutation_reads==selected);
        assert(backend.read_mask==selected&&backend.target_mask==selected&&backend.strength_mask==selected);
    }
    for(const unsigned invalid:{0u,16u,31u,~0u}){
        Backend backend;Lease lease;
        assert(!lease.acquire(invalid,targets(),weight,backend));
        assert(lease.faulted()&&!lease.dirty()&&backend.mutations==0&&backend.read_call==0);
        assert(backend.all_original());
    }
}
void partial_acquire_rolls_back_all_channels(){
    for(int i=0;i<4;++i)for(auto mode:{Backend::Mode::fail_before,
                                      Backend::Mode::fail_after,Backend::Mode::no_change}){
        Backend backend;Lease lease;backend.fail_target=i;backend.target_mode=mode;
        assert(!lease.acquire(targets(),weight,backend));
        assert(lease.faulted()&&!lease.dirty()&&backend.all_original());
        assert(lease.reset()&&!lease.faulted());
    }
    for(int i=0;i<4;++i)for(auto mode:{Backend::Mode::fail_before,
                                      Backend::Mode::fail_after,Backend::Mode::no_change}){
        Backend backend;Lease lease;backend.fail_strength=i;backend.strength_mode=mode;
        assert(!lease.acquire(targets(),weight,backend));
        assert(lease.faulted()&&!lease.dirty()&&backend.all_original());
    }
    // Read failure during final verification still rolls back every channel.
    Backend backend;Lease lease;backend.fail_read=8;
    assert(!lease.acquire(targets(),weight,backend));
    assert(lease.faulted()&&!lease.dirty()&&backend.all_original());
}
void foreign_channel_preserves_values(){
    for(unsigned i=0;i<4;++i)for(unsigned field=0;field<3;++field){
        Backend backend;Lease lease;assert(lease.acquire(targets(),weight,backend));
        if(field==0)backend.values[i].target[11]^=1;
        if(field==1)backend.values[i].strength[0]=std::nextafter(weight,1.f);
        if(field==2)backend.values[i].strength[2]=-0.f;
        const auto foreign=backend.values[i];
        assert(!lease.restore(backend)&&lease.dirty()&&lease.faulted()&&!lease.reset());
        assert(exact(backend.values[i].target,foreign.target));
        assert(exact(backend.values[i].strength,foreign.strength));
        for(unsigned other=0;other<4;++other)if(other!=i)assert(backend.is_original(other));
        // The external owner puts its state back; exact matching permits cleanup.
        backend.values[i].target=targets()[i];
        backend.values[i].strength={{weight,weight,0.f}};
        assert(lease.restore(backend)&&!lease.dirty()&&lease.faulted());
        assert(lease.reset()&&!lease.faulted()&&backend.all_original());
    }
}
void partial_strength_stays_unresolved(){
    for(int i=0;i<4;++i){
        Backend backend;Lease lease;backend.fail_strength=i;
        backend.strength_mode=Backend::Mode::partial;
        assert(!lease.acquire(targets(),weight,backend));
        assert(lease.dirty()&&lease.faulted()&&!lease.reset());
        assert(exact(backend.values[i].target,targets()[i]));
        assert(exact(backend.values[i].strength,Strength{{weight,0.f,0.f}}));
        for(unsigned other=0;other<4;++other)if(other!=unsigned(i))assert(backend.is_original(other));
    }
    Backend backend;Lease lease;assert(lease.acquire(targets(),weight,backend));
    backend.fail_strength=backend.strength_call;backend.strength_mode=Backend::Mode::partial;
    assert(!lease.restore(backend)&&lease.dirty()&&lease.faulted());
    assert(exact(backend.values[0].strength,Strength{{0.f,weight,0.f}}));
    assert(exact(backend.values[0].target,targets()[0]));
    for(unsigned i=1;i<4;++i)assert(backend.is_original(i));
}
void restore_retries_and_false_after_write(){
    for(auto operation:{0,1}){
        Backend backend;Lease lease;assert(lease.acquire(targets(),weight,backend));
        if(operation==0){backend.fail_target=backend.target_call;backend.target_mode=Backend::Mode::fail_after;}
        else {backend.fail_strength=backend.strength_call;backend.strength_mode=Backend::Mode::fail_after;}
        assert(lease.restore(backend)&&backend.all_original());
        assert(!lease.dirty()&&lease.faulted());
    }
    for(auto operation:{0,1,2,3,4,5,6}){
        Backend backend;Lease lease;assert(lease.acquire(targets(),weight,backend));
        if(operation==0){backend.fail_target=backend.target_call;backend.target_mode=Backend::Mode::fail_before;}
        if(operation==1){backend.fail_strength=backend.strength_call;backend.strength_mode=Backend::Mode::fail_before;}
        if(operation==2)backend.fail_read=backend.read_call;
        if(operation==3){backend.fail_target=backend.target_call;backend.target_mode=Backend::Mode::no_change;}
        if(operation==4){backend.fail_strength=backend.strength_call;backend.strength_mode=Backend::Mode::no_change;}
        if(operation==5)backend.fail_read=backend.read_call+1;
        if(operation==6)backend.fail_read=backend.read_call+2;
        assert(!lease.restore(backend)&&lease.dirty()&&lease.faulted());
        for(unsigned i=1;i<4;++i)assert(backend.is_original(i));
        assert(lease.restore(backend)&&!lease.dirty()&&backend.all_original());
    }
}
void inactive_loop_is_not_pose_contribution(){
    assert(custom_child_zero_pose(31,0.f,0.f,0.f,true)); // Retail Lancer Saw_Idle after StopCustomAnim.
    assert(custom_child_zero_pose(31,-0.f,0.f,-0.f,false));
    for(unsigned missing=0;missing<5;++missing)
        assert(!custom_child_zero_pose(31u&~(1u<<missing),0.f,0.f,0.f,true));
    for(const auto nonzero:{std::numeric_limits<float>::denorm_min(),
                           -std::numeric_limits<float>::denorm_min(),
                           std::numeric_limits<float>::infinity(),
                           std::numeric_limits<float>::quiet_NaN()}){
        assert(!custom_child_zero_pose(31,nonzero,0.f,0.f,false));
        assert(!custom_child_zero_pose(31,0.f,nonzero,0.f,true));
        assert(!custom_child_zero_pose(31,0.f,0.f,nonzero,true));
    }
#if defined(_M_X64) || defined(_M_IX86)
    const auto saved=_mm_getcsr();_mm_setcsr(saved|0x8040u); // A game's DAZ/FTZ must not turn a nonzero read into permission.
    volatile float tiny=std::numeric_limits<float>::denorm_min();
    assert(!custom_child_zero_pose(31,tiny,0.f,0.f,true));
    assert(!custom_child_zero_pose(31,0.f,tiny,0.f,true));
    assert(!custom_child_zero_pose(31,0.f,0.f,tiny,true));
    assert(!pose_weight_zero(tiny));
    _mm_setcsr(saved);
#endif
}
} // namespace
int main(){
    success_and_reset();preflight_has_no_writes();partial_acquire_rolls_back_all_channels();
    selected_channels_are_independent();
    foreign_channel_preserves_values();partial_strength_stays_unresolved();
    restore_retries_and_false_after_write();
    inactive_loop_is_not_pose_contribution();
    std::puts("Selected-channel hand IK ownership, partial failure and exact rollback passed");
}
