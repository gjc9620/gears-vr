#include "dlss_route_observer.h"
#include "dlss_output_control_live.h"
namespace dlss_route_observer {
#if defined(GEARS_DLSS_ROUTE_OBSERVER) && GEARS_DLSS_ROUTE_OBSERVER
#if defined(GEARS_DEATH_CONSTANT_OBSERVER) && GEARS_DEATH_CONSTANT_OBSERVER
#error DLSS route and death constant observers detour the same setter
#endif
using namespace gears_dlss_route;
using OutputFn=uint32_t(__fastcall*)(void*,uintptr_t,void*);
using SetterFn=uint64_t(__fastcall*)(uintptr_t,const uint16_t*,const void*,unsigned);
OutputFn original_output{};SetterFn original_setter{};void* output_target{};void* setter_target{};
bool attested{},printed{};Recorder recorder;thread_local Context* current{};
// Only allocation/counters are shared during capture. A reserved array element
// never moves, is never reused in-session, and is written only by its TLS owner.
// enable resets before installation. Cleanup may time out with owners in flight;
// print therefore checks recording=false AND active==0 before reading any row.
// Each owner writes before its Scope's atomic active decrement. With recording
// closed, subsequent bypass entries cannot write rows; observing the completed
// active drain publishes owner writes. frame_hook_mutex alone does not do so.
template<class T> bool read(T& out,uintptr_t address,uintptr_t offset=0){
    return address&&offset<=UINTPTR_MAX-address&&sizeof(T)-1<=UINTPTR_MAX-(address+offset)&&
        safe_copy(&out,reinterpret_cast<void*>(address+offset),sizeof(T));
}
View view(uintptr_t address){
    View out;out.address=address;out.native.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
    uintptr_t slot{};const auto root=__readgsqword(0x58);
    if(read(slot,root)&&slot&&read(out.context,slot,0x20)&&out.context&&read(out.native.current_view,out.context,0x14c0))out.native.read_mask|=1;
    if(read(out.native.renderer_views,out.native.renderer,0x68))out.native.read_mask|=2;
    if(read(out.native.renderer_count,out.native.renderer,0x70))out.native.read_mask|=4;
    out.native.eye_valid=out.native.read_mask==7&&gears_unpossessed_quad::native_eye_index(out.native.current_view,out.native.renderer_views,out.native.renderer_count,out.native.eye);
    out.eye_valid=(out.native.read_mask&6)==6&&gears_unpossessed_quad::native_eye_index(address,out.native.renderer_views,out.native.renderer_count,out.eye);
    out.same_current=(out.native.read_mask&1)&&address==out.native.current_view;
    if(read(out.source,address,0x48))out.mask|=1;
    if(read(out.target,address,0x58))out.mask|=2;
    if(read(out.sampling,address,0x3d0))out.mask|=4;
    if(read(out.family,address)&&out.family){out.mask|=8;if(read(out.marker,out.family,0x4c))out.mask|=16;}
    return out;
}
Destination destination_snapshot(Descriptor const& descriptor,unsigned index){
    // Same proven uint16 descriptor / bank layout as death_constant_observer.
    // Missing reads are observations, never reasons to change the engine call.
    Destination out;if(index||descriptor[0]>=16||descriptor[2]<16)return out;
    out.root=__readgsqword(0x58);if(!out.root)return out;out.read_mask|=1;
    if(!read(out.slot,out.root)||!out.slot)return out;out.read_mask|=2;
    if(!read(out.active,out.slot,0x10))return out;out.read_mask|=4;if(!out.active)return out;
    if(!read(out.context,out.slot,0x20)||!out.context)return out;out.read_mask|=8;
    if(!read(out.table,out.context,0x13ac)||!out.table)return out;out.read_mask|=16;
    if(!read(out.bank,out.table,uintptr_t(descriptor[0])*8)||!out.bank)return out;out.read_mask|=32;
    if(!read(out.data,out.bank,0x3c)||!out.data)return out;out.read_mask|=64;
    if(!read(out.highwater,out.bank,0x44))return out;out.read_mask|=128;
    if(!gears_death_constants::destination(descriptor,index,out.data,out.address))return out;
    out.readable=read(out.value,out.address);if(out.readable)out.read_mask|=256;
    out.finite_value=out.readable&&gears_death_constants::finite(out.value);return out;
}
bool eligible(bool world){return gears_dlss_route::eligible(world,attested,recording.load(),frame_link_diagnostics,consuming_renderer!=nullptr,
    consuming_frozen_frame.enabled,consuming_frame_ticket,consuming_frozen_frame.id,committed_world_generation.load(),consuming_frozen_frame.world_generation);}
bool matching(Row const& row){return eligible(true)&&row.ticket==consuming_frame_ticket&&row.world==consuming_frozen_frame.world_generation&&
    row.renderer==reinterpret_cast<uintptr_t>(consuming_renderer)&&row.thread==GetCurrentThreadId();}
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER) && GEARS_DLSS_INPUT_BIND_OBSERVER
using BindFn=uint64_t(__fastcall*)(const uint16_t*,void*,void*,uint32_t,int32_t);
BindFn original_bind{};void* bind_target{};bool input_attested{};
constexpr unsigned input_slot_limit=32; // Observation bound, not a claimed native capacity.
InputSample input_sample(InputBind const& b,bool fixed_slot=false,uintptr_t proved_public_global=0){
    InputSample out;out.view=view(b.view_address);
    if(read(out.descriptor,b.descriptor_address))out.mask|=1;
    if(read(out.shader_handle,b.shader_owner,0x50))out.mask|=2;
    if(read(out.owner_head,b.shader_owner))out.mask|=4;
    if(read(out.selector,b.view_address,0x3e8))out.mask|=8;
    if(read(out.highwater,out.view.context,0x65d))out.mask|=16;
    const bool known=fixed_slot?b.tracked_slot_known:(out.mask&1)!=0;
    out.slot=known?(fixed_slot?b.tracked_slot:unsigned(out.descriptor[0])):UINT_MAX;
    if(known&&out.slot<input_slot_limit){out.mask|=512;
        if(read(out.resource.object,out.view.context,0x768+uintptr_t(out.slot)*8)){out.mask|=32;
            if(out.resource.object){ResourceIdentity identity{};query_bound_resource(out.resource.object,identity);auto const& d=identity.desc;
                out.resource.queried=true;out.resource.native=identity.native;out.resource.canonical=identity.canonical;out.resource.result=identity.result;
                out.resource.width=d.Width;out.resource.height=d.Height;out.resource.dimension=unsigned(d.Dimension);out.resource.format=unsigned(d.Format);
                out.resource.depth=unsigned(d.DepthOrArraySize);out.resource.mips=unsigned(d.MipLevels);out.resource.samples=d.SampleDesc.Count;}}}
    // Only the exact selection=0/alternate=0 branch is named here. The public
    // pointer is loaded from the proved global cell, never inferred by size.
    if(!b.selection&&!b.alternate&&(out.mask&8)){
        // Optional Output-tail re-read of the already proved bank0 source.
        // The actual selector above is never replaced. No other supplied cell
        // is accepted, and ordinary observer/checkpoint reads stay unchanged.
        if(proved_public_global&&proved_public_global!=base+0x1c3b410)return out;
        out.public_global=proved_public_global?proved_public_global:base+(out.selector?0x1c3b488:0x1c3b410);
        if(read(out.public_pointer,out.public_global)){out.mask|=64;
            if(out.public_pointer>=0x13c){out.public_wrapper=out.public_pointer-0x13c;
                if(read(out.public_index,out.public_wrapper,0x138)){out.mask|=128;
                    if(out.public_index<input_slot_limit&&read(out.public_object,out.public_wrapper,0x38+uintptr_t(out.public_index)*8))out.mask|=256;}}}}
    return out;
}
struct InputFinish {
    InputBind& bind;int exceptions=std::uncaught_exceptions();
    ~InputFinish(){bind.after=input_sample(bind);bind.exit=now();bind.exited=true;bind.unwinding=std::uncaught_exceptions()>exceptions;
        if(!bind.tracked_slot_known&&(bind.after.mask&1)&&bind.after.slot<input_slot_limit){bind.tracked_slot=bind.after.slot;bind.tracked_slot_known=true;}}
};
uint64_t __fastcall hook_bind(const uint16_t* descriptor,void* input_view,void* shader_owner,uint32_t selection,int32_t alternate){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;Scope scope;auto* row=current?current->row:nullptr;
    dlss_output_control::BindScope control(scope.world(),caller,descriptor,input_view,shader_owner,selection,alternate);
    if(!scope.world()||!input_attested||caller!=0xd831c8||!row||!matching(*row))return original_bind(descriptor,input_view,shader_owner,selection,alternate);
    auto& b=row->input;if(b.present){++b.duplicates;return original_bind(descriptor,input_view,shader_owner,selection,alternate);}
    b.present=true;b.caller=caller;b.descriptor_address=reinterpret_cast<uintptr_t>(descriptor);b.view_address=reinterpret_cast<uintptr_t>(input_view);
    b.shader_owner=reinterpret_cast<uintptr_t>(shader_owner);b.selection=selection;b.alternate=alternate;b.entry=now();
    b.owner_descriptor=b.shader_owner&&b.shader_owner<=UINTPTR_MAX-0xb4&&b.descriptor_address==b.shader_owner+0xb4;
    b.output_view_matches=b.view_address==current->view;b.before=input_sample(b);
    if((b.before.mask&1)&&b.before.slot<input_slot_limit){b.tracked_slot=b.before.slot;b.tracked_slot_known=true;}
    InputFinish finish{b};b.result=original_bind(descriptor,input_view,shader_owner,selection,alternate);return b.result;
}
void input_quad(Row& row,uintptr_t caller){auto& b=row.input;if(!b.present||b.quad.present)return;
    b.quad.present=true;b.quad.caller=caller;b.quad.qpc=now();b.quad.sample=input_sample(b,true);}
void draw(uintptr_t raw_list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances){
    auto* row=current?current->row:nullptr;
    if(!input_attested||!row||!matching(*row)||!row->input.present||row->input.draw.present||!late_quad_candidate.active||
       (late_quad_candidate.caller!=0xd83b1c&&late_quad_candidate.caller!=0xd83cbf))return;
    auto& b=row->input;auto& d=b.draw;d.present=true;d.caller=caller;d.raw_list=raw_list;d.kind=kind;d.elements=elements;d.instances=instances;
    d.qpc=now();d.sample=input_sample(b,true);
    uintptr_t owner{},table{},method{};
    if(read(owner,d.sample.view.context,0x127c)&&owner&&read(d.context_list,owner,0x14)&&d.context_list&&read(table,d.context_list)&&table&&
       read(method,table,12*8)&&system_resource_methods(d.context_list)&&viewport_targets[3]&&method==reinterpret_cast<uintptr_t>(viewport_targets[3])){
        d.canonical_list=canonical_identity(reinterpret_cast<IUnknown*>(d.context_list));const auto ref=frame_link.current(d.canonical_list);d.generation=ref.generation;
        d.list_matches=raw_list==d.context_list&&d.canonical_list&&ref.identity==d.canonical_list&&ref.generation;}
}
#else
void input_quad(Row&,uintptr_t){}
void draw(uintptr_t,uintptr_t,unsigned,unsigned,unsigned){}
#endif
struct OutputFinish {
    Context& context;int exceptions=std::uncaught_exceptions();
    ~OutputFinish(){if(auto* r=context.row){r->after=view(context.view);r->exit=now();r->exited=true;r->unwinding=std::uncaught_exceptions()>exceptions;}}
};
uint32_t __fastcall hook_output(void* pass,uintptr_t unused,void* input_view){
    Scope scope;
    Context context{reinterpret_cast<uintptr_t>(pass),reinterpret_cast<uintptr_t>(input_view),nullptr};Binding binding(current,context);
    dlss_output_control::OutputScope output_control(scope.world(),context.pass,context.view);
    if(!eligible(scope.world()))return original_output(pass,unused,input_view);
    bool candidate=false;
    {
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        if(binding.nested)++recorder.nested;
        else if(recorder.size==recorder.rows.size())++recorder.limited;
        else candidate=true;
    }
    if(!candidate)return original_output(pass,unused,input_view);
    const auto before=view(context.view);uint32_t flags{};const bool flags_read=read(flags,context.pass,8);
    // Skip incomplete startup records without consuming one of the eight rows.
    {
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        context.row=recorder.reserve(flags_read&&(before.mask&7)==7&&before.native.eye_valid);
    }
    if(!context.row)return original_output(pass,unused,input_view);
    auto& row=*context.row;row.pass=context.pass;row.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);
    row.caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;row.ticket=consuming_frame_ticket;row.world=consuming_frozen_frame.world_generation;
    row.thread=GetCurrentThreadId();row.eye=before.native.eye;row.entry=now();row.before=before;row.flags=flags;row.flags_read=flags_read;
    OutputFinish finish{context};row.result=original_output(pass,unused,input_view);return row.result;
}
struct SetterFinish {
    Setter& row;int exceptions=std::uncaught_exceptions();
    ~SetterFinish(){row.exit=now();row.exited=true;row.unwinding=std::uncaught_exceptions()>exceptions;
        if(row.mask&1)row.after=destination_snapshot(row.descriptor,row.index);}
};
uint64_t __fastcall hook_setter(uintptr_t rcx,const uint16_t* descriptor,const void* payload,unsigned index){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-base;
    Scope scope;dlss_output_control::SetterScope output_control(scope.world(),caller,descriptor,index);auto* row=current?current->row:nullptr;
    if(!scope.world()||!row||setter_role(caller)>1||index||!matching(*row))return original_setter(rcx,descriptor,payload,index);
    // This row belongs exclusively to the current thread's Output invocation.
    // Do not hold the global frame lock across the unchanged native setter.
    if(row->setter_count==row->setters.size()){++row->setter_overflow;return original_setter(rcx,descriptor,payload,index);}
    auto& setter=row->setters[row->setter_count++];setter.caller=caller;setter.rcx=rcx;setter.index=index;setter.entry=now();
    setter.descriptor_address=reinterpret_cast<uintptr_t>(descriptor);setter.source_address=reinterpret_cast<uintptr_t>(payload);
    setter.owner=setter.descriptor_address>=0xaa?setter.descriptor_address-0xaa:0;
    if(read(setter.descriptor,setter.descriptor_address)){setter.mask|=1;setter.before=destination_snapshot(setter.descriptor,index);}
    if(read(setter.payload,setter.source_address))setter.mask|=2;
    // Keep the submitted float4 copy; R8 storage may be reused after return.
    SetterFinish finish{setter};setter.result=original_setter(rcx,descriptor,payload,index);return setter.result;
}
void quad(uintptr_t caller,std::array<float,8> const& args,std::array<unsigned,4> const& dimensions){
    auto* row=current?current->row:nullptr;
    if(!row||!matching(*row)||(caller!=0xd83b1c&&caller!=0xd83cbf))return;
    input_quad(*row,caller);
    if(row->quad_count==row->quads.size()){++row->quad_overflow;return;}
    auto& quad=row->quads[row->quad_count++];quad.caller=caller;quad.qpc=now();quad.args=args;quad.dimensions=dimensions;quad.view=view(current->view);
    for(unsigned i=0;i<row->setter_count;++i){const auto& setter=row->setters[i];Descriptor current_descriptor{};
        quad.descriptor_matches[i]=(setter.mask&1)&&read(current_descriptor,setter.descriptor_address)&&current_descriptor==setter.descriptor;
        if(quad.descriptor_matches[i])quad.consumed[i]=destination_snapshot(current_descriptor,setter.index);}
}
bool verify_chunk(FILE* file,uintptr_t rva,ULONG size,const char* expected){
    // Authenticate complete saved chunks before either hook is installed. The
    // largest chunk (0xc78) exceeds final_transfer_live's small local buffer.
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,0x1000> bytes{};
    bool ok=size&&size<=bytes.size()&&BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+rva),size)&&BCryptHashData(hash,bytes.data(),size,0)>=0;
    unsigned char digest[32]{};char hex[65]{};if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    ok=ok&&!strcmp(hex,expected);
    fprintf(file,"dlss_route_identity rva=%llx size=%lu sha256=%s match=%u module_base=%llx source=historical_runtime_artifact1 snapshot_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6\n",
        static_cast<unsigned long long>(rva),size,hex,unsigned(ok),static_cast<unsigned long long>(base));return ok;
}
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER) && GEARS_DLSS_INPUT_BIND_OBSERVER
bool verify_input_chunks(FILE* file,const char* expected){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};std::array<unsigned char,0x1cc+0x2b> bytes{};
    bool ok=BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=safe_copy(bytes.data(),reinterpret_cast<void*>(base+0x1665dc),0x1cc)&&
        safe_copy(bytes.data()+0x1cc,reinterpret_cast<void*>(base+0x8b492c),0x2b)&&BCryptHashData(hash,bytes.data(),ULONG(bytes.size()),0)>=0;
    unsigned char digest[32]{};char hex[65]{};if(ok)ok=BCryptFinishHash(hash,digest,32,0)>=0;
    for(unsigned i=0;i<32;++i)sprintf_s(hex+2*i,3,"%02x",digest[i]);
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    const bool matches=ok&&!strcmp(hex,expected);
    fprintf(file,"dlss_input_bind_identity enabled=1 match=%u chunks=1665dc:460,8b492c:43 combined_size=503 sha256=%s module_base=%llx source=historical_runtime_artifact1 snapshot_sha256=911ab9bcb24cc078420400369a78ffd3f747d915e42c4935a5c82c2dcecc6ae6\n",
        unsigned(matches),hex,static_cast<unsigned long long>(base));return matches;
}
#endif
bool enable_input_bind(FILE* file){
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER) && GEARS_DLSS_INPUT_BIND_OBSERVER
    bind_target=reinterpret_cast<void*>(base+0x1665dc);
    input_attested=verify_input_chunks(file,"566929fd4f84a21529de0ecb749d2dc75a1bf2ab3cc04876a65f65517222a339");return input_attested;
#else
    (void)file;return true;
#endif
}
bool enable(FILE* file){
    attested=false;recorder={};printed=false;
    output_target=reinterpret_cast<void*>(base+0xd82f58);setter_target=reinterpret_cast<void*>(base+0x1760b4);
    if(self_testing||!continuous_session||!frame_link_diagnostics||!viewport_diagnostics)return false;
    const bool output=verify_chunk(file,0xd82f58,0xc78,"1efb10541bf56d87b31aae4f452da0d281afd552da16b4bfd16d653f2cd425ea");
    const bool continuation=verify_chunk(file,0xd83bd0,0x1e9,"e051a2d325a4d5ebc12be670d63cd8eae1eead437f33dfa39946de852e61e724");
    const bool setter=verify_chunk(file,0x1760b4,0xcd,"d7d20d3f5c510685b8b3ffb12d12d51ae583d4070e00cf11a39655c7e086c45b");
    attested=output&&continuation&&setter;return attested&&enable_input_bind(file)&&dlss_output_control::enable(file,attested);
}
void values(FILE* file,Float4 const& v){fprintf(file,"%.9g,%.9g,%.9g,%.9g",v[0],v[1],v[2],v[3]);}
void print_view(FILE* file,const char* label,View const& v){
    fprintf(file," %s_view=%llx %s_context=%llx %s_current_view=%llx %s_same_current=%u %s_native_mask=%u %s_eye_valid=%u %s_eye=%u %s_tls_eye_valid=%u %s_tls_eye=%u %s_renderer_views=%llx %s_renderer_count=%d %s_mask=%u %s_family=%llx %s_family_marker=%u %s_source=",
        label,static_cast<unsigned long long>(v.address),label,static_cast<unsigned long long>(v.context),label,static_cast<unsigned long long>(v.native.current_view),label,unsigned(v.same_current),label,v.native.read_mask,label,unsigned(v.eye_valid),label,v.eye,label,unsigned(v.native.eye_valid),label,v.native.eye,label,static_cast<unsigned long long>(v.native.renderer_views),label,v.native.renderer_count,label,v.mask,label,static_cast<unsigned long long>(v.family),label,v.marker,label);
    values(file,v.source);fprintf(file," %s_target=%d,%d,%d,%d %s_sampling=",label,v.target[0],v.target[1],v.target[2],v.target[3],label);values(file,v.sampling);
}
void print_destination(FILE* file,const char* label,Destination const& d){
    fprintf(file," %s_mask=%u %s_active=%u %s_context=%llx %s_table=%llx %s_bank=%llx %s_data=%llx %s_address=%llx %s_highwater=%u %s_readable=%u %s_value=",
        label,d.read_mask,label,unsigned(d.active),label,static_cast<unsigned long long>(d.context),label,static_cast<unsigned long long>(d.table),label,static_cast<unsigned long long>(d.bank),label,static_cast<unsigned long long>(d.data),label,static_cast<unsigned long long>(d.address),label,d.highwater,label,unsigned(d.readable),label);values(file,d.value);
}
void print_input(FILE* file,unsigned row,InputBind const& b){
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER) && GEARS_DLSS_INPUT_BIND_OBSERVER
    if(!b.present)return;
    fprintf(file,"dlss_input_bind row=%u caller=%llx descriptor_address=%llx view=%llx shader_owner=%llx owner_plus_b4=%u output_view_matches=%u selection=%u alternate=%d entry=%lld exit=%lld exited=%u unwinding=%u result=%llx tracked_slot_known=%u tracked_slot=%u duplicates=%u shader_sampling_verified=0\n",
        row,static_cast<unsigned long long>(b.caller),static_cast<unsigned long long>(b.descriptor_address),static_cast<unsigned long long>(b.view_address),static_cast<unsigned long long>(b.shader_owner),unsigned(b.owner_descriptor),unsigned(b.output_view_matches),b.selection,b.alternate,b.entry,b.exit,unsigned(b.exited),unsigned(b.unwinding),static_cast<unsigned long long>(b.result),unsigned(b.tracked_slot_known),b.tracked_slot,b.duplicates);
    auto sample=[&](const char* stage,InputSample const& s,bool checkpoint=false){auto const& r=s.resource;
        const bool comparable=checkpoint&&(s.mask&32)&&(b.after.mask&32)&&s.slot==b.after.slot&&s.view.context&&s.view.context==b.after.view.context;
        fprintf(file,"dlss_input_sample row=%u stage=%s mask=%u descriptor=%u,%u,%u descriptor_active=%u slot=%u context=%llx current_view=%llx view_same_current=%u native_mask=%u highwater=%u selector=%u shader_handle=%llx public_global=%llx public_pointer=%llx public_wrapper=%llx public_index=%u public_object=%llx selected_public_object_matches_slot=%u object=%llx object_comparison_readable=%u object_overwritten_since_bind=%u queried=%u native=%llx canonical=%llx result=%ld dimensions=%llu,%u dimension=%u format=%u depth=%u mips=%u samples=%u\n",
            row,stage,s.mask,s.descriptor[0],s.descriptor[1],s.descriptor[2],unsigned((s.mask&1)&&s.descriptor[1]>0),s.slot,static_cast<unsigned long long>(s.view.context),static_cast<unsigned long long>(s.view.native.current_view),unsigned(s.view.same_current),s.view.native.read_mask,unsigned(s.highwater),s.selector,static_cast<unsigned long long>(s.shader_handle),
            static_cast<unsigned long long>(s.public_global),static_cast<unsigned long long>(s.public_pointer),static_cast<unsigned long long>(s.public_wrapper),s.public_index,static_cast<unsigned long long>(s.public_object),unsigned((s.mask&256)&&(s.mask&32)&&s.public_object&&s.public_object==r.object),
            static_cast<unsigned long long>(r.object),unsigned(comparable),unsigned(comparable&&r.object!=b.after.resource.object),unsigned(r.queried),static_cast<unsigned long long>(r.native),static_cast<unsigned long long>(r.canonical),r.result,static_cast<unsigned long long>(r.width),r.height,r.dimension,r.format,r.depth,r.mips,r.samples);};
    sample("before",b.before);sample("after",b.after);
    fprintf(file,"dlss_input_owner_head row=%u before_readable=%u after_readable=%u before=",row,unsigned((b.before.mask&4)!=0),unsigned((b.after.mask&4)!=0));
    for(auto byte:b.before.owner_head)fprintf(file,"%02x",unsigned(byte));fprintf(file," after=");for(auto byte:b.after.owner_head)fprintf(file,"%02x",unsigned(byte));fprintf(file,"\n");
    auto checkpoint=[&](const char* stage,InputCheckpoint const& c){if(!c.present)return;
        fprintf(file,"dlss_input_checkpoint row=%u stage=%s caller=%llx qpc=%lld raw_list=%llx context_list=%llx canonical_list=%llx generation=%llu list_matches=%u kind=%u elements=%u instances=%u descriptor_unchanged=%u context_unchanged=%u\n",
            row,stage,static_cast<unsigned long long>(c.caller),c.qpc,static_cast<unsigned long long>(c.raw_list),static_cast<unsigned long long>(c.context_list),static_cast<unsigned long long>(c.canonical_list),c.generation,unsigned(c.list_matches),c.kind,c.elements,c.instances,
            unsigned((c.sample.mask&1)&&(b.before.mask&1)&&c.sample.descriptor==b.before.descriptor),unsigned(c.sample.view.context&&c.sample.view.context==b.after.view.context));sample(stage,c.sample,true);};
    checkpoint("quad",b.quad);checkpoint("draw",b.draw);
#else
    (void)file;(void)row;(void)b;
#endif
}
void print(FILE* file){
    // Cleanup can retain resources on timeout. Never inspect an unfinished row.
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(printed)return;printed=true;
    const bool quiescent=!recording.load()&&active.load()==0;
    fprintf(file,"dlss_route_observer enabled=1 attested=%u cpu_only=1 gpu_verified=0 semantic_verified=0 mutation=0 rows=%u rows_unavailable=%u limit=8 missing=%llu nested=%llu limited=%llu highwater_is_capacity=0 old_r8_read_at_quad=0\n",
        unsigned(attested),recorder.size,unsigned(!quiescent),recorder.missing,recorder.nested,recorder.limited);
#if defined(GEARS_DLSS_INPUT_BIND_OBSERVER) && GEARS_DLSS_INPUT_BIND_OBSERVER
    fprintf(file,"dlss_input_bind_observer enabled=1 attested=%u output_limit=8 binds_per_output=1 observed_slot_limit=32 highwater_is_capacity=0 shader_sampling_verified=0 slots1_control_unchanged=1\n",unsigned(input_attested));
#endif
    if(!quiescent)return; // Counters above are locked; all row fields stay unread.
    for(unsigned i=0;i<recorder.size;++i){const auto& r=recorder.rows[i];
        fprintf(file,"dlss_route_output row=%u ticket=%llu world=%llu eye=%u thread=%u caller=%llx pass=%llx renderer=%llx flags_read=%u flags=%u entry=%lld exit=%lld exited=%u unwinding=%u result=%u setters=%u quads=%u setter_overflow=%u quad_overflow=%u",
            i,r.ticket,r.world,r.eye,r.thread,static_cast<unsigned long long>(r.caller),static_cast<unsigned long long>(r.pass),static_cast<unsigned long long>(r.renderer),unsigned(r.flags_read),r.flags,r.entry,r.exit,unsigned(r.exited),unsigned(r.unwinding),r.result,r.setter_count,r.quad_count,r.setter_overflow,r.quad_overflow);
        print_view(file,"entry",r.before);print_view(file,"exit",r.after);fprintf(file,"\n");
        print_input(file,i,r.input);
        for(unsigned j=0;j<r.setter_count;++j){const auto& s=r.setters[j];
            fprintf(file,"dlss_route_setter row=%u setter=%u caller=%llx rcx=%llx descriptor_address=%llx source_address=%llx owner=%llx index=%u mask=%u descriptor=%u,%u,%u entry=%lld exit=%lld exited=%u unwinding=%u result=%llx payload=",
                i,j,static_cast<unsigned long long>(s.caller),static_cast<unsigned long long>(s.rcx),static_cast<unsigned long long>(s.descriptor_address),static_cast<unsigned long long>(s.source_address),static_cast<unsigned long long>(s.owner),s.index,s.mask,s.descriptor[0],s.descriptor[1],s.descriptor[2],s.entry,s.exit,unsigned(s.exited),unsigned(s.unwinding),static_cast<unsigned long long>(s.result));
            values(file,s.payload);print_destination(file,"before",s.before);print_destination(file,"after",s.after);fprintf(file,"\n");}
        for(unsigned j=0;j<r.quad_count;++j){const auto& q=r.quads[j];
            const bool exit_same=q.view.mask==r.after.mask&&q.view.address==r.after.address&&q.view.context==r.after.context&&q.view.native==r.after.native&&
                gears_death_constants::equal(q.view.source,r.after.source)&&q.view.target==r.after.target&&gears_death_constants::equal(q.view.sampling,r.after.sampling);
            fprintf(file,"dlss_route_quad row=%u quad=%u caller=%llx qpc=%lld args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u exit_view_same=%u",
                i,j,static_cast<unsigned long long>(q.caller),q.qpc,q.args[0],q.args[1],q.args[2],q.args[3],q.args[4],q.args[5],q.args[6],q.args[7],q.dimensions[0],q.dimensions[1],q.dimensions[2],q.dimensions[3],unsigned(exit_same));
            print_view(file,"quad",q.view);for(unsigned k=0;k<r.setter_count;++k){fprintf(file," setter%u_descriptor_matches=%u",k,unsigned(q.descriptor_matches[k]));print_destination(file,k?"consumed1":"consumed0",q.consumed[k]);}fprintf(file,"\n");}
    }
}
#else
bool enable(FILE*){return true;}
void quad(uintptr_t,std::array<float,8> const&,std::array<unsigned,4> const&){}
void draw(uintptr_t,uintptr_t,unsigned,unsigned,unsigned){}
void print(FILE*){}
#endif
}
