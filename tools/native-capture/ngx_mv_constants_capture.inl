// One selected compute CB0 upload/copy witness. CPU bytes only: the native
// mapped-ring address is read, never converted into a COM interface or mapped
// again. No shader constants, resource states or commands are changed.
#include "ngx_mv_constant_snapshot.h"
namespace ngx_mv_constants_capture {
#if GEARS_NGX_MV_TARGET_CAPTURE
using namespace gears_ngx_mv_constants;
struct Upload {
    bool valid{};uint64_t world{},ticket{},serial{},gpu{};uintptr_t renderer{},context{},bank{},meta{},data{},mapped{},ring{},resource{},cpu{};
    uint32_t offset{},used{},size{},stride{};Data source{},upload{};
};
struct Binding {bool valid{};Upload upload;uintptr_t raw{},canonical{},view{};uint64_t generation{},gpu{},serial{};unsigned eye{};std::array<float,8> rectangles{};};
Upload pending;Binding current;uint64_t serial{},writes{},matches{},copies{},bindings{},read_failures{};
template<class T> bool read(T& out,uintptr_t ptr,size_t offset=0){return ptr&&offset<=UINTPTR_MAX-ptr&&sizeof(T)-1<=UINTPTR_MAX-(ptr+offset)&&safe_copy(&out,reinterpret_cast<void const*>(ptr+offset),sizeof(T));}
bool observing(){return ngx_stereo_host::attested&&ngx_mv_target_capture::pending()&&recording.load()&&consuming_renderer&&consuming_frame_ticket&&consuming_frozen_frame.enabled;}
uintptr_t context(){uintptr_t slot{},out{};const auto tls=__readgsqword(0x58);return read(slot,tls)&&read(out,slot,0x20)?out:0;}
bool capture(Upload& out){
    out.context=context();uintptr_t table{},ring_owner{},ring_resource{},cpu_base{};uint64_t gpu_base{};
    if(!read(table,out.context,0x13cc)||!read(out.bank,table)||!read(out.data,out.bank,0x3c)||
       !read(out.used,out.bank,0x48)||!read(out.meta,out.bank,0x4c)||!read(out.resource,out.meta,0x0c)||
       !read(out.ring,out.meta,0x14)||!read(out.offset,out.meta,0x1c)||!read(out.size,out.meta,0x2c)||
       !read(ring_owner,out.context,0x12c8)||!read(cpu_base,out.context,0x12d4)||!read(ring_resource,ring_owner,0x0c)||
       ring_owner!=out.ring||ring_resource!=out.resource||out.used!=out.size||!read(gpu_base,out.resource,0x8c)||
       !bounded_upload(out.offset,out.used,cpu_base,gpu_base))return false;
    out.mapped=cpu_base+out.offset;out.gpu=gpu_base+out.offset;
    if(!read(out.cpu,out.context,0x11d8)||!read(out.stride,out.context,0xfb8)||!out.stride||!read(out.source,out.data)||!read(out.upload,out.mapped)||!same(out.source,out.upload))return false;
    out.world=committed_world_generation.load();out.ticket=consuming_frame_ticket;out.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);return true;
}
bool unchanged(Upload const& u){
    Upload fresh;if(!capture(fresh))return false;
    return fresh.world==u.world&&fresh.ticket==u.ticket&&fresh.renderer==u.renderer&&fresh.context==u.context&&fresh.bank==u.bank&&
        fresh.meta==u.meta&&fresh.data==u.data&&fresh.mapped==u.mapped&&fresh.ring==u.ring&&fresh.resource==u.resource&&fresh.cpu==u.cpu&&
        fresh.offset==u.offset&&fresh.used==u.used&&fresh.gpu==u.gpu&&fresh.stride==u.stride&&same(fresh.source,u.source)&&same(fresh.upload,u.upload);
}
void invalidate(){++serial;current.valid=false;}
void write(uint64_t cpu){if(!recording.load())return;++writes;invalidate();if(pending.valid&&pending.cpu==cpu)pending.valid=false;}
void cbv(uintptr_t caller,D3D12_CONSTANT_BUFFER_VIEW_DESC const* desc,D3D12_CPU_DESCRIPTOR_HANDLE cpu){
    if(!observing()||caller!=base+0x1ad2d9||!desc)return;Upload value;D3D12_CONSTANT_BUFFER_VIEW_DESC d{},after{};
    if(!read(d,reinterpret_cast<uintptr_t>(desc))||!capture(value)){++read_failures;return;}
    const uint64_t padded=(uint64_t(value.used)+255)&~uint64_t(255);
    if(value.cpu!=cpu.ptr||d.BufferLocation!=value.gpu||d.SizeInBytes!=padded||!unchanged(value)||
       !read(after,reinterpret_cast<uintptr_t>(desc))||after.BufferLocation!=d.BufferLocation||after.SizeInBytes!=d.SizeInBytes)return;
    value.valid=true;value.serial=++serial;pending=value;++matches;
}
struct Copy {bool valid{};Upload upload;uint64_t destination{},count{},serial{};} copied;
struct TableFailure {unsigned stage{};uintptr_t raw{},canonical{},context{},native{},view{},views{};uint64_t generation{},gpu{},cpu_base{},gpu_base{},copy_cpu{},copy_count{};uint32_t stride{},view_count{};std::array<uintptr_t,2> view_list{};} first_table_failure;
uint64_t table_attempts{};std::array<uint64_t,6> table_failures{};
template<class R> void copy(uintptr_t caller,R const& dst,R const& src){
    if(!recording.load())return;
    // Any later descriptor copy revokes the one pending copy receipt. It must
    // be followed immediately by the authenticated table1 setter.
    copied.valid=false;invalidate();
    if(pending.valid){
        if(!dst.readable||dst.count>dst.starts.size())pending.valid=false;
        else for(unsigned i=0;i<dst.count;++i)if(range_contains(pending.cpu,dst.starts[i],pending.stride,dst.sizes_present?dst.sizes[i]:1)){pending.valid=false;break;}}
    if(!observing())return;
    if(caller!=base+0x17bea9||!pending.valid||!dst.readable||!src.readable||dst.count!=1||src.count!=1||
       !dst.sizes_present||!src.sizes_present||dst.sizes[0]!=src.sizes[0]||!dst.sizes[0]||dst.sizes[0]>8||
       src.starts[0]!=pending.cpu||!unchanged(pending))return;
    copied={true,pending,dst.starts[0],dst.sizes[0],++serial};++copies;
}
void table(uintptr_t raw,uintptr_t canonical,uint64_t generation,UINT index,uint64_t gpu,uintptr_t caller){
    if(index!=1)return;invalidate();if(!observing()||caller!=base+0x17bed7||!copied.valid||!canonical||!generation)return;
    auto const& u=copied.upload;uintptr_t owner{},native{},views{};uint64_t cpu_base{},gpu_base{};uint32_t stride{},count{};
    ++table_attempts;TableFailure failure;failure.raw=raw;failure.canonical=canonical;failure.context=u.context;failure.generation=generation;failure.gpu=gpu;failure.copy_cpu=copied.destination;failure.copy_count=copied.count;
    auto reject=[&](unsigned stage){failure.stage=stage;++table_failures[stage];if(!first_table_failure.stage)first_table_failure=failure;};
    Binding b;b.upload=u;b.raw=raw;b.canonical=canonical;b.generation=generation;b.gpu=gpu;b.serial=serial;
    if(!unchanged(u)){reject(1);return;}
    const bool raw_read=read(owner,u.context,0x127c)&&read(native,owner,0x14);failure.native=native;
    if(!raw_read||native!=raw){reject(2);return;}
    const bool heap_read=read(cpu_base,u.context,0xfbc)&&read(gpu_base,u.context,0xfc4)&&read(stride,u.context,0xfb8);
    failure.cpu_base=cpu_base;failure.gpu_base=gpu_base;failure.stride=stride;
    if(!heap_read||!copied_slot_zero(gpu,gpu_base,cpu_base,stride,copied.destination,unsigned(copied.count))){reject(3);return;}
    const bool view_read=read(b.view,u.context,0x14c0)&&read(views,u.renderer,0x68)&&read(count,u.renderer,0x70);
    failure.view=b.view;failure.views=views;failure.view_count=count;
    if(!view_read||count!=2){reject(4);return;}
    // renderer+68 owns contiguous native views, not an array of pointers.
    // Use the same exact two-view/0xd30-stride check as renderer ownership;
    // the constructor also verifies these addresses against family_views.
    if(!gears_unpossessed_quad::native_eye_index(b.view,views,int(count),b.eye)||
       !read(b.rectangles,b.view,0x48)){reject(5);return;}
    failure.view_list={views,views+0xd30};
    b.valid=true;current=b;++bindings;
}
Binding snapshot(uintptr_t raw,uintptr_t canonical,uint64_t generation){
    if(!current.valid||current.raw!=raw||current.canonical!=canonical||current.generation!=generation||
       current.serial!=serial||!observing()||!unchanged(current.upload))return {};
    return current;
}
bool stable(Binding const& b){return b.valid&&current.valid&&b.serial==serial&&unchanged(b.upload);}
void print(FILE* f){fprintf(f,"ngx_mv_constants writes=%llu upload_matches=%llu native_copies=%llu table_bindings=%llu read_failures=%llu serial=%llu cpu_bytes_only=1 gpu_execution_verified=0\n",writes,matches,copies,bindings,read_failures,serial);
    auto const& e=first_table_failure;
    fprintf(f,"ngx_mv_constants_table_failure attempts=%llu stages=%llu,%llu,%llu,%llu,%llu first_stage=%u raw=%llx canonical=%llx generation=%llu context=%llx native=%llx gpu=%llx cpu_base=%llx gpu_base=%llx stride=%u copy_cpu=%llx copy_count=%llu view=%llx views=%llx view_count=%u view_list=%llx,%llx binding_authorized=0\n",
        table_attempts,table_failures[1],table_failures[2],table_failures[3],table_failures[4],table_failures[5],e.stage,e.raw,e.canonical,e.generation,e.context,e.native,e.gpu,e.cpu_base,e.gpu_base,e.stride,e.copy_cpu,e.copy_count,e.view,e.views,e.view_count,e.view_list[0],e.view_list[1]);}
#else
void invalidate(){}void print(FILE*){}
#endif
}
