#include "death_constant_observer.h"
namespace death_constant_observer {
#if defined(GEARS_DEATH_CONSTANT_OBSERVER) && GEARS_DEATH_CONSTANT_OBSERVER
using namespace gears_death_constants;
using Fn=uint64_t(__fastcall*)(uintptr_t,const uint16_t*,const void*,unsigned);
Fn original{};void* target{};bool attested{};Recorder recorder;
template<class T> bool read(T& out,uintptr_t address,uintptr_t offset=0){
    return address&&offset<=UINTPTR_MAX-address&&sizeof(T)-1<=UINTPTR_MAX-(address+offset)&&
        safe_copy(&out,reinterpret_cast<void*>(address+offset),sizeof(T));
}
View view(){
    View out;out.thread=GetCurrentThreadId();out.native.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
    uintptr_t root=__readgsqword(0x58),slot{};
    if(read(slot,root)&&slot&&read(out.context,slot,0x20)&&out.context&&read(out.native.current_view,out.context,0x14c0))out.native.read_mask|=1;
    if(read(out.native.renderer_views,out.native.renderer,0x68))out.native.read_mask|=2;
    if(read(out.native.renderer_count,out.native.renderer,0x70))out.native.read_mask|=4;
    out.native.eye_valid=out.native.read_mask==7&&gears_unpossessed_quad::native_eye_index(out.native.current_view,out.native.renderer_views,out.native.renderer_count,out.native.eye);
    out.rectangles=out.native.eye_valid&&read(out.source_rect,out.native.current_view,0x48)&&finite(out.source_rect)&&read(out.target_rect,out.native.current_view,0x58);
    return out;
}
Destination destination_snapshot(Descriptor const& descriptor,unsigned index){
    Destination out;
    if(index||descriptor[0]>=16||descriptor[2]<16)return out;
    out.root=__readgsqword(0x58);if(!out.root)return out;out.read_mask|=1;
    if(!read(out.slot,out.root)||!out.slot)return out;out.read_mask|=2;
    if(!read(out.active,out.slot,0x10))return out;out.read_mask|=4;if(!out.active)return out;
    if(!read(out.context,out.slot,0x20)||!out.context)return out;out.read_mask|=8;
    if(!read(out.table,out.context,0x13ac)||!out.table)return out;out.read_mask|=16;
    if(!read(out.bank,out.table,uintptr_t(descriptor[0])*8)||!out.bank)return out;out.read_mask|=32;
    if(!read(out.data,out.bank,0x3c)||!out.data)return out;out.read_mask|=64;
    if(!read(out.highwater,out.bank,0x44))return out;out.read_mask|=128;
    if(!destination(descriptor,index,out.data,out.address))return out;
    out.readable=read(out.value,out.address);if(out.readable)out.read_mask|=256;
    out.finite_value=out.readable&&finite(out.value);return out;
}
bool attest(FILE* file){
    attested=!self_testing&&continuous_session&&frame_link_diagnostics&&viewport_diagnostics&&
        final_transfer_live::verify_range(file,0x1760b4,205,"d7d20d3f5c510685b8b3ffb12d12d51ae583d4070e00cf11a39655c7e086c45b")&&
        final_transfer_live::verify_range(file,0xd80630,1104,"cd8bb3b4a32cf5de1893c8cb9dcf85dabfbe5b3614d90d9ed9c1834afe548314");
    target=reinterpret_cast<void*>(base+0x1760b4);return attested;
}
uint64_t __fastcall hook(uintptr_t rcx,const uint16_t* descriptor,const void* data,unsigned index){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;
    Scope scope;if(!scope.world())return original(rcx,descriptor,data,index);
    if(role(caller)>1||!attested||!recording.load()||!consuming_frozen_frame.unpossessed)return original(rcx,descriptor,data,index);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    auto* selected=current_unpossessed_quad_bank();
    if(!selected||!eligible(attested,recording.load(),frame_link_diagnostics,consuming_frozen_frame.enabled,consuming_frozen_frame.unpossessed,
        consuming_frame_ticket,consuming_frozen_frame.id,selected->ordinal,caller,index))return original(rcx,descriptor,data,index);
    const auto before=view();if(!before.native.eye_valid){++recorder.rejected;return original(rcx,descriptor,data,index);}
    auto* row=recorder.reserve(before.native.eye,role(caller));if(!row)return original(rcx,descriptor,data,index);
    row->ticket=selected->ticket.id;row->world=selected->ticket.world;row->caller=caller;row->rcx=rcx;row->index=index;
    row->descriptor_address=reinterpret_cast<uintptr_t>(descriptor);row->source_address=reinterpret_cast<uintptr_t>(data);
    row->owner=owner(row->descriptor_address,row->kind);row->before_view=before;row->before_qpc=now();
    row->descriptor_before=read(row->before_descriptor,row->descriptor_address);
    row->source_before=read(row->before_source,row->source_address)&&finite(row->before_source);
    if(row->owner&&row->descriptor_before&&row->source_before)row->before_destination=destination_snapshot(row->before_descriptor,index);
    row->result=original(rcx,descriptor,data,index); // Exactly one unchanged original call on every path.
    row->after_qpc=now();row->after_view=view();
    row->descriptor_after=read(row->after_descriptor,row->descriptor_address);
    row->source_after=read(row->after_source,row->source_address)&&finite(row->after_source);
    if(row->owner&&row->descriptor_after&&row->source_after)row->after_destination=destination_snapshot(row->after_descriptor,index);
    return row->result;
}
void quad(uintptr_t caller,std::array<float,8> const& args,std::array<unsigned,4> const& dimensions){
    if(caller!=0xd80a7b||!attested||!recording.load()||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* selected=current_unpossessed_quad_bank();
    if(!selected||selected->ordinal!=49)return;const auto current=view();if(!current.native.eye_valid){++recorder.rejected;return;}
    auto& final=recorder.finals[current.native.eye];if(final.present){++recorder.duplicate_finals;return;}
    final.present=true;final.eye=current.native.eye;final.view=current;final.ticket=selected->ticket.id;final.world=selected->ticket.world;
    final.qpc=now();final.args=args;final.dimensions=dimensions;
    const auto& ratio=recorder.setters[final.eye*2];const auto& transform=recorder.setters[final.eye*2+1];
    final.owner_pair=ratio.present&&transform.present&&ratio.owner&&ratio.owner==transform.owner&&ratio.ticket==final.ticket&&transform.ticket==final.ticket&&ratio.world==final.world&&transform.world==final.world;
    final.view_pair=final.owner_pair&&current.rectangles&&ratio.before_view.rectangles&&ratio.after_view.rectangles&&transform.before_view.rectangles&&transform.after_view.rectangles&&
        ratio.before_view==ratio.after_view&&transform.before_view==transform.after_view&&ratio.after_view==current&&transform.after_view==current;
    for(unsigned kind=0;kind<2;++kind){const auto& setter=recorder.setters[final.eye*2+kind];
        final.source[kind]=setter.before_source;final.source_valid[kind]=setter.present&&setter.source_before;
        // R8 may be a reused caller stack temporary. Never reread it here.
        if(!final.owner_pair)continue;
        final.descriptor_current[kind]=read(final.descriptors[kind],setter.descriptor_address);
        final.descriptor_matches[kind]=final.descriptor_current[kind]&&setter.descriptor_before&&setter.descriptor_after&&
            final.descriptors[kind]==setter.before_descriptor&&final.descriptors[kind]==setter.after_descriptor;
        if(final.view_pair&&final.descriptor_matches[kind])final.destination[kind]=destination_snapshot(final.descriptors[kind],0);
    }
}
void values(FILE* file,Float4 const& v,bool valid){if(valid)fprintf(file,"%.9g,%.9g,%.9g,%.9g",v[0],v[1],v[2],v[3]);else fprintf(file,"unverified");}
void print_destination(FILE* file,const char* label,Destination const& d){
    fprintf(file," %s_read_mask=%u %s_active=%u %s_context=%llx %s_bank=%llx %s_data=%llx %s_address=%llx %s_highwater=%u %s_value=",
        label,d.read_mask,label,unsigned(d.active),label,static_cast<unsigned long long>(d.context),label,static_cast<unsigned long long>(d.bank),label,static_cast<unsigned long long>(d.data),label,static_cast<unsigned long long>(d.address),label,d.highwater,label);
    values(file,d.value,d.finite_value);
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"death_constant_observer enabled=1 attested=%u cpu_only=1 gpu_verified=0 setters_limit=4 finals_limit=2 duplicate_setters=%llu duplicate_finals=%llu rejected=%llu role_offsets=ratio:0xb2,transform:0xaa setter_index=0 bank_bound=16 highwater_is_capacity=0 source_lifetime=setter_call_only old_r8_read_at_quad=0\n",
        unsigned(attested),recorder.duplicate_setters,recorder.duplicate_finals,recorder.rejected);
    for(unsigned i=0;i<recorder.setters.size();++i){auto const& r=recorder.setters[i];if(!r.present)continue;
        const bool descriptor_stable=r.descriptor_before&&r.descriptor_after&&r.before_descriptor==r.after_descriptor;
        const bool path_stable=r.before_destination.readable&&r.after_destination.readable&&r.before_destination.same_path(r.after_destination);
        fprintf(file,"death_constant_setter row=%u eye=%u role=%u ticket=%llu world=%llu caller=%llx rcx=%llx descriptor=%llx owner=%llx source=%llx index=%u before_qpc=%lld after_qpc=%lld result=%llx before_view=%llx after_view=%llx before_context=%llx after_context=%llx thread=%u descriptor_before_readable=%u descriptor_after_readable=%u descriptor_before=%u,%u,%u descriptor_after=%u,%u,%u descriptor_stable=%u view_stable=%u tls_path_stable=%u source_stable=%u source_before=",
            i,r.eye,r.kind,r.ticket,r.world,static_cast<unsigned long long>(r.caller),static_cast<unsigned long long>(r.rcx),static_cast<unsigned long long>(r.descriptor_address),static_cast<unsigned long long>(r.owner),static_cast<unsigned long long>(r.source_address),r.index,r.before_qpc,r.after_qpc,static_cast<unsigned long long>(r.result),
            static_cast<unsigned long long>(r.before_view.native.current_view),static_cast<unsigned long long>(r.after_view.native.current_view),static_cast<unsigned long long>(r.before_view.context),static_cast<unsigned long long>(r.after_view.context),r.before_view.thread,unsigned(r.descriptor_before),unsigned(r.descriptor_after),
            r.before_descriptor[0],r.before_descriptor[1],r.before_descriptor[2],r.after_descriptor[0],r.after_descriptor[1],r.after_descriptor[2],unsigned(descriptor_stable),unsigned(r.before_view==r.after_view),unsigned(path_stable),unsigned(r.source_before&&r.source_after&&equal(r.before_source,r.after_source)));
        values(file,r.before_source,r.source_before);fprintf(file," source_after=");values(file,r.after_source,r.source_after);
        print_destination(file,"tls_before",r.before_destination);print_destination(file,"tls_after",r.after_destination);fprintf(file,"\n");
    }
    for(auto const& f:recorder.finals){if(!f.present)continue;
        fprintf(file,"death_constant_quad eye=%u ticket=%llu world=%llu qpc=%lld owner_pair=%u view_pair=%u current_view=%llx thread=%u rectangles_verified=%u native_source_rect=%.9g,%.9g,%.9g,%.9g native_target_rect=%d,%d,%d,%d args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u\n",
            f.eye,f.ticket,f.world,f.qpc,unsigned(f.owner_pair),unsigned(f.view_pair),static_cast<unsigned long long>(f.view.native.current_view),f.view.thread,unsigned(f.view.rectangles),f.view.source_rect[0],f.view.source_rect[1],f.view.source_rect[2],f.view.source_rect[3],
            f.view.target_rect[0],f.view.target_rect[1],f.view.target_rect[2],f.view.target_rect[3],f.args[0],f.args[1],f.args[2],f.args[3],f.args[4],f.args[5],f.args[6],f.args[7],f.dimensions[0],f.dimensions[1],f.dimensions[2],f.dimensions[3]);
        for(unsigned kind=0;kind<2;++kind){auto const& r=recorder.setters[f.eye*2+kind];auto const& d=f.destination[kind];
            const bool verified=!recorder.duplicate_setters&&!recorder.duplicate_finals&&f.owner_pair&&f.view_pair&&f.descriptor_matches[kind]&&
                f.source_valid[kind]&&r.source_before&&r.source_after&&equal(r.before_source,r.after_source)&&
                d.finite_value&&r.after_destination.finite_value&&d.same_path(r.after_destination)&&
                d.context==f.view.context&&r.before_destination.context==r.before_view.context&&r.after_destination.context==r.after_view.context;
            fprintf(file,"death_constant_compare eye=%u role=%u descriptor_current=%u descriptor_matches=%u current_descriptor=%u,%u,%u comparison_verified=%u destination_matches_submitted=%u destination_matches_postsetter=%u submitted_copy=",
                f.eye,kind,unsigned(f.descriptor_current[kind]),unsigned(f.descriptor_matches[kind]),f.descriptors[kind][0],f.descriptors[kind][1],f.descriptors[kind][2],unsigned(verified),unsigned(verified&&equal(d.value,f.source[kind])),unsigned(verified&&equal(d.value,r.after_destination.value)));
            values(file,f.source[kind],f.source_valid[kind]);print_destination(file,"tls_quad",d);fprintf(file,"\n");
        }
    }
}
#else
bool attest(FILE*){return true;}
void quad(uintptr_t,std::array<float,8> const&,std::array<unsigned,4> const&){}
void print(FILE* file){fprintf(file,"death_constant_observer enabled=0\n");}
#endif
}
