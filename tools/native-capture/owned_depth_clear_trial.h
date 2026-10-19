#pragma once
#include "owned_attachment_observer.h"
#include "source_output_extent.h"

namespace gears_owned_depth_clear {
enum Reason:unsigned {accepted,disabled,identity,arguments,owner,output,attachments,rectangles,route,desktop,already_correct,source_extent,reason_count};
struct Viewport {gears_viewport::Rect rectangle{};float minimum{},maximum{};};
struct Input {
    bool enabled{},attested{},owned_current{},owned_active{},plan_valid{},desktop_valid{};
    uintptr_t module_base{},observer_base{},caller{};uint64_t ticket{},world{};
    uintptr_t desktop_chain{},desktop_queue{},owned_queue{};
    uint64_t desktop_epoch{},desktop_generation{},matched_generation{};long long inventory_qpc{};
    unsigned module_size{},observer_size{};
    unsigned vertices{},instances{},start{},first_instance{},width{},height{},stack_count{};
    unsigned plan_width{},plan_height{},desktop_width{},desktop_height{};
    unsigned desktop_count{},binding_count{};std::array<uintptr_t,16> inventory_resources{},binding_resources{};
    gears_viewport::DrawState state{};gears_owned_attachments::Snapshot om{};
    gears_source_output::Extent source_output{};
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
    if(!in.owned_active||!in.plan_valid||in.width!=in.plan_width||in.height!=in.plan_height||
       in.width<128||in.width>8192||(in.width&1)||in.height<64||in.height>8192){p.reason=output;return p;}
    if(!in.om.observed||!in.om.serial||!in.om.complete()||in.om.count||in.om.descriptor_capacity_lost||
       !in.om.depth_present||!in.om.depth_read||!in.om.depth_handle){p.reason=attachments;return p;}
    if(!in.state.tracked||in.state.viewport_count!=1||in.state.scissor_count!=1||!in.state.depth_known||
       in.state.min_depth!=0||in.state.max_depth!=1||
       !(in.state.scissor==gears_viewport::Rect{0,0,16384,16384})){p.reason=rectangles;return p;}
    const bool correct=in.state.viewport==p.after.rectangle;
    bool buffers_match=in.desktop_count&&in.desktop_count<=in.inventory_resources.size()&&
        in.desktop_count==in.binding_count&&in.inventory_resources==in.binding_resources;
    for(unsigned i=0;buffers_match&&i<in.desktop_count;++i)if(!in.inventory_resources[i])buffers_match=false;
    if(!correct&&(!in.desktop_valid||!in.desktop_chain||!in.inventory_qpc||!in.desktop_epoch||!in.desktop_generation||
       in.desktop_generation!=in.matched_generation||!in.desktop_queue||in.desktop_queue!=in.owned_queue||
       !buffers_match||
       !in.desktop_width||!in.desktop_height)){p.reason=desktop;return p;}
    // Present may legitimately be tiny while the native scene still uses the
    // original engine extent. The frozen source is bound to this exact ticket
    // and world; it does not establish the DSV's resource dimensions.
    if(!correct&&(!in.source_output.matches(in.ticket,in.world)||
       !(in.state.viewport==gears_viewport::Rect{0,0,float(in.source_output.rectangle[2]),float(in.source_output.rectangle[3])}))){
        p.reason=source_extent;return p;}
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
    p.reason=native_route?(correct?already_correct:accepted):route;return p;
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
