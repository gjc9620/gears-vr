// Included in capture.cpp's private namespace. Bounded CPU state observation at
// the existing final-quad hook. Resource identity queries use COM GetDesc;
// no new hook, GPU commands or resource-state changes.
struct FinalInputSample {
    uintptr_t context{},view{},candidate{},selected{},object{};
    uint32_t selector{},buffer_index{};unsigned slots{},valid{};
    std::array<uintptr_t,32> textures{};
    std::array<ResourceIdentity,4> resources{};
    uintptr_t color_target{},default_resolve_wrapper{};unsigned color_links_valid{};
    ResourceIdentity color_target_resource{};
};
struct FinalInputTrace {FinalInputSample first{};uint64_t calls{};};
std::array<FinalInputTrace,2> final_input_traces{};
std::mutex final_input_mutex;
uint64_t final_input_rejected{};
bool collect_final_inputs(uintptr_t context,uintptr_t view,uintptr_t candidate,FinalInputSample& out){
    FinalInputSample sample{};sample.context=context;sample.view=view;sample.candidate=candidate;
    unsigned char count{};
    if(!context||!view||!safe_copy(&count,reinterpret_cast<void*>(context+0x65d),1)||
       !safe_copy(&sample.selector,reinterpret_cast<void*>(view+0x3e8),4))return false;
    sample.slots=std::min(unsigned(count),unsigned(sample.textures.size()));
    if(sample.slots&&!safe_copy(sample.textures.data(),reinterpret_cast<void*>(context+0x768),sample.slots*8))return false;
    sample.valid=1;
    if(candidate&&safe_copy(&sample.selected,reinterpret_cast<void*>(candidate),8)){
        sample.valid|=2;
        if(sample.selected>=0x13c){
            uintptr_t wrapper=sample.selected-0x13c;
            if(safe_copy(&sample.buffer_index,reinterpret_cast<void*>(wrapper+0x138),4)&&sample.buffer_index<8&&
               safe_copy(&sample.object,reinterpret_cast<void*>(wrapper+0x38+sample.buffer_index*8),8))sample.valid|=4;
        }
    }
    out=sample;return true;
}
void observe_final_inputs(){
    if(self_testing||!recording.load()||!viewport_diagnostics||!trial_draw_scope||!consuming_renderer)return;
    uintptr_t tls{},context{},view{},views{};int count{};
    if(!safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)||!tls||
       !safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)||!context||
       !safe_copy(&view,reinterpret_cast<void*>(context+0x14c0),8)||
       !safe_copy(&views,reinterpret_cast<char*>(consuming_renderer)+0x68,8)||
       !safe_copy(&count,reinterpret_cast<char*>(consuming_renderer)+0x70,4)||
       !views||count<1||count>2||view<views||(view-views)%0xd30||(view-views)/0xd30>=unsigned(count)){
        std::lock_guard<std::mutex> lock(final_input_mutex);++final_input_rejected;return;
    }
    unsigned eye=unsigned((view-views)/0xd30);
    std::lock_guard<std::mutex> lock(final_input_mutex);
    auto& trace=final_input_traces[eye];
    if(trace.calls){++trace.calls;return;}
    uint32_t selector{};
    if(!safe_copy(&selector,reinterpret_cast<void*>(view+0x3e8),4))return;
    // 1665dc selects +3b410 for selector==0; this final caller supplies
    // param5==0, so selector!=0 selects +3b488 (not +3b4c4).
    uintptr_t candidate=base+(selector?0x1c3b488:0x1c3b410);
    if(collect_final_inputs(context,view,candidate,trace.first)){
        for(unsigned i=0;i<std::min(trace.first.slots,4U);++i)query_bound_resource(trace.first.textures[i],trace.first.resources[i]);
        auto& first=trace.first;
        if(safe_copy(&first.color_target,reinterpret_cast<void*>(base+0x1c3b420),8)&&first.color_target>=0x13c){
            first.color_links_valid|=1;
            if(safe_copy(&first.default_resolve_wrapper,reinterpret_cast<void*>(first.color_target-0x48),8))first.color_links_valid|=2;
            query_public_resource(first.color_target,first.color_target_resource);
        }
        trace.calls=1;
    }
    else ++final_input_rejected;
}
void print_final_inputs(FILE* file){
    fprintf(file,"final_input_summary rejected=%llu cpu_only=1 first_sample_only=1 pixels_verified=0\n",final_input_rejected);
    for(unsigned eye=0;eye<2;++eye){const auto& t=final_input_traces[eye];if(!t.calls)continue;const auto& s=t.first;
        fprintf(file,"final_input eye=%u calls=%llu valid=%u context=%llx view=%llx selector=%u candidate=%llx selected=%llx buffer_index=%u object=%llx slots=%u textures=",
            eye,t.calls,s.valid,static_cast<unsigned long long>(s.context),static_cast<unsigned long long>(s.view),s.selector,
            static_cast<unsigned long long>(s.candidate),static_cast<unsigned long long>(s.selected),s.buffer_index,
            static_cast<unsigned long long>(s.object),s.slots);
        for(unsigned i=0;i<s.slots;++i)fprintf(file,"%s%llx",i?",":"",static_cast<unsigned long long>(s.textures[i]));
        fprintf(file,"\n");
        for(unsigned slot=0;slot<std::min(s.slots,4U);++slot){const auto& r=s.resources[slot];const auto& d=r.desc;
            fprintf(file,"input_resource eye=%u slot=%u object=%llx native=%llx resource=%llx hr=%08lx dimension=%u width=%llu height=%u depth=%u mips=%u format=%u samples=%u flags=%u\n",
                eye,slot,static_cast<unsigned long long>(s.textures[slot]),static_cast<unsigned long long>(r.native),
                static_cast<unsigned long long>(r.resource),static_cast<unsigned long>(r.result),unsigned(d.Dimension),d.Width,d.Height,
                unsigned(d.DepthOrArraySize),unsigned(d.MipLevels),unsigned(d.Format),d.SampleDesc.Count,unsigned(d.Flags));
            fprintf(file,"input_engine_state eye=%u slot=%u wrapper=%llx readable=%u state=%u flags=%u gpu_state_verified=0\n",
                eye,slot,static_cast<unsigned long long>(r.engine_wrapper),r.engine_state_read,r.engine_state,unsigned(r.engine_flags));
        }
        const auto& target=s.color_target_resource;
        fprintf(file,"input_color_links eye=%u valid=%u target_public=%llx default_resolve_wrapper=%llx selected_wrapper=%llx target_native=%llx target_hr=%08lx target_width=%llu target_height=%u target_format=%u target_engine_state=%u target_state_readable=%u target_engine_flags=%u same_native_as_slot0=%u\n",
            eye,s.color_links_valid,static_cast<unsigned long long>(s.color_target),static_cast<unsigned long long>(s.default_resolve_wrapper),
            static_cast<unsigned long long>(s.selected>=0x13c?s.selected-0x13c:0),static_cast<unsigned long long>(target.resource),
            static_cast<unsigned long>(target.result),target.desc.Width,target.desc.Height,unsigned(target.desc.Format),target.engine_state,target.engine_state_read,unsigned(target.engine_flags),
            SUCCEEDED(target.result)&&target.resource&&target.resource==s.resources[0].resource);
    }
}
bool final_input_decoder_test(){
    std::array<unsigned char,0x1000> context{};std::array<unsigned char,0xd30> view{};
    std::array<unsigned char,0x150> wrapper{};uintptr_t selected=reinterpret_cast<uintptr_t>(wrapper.data())+0x13c;
    uintptr_t object=0x12345678,texture=0x87654321;uint32_t index=1,selector=7;
    context[0x65d]=255;memcpy(context.data()+0x768,&texture,8);memcpy(view.data()+0x3e8,&selector,4);
    memcpy(wrapper.data()+0x138,&index,4);memcpy(wrapper.data()+0x40,&object,8);
    auto before=context;FinalInputSample sample{};
    if(!collect_final_inputs(reinterpret_cast<uintptr_t>(context.data()),reinterpret_cast<uintptr_t>(view.data()),reinterpret_cast<uintptr_t>(&selected),sample)||
       sample.valid!=7||sample.slots!=32||sample.textures[0]!=texture||sample.selector!=7||sample.object!=object||context!=before)return false;
    index=8;memcpy(wrapper.data()+0x138,&index,4);
    if(!collect_final_inputs(reinterpret_cast<uintptr_t>(context.data()),reinterpret_cast<uintptr_t>(view.data()),reinterpret_cast<uintptr_t>(&selected),sample)||sample.valid!=3)return false;
    return !collect_final_inputs(1,reinterpret_cast<uintptr_t>(view.data()),0,sample);
}
