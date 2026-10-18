#pragma once
#include "owned_attachment_observer.h"

namespace gears_owned_depth_clear {
enum Reason:unsigned {accepted,disabled,identity,arguments,owner,output,attachments,rectangles,route,reason_count};
struct Viewport {gears_viewport::Rect rectangle{};float minimum{},maximum{};};
struct Input {
    bool enabled{},attested{},owned_current{},owned_active{};
    uintptr_t module_base{},observer_base{},caller{};uint64_t ticket{},world{};
    unsigned module_size{},observer_size{};
    unsigned vertices{},instances{},start{},first_instance{},width{},height{},stack_count{};
    gears_viewport::DrawState state{};gears_owned_attachments::Snapshot om{};
    std::array<uintptr_t,12> stack{};
};
struct Plan {Reason reason=disabled;Viewport before{},after{};bool apply()const{return reason==accepted;}};
inline Plan plan(Input const& in){
    Plan p{};p.before={in.state.viewport,in.state.min_depth,in.state.max_depth};
    p.after={{0,0,float(in.width),float(in.height)},in.state.min_depth,in.state.max_depth};
    if(!in.enabled)return p;
    if(!in.attested){p.reason=identity;return p;}
    if(!in.module_base||in.caller!=in.module_base+0x1af345||in.vertices!=4||in.instances!=1||in.start||in.first_instance){p.reason=arguments;return p;}
    if(!in.owned_current||!in.ticket||!in.world){p.reason=owner;return p;}
    if(!in.owned_active||in.width!=5120||in.height!=2800){p.reason=output;return p;}
    if(!in.om.observed||!in.om.serial||!in.om.complete()||in.om.count||in.om.descriptor_capacity_lost||
       !in.om.depth_present||!in.om.depth_read||!in.om.depth_handle){p.reason=attachments;return p;}
    if(!in.state.tracked||in.state.viewport_count!=1||in.state.scissor_count!=1||!in.state.depth_known||
       in.state.min_depth!=0||in.state.max_depth!=1||!(in.state.viewport==gears_viewport::Rect{0,0,3840,2160})||
       !(in.state.scissor==gears_viewport::Rect{0,0,16384,16384})){p.reason=rectangles;return p;}
    // Only our injected module may sit between these native returns. Never
    // search past another game return, or silently skip an unrelated DLL.
    const bool bounds=in.module_size>0x27e752&&in.module_base<=UINTPTR_MAX-in.module_size&&in.observer_base&&in.observer_size&&
        in.observer_base<=UINTPTR_MAX-in.observer_size&&
        (in.module_base+in.module_size<=in.observer_base||in.observer_base+in.observer_size<=in.module_base);
    auto contains=[](uintptr_t address,uintptr_t start,unsigned size){return address>=start&&address-start<size;};
    const uintptr_t expected[]{in.module_base+0x1af345,in.module_base+0x1ade10,in.module_base+0x27e752};
    bool native_route=false;unsigned next{};
    for(unsigned i=0;bounds&&i<in.stack_count&&i<in.stack.size();++i){const auto address=in.stack[i];
        if(contains(address,in.module_base,in.module_size)){
            if(address!=expected[next])break;
            if(++next==3){native_route=true;break;}
        }else if(!contains(address,in.observer_base,in.observer_size))break;
    }
    p.reason=native_route?accepted:route;return p;
}
// The sole draw is forwarded unchanged. Restore all six viewport values while
// unwinding a C++ draw exception too; scissor and engine metadata are untouched.
template<class SetViewport,class Draw>
void forward(Plan const& p,SetViewport set,Draw draw){
    if(!p.apply()){draw();return;}
    struct Restore {Viewport before;SetViewport& set;~Restore(){set(before);}} restore{p.before,set};
    set(p.after);draw();
}
}
