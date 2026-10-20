#include "final_transfer_policy.h"
#include "owned_plain_output.h"
#include <cassert>
#include <limits>
int main(){
    // Plain Output requires a current owned renderer, attested code and exact
    // identity UVs. DLSS/control output retains its separate proof path.
    const std::array<float,8> plain_args{0,0,3072,3264,0,0,3072,3264};
    const std::array<unsigned,4> plain_sizes{6144,3264,6144,3264};
    auto plain=[&](bool attested,bool owned,bool current,bool controlled,uintptr_t caller,
                   auto args,auto sizes){return gears_owned_plain_output::eligible(attested,owned,current,controlled,caller,args,sizes);};
    assert(plain(true,true,true,false,0xd83cbf,plain_args,plain_sizes));
    auto right=plain_args;right[0]=right[4]=3072;
    assert(plain(true,true,true,false,0xd83cbf,right,plain_sizes));
    assert(!plain(false,true,true,false,0xd83cbf,plain_args,plain_sizes));
    assert(!plain(true,false,true,false,0xd83cbf,plain_args,plain_sizes));
    assert(!plain(true,true,false,false,0xd83cbf,plain_args,plain_sizes));
    assert(!plain(true,true,true,true,0xd83cbf,plain_args,plain_sizes));
    assert(!plain(true,true,true,false,0xd83cc0,plain_args,plain_sizes));
    for(unsigned i=0;i<8;++i){auto args=plain_args;args[i]+=.5f;
        assert(!plain(true,true,true,false,0xd83cbf,args,plain_sizes));
        args[i]=std::numeric_limits<float>::quiet_NaN();
        assert(!plain(true,true,true,false,0xd83cbf,args,plain_sizes));}
    for(unsigned i=0;i<4;++i){auto sizes=plain_sizes;sizes[i]+=1;
        assert(!plain(true,true,true,false,0xd83cbf,plain_args,sizes));}
    // A wrapped DLSS destination or full-width source cannot use this path.
    auto wrapped=plain_args;wrapped[0]=4294967296.0f;
    assert(!plain(true,true,true,false,0xd83cbf,wrapped,plain_sizes));
    auto full=plain_args;full[6]=6144;
    assert(!plain(true,true,true,false,0xd83cbf,full,plain_sizes));
    // Recorded Recovery5 ticket2: the final helper copies both eye pixel
    // rectangles from a 3840x2160 pool into the 1792x980 owned target.
    const std::array<unsigned,4> copied_sizes{1792,980,3840,2160};
    const std::array<float,8> copied_left{0,0,896,980,0,0,896,980};
    auto copied_right=copied_left;copied_right[0]=copied_right[4]=896;
    assert(plain(true,true,true,false,0x2b11a0,copied_left,copied_sizes));
    assert(plain(true,true,true,false,0x2b11a0,copied_right,copied_sizes));
    assert(!plain(true,true,true,false,0xd83cbf,copied_left,copied_sizes));
    for(unsigned key=0;key<8;++key){auto args=copied_left;args[key]+=.5f;
        assert(!plain(true,true,true,false,0x2b11a0,args,copied_sizes));}
    for(unsigned gate=0;gate<4;++gate)
        assert(!plain(gate!=0,gate!=1,gate!=2,gate==3,0x2b11a0,copied_left,copied_sizes));
    for(auto bad: {std::array<unsigned,4>{1792,980,1791,2160},
                  std::array<unsigned,4>{1792,980,3840,979},
                  std::array<unsigned,4>{1792,980,8193,2160},
                  std::array<unsigned,4>{1792,980,3840,8193}})
        assert(!plain(true,true,true,false,0x2b11a0,copied_left,bad));
    // End of plain Output policy checks.

    using namespace gears_final_transfer;
    // Failed Ultra run: already classified left/right half-quad on the
    // current 3840x2160 backbuffer, with an authenticated consuming ticket.
    gears_viewport::TargetDraw direct{0,1,{456,3840,2160,24,0},0};
    auto eye=[&](uintptr_t caller,bool classified,uint64_t ticket,bool attested,
                 gears_viewport::TargetDraw const& target){
        return eligible_eye_target(caller,classified,ticket,3840,2160,target,attested);};
    assert(eye(0x2b11a0,true,1636,true,direct));
    auto intermediate=direct;intermediate.target.resource=123;
    assert(eye(0x2b11a0,true,1636,true,intermediate));
    assert(!eye(0x2b11a0,false,1636,true,direct)); // Full quad/bad UV never classified.
    assert(!eye(0x2b11a0,true,0,true,direct));
    assert(!eye(0x2b11a0,true,1636,false,direct));
    assert(!eye(0xd83cbf,true,1636,true,direct));
    assert(!eye(0x9a7377,true,1636,true,direct)); // Still requires explicit transfer.
    auto invalid=direct;invalid.target.format=28;assert(!eye(0x2b11a0,true,1636,true,invalid));
    invalid=direct;invalid.target.width=1920;assert(!eye(0x2b11a0,true,1636,true,invalid));
    invalid=direct;invalid.target.height=2161;assert(!eye(0x2b11a0,true,1636,true,invalid));
    invalid=direct;invalid.target.mip=1;assert(!eye(0x2b11a0,true,1636,true,invalid));
    invalid=direct;invalid.targets=2;assert(!eye(0x2b11a0,true,1636,true,invalid));
    assert(eye(0xd83b1c,true,1636,false,direct));
    assert(eye(0xd808d8,true,1636,false,direct));
    Input a{};a.quad_caller=0x9a7377;a.draw_caller=0x1af92b;a.kind=1;a.elements=6;a.instances=1;a.slots=1;
    a.known_buffer=true;a.source={123,3,1920,1080,1,1,24,1};
    a.binding={0,1,{456,1920,1081,24,0},0};
    a.dimensions={1920,1081,1920,1080};a.args={0,0,1920,1081,0,-.5f,1920,1081};
    a.rectangles={true,{0,0,1920,1081},{0,0,16384,16384},1,1,true,0,1};
    assert(eligible(a));
    auto reject=[&](Input b){assert(!eligible(b));};
    auto b=a;b.source.resource=456;reject(b);b=a;b.slots=2;reject(b);b=a;b.quad_caller=0x2b11a0;reject(b);
    b=a;b.known_buffer=false;reject(b);b=a;b.args[5]=0;reject(b);b=a;b.args[4]=.5f;reject(b);
    b=a;b.dimensions[3]=1081;reject(b);b=a;b.source.width=1919;reject(b);b=a;b.source.height=1079;reject(b);
    b=a;b.source.mips=2;reject(b);b=a;b.source.samples=2;reject(b);b=a;b.source.format=28;reject(b);
    b=a;b.rectangles.scissor.height=1080;reject(b);b=a;b.rectangles.viewport.height=1080;reject(b);
    b=a;b.rectangles.max_depth=.5f;reject(b);b=a;b.args[7]=std::numeric_limits<float>::quiet_NaN();reject(b);
    b=a;b.descriptor_lost=true;reject(b);b=a;b.binding.target.height=1082;reject(b);
    a.binding.target.height=1080;a.dimensions[1]=1080;a.args[3]=a.args[7]=1080;a.args[5]=0;
    a.rectangles.viewport.height=1080;assert(eligible(a));
}
