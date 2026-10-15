// A/B/A geometry-only diagnostic at the verified family mask call. The full
// original function still runs; only its float t can change from 0 to 1.
using FirstViewMaskFn=void(__fastcall*)(float);
FirstViewMaskFn original_first_view_mask{};void* first_view_mask_target{};
bool first_view_mask_control{},first_view_mask_reserved{};HANDLE first_view_mask_reserved_event{};
bool view_union_mask_fix{};
bool first_view_mask_hook_enabled(){return first_view_mask_control||view_union_mask_fix;}
constexpr unsigned char first_view_mask_signature[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x70,0x10,0x48,0x89,0x78,0x18,0x55};
constexpr unsigned char first_view_mask_call_signature[]={0xe8,0x66,0xe2,0x18,0x00};
namespace gears_first_view_mask {
enum Reject : unsigned { accepted,disabled,scope,caller,read_failed,vtable,views,rectangles,dimensions,viewport,argument,clock,reject_count };
struct Snapshot {
    bool readable{},command_list_valid{};
    uintptr_t renderer{},storage{},context{},list{},list_vtable{};int count{},capacity{};
    std::array<std::array<float,4>,2> source{};std::array<std::array<int,4>,2> target{};
    std::array<unsigned,2> color_dimensions{};
    std::array<float,6> viewport{};unsigned char viewport_count{},scissor_count{};
    std::array<int,4> scissor{};std::array<uint32_t,3> depth{};
    std::array<unsigned char,8> requested_masks{},actual_masks{};
};
struct Gate {bool enabled{},recording{},diagnostics{},trial{},renderer{};uintptr_t caller{};};
struct Decision {float forwarded{};unsigned phase{gears_quad_source::waiting},rejection{disabled};bool modified{};};
Decision decide(float value,const Gate& gate,const Snapshot& sample,long long tick,long long frequency,long long& origin){
    Decision result;result.forwarded=value;result.phase=gears_quad_source::phase_at(origin,tick,frequency);
    if(!gate.enabled)return result;
    result.rejection=scope;if(!gate.recording||!gate.diagnostics||!gate.trial||!gate.renderer)return result;
    result.rejection=caller;if(gate.caller!=0x119be2)return result;
    result.rejection=read_failed;if(!sample.readable)return result;
    result.rejection=vtable;if(!sample.command_list_valid)return result;
    result.rejection=views;if(!sample.storage||sample.storage%8||sample.count!=2||sample.capacity<2||sample.capacity>1024)return result;
    result.rejection=rectangles;
    if(sample.source[0]!=std::array<float,4>{0,0,960,1080}||sample.source[1]!=std::array<float,4>{960,0,960,1080}||
       sample.target[0]!=std::array<int,4>{0,0,960,1080}||sample.target[1]!=std::array<int,4>{960,0,960,1080})return result;
    result.rejection=dimensions;if(sample.color_dimensions!=std::array<unsigned,2>{1920,1080})return result;
    result.rejection=viewport;if(sample.viewport_count!=1||sample.viewport!=std::array<float,6>{0,0,1920,1080,0,1})return result;
    uint32_t argument_bits{};memcpy(&argument_bits,&value,4);
    result.rejection=argument;if(argument_bits!=0)return result;
    result.rejection=clock;if(tick<=0||frequency<=0||(origin&&tick<origin))return result;
    if(!origin)origin=tick;
    result.phase=gears_quad_source::phase_at(origin,tick,frequency);result.rejection=accepted;
    if(result.phase==gears_quad_source::b){result.forwarded=1.f;result.modified=true;}
    return result;
}
Snapshot valid_test_snapshot(){
    Snapshot s{};s.readable=s.command_list_valid=true;s.storage=0x1000;s.count=s.capacity=2;
    s.source={std::array<float,4>{0,0,960,1080},std::array<float,4>{960,0,960,1080}};
    s.target={std::array<int,4>{0,0,960,1080},std::array<int,4>{960,0,960,1080}};
    s.color_dimensions={1920,1080};s.viewport={0,0,1920,1080,0,1};s.viewport_count=1;s.scissor_count=1;s.scissor={0,0,16384,16384};return s;
}
bool self_test(){
    const Gate valid{true,true,true,true,true,0x119be2};const auto sample=valid_test_snapshot();
    long long origin{};auto result=decide(0,valid,sample,10000,1000,origin);
    if(origin!=10000||result.rejection||result.modified||result.forwarded!=0)return false;
    const long long ticks[]={12999,13000,15999,16000,19999,20000};const unsigned phases[]={0,1,1,2,2,3};
    for(unsigned i=0;i<6;++i){result=decide(0,valid,sample,ticks[i],1000,origin);
        if(result.rejection||result.phase!=phases[i]||result.modified!=(phases[i]==1)||result.forwarded!=(phases[i]==1?1.f:0.f))return false;}
    for(unsigned i=0;i<15;++i){auto gate=valid;auto s=sample;float value=0;
        switch(i){case 0:gate.enabled=false;break;case 1:gate.recording=false;break;case 2:gate.diagnostics=false;break;
        case 3:gate.trial=false;break;case 4:gate.renderer=false;break;case 5:++gate.caller;break;
        case 6:s.readable=false;break;case 7:s.command_list_valid=false;break;case 8:++s.storage;break;
        case 9:s.count=1;break;case 10:s.capacity=1;break;case 11:++s.target[1][0];break;
        case 12:++s.color_dimensions[0];break;case 13:s.viewport_count=2;break;case 14:value=.5f;break;}
        result=decide(value,gate,s,14000,1000,origin);
        if(!result.rejection||result.modified||result.forwarded!=value||origin!=10000)return false;
    }
    for(unsigned i=0;i<6;++i){auto s=sample;s.viewport[i]+=.25f;
        if(decide(0,valid,s,14000,1000,origin).rejection!=viewport)return false;}
    for(unsigned eye=0;eye<2;++eye)for(unsigned axis=0;axis<4;++axis){auto s=sample;s.source[eye][axis]+=.25f;
        if(decide(0,valid,s,14000,1000,origin).rejection!=rectangles)return false;
        s=sample;++s.target[eye][axis];if(decide(0,valid,s,14000,1000,origin).rejection!=rectangles)return false;}
    if(decide(0,valid,sample,9999,1000,origin).rejection!=clock||decide(0,valid,sample,14000,0,origin).rejection!=clock)return false;
    return true;
}
}
std::mutex first_view_mask_mutex;long long first_view_mask_origin{};
struct FirstViewMaskTrace {uint64_t calls{},modified{},rejected{};long long first{},last{};bool has_sample{};gears_first_view_mask::Snapshot sample{};};
std::array<FirstViewMaskTrace,gears_quad_source::phase_count> first_view_mask_traces{};
std::array<uint64_t,gears_first_view_mask::reject_count> first_view_mask_rejections{};
std::array<HANDLE,3> first_view_mask_events{};unsigned first_view_mask_event_phase{gears_quad_source::waiting};
bool collect_first_view_mask(uintptr_t renderer,uintptr_t context,gears_first_view_mask::Snapshot& sample){
    sample.renderer=renderer;sample.context=context;
    struct Header {uintptr_t data;int count,capacity;} header{};uintptr_t owner{},draw{};
    if(!renderer||!context||!safe_copy(&header,reinterpret_cast<void*>(renderer+0x68),sizeof(header)))return false;
    sample.storage=header.data;sample.count=header.count;sample.capacity=header.capacity;
    if(!header.data||header.data%8||header.count!=2||header.capacity<2||header.capacity>1024)return false;
    for(unsigned eye=0;eye<2;++eye){const auto view=header.data+uintptr_t(eye)*0xd30;
        if(!safe_copy(sample.source[eye].data(),reinterpret_cast<void*>(view+0x48),16)||
           !safe_copy(sample.target[eye].data(),reinterpret_cast<void*>(view+0x58),16))return false;
    }
    if(!safe_copy(sample.color_dimensions.data(),reinterpret_cast<void*>(base+0x1c3b3b8),8)||
       !safe_copy(sample.viewport.data(),reinterpret_cast<void*>(context+0x290),24)||
       !safe_copy(&sample.viewport_count,reinterpret_cast<void*>(context+0x560),1)||
       !safe_copy(&sample.scissor_count,reinterpret_cast<void*>(context+0x561),1)||
       !safe_copy(sample.scissor.data(),reinterpret_cast<void*>(context+0x410),16)||
       !safe_copy(sample.depth.data(),reinterpret_cast<void*>(context+0xe4),12)||
       !safe_copy(sample.requested_masks.data(),reinterpret_cast<void*>(context+0x136c),8))return false;
    for(unsigned i=0;i<8;++i)if(!safe_copy(&sample.actual_masks[i],reinterpret_cast<void*>(context+0x174+i*0x28),1))return false;
    if(!safe_copy(&owner,reinterpret_cast<void*>(context+0x127c),8)||!owner||
       !safe_copy(&sample.list,reinterpret_cast<void*>(owner+0x14),8)||!sample.list||
       !safe_copy(&sample.list_vtable,reinterpret_cast<void*>(sample.list),8)||!sample.list_vtable||
       !safe_copy(&draw,reinterpret_cast<void*>(sample.list_vtable+12*8),8))return false;
    sample.command_list_valid=system_resource_methods(sample.list)&&viewport_targets[3]&&draw==reinterpret_cast<uintptr_t>(viewport_targets[3]);
    sample.readable=true;return true;
}
void clear_first_view_mask_events_locked(){
    for(auto event:first_view_mask_events)if(event)ResetEvent(event);first_view_mask_event_phase=gears_quad_source::waiting;
}
bool create_first_view_mask_events(){
    if(!first_view_mask_control)return true;
    for(unsigned i=0;i<3;++i){wchar_t name[128];swprintf_s(name,L"Local\\GearsVR.Capture.FirstViewMask.%lu.%ls",GetCurrentProcessId(),gears_quad_source::event_names[i]);
        first_view_mask_events[i]=CreateEventW(nullptr,TRUE,FALSE,name);
        if(!first_view_mask_events[i]){for(auto& event:first_view_mask_events)if(event){CloseHandle(event);event=nullptr;}return false;}
        ResetEvent(first_view_mask_events[i]);
    }
    return true;
}
void refresh_first_view_mask_events(){
    if(!first_view_mask_control)return;std::lock_guard<std::mutex> lock(first_view_mask_mutex);
    if(gears_quad_source::phase_at(first_view_mask_origin,now(),qpc_frequency)!=first_view_mask_event_phase)clear_first_view_mask_events_locked();
}
void close_first_view_mask_events(){
    std::lock_guard<std::mutex> lock(first_view_mask_mutex);clear_first_view_mask_events_locked();
    for(auto& event:first_view_mask_events)if(event){CloseHandle(event);event=nullptr;}
}
#include "view_union_mask_fix.h"
void __fastcall hook_first_view_mask(float value){
    Scope scope;if(!scope.world())return original_first_view_mask(value);using namespace gears_first_view_mask;
    Gate gate{first_view_mask_hook_enabled(),recording.load(),viewport_diagnostics,trial_draw_scope,consuming_renderer!=nullptr,
        reinterpret_cast<uintptr_t>(_ReturnAddress())-base};
    Snapshot sample{};
    if(gate.enabled&&gate.recording&&gate.diagnostics&&gate.trial&&gate.renderer&&gate.caller==0x119be2){
        uintptr_t tls{},context{};
        if(safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
           safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8))collect_first_view_mask(reinterpret_cast<uintptr_t>(consuming_renderer),context,sample);
    }
    if(view_union_mask_fix){
        gears_view_union_mask::Decision union_decision;
        bool owned_height_padding=false;
#if defined(GEARS_NATIVE_RESIZE_REUSE) && GEARS_NATIVE_RESIZE_REUSE
        owned_height_padding=consuming_owned_scene&&consuming_frame_ticket&&consuming_renderer;
#endif
        {std::lock_guard<std::mutex> lock(view_union_mask_mutex);union_decision=gears_view_union_mask::decide_union(value,gate,sample,now(),qpc_frequency,view_union_mask_origin,owned_height_padding);}
        original_first_view_mask(union_decision.forwarded);
        complete_view_union_mask(value,union_decision,sample);return;
    }
    Decision decision{};
    {std::lock_guard<std::mutex> lock(first_view_mask_mutex);decision=decide(value,gate,sample,now(),qpc_frequency,first_view_mask_origin);}
    original_first_view_mask(decision.forwarded);
    if(!first_view_mask_control)return;
    const auto tick=now();std::lock_guard<std::mutex> lock(first_view_mask_mutex);auto& trace=first_view_mask_traces[decision.phase];
    ++trace.calls;trace.modified+=decision.modified;
    if(decision.rejection!=accepted){++trace.rejected;++first_view_mask_rejections[decision.rejection];}
    else {if(!trace.first)trace.first=tick;trace.last=tick;if(!trace.has_sample){trace.sample=sample;trace.has_sample=true;}}
    const auto current=gears_quad_source::phase_at(first_view_mask_origin,tick,qpc_frequency);
    if(current!=first_view_mask_event_phase)clear_first_view_mask_events_locked();
    if(decision.rejection==accepted&&current==decision.phase&&current<3&&current!=first_view_mask_event_phase&&first_view_mask_events[current]){
        SetEvent(first_view_mask_events[current]);first_view_mask_event_phase=current;
    }
}
void print_first_view_mask(FILE* file){
    std::lock_guard<std::mutex> lock(first_view_mask_mutex);
    fprintf(file,"first_view_mask_control enabled=%u origin_qpc=%lld frequency=%lld boundaries_seconds=3,6,10 caller=119be2 original_t=0 modified_t=1 cpu_phase_only=1\n",first_view_mask_control,first_view_mask_origin,qpc_frequency);
    for(unsigned phase=0;phase<first_view_mask_traces.size();++phase){const auto& t=first_view_mask_traces[phase];if(!t.calls)continue;
        fprintf(file,"first_view_mask_phase phase=%s calls=%llu modified=%llu rejected=%llu first_qpc=%lld last_qpc=%lld sample_valid=%u\n",gears_quad_source::phase_names[phase],t.calls,t.modified,t.rejected,t.first,t.last,t.has_sample);
        if(t.has_sample){const auto& s=t.sample;
            fprintf(file,"first_view_mask_context phase=%s renderer=%llx storage=%llx context=%llx list=%llx list_vtable=%llx viewport_count=%u viewport=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g scissor_count=%u scissor=%d,%d,%d,%d depth=%u,%u,%u requested_masks=",
                gears_quad_source::phase_names[phase],static_cast<unsigned long long>(s.renderer),static_cast<unsigned long long>(s.storage),static_cast<unsigned long long>(s.context),static_cast<unsigned long long>(s.list),static_cast<unsigned long long>(s.list_vtable),s.viewport_count,
                s.viewport[0],s.viewport[1],s.viewport[2],s.viewport[3],s.viewport[4],s.viewport[5],s.scissor_count,s.scissor[0],s.scissor[1],s.scissor[2],s.scissor[3],s.depth[0],s.depth[1],s.depth[2]);
            for(unsigned i=0;i<8;++i)fprintf(file,"%s%u",i?",":"",s.requested_masks[i]);fprintf(file," actual_masks=");
            for(unsigned i=0;i<8;++i)fprintf(file,"%s%u",i?",":"",s.actual_masks[i]);fprintf(file,"\n");
        }
    }
    for(unsigned reason=1;reason<first_view_mask_rejections.size();++reason)if(first_view_mask_rejections[reason])
        fprintf(file,"first_view_mask_rejected reason=%u calls=%llu\n",reason,first_view_mask_rejections[reason]);
}
