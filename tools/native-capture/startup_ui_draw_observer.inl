// Included inside ui_scene_root_live. Private opt-in over its already-reserved
// original adapter. No scene changes, new renderer or gameplay admission.
namespace startup_draw {
inline constexpr unsigned capacity=16;
struct Row {
    uintptr_t client{},canvas{},scene{},caller{},client_table{},scene_table{},adapter{},render{};uint64_t ordinal{};
    long long enter{},exit{};
    unsigned thread{},before_keys{},before_vertices{},added_vertices{},materials{};
    bool structural{},shared_render{},completed{},before_valid{},after_valid{},delta_valid{},matrix_valid{},matrix_restored{},alpha_valid{};
    float canvas_alpha{},scene_opacity{},alpha_after{};
    std::array<unsigned char,80> matrix{};
    uintptr_t payload{};unsigned first_vertex{},sample_vertices{};
    std::array<gears_ui_batch::Vertex,4> vertices{};
    std::array<unsigned char,80> vertex_transform{};
};
std::array<Row,capacity> rows{};
std::array<std::atomic<unsigned>,capacity> ready{};
struct Scene {uintptr_t address{};uint64_t calls{};};
std::array<Scene,8> scenes{};
SRWLOCK scene_lock=SRWLOCK_INIT;
std::atomic<bool> armed{};
std::atomic<uint64_t> calls{},accepted{},rejected{},overflow{};
std::atomic<unsigned> count{};
std::array<unsigned char,32> original_bytes{};
unsigned printed_mask{}; // The prime watchdog is the only output writer.
struct Entry {void* target{};std::array<unsigned char,32> original{};};
std::array<Entry,14> extra_entries{};unsigned entry_count{};
bool remember(void* at) {
    if(!at||entry_count==extra_entries.size())return false;
    Entry e{};e.target=at;if(!safe_copy(e.original.data(),at,e.original.size()))return false;
    extra_entries[entry_count++]=e;return true;
}
bool observing() {return armed.load(std::memory_order_acquire)&&prime_recording.load()&&!recording.load();}
bool reserved_target(void* at) {
    if(!armed.load())return false;
    for(unsigned i=0;i<entry_count;++i)if(at==extra_entries[i].target)return true;
    return false;
}
struct ExecuteContext {uintptr_t command{},payload{};uint64_t sequence{};};
thread_local ExecuteContext current_execute;
struct ExecuteScope {ExecuteContext saved=current_execute;~ExecuteScope(){current_execute=saved;}};
struct ListGeneration {uintptr_t list{};uint64_t generation{};};
std::array<ListGeneration,64> list_generations{};SRWLOCK list_lock=SRWLOCK_INIT;
uint64_t generation_sequence{};
uint64_t list_generation(uintptr_t list,bool reset=false,bool reset_ok=true) {
    uint64_t result{};AcquireSRWLockExclusive(&list_lock);
    for(auto& g:list_generations)if(g.list==list||!g.list){g.list=list;
        if(reset)g.generation=reset_ok&&generation_sequence!=UINT64_MAX?++generation_sequence:0;
        result=g.generation;break;}
    ReleaseSRWLockExclusive(&list_lock);return result;
}
inline constexpr unsigned gpu_capacity=64;
struct GpuRow {uintptr_t list{},caller{},command{},payload{};uint64_t generation{},execute_sequence{};
    long long qpc{};unsigned thread{},indexed{},vertices{},instances{},targets{};bool known_buffer{},descriptor_lost{};
    gears_viewport::Target target{};gears_viewport::DrawState state{};};
std::array<GpuRow,gpu_capacity> gpu_rows{};
std::array<std::atomic<unsigned>,gpu_capacity> gpu_ready{},gpu_submitted{};
std::array<std::atomic<uintptr_t>,gpu_capacity> gpu_queue{};
std::atomic<uint64_t> gpu_calls{},gpu_ui_calls{},queue_calls{};
// One bounded settled interval. Original rendering is forwarded unchanged.
// Record copies/clears and actual current swapchain buffers in the same interval
// rather than comparing an early draw with a later, potentially resized pool.
inline constexpr unsigned trace_capacity=1024;
struct QuadContext {uintptr_t caller{};std::array<float,8> args{};std::array<unsigned,4> dimensions{};};
thread_local QuadContext current_quad;
struct QuadScope {QuadContext saved=current_quad;~QuadScope(){current_quad=saved;}};
struct TraceRow {unsigned kind{},thread{},a{},b{},c{},d{},elements{};
    uintptr_t list{},caller{},resource{},source{},command{};uint64_t generation{},execute_sequence{};
    long long qpc{};uint64_t width{},source_width{};unsigned height{},format{},source_height{},source_format{};
    std::array<float,4> rgba{};QuadContext quad{};uintptr_t context{};unsigned read_mask{},slot_count{};
    std::array<uintptr_t,4> objects{},inputs{};std::array<unsigned,4> input_width{},input_height{},input_format{};};
std::array<TraceRow,trace_capacity> trace_rows{};
std::array<std::atomic<unsigned>,trace_capacity> trace_ready{},trace_submitted{};
std::array<std::atomic<uintptr_t>,trace_capacity> trace_queue{};
std::atomic<unsigned> trace_count{};std::atomic<ULONGLONG> first_ui_ms{};
void bound_inputs(TraceRow&);
bool tracing() {const auto first=first_ui_ms.load();if(!first||!observing())return false;
    const auto elapsed=GetTickCount64()-first;return elapsed>=7000&&elapsed<7350;}
void trace(TraceRow row) {const auto slot=trace_count.fetch_add(1);if(slot>=trace_capacity)return;
    row.thread=GetCurrentThreadId();row.qpc=now();row.command=current_execute.command;row.execute_sequence=current_execute.sequence;
    row.quad=current_quad;if(row.kind==1&&row.quad.caller)bound_inputs(row);
    if(row.list)row.generation=list_generation(row.list);trace_rows[slot]=row;trace_ready[slot].store(1,std::memory_order_release);}
bool texture(ID3D12Resource* resource,TraceRow& row,bool source=false) {
    if(!resource)return false;const auto desc=resource->GetDesc();
    if(source){row.source=canonical_identity(resource);row.source_width=desc.Width;row.source_height=desc.Height;row.source_format=unsigned(desc.Format);}
    else {row.resource=canonical_identity(resource);row.width=desc.Width;row.height=desc.Height;row.format=unsigned(desc.Format);}
    return desc.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&desc.Width>=640&&desc.Height>=360&&
        (desc.Format==DXGI_FORMAT_R10G10B10A2_UNORM||desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM||desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM);
}
void transfer(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned kind,ID3D12Resource* dst,ID3D12Resource* src,unsigned a=0,unsigned b=0,unsigned c=0,unsigned d=0) {
    if(!tracing())return;TraceRow row{};row.kind=kind;row.list=reinterpret_cast<uintptr_t>(list);row.caller=caller;
    const bool target=texture(dst,row),source=texture(src,row,true);if(!target&&!source)return;
    row.a=a;row.b=b;row.c=c;row.d=d;trace(row);
}
void clear(ID3D12GraphicsCommandList* list,uintptr_t caller,D3D12_CPU_DESCRIPTOR_HANDLE handle,const float* rgba,unsigned rectangles) {
    if(!tracing())return;const auto target=frame_target_stats.lookup(handle.ptr);
    if(target.width<640||target.height<360)return;TraceRow row{};row.kind=4;row.list=reinterpret_cast<uintptr_t>(list);row.caller=caller;
    row.resource=target.resource;row.width=target.width;row.height=target.height;row.format=target.format;row.a=rectangles;
    if(!safe_copy(row.rgba.data(),rgba,sizeof(row.rgba)))return;trace(row);
}
void barriers(ID3D12GraphicsCommandList* list,uintptr_t caller,unsigned count,const D3D12_RESOURCE_BARRIER* input) {
    if(!tracing()||count>1024)return;
    for(unsigned i=0;i<count;++i){D3D12_RESOURCE_BARRIER b{};if(!safe_copy(&b,input+i,sizeof(b)))return;
        if(b.Type!=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)continue;
        TraceRow row{};if(!texture(b.Transition.pResource,row))continue;row.kind=5;row.list=reinterpret_cast<uintptr_t>(list);row.caller=caller;
        row.a=unsigned(b.Transition.StateBefore);row.b=unsigned(b.Transition.StateAfter);row.c=b.Transition.Subresource;row.d=unsigned(b.Flags);trace(row);}
}
void present(IDXGISwapChain* chain,unsigned flags) {
    if(!tracing()||(flags&DXGI_PRESENT_TEST))return;
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    gears_vr::ComPtr<IDXGISwapChain3> actual;DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(chain->QueryInterface(IID_PPV_ARGS(&actual)))||FAILED(chain->GetDesc(&desc))||desc.BufferCount>16)return;
    const auto current=actual->GetCurrentBackBufferIndex();
    for(unsigned i=0;i<desc.BufferCount;++i){gears_vr::ComPtr<ID3D12Resource> buffer;TraceRow row{};
        if(FAILED(chain->GetBuffer(i,IID_PPV_ARGS(&buffer)))||!texture(buffer.Get(),row))continue;
        row.kind=6;row.source=canonical_identity(chain);row.a=i;row.b=current;trace(row);}
}
uint64_t gpu_printed{};
struct QueueRow {uintptr_t queue{};unsigned count{},matches{};long long qpc{};};
std::array<QueueRow,16> queue_rows{};std::array<std::atomic<unsigned>,16> queue_ready{};
unsigned queue_printed{};
void gpu_draw(ID3D12GraphicsCommandList* list,uintptr_t caller,bool indexed,unsigned vertices,unsigned instances) {
    const auto all=++gpu_calls;const auto matched=current_execute.command?++gpu_ui_calls:0;
    if(tracing()){const auto binding=frame_target_stats.binding(reinterpret_cast<uintptr_t>(list));
        if(binding.targets==1&&binding.target.width>=640&&binding.target.height>=360){TraceRow t{};t.kind=1;t.list=reinterpret_cast<uintptr_t>(list);t.caller=caller;
            t.resource=binding.target.resource;t.width=binding.target.width;t.height=binding.target.height;t.format=binding.target.format;
            t.a=indexed;t.b=instances;t.elements=vertices;trace(t);}}
    const auto slot=all<=16?unsigned(all-1):matched&&matched<=48?unsigned(16+matched-1):gpu_capacity;
    if(slot>=gpu_capacity)return;
    GpuRow row{};row.list=reinterpret_cast<uintptr_t>(list);row.caller=caller;row.qpc=now();row.thread=GetCurrentThreadId();
    row.command=current_execute.command;row.payload=current_execute.payload;row.execute_sequence=current_execute.sequence;
    row.generation=list_generation(row.list);row.indexed=indexed;row.vertices=vertices;row.instances=instances;
    row.state=viewport_stats.draw_state(row.list);const auto binding=frame_target_stats.binding(row.list);
    row.targets=binding.targets;row.target=binding.target;row.descriptor_lost=frame_target_stats.descriptor_capacity_lost();
    row.known_buffer=!row.descriptor_lost&&swapchain_bindings.known_buffer(row.target.resource);
    gpu_rows[slot]=row;gpu_ready[slot].store(1,std::memory_order_release);
}
struct Submission {uint64_t ordinal{};QueueRow row{};std::array<uintptr_t,64> lists{};std::array<uint64_t,64> generations{};bool valid{};};
Submission submission_begin(ID3D12CommandQueue* queue,unsigned count,ID3D12CommandList* const* lists) {
    Submission out{};out.ordinal=++queue_calls;out.row.queue=canonical_identity(queue);out.row.count=count;out.row.qpc=now();
    if(!count||count>out.lists.size()||!safe_copy(out.lists.data(),lists,count*sizeof(out.lists[0])))return out;
    for(unsigned i=0;i<count;++i)out.generations[i]=list_generation(out.lists[i]);out.valid=true;return out;
}
void submitted(Submission const& submission) {
    if(!submission.valid)return;auto row=submission.row;
    for(unsigned i=0;i<row.count;++i){const auto list=submission.lists[i],generation=submission.generations[i];
        if(!generation||list_generation(list)!=generation)continue;
        const auto size=(std::min)(trace_count.load(),trace_capacity);
        for(unsigned j=0;j<size;++j)if(trace_ready[j].load(std::memory_order_acquire)&&trace_rows[j].list==list&&trace_rows[j].generation==generation){
            trace_queue[j].store(row.queue);trace_submitted[j].store(1,std::memory_order_release);}
        for(unsigned j=0;j<gpu_capacity;++j)if(gpu_ready[j].load(std::memory_order_acquire)&&
            gpu_rows[j].list==list&&gpu_rows[j].generation==generation){
            gpu_queue[j].store(row.queue);gpu_submitted[j].store(1,std::memory_order_release);++row.matches;}}
    if(submission.ordinal<=queue_rows.size()){queue_rows[submission.ordinal-1]=row;queue_ready[submission.ordinal-1].store(1,std::memory_order_release);}
}
std::atomic<uintptr_t> last_scene_hint{};
struct TransportRow {
    uintptr_t object{},caller{},payload{},scene_hint{},target{},secondary{},return_bits{};uint64_t ordinal{};
    long long qpc{};unsigned thread{},vertices{},sample_vertices{};uint32_t result{};
    bool readable{},completed{};
    std::array<unsigned char,48> command{};
    std::array<unsigned char,64> matrix{};
    std::array<gears_ui_batch::Vertex,4> sample{};
};
std::array<TransportRow,capacity> executes{},flushes{};
std::array<std::atomic<unsigned>,capacity> execute_ready{},flush_ready{};
std::atomic<uint64_t> execute_calls{},flush_calls{};
std::atomic<unsigned> execute_count{},flush_count{};
unsigned execute_printed{},flush_printed{};
struct ExecuteKey {uintptr_t scene{},caller{};unsigned vertices{};uint64_t calls{};};
std::array<ExecuteKey,8> execute_keys{};SRWLOCK execute_lock=SRWLOCK_INIT;

unsigned execute_begin(void* command,uintptr_t caller) {
    ULONGLONG empty{};first_ui_ms.compare_exchange_strong(empty,GetTickCount64());
    const auto sequence=++execute_calls;TransportRow row{};row.object=reinterpret_cast<uintptr_t>(command);row.caller=caller;
    row.qpc=now();row.thread=GetCurrentThreadId();row.scene_hint=last_scene_hint.load();
    gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};gears_ui_batch::detail::Array vertices{};uintptr_t table{};
    if(read.bytes(row.command.data(),row.object,row.command.size())){
        memcpy(&table,row.command.data(),8);memcpy(&row.payload,row.command.data()+8,8);
        row.readable=table==base+0x13394f0&&read.field(vertices,row.payload,0x48)&&
            gears_ui_batch::detail::valid(vertices,16384,gears_ui_batch::vertex_stride)&&
            read.field(row.matrix,row.payload,0x80);
        if(row.readable)row.vertices=unsigned(vertices.count);
    }
    current_execute={row.object,row.payload,sequence};
    unsigned slot=capacity;AcquireSRWLockExclusive(&execute_lock);
    for(auto& k:execute_keys)if(!k.calls||(k.scene==row.scene_hint&&k.caller==caller&&k.vertices==row.vertices)){
        k.scene=row.scene_hint;k.caller=caller;k.vertices=row.vertices;row.ordinal=++k.calls;
        if(k.calls==1||k.calls==30)slot=execute_count.fetch_add(1);break;
    }
    ReleaseSRWLockExclusive(&execute_lock);
    if(slot>=capacity)return capacity;
    if(row.readable){row.sample_vertices=(std::min)(row.vertices,4u);
        for(unsigned i=0;i<row.sample_vertices;++i)if(!read.bytes(row.sample[i].data(),vertices.data+size_t(i)*gears_ui_batch::vertex_stride,row.sample[i].size())){
            row.readable=false;row.sample_vertices=0;break;}}
    executes[slot]=row;return slot;
}
void execute_end(unsigned slot,uint32_t result) {
    if(slot>=capacity)return;executes[slot].result=result;executes[slot].completed=true;
    execute_ready[slot].store(1,std::memory_order_release);
}
unsigned flush_begin(void* canvas,uintptr_t caller) {
    const auto ordinal=++flush_calls;
    if(ordinal!=1&&ordinal!=30&&ordinal!=300)return capacity;
    const auto slot=flush_count.fetch_add(1);if(slot>=capacity)return capacity;
    TransportRow row{};row.object=reinterpret_cast<uintptr_t>(canvas);row.caller=caller;row.ordinal=ordinal;
    row.qpc=now();row.thread=GetCurrentThreadId();row.scene_hint=last_scene_hint.load();
    gears_ui_batch::detail::Reader<decltype(&safe_copy)> read{safe_copy};
    row.readable=read.field(row.target,row.object,4)&&read.field(row.secondary,row.object,12);
    flushes[slot]=row;return slot;
}
void flush_end(unsigned slot,uintptr_t result) {
    if(slot>=capacity)return;flushes[slot].return_bits=result;flushes[slot].completed=true;
    flush_ready[slot].store(1,std::memory_order_release);
}

bool queue(bool owned) {
    wchar_t option[4]{};
    if(!owned||!reservation_ready||GetEnvironmentVariableW(L"GEARSVR_TEST_STARTUP_UI_DRAW",option,4)!=1||option[0]!=L'1')return true;
    if(!reserved||!target||!original||MH_QueueEnableHook(target)!=MH_OK)return false;
    armed.store(true,std::memory_order_release);
    for(unsigned i=0;i<entry_count;++i)if(MH_QueueEnableHook(extra_entries[i].target)!=MH_OK)return false;
    return true;
}
bool primed_bytes(void* at,std::array<unsigned char,32>& out) {
    if(!armed.load(std::memory_order_acquire))return false;
    if(at==target){out=original_bytes;return true;}
    for(unsigned i=0;i<entry_count;++i)if(at==extra_entries[i].target){out=extra_entries[i].original;return true;}
    return false;
}
bool disable() {
    if(!armed.load(std::memory_order_acquire))return true;
    auto close=[](void* at){const auto queued=MH_QueueDisableHook(at),disabled=MH_DisableHook(at);
        return queued==MH_OK&&(disabled==MH_OK||disabled==MH_ERROR_DISABLED);};
    bool ok=close(target);for(unsigned i=0;i<entry_count;++i)ok=close(extra_entries[i].target)&&ok;return ok;
}
bool restored() {
    if(!armed.load(std::memory_order_acquire))return true;
    std::array<unsigned char,32> bytes{};
    if(active.load()||!safe_copy(bytes.data(),target,bytes.size())||bytes!=original_bytes)return false;
    for(unsigned i=0;i<entry_count;++i)if(!safe_copy(bytes.data(),extra_entries[i].target,bytes.size())||bytes!=extra_entries[i].original)return false;
    return true;
}
bool shared_scene_render(Row& row) {
    Row actual{};actual.client=row.client;actual.scene=row.scene;
    std::array<unsigned char,6> caller{};std::array<unsigned char,16> code{};
    if(!safe_copy(&actual.client_table,reinterpret_cast<void*>(row.client),8)||
       !safe_copy(&actual.scene_table,reinterpret_cast<void*>(row.scene),8)||
       actual.client_table!=base+gears_ui_scene_root::client_table_rva||!actual.scene_table||
       actual.scene_table>UINTPTR_MAX-0x4a8||row.caller!=base+gears_ui_scene_root::caller_return_rva||
       !safe_copy(&actual.adapter,reinterpret_cast<void*>(actual.client_table+0x2b8),8)||
       actual.adapter!=base+gears_ui_scene_root::adapter_rva||
       !safe_copy(&actual.render,reinterpret_cast<void*>(actual.scene_table+0x4a8),8)||
       actual.render!=base+gears_ui_scene_root::scene_render_rva||
       !safe_copy(caller.data(),reinterpret_cast<void*>(base+gears_ui_scene_root::caller_site_rva),caller.size())||
       caller!=gears_ui_scene_root::caller_bytes||!safe_copy(code.data(),reinterpret_cast<void*>(actual.render),code.size())||
       code!=std::array<unsigned char,16>{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x30,0xc5,0xf8,0x29,0x74,0x24,0x20})return false;
    MEMORY_BASIC_INFORMATION table{};
    if(VirtualQuery(reinterpret_cast<void*>(actual.scene_table),&table,sizeof(table))!=sizeof(table)||
       table.AllocationBase!=reinterpret_cast<void*>(base)||table.State!=MEM_COMMIT||table.Type!=MEM_IMAGE||
       (table.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
       ((table.Protect&0xff)!=PAGE_READONLY&&(table.Protect&0xff)!=PAGE_EXECUTE_READ))return false;
    row.client_table=actual.client_table;row.scene_table=actual.scene_table;row.adapter=actual.adapter;row.render=actual.render;
    return true;
}
uintptr_t invoke(void* client,void* canvas,void* scene,uintptr_t caller) {
    last_scene_hint.store(reinterpret_cast<uintptr_t>(scene));
    ++calls;gears_ui_scene_root::Candidate c{};
    const bool structural=gears_ui_scene_root::candidate({base},safe_copy,reinterpret_cast<uintptr_t>(client),
        reinterpret_cast<uintptr_t>(canvas),reinterpret_cast<uintptr_t>(scene),caller,c);
    if(structural)++accepted;else ++rejected;
    const auto scene_address=reinterpret_cast<uintptr_t>(scene);
    uint64_t ordinal{};unsigned slot=capacity;
    AcquireSRWLockExclusive(&scene_lock);
    for(auto& s:scenes)if(s.address==scene_address||!s.address){
        s.address=scene_address;ordinal=++s.calls;
        // First draw and a settled draw for each of at most eight scene identities.
        if(ordinal==1||ordinal==30)slot=count.fetch_add(1);
        break;
    }
    ReleaseSRWLockExclusive(&scene_lock);
    if(slot>=capacity){if(!ordinal)++overflow;return original(client,canvas,scene);}
    Row row{};row.client=reinterpret_cast<uintptr_t>(client);row.canvas=reinterpret_cast<uintptr_t>(canvas);
    row.scene=scene_address;row.caller=caller;row.structural=structural;
    // Actual operands are diagnostic hints even when the saved generic UIScene
    // vtable is not this scene's concrete subclass. No layout/geometry traversal
    // is permitted by these raw values alone.
    safe_copy(&row.client_table,client,sizeof(row.client_table));
    safe_copy(&row.scene_table,scene,sizeof(row.scene_table));
    if(row.client_table&&row.client_table<=UINTPTR_MAX-0x2b8)
        safe_copy(&row.adapter,reinterpret_cast<void*>(row.client_table+0x2b8),sizeof(row.adapter));
    if(row.scene_table&&row.scene_table<=UINTPTR_MAX-0x4a8)
        safe_copy(&row.render,reinterpret_cast<void*>(row.scene_table+0x4a8),sizeof(row.render));
    // A concrete subclass may have a different readonly vtable but still invoke
    // the exact same certified native render body/layout. This private read-only
    // proof is not used by production scene, geometry or action admission.
    row.shared_render=shared_scene_render(row);
    if(row.shared_render&&row.scene<=UINTPTR_MAX-0xf0)
        row.alpha_valid=safe_copy(&row.canvas_alpha,canvas,4)&&
            safe_copy(&row.scene_opacity,reinterpret_cast<void*>(row.scene+0xf0),4);
    row.ordinal=ordinal;row.thread=GetCurrentThreadId();row.enter=now();
    gears_ui_scene_root::MatrixSnapshot matrix_before{},matrix_after{};
    row.matrix_valid=row.shared_render&&gears_ui_scene_root::matrix_stack(safe_copy,row.canvas,matrix_before);
    if(row.matrix_valid)row.matrix=matrix_before.top;
    gears_ui_scene_batch::Inventory before{};
    row.before_valid=row.shared_render&&gears_ui_scene_batch::inventory({base},safe_copy,row.canvas,before);
    if(row.before_valid){row.before_keys=unsigned(before.keys.size());
        for(auto const& key:before.keys)for(auto const& item:key.items)row.before_vertices+=unsigned(item.vertices.size());}
    const auto result=original(client,canvas,scene);row.exit=now();row.completed=true; // Exactly one original call.
    Row after=row;
    row.after_valid=row.shared_render&&shared_scene_render(after)&&after.client_table==row.client_table&&after.scene_table==row.scene_table;
    row.alpha_valid=row.alpha_valid&&safe_copy(&row.alpha_after,canvas,4);
    if(row.before_valid&&row.after_valid){gears_ui_scene_batch::Delta delta{};
        row.delta_valid=gears_ui_scene_batch::delta({base},safe_copy,before,row.canvas,delta);
        if(row.delta_valid){for(auto const& range:delta.simple){row.added_vertices+=unsigned(range.vertices.size());
                if(!row.sample_vertices&&!range.vertices.empty()){
                    row.payload=range.payload;row.first_vertex=range.begin;row.vertex_transform=range.transform;
                    row.sample_vertices=(std::min)(unsigned(range.vertices.size()),4u);
                    for(unsigned i=0;i<row.sample_vertices;++i)row.vertices[i]=range.vertices[i];}}
            row.materials=unsigned(delta.material.size());}}
    row.matrix_restored=row.matrix_valid&&gears_ui_scene_root::matrix_stack(safe_copy,row.canvas,matrix_after)&&
        gears_ui_batch::detail::same(matrix_before.stack,matrix_after.stack)&&matrix_before.top==matrix_after.top;
    rows[slot]=row;ready[slot].store(1,std::memory_order_release);return result;
}
void print(FILE* status,bool final=false) {
    if(!armed.load())return;
    unsigned available{};
    for(unsigned i=0;i<capacity;++i)if(ready[i].load(std::memory_order_acquire))available|=1u<<i;
    unsigned available_execute{},available_flush{};
    for(unsigned i=0;i<capacity;++i){if(execute_ready[i].load(std::memory_order_acquire))available_execute|=1u<<i;
        if(flush_ready[i].load(std::memory_order_acquire))available_flush|=1u<<i;}
    const auto fresh=available&~printed_mask,fresh_execute=available_execute&~execute_printed,fresh_flush=available_flush&~flush_printed;
    uint64_t gpu_available{};unsigned queues_available{};
    for(unsigned i=0;i<gpu_capacity;++i)if(gpu_ready[i].load(std::memory_order_acquire))gpu_available|=uint64_t(1)<<i;
    for(unsigned i=0;i<queue_rows.size();++i)if(queue_ready[i].load(std::memory_order_acquire))queues_available|=1u<<i;
    const auto fresh_gpu=gpu_available&~gpu_printed;const auto fresh_queue=queues_available&~queue_printed;
    if(!fresh&&!fresh_execute&&!fresh_flush&&!fresh_gpu&&!fresh_queue&&!final)return;
    fprintf(status,"startup_ui_draw armed=1 calls=%llu candidates=%llu rejected=%llu rows=%u overflow=%llu extra_hooks=%u execute_calls=%llu flush_calls=%llu gpu_calls=%llu gpu_ui_calls=%llu queue_calls=%llu observation_only=1 gpu_visible_proven=0\n",
        calls.load(),accepted.load(),rejected.load(),(std::min)(count.load(),capacity),overflow.load(),entry_count,execute_calls.load(),flush_calls.load(),gpu_calls.load(),gpu_ui_calls.load(),queue_calls.load());
    for(unsigned i=0;i<capacity;++i)if(fresh&(1u<<i)){
        auto const& r=rows[i];fprintf(status,"startup_ui_draw_row slot=%u client=%p canvas=%p scene=%p caller=%p ordinal=%llu thread=%u enter=%lld exit=%lld client_table=%p scene_table=%p adapter=%p render=%p structural=%u shared_render=%u completed=%u before_valid=%u after_valid=%u delta_valid=%u before_keys=%u before_vertices=%u added_vertices=%u materials=%u matrix_valid=%u matrix_restored=%u alpha_valid=%u canvas_alpha=%.9g scene_opacity=%.9g alpha_after=%.9g matrix=",
            i,reinterpret_cast<void*>(r.client),reinterpret_cast<void*>(r.canvas),reinterpret_cast<void*>(r.scene),reinterpret_cast<void*>(r.caller),r.ordinal,r.thread,r.enter,r.exit,
            reinterpret_cast<void*>(r.client_table),reinterpret_cast<void*>(r.scene_table),reinterpret_cast<void*>(r.adapter),reinterpret_cast<void*>(r.render),unsigned(r.structural),unsigned(r.shared_render),unsigned(r.completed),
            unsigned(r.before_valid),unsigned(r.after_valid),unsigned(r.delta_valid),r.before_keys,r.before_vertices,r.added_vertices,r.materials,unsigned(r.matrix_valid),unsigned(r.matrix_restored),unsigned(r.alpha_valid),r.canvas_alpha,r.scene_opacity,r.alpha_after);
        for(auto byte:r.matrix)fprintf(status,"%02x",byte);fprintf(status,"\n");
        if(r.sample_vertices){fprintf(status,"startup_ui_vertices slot=%u payload=%p first_vertex=%u samples=%u transform=",i,reinterpret_cast<void*>(r.payload),r.first_vertex,r.sample_vertices);
            for(auto byte:r.vertex_transform)fprintf(status,"%02x",byte);
            for(unsigned v=0;v<r.sample_vertices;++v){fprintf(status," v%u=",v);for(auto byte:r.vertices[v])fprintf(status,"%02x",byte);}fprintf(status,"\n");}
    }
    printed_mask|=fresh;
    for(unsigned kind=0;kind<2;++kind)for(unsigned i=0;i<capacity;++i)if((kind?fresh_flush:fresh_execute)&(1u<<i)){
        const auto& r=kind?flushes[i]:executes[i];fprintf(status,"startup_ui_transport kind=%u slot=%u object=%p caller=%p payload=%p scene_hint=%p target=%p secondary=%p ordinal=%llu qpc=%lld thread=%u readable=%u completed=%u result=%u return_bits=%llx vertices=%u samples=%u command=",
            kind,i,reinterpret_cast<void*>(r.object),reinterpret_cast<void*>(r.caller),reinterpret_cast<void*>(r.payload),reinterpret_cast<void*>(r.scene_hint),reinterpret_cast<void*>(r.target),reinterpret_cast<void*>(r.secondary),r.ordinal,r.qpc,r.thread,unsigned(r.readable),unsigned(r.completed),r.result,static_cast<unsigned long long>(r.return_bits),r.vertices,r.sample_vertices);
        for(auto byte:r.command)fprintf(status,"%02x",byte);fprintf(status," matrix=");for(auto byte:r.matrix)fprintf(status,"%02x",byte);
        for(unsigned v=0;v<r.sample_vertices;++v){fprintf(status," v%u=",v);for(auto byte:r.sample[v])fprintf(status,"%02x",byte);}fprintf(status,"\n");
    }
    execute_printed|=fresh_execute;flush_printed|=fresh_flush;
    for(unsigned i=0;i<gpu_capacity;++i)if((final?gpu_available:fresh_gpu)&(uint64_t(1)<<i)){
        const auto& r=gpu_rows[i];fprintf(status,"startup_ui_gpu slot=%u list=%p caller=%p command=%p payload=%p generation=%llu execute_sequence=%llu qpc=%lld thread=%u indexed=%u vertices=%u instances=%u targets=%u resource=%p size=%llu,%u format=%u known_backbuffer=%u descriptor_lost=%u tracked=%u viewport_count=%u scissor_count=%u viewport=%.9g,%.9g,%.9g,%.9g scissor=%.9g,%.9g,%.9g,%.9g depth_known=%u depth=%.9g,%.9g submitted=%u queue=%p\n",
            i,reinterpret_cast<void*>(r.list),reinterpret_cast<void*>(r.caller),reinterpret_cast<void*>(r.command),reinterpret_cast<void*>(r.payload),r.generation,r.execute_sequence,r.qpc,r.thread,r.indexed,r.vertices,r.instances,r.targets,
            reinterpret_cast<void*>(r.target.resource),static_cast<unsigned long long>(r.target.width),r.target.height,r.target.format,unsigned(r.known_buffer),unsigned(r.descriptor_lost),unsigned(r.state.tracked),r.state.viewport_count,r.state.scissor_count,
            r.state.viewport.x,r.state.viewport.y,r.state.viewport.width,r.state.viewport.height,r.state.scissor.x,r.state.scissor.y,r.state.scissor.width,r.state.scissor.height,
            unsigned(r.state.depth_known),r.state.min_depth,r.state.max_depth,gpu_submitted[i].load(std::memory_order_acquire),reinterpret_cast<void*>(gpu_queue[i].load()));}
    gpu_printed|=fresh_gpu;
    for(unsigned i=0;i<queue_rows.size();++i)if(fresh_queue&(1u<<i)){const auto& r=queue_rows[i];
        fprintf(status,"startup_ui_queue slot=%u queue=%p count=%u matched_draw_rows=%u qpc=%lld\n",i,reinterpret_cast<void*>(r.queue),r.count,r.matches,r.qpc);}
    queue_printed|=fresh_queue;
    if(final){const auto size=(std::min)(trace_count.load(),trace_capacity);
        fprintf(status,"startup_ui_trace_summary rows=%u overflow=%u first_ui_ms=%llu begin_ms=7000 end_ms=7350\n",size,trace_count.load()>trace_capacity?trace_count.load()-trace_capacity:0,first_ui_ms.load());
        for(unsigned i=0;i<size;++i)if(trace_ready[i].load(std::memory_order_acquire)){const auto& r=trace_rows[i];
            fprintf(status,"startup_ui_trace slot=%u kind=%u qpc=%lld thread=%u list=%p generation=%llu caller=%p command=%p execute_sequence=%llu resource=%p width=%llu height=%u format=%u source=%p source_width=%llu source_height=%u source_format=%u a=%u b=%u c=%u d=%u elements=%u rgba=%.9g,%.9g,%.9g,%.9g submitted=%u queue=%p\n",
                i,r.kind,r.qpc,r.thread,reinterpret_cast<void*>(r.list),r.generation,reinterpret_cast<void*>(r.caller),reinterpret_cast<void*>(r.command),r.execute_sequence,
                reinterpret_cast<void*>(r.resource),r.width,r.height,r.format,reinterpret_cast<void*>(r.source),r.source_width,r.source_height,r.source_format,
                r.a,r.b,r.c,r.d,r.elements,r.rgba[0],r.rgba[1],r.rgba[2],r.rgba[3],trace_submitted[i].load(),reinterpret_cast<void*>(trace_queue[i].load()));}}
    if(final){const auto size=(std::min)(trace_count.load(),trace_capacity);
        for(unsigned i=0;i<size;++i)if(trace_ready[i].load(std::memory_order_acquire)){const auto& r=trace_rows[i];if(!r.quad.caller)continue;
            fprintf(status,"startup_ui_quad slot=%u caller=%p args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u context=%p read_mask=%u slot_count=%u",
                i,reinterpret_cast<void*>(r.quad.caller),r.quad.args[0],r.quad.args[1],r.quad.args[2],r.quad.args[3],r.quad.args[4],r.quad.args[5],r.quad.args[6],r.quad.args[7],
                r.quad.dimensions[0],r.quad.dimensions[1],r.quad.dimensions[2],r.quad.dimensions[3],reinterpret_cast<void*>(r.context),r.read_mask,r.slot_count);
            for(unsigned s=0;s<4;++s)fprintf(status," input%u=%p,%p,%u,%u,%u",s,reinterpret_cast<void*>(r.objects[s]),reinterpret_cast<void*>(r.inputs[s]),r.input_width[s],r.input_height[s],r.input_format[s]);
            fprintf(status,"\n");}}
    fflush(status);
}
}
