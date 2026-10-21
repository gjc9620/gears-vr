#include "death_dof_control.h"
namespace death_dof_control {
#if defined(GEARS_DEATH_DOF_CONTROL) && GEARS_DEATH_DOF_CONTROL
using namespace gears_death_dof;
using View=gears_death_constants::View;using Destination=gears_death_constants::Destination;
using Fn=uint64_t(__fastcall*)(uintptr_t,const uint16_t*,const void*,unsigned);
Fn original{};void* target{};bool attested{};Recorder recorder;
std::array<uint64_t,reject_count> rejected{};
uint64_t left_calls{},detail_duplicates{};
struct Capture {
    bool present{},modified{},descriptor_after_valid{},source_after_valid{},proof{};unsigned ordinal{},eye{};uint64_t ticket{},world{},result{},duplicates{};
    uintptr_t rcx{},descriptor_address{},source_address{},owner{};long long before_qpc{},after_qpc{};
    Descriptor descriptor{},descriptor_after{};Float4 source{},forwarded{},source_after{};
    View before_view{},after_view{};Destination before_destination{},after_destination{};
};
std::array<Capture,limit> modified{};
std::array<Capture,8> details{};
struct Rejected {unsigned reason{},eye{};uint64_t ticket{},world{};uintptr_t descriptor_address{},owner{};Descriptor descriptor{};Float4 source{};bool descriptor_read{},source_read{};};
std::array<Rejected,16> reject_rows{};unsigned reject_size{};uint64_t reject_drop{};
struct Final {bool present{},descriptor_valid{};uint64_t ticket{},world{},duplicates{};unsigned eye{};long long qpc{};View view{};Descriptor descriptor{};Destination destination{};
    std::array<float,8> args{};std::array<unsigned,4> dimensions{};};
std::array<Final,2> finals{};
template<class T> bool read(T& out,uintptr_t address,uintptr_t offset=0){
    return address&&offset<=UINTPTR_MAX-address&&sizeof(T)-1<=UINTPTR_MAX-(address+offset)&&safe_copy(&out,reinterpret_cast<void*>(address+offset),sizeof(T));
}
View view(){
    View out;out.thread=GetCurrentThreadId();out.native.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);uintptr_t root=__readgsqword(0x58),slot{};
    if(read(slot,root)&&slot&&read(out.context,slot,0x20)&&out.context&&read(out.native.current_view,out.context,0x14c0))out.native.read_mask|=1;
    if(read(out.native.renderer_views,out.native.renderer,0x68))out.native.read_mask|=2;
    if(read(out.native.renderer_count,out.native.renderer,0x70))out.native.read_mask|=4;
    out.native.eye_valid=out.native.read_mask==7&&gears_unpossessed_quad::native_eye_index(out.native.current_view,out.native.renderer_views,out.native.renderer_count,out.native.eye);
    out.rectangles=out.native.eye_valid&&read(out.source_rect,out.native.current_view,0x48)&&finite(out.source_rect)&&read(out.target_rect,out.native.current_view,0x58);return out;
}
Destination destination(){
    Destination out;out.root=__readgsqword(0x58);if(!out.root)return out;out.read_mask|=1;
    if(!read(out.slot,out.root)||!out.slot)return out;out.read_mask|=2;
    if(!read(out.active,out.slot,0x10))return out;out.read_mask|=4;if(!out.active)return out;
    if(!read(out.context,out.slot,0x20)||!out.context)return out;out.read_mask|=8;
    if(!read(out.table,out.context,0x13bc)||!out.table)return out;out.read_mask|=16;
    if(!read(out.bank,out.table)||!out.bank)return out;out.read_mask|=32; // Exact descriptor bank 0 only.
    if(!read(out.data,out.bank,0x3c)||!out.data)return out;out.read_mask|=64;
    if(!read(out.highwater,out.bank,0x44))return out;out.read_mask|=128;
    if(!gears_death_constants::destination({0,192,16},0,out.data,out.address))return out;
    out.readable=read(out.value,out.address);if(out.readable)out.read_mask|=256;out.finite_value=out.readable&&finite(out.value);return out;
}
bool other_controls(){return owned_bindings_enabled||quad_source_control_enabled()||first_view_mask_control||submit_duplicate_control||
    submit_mono_control||submit_mono_independent_state||projection_control||origin_trial_enabled;}
bool attest(FILE* file){
    const bool configuration=!self_testing&&continuous_session&&frame_link_diagnostics&&viewport_diagnostics&&!other_controls();
    attested=configuration&&final_transfer_live::verify_range(file,0x1c4b90,200,"10b2617802b5648fea992132f8b9e20d68d58940de7e7779f34298b8be240cb0")&&
        final_transfer_live::verify_range(file,0xd80360,96,"bbdd0a157bda57262b76ad682af9363b24c23e225d41859857a0e8ab7c98c85b");
    target=reinterpret_cast<void*>(base+0x1c4b90);
    fprintf(file,"death_dof_identity enabled=1 configuration=%u runtime_ranges_match=%u setter_rva=1c4b90 setter_size=200 caller_rva=d80360 caller_size=96 historical_code_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6\n",unsigned(configuration),unsigned(attested));return attested;
}
void reject(Reject reason,Gate const& g,uintptr_t descriptor_address,Descriptor const& descriptor,Float4 const& source,bool descriptor_read,bool source_read){
    ++rejected[reason];if(reject_size==reject_rows.size()){++reject_drop;return;}
    reject_rows[reject_size++]={unsigned(reason),g.eye,g.ticket,g.world,descriptor_address,g.owner,descriptor,source,descriptor_read,source_read};
}
uint64_t __fastcall hook(uintptr_t rcx,const uint16_t* descriptor,const void* data,unsigned index){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;
    Scope scope;if(!scope.world()||caller!=0xd803bf)return original(rcx,descriptor,data,index);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    Gate g{attested&&!other_controls(),recording.load(),frame_link_diagnostics,consuming_frozen_frame.enabled,consuming_frozen_frame.unpossessed,false,
        consuming_frame_ticket,consuming_frozen_frame.id,committed_world_generation.load(),consuming_frozen_frame.world_generation,caller,descriptor_owner(reinterpret_cast<uintptr_t>(descriptor)),index,0};
    if(!g.enabled||!g.recording||!g.diagnostics||!g.frozen||!g.unpossessed){++rejected[configuration];return original(rcx,descriptor,data,index);}
    Capture row;row.ticket=g.ticket;row.world=g.world;row.rcx=rcx;row.owner=g.owner;row.descriptor_address=reinterpret_cast<uintptr_t>(descriptor);row.source_address=reinterpret_cast<uintptr_t>(data);
    row.before_view=view();g.native_valid=row.before_view.native.eye_valid&&row.before_view.rectangles;g.eye=row.before_view.native.eye;row.eye=g.eye;
    const bool descriptor_read=read(row.descriptor,row.descriptor_address),source_read=read(row.source,row.source_address);
    auto reason=!descriptor_read?gears_death_dof::descriptor:!source_read?gears_death_dof::source:gate(g,row.descriptor,row.source);
    // Any repeated specific right setter is ambiguous, even if its descriptor
    // or payload now fails another gate. Forwarding it can restore X=1.
    if(g.native_valid&&g.eye==1&&recorder.find(g.world,g.ticket)){unsigned ignored{};reason=recorder.admit(g.world,g.ticket,ignored);}
    if(reason==accepted&&!recorder.bind(g.world,g.owner))reason=owner;
    if(reason==accepted){row.before_destination=destination();if(!row.before_destination.readable||row.before_destination.context!=row.before_view.context)reason=tls;}
    unsigned slot{};if(reason==accepted&&g.eye==1)reason=recorder.admit(g.world,g.ticket,slot);
    if(reason!=accepted){reject(reason,g,row.descriptor_address,row.descriptor,row.source,descriptor_read,source_read);return original(rcx,descriptor,data,index);}
    row.present=true;row.modified=g.eye==1;row.forwarded=gears_death_dof::forwarded(row.source,row.modified);row.before_qpc=now();
    if(!row.modified)++left_calls;
    auto* selected=current_unpossessed_quad_bank();unsigned detail=8;
    if(selected){row.ordinal=selected->ordinal;detail=(row.ordinal==49?3:row.ordinal<=3?row.ordinal-1:4)*2+g.eye;}
    row.result=original(rcx,descriptor,row.modified?row.forwarded.data():data,index); // Exactly once; only right R8 uses a local 16-byte copy.
    row.after_qpc=now();row.after_view=view();row.descriptor_after_valid=read(row.descriptor_after,row.descriptor_address);row.source_after_valid=read(row.source_after,row.source_address);
    row.after_destination=destination();
    row.proof=row.descriptor_after_valid&&row.descriptor_after==row.descriptor&&row.source_after_valid&&equal(row.source,row.source_after)&&
        row.before_view==row.after_view&&row.before_view.rectangles&&row.after_view.rectangles&&row.before_destination.same_path(row.after_destination)&&
        row.after_destination.context==row.after_view.context&&row.after_destination.finite_value&&equal(row.after_destination.value,row.forwarded);
    if(row.modified)modified[slot]=row;
    if(detail<details.size()){if(details[detail].present){++details[detail].duplicates;++detail_duplicates;}else details[detail]=row;}
    return row.result;
}
void quad(uintptr_t caller,std::array<float,8> const& args,std::array<unsigned,4> const& dimensions){
    if(caller!=0xd80a7b||!attested||!recording.load()||!consuming_frozen_frame.enabled||!consuming_frozen_frame.unpossessed)return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* selected=current_unpossessed_quad_bank();
    if(!selected||selected->ordinal!=49||consuming_frozen_frame.world_generation!=selected->ticket.world||consuming_frozen_frame.id!=selected->ticket.id)return;
    const auto current=view();if(!current.native.eye_valid)return;auto& row=finals[current.native.eye];if(row.present){++row.duplicates;return;}
    row.present=true;row.ticket=selected->ticket.id;row.world=selected->ticket.world;row.eye=current.native.eye;row.view=current;row.qpc=now();row.args=args;row.dimensions=dimensions;
    const auto& setter=details[6+row.eye];if(!setter.present||setter.ticket!=row.ticket||setter.world!=row.world)return;
    row.descriptor_valid=read(row.descriptor,setter.descriptor_address);row.destination=destination(); // Never reread the caller's old R8 temporary.
}
void values(FILE* file,Float4 const& value,bool valid=true){if(valid)fprintf(file,"%.9g,%.9g,%.9g,%.9g",value[0],value[1],value[2],value[3]);else fprintf(file,"unverified");}
void path(FILE* file,const char* label,Destination const& d){
    fprintf(file," %s_mask=%u %s_active=%u %s_context=%llx %s_table=%llx %s_bank=%llx %s_data=%llx %s_address=%llx %s_highwater=%u %s_value=",label,d.read_mask,label,unsigned(d.active),label,static_cast<unsigned long long>(d.context),label,static_cast<unsigned long long>(d.table),label,static_cast<unsigned long long>(d.bank),label,static_cast<unsigned long long>(d.data),label,static_cast<unsigned long long>(d.address),label,d.highwater,label);values(file,d.value,d.finite_value);
}
void print_capture(FILE* file,const char* label,unsigned index,Capture const& r){
    const auto* ticket=recorder.find(r.world,r.ticket);const auto duplicates=r.duplicates+(ticket?ticket->duplicates:0);
    fprintf(file,"death_dof_%s row=%u ticket=%llu world=%llu ordinal=%u eye=%u modified=%u proof=%u duplicates=%llu rcx=%llx descriptor_address=%llx source_address=%llx owner=%llx descriptor=%u,%u,%u after_descriptor=%u,%u,%u descriptor_after_valid=%u source_after_valid=%u before_qpc=%lld after_qpc=%lld result=%llx native_view=%llx native_views=%llx native_count=%d context=%llx thread=%u native_source_rect=%.9g,%.9g,%.9g,%.9g native_target_rect=%d,%d,%d,%d source=",
        label,index,r.ticket,r.world,r.ordinal,r.eye,unsigned(r.modified),unsigned(r.proof&&!duplicates),duplicates,static_cast<unsigned long long>(r.rcx),static_cast<unsigned long long>(r.descriptor_address),static_cast<unsigned long long>(r.source_address),static_cast<unsigned long long>(r.owner),r.descriptor[0],r.descriptor[1],r.descriptor[2],r.descriptor_after[0],r.descriptor_after[1],r.descriptor_after[2],unsigned(r.descriptor_after_valid),unsigned(r.source_after_valid),r.before_qpc,r.after_qpc,static_cast<unsigned long long>(r.result),static_cast<unsigned long long>(r.before_view.native.current_view),static_cast<unsigned long long>(r.before_view.native.renderer_views),r.before_view.native.renderer_count,static_cast<unsigned long long>(r.before_view.context),r.before_view.thread,r.before_view.source_rect[0],r.before_view.source_rect[1],r.before_view.source_rect[2],r.before_view.source_rect[3],r.before_view.target_rect[0],r.before_view.target_rect[1],r.before_view.target_rect[2],r.before_view.target_rect[3]);
    values(file,r.source);fprintf(file," forwarded=");values(file,r.forwarded);fprintf(file," source_after=");values(file,r.source_after,r.source_after_valid);path(file,"tls_before",r.before_destination);path(file,"tls_after",r.after_destination);fprintf(file,"\n");
}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    fprintf(file,"death_dof_control enabled=1 attested=%u experimental_only=1 product_fix=0 gpu_verified=0 changed_component=right_PS_cb0_12_x unique_modified=%u modified_limit=256 bound_world=%llu bound_owner=%llx left_forwarded=%llu duplicates=%llu capacity_drop=%llu detail_duplicates=%llu reject_rows=%u reject_drop=%llu original_source_written=0 other_components_changed=0 gpu_commands_added=0 ps_table_offset=13bc highwater_is_capacity=0 old_r8_read_at_quad=0\n",unsigned(attested),recorder.count,recorder.bound_world,static_cast<unsigned long long>(recorder.bound_owner),left_calls,recorder.duplicates,recorder.overflow,detail_duplicates,reject_size,reject_drop);
    for(unsigned i=1;i<reject_count;++i)fprintf(file,"death_dof_rejected reason=%u count=%llu\n",i,rejected[i]);
    for(unsigned i=0;i<reject_size;++i){auto const& r=reject_rows[i];fprintf(file,"death_dof_reject row=%u reason=%u ticket=%llu world=%llu eye=%u descriptor_address=%llx owner=%llx descriptor_read=%u descriptor=%u,%u,%u source_read=%u source=",i,r.reason,r.ticket,r.world,r.eye,static_cast<unsigned long long>(r.descriptor_address),static_cast<unsigned long long>(r.owner),unsigned(r.descriptor_read),r.descriptor[0],r.descriptor[1],r.descriptor[2],unsigned(r.source_read));values(file,r.source,r.source_read);fprintf(file,"\n");}
    for(unsigned i=0;i<recorder.count;++i)print_capture(file,"modified",i,modified[i]);
    for(unsigned i=0;i<details.size();++i)if(details[i].present)print_capture(file,"detail",i,details[i]);
    for(auto const& f:finals)if(f.present){const auto& s=details[6+f.eye];const auto* ticket=recorder.find(f.world,f.ticket);
        const bool verified=s.present&&s.proof&&!s.duplicates&&!f.duplicates&&(!ticket||!ticket->duplicates)&&s.ticket==f.ticket&&s.world==f.world&&s.after_qpc<=f.qpc&&
            f.descriptor_valid&&f.descriptor==s.descriptor&&f.view.rectangles&&s.after_view==f.view&&f.destination.finite_value&&
            f.destination.context==f.view.context&&f.destination.same_path(s.after_destination)&&equal(f.destination.value,s.forwarded);
        fprintf(file,"death_dof_prequad ticket=%llu world=%llu eye=%u qpc=%lld setter_modified=%u verified=%u duplicates=%llu native_view=%llx descriptor_read=%u descriptor=%u,%u,%u args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u committed_payload=",f.ticket,f.world,f.eye,f.qpc,unsigned(s.modified),unsigned(verified),f.duplicates,static_cast<unsigned long long>(f.view.native.current_view),unsigned(f.descriptor_valid),f.descriptor[0],f.descriptor[1],f.descriptor[2],f.args[0],f.args[1],f.args[2],f.args[3],f.args[4],f.args[5],f.args[6],f.args[7],f.dimensions[0],f.dimensions[1],f.dimensions[2],f.dimensions[3]);values(file,s.forwarded,s.present);path(file,"tls_prequad",f.destination);fprintf(file,"\n");}
}
#else
bool attest(FILE*){return true;}
void quad(uintptr_t,std::array<float,8> const&,std::array<unsigned,4> const&){}
void print(FILE* file){fprintf(file,"death_dof_control enabled=0\n");}
#endif
}
