#include "ui_call_boundary.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>

namespace cb=gears_ui_call_boundary;
struct Memory {
    std::map<std::uintptr_t,std::array<unsigned char,0x40>> frames;
    unsigned reads{};
    void set(std::uintptr_t address,std::uintptr_t node,std::uintptr_t object,std::uintptr_t previous){
        auto& raw=frames[address];const std::uintptr_t code=0x9000;
        std::memcpy(raw.data()+0x18,&node,8);std::memcpy(raw.data()+0x20,&object,8);
        std::memcpy(raw.data()+0x28,&code,8);std::memcpy(raw.data()+0x38,&previous,8);
    }
    bool read(void* output,void const* pointer,std::size_t size){
        ++reads;const auto address=reinterpret_cast<std::uintptr_t>(pointer);
        for(auto const& f:frames)if(address>=f.first&&address-f.first<=f.second.size()&&size<=f.second.size()-(address-f.first)){
            std::memcpy(output,f.second.data()+address-f.first,size);return true;
        }return false;
    }
};
int main(){
    Memory memory;auto read=[&](void* out,void const* pointer,std::size_t size){return memory.read(out,pointer,size);};
    constexpr std::uintptr_t caller=0x1000,callee_frame=0x2000,draw_frame=0x3000,node=0x5000,self=0x6000;
    memory.set(caller,0x7000,0x8000,0);memory.set(callee_frame,node,self,caller);
    assert(cb::body_ancestry(read,callee_frame,callee_frame,caller,node,self)); // Direct body.
    memory.set(draw_frame,0x7100,0x8100,callee_frame);
    assert(cb::body_ancestry(read,draw_frame,callee_frame,caller,node,self)); // Nested unselected helper.
    assert(!cb::body_ancestry(read,draw_frame,callee_frame,0x4000,node,self));
    assert(!cb::body_ancestry(read,draw_frame,callee_frame,caller,node+8,self));
    assert(!cb::body_ancestry(read,draw_frame,callee_frame,caller,node,self+8));
    assert(!cb::body_ancestry(read,draw_frame,draw_frame,caller,node,self)); // Wrong nearest selected scope.
    assert(!cb::body_ancestry(read,caller,caller,caller,node,self)); // Parameter evaluation at entry caller.
    memory.set(draw_frame,0x7100,0x8100,caller);
    assert(!cb::body_ancestry(read,draw_frame,callee_frame,caller,node,self)); // Missing actual callee.
    memory.set(draw_frame,0x7100,0x8100,draw_frame);
    assert(!cb::body_ancestry(read,draw_frame,callee_frame,caller,node,self)); // Cycle.
    memory.set(draw_frame,0x7100,0x8100,draw_frame+1);
    assert(!cb::body_ancestry(read,draw_frame,callee_frame,caller,node,self)); // Misaligned link.
    assert(!cb::body_ancestry(read,callee_frame+1,callee_frame,caller,node,self));
    assert(!cb::body_ancestry(read,callee_frame,callee_frame,caller+1,node,self));
    cb::Frame frame{};const auto before=memory.reads;
    assert(!cb::frame(read,(std::numeric_limits<std::uintptr_t>::max)()-7,frame));
    assert(memory.reads==before); // Overflow rejected before calling reader.
    assert(!cb::frame(read,0x4000,frame)); // Unreadable.
    memory.set(callee_frame,0,self,caller);assert(!cb::frame(read,callee_frame,frame));
    memory.set(callee_frame,node,self,caller);
    for(unsigned i=0;i<cb::frame_limit;++i)memory.set(0x10000+i*0x100,0x7100,0x8100,i?0x10000+(i-1)*0x100:callee_frame);
    assert(cb::body_ancestry(read,0x10000+(cb::frame_limit-2)*0x100,callee_frame,caller,node,self));
    const auto bounded_before=memory.reads;
    assert(!cb::body_ancestry(read,0x10000+(cb::frame_limit-1)*0x100,callee_frame,caller,node,self));
    assert(memory.reads-bounded_before==cb::frame_limit*4);

    cb::Nesting tls;std::uint64_t ids{};cb::Child first{++ids,99,0,self,caller,node,0,1};
    {cb::Nest outer(tls);outer.select(&first);assert(tls.depth==1&&tls.current->id==1);
        cb::Child nested{++ids,99,first.id,self,caller,node,0,2};
        {cb::Nest inner(tls);inner.select(&nested);assert(tls.depth==2&&tls.current->id==2&&tls.current->parent_child==1);}
        assert(tls.current==&first&&tls.depth==1);
        {cb::Nest rejected(tls);rejected.select(nullptr);assert(!tls.current&&tls.depth==2);}
        assert(tls.current==&first&&tls.depth==1);
        try {cb::Nest exceptional(tls);exceptional.select(nullptr);throw std::runtime_error("test");}catch(std::runtime_error const&){}
        assert(tls.current==&first&&tls.depth==1);
    }
    assert(!tls.current&&!tls.depth);
    auto deep=[&](auto const& recur,unsigned remaining)->void{
        const auto saved=tls.current;const auto depth=tls.depth;
        {cb::Nest next(tls);next.select(cb::selected_depth(tls)?&first:nullptr);
            if(tls.depth<=cb::frame_limit)assert(tls.current==&first);else assert(!tls.current);
            if(remaining)recur(recur,remaining-1);}
        assert(tls.current==saved&&tls.depth==depth);
    };
    deep(deep,cb::frame_limit);assert(!tls.current&&!tls.depth);
    cb::Child sibling{++ids,99,0,self,caller,node,0,1};assert(sibling.id!=first.id&&sibling.id==3);
    {cb::Nest next(tls);next.select(&sibling);assert(tls.current->id==3&&tls.depth==1);}
    assert(!tls.current&&!tls.depth);
    cb::Samples<unsigned,2> samples;
    assert(samples.append(10,false)&&samples.append(11,false));
    assert(!samples.append(12,false)&&samples.counts[1]==0);
    assert(samples.append(20,true)&&samples.append(21,true));
    assert(!samples.append(22,true));
    assert(samples.counts[0]==2&&samples.counts[1]==2&&samples.dropped[0]==1&&samples.dropped[1]==1);
    assert(samples.rows[0][0]==10&&samples.rows[0][1]==11&&samples.rows[1][0]==20&&samples.rows[1][1]==21);
    std::cout<<"UiCallBoundaryTest passed: body ancestry, exclusions, bounds, nesting restoration, separate sample budgets\n";
}
