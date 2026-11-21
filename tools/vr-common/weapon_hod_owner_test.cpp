#include "weapon_hod_owner.h"
#include <cassert>
#include <cstdio>
using namespace gears_weapon;
constexpr HammerOfDawnIdentity id{1,2,3,4,5,6,7,8,9};
HammerOfDawnStopReadback proof(unsigned serial) {
    return {id.weapon,id.weapon_generation,serial,100+serial,true,true,true,true,true,true,true};
}
int main(){
    HammerOfDawnUseOwner owner;assert(!owner.retained());
    assert(owner.reserve(id,10));assert(!owner.reserve(id,11));
    assert(owner.pressed(10,false,false));assert(!owner.retained());
    assert(!owner.reserve(id,10));assert(owner.reserve(id,11));
    assert(!owner.pressed(10,true,true));assert(owner.pressed(11,true,false));
    assert(owner.call_unknown()&&owner.retained());
    auto other=id;other.weapon++;
    assert(!owner.begin_release(11,other));assert(!owner.begin_release(10,id));
    assert(owner.begin_release(11,id));assert(!owner.begin_release(11,id));
    assert(owner.released(11,false,false));assert(owner.begin_release(11,id));
    assert(owner.released(11,true,true));assert(!owner.begin_release(11,id));
    assert(!owner.begin_fallback(11,other,true));assert(!owner.begin_fallback(11,id,false));
    assert(owner.begin_fallback(11,id,true));assert(owner.fallback(11,false));
    assert(owner.begin_fallback(11,id,true));assert(owner.fallback(11,true));
    assert(!owner.begin_fallback(11,id,true));
    auto stopped=proof(11);stopped.loop_sound_stopped=false;
    assert(!owner.stopped(11,stopped)&&owner.retained());
    stopped=proof(11);stopped.weapon_generation++;
    assert(!owner.stopped(11,stopped)&&owner.retained());
    stopped=proof(11);assert(!owner.stopped(10,stopped));
    assert(owner.stopped(11,stopped)&&!owner.retained());
    assert(owner.reserve(id,12));assert(!owner.stopped(11,stopped));
    assert(owner.pressed(12,true,true));assert(owner.begin_release(12,id));
    assert(owner.released(12,true,false));assert(owner.call_unknown());
    assert(!owner.stopped(12,proof(11)));assert(owner.stopped(12,proof(12)));
    assert(!owner.reserve(id,12));
    std::puts("HOD owner: old serial, old incarnation, unknown call, one-shot stop and complete readback boundaries passed (synthetic only)");
}
