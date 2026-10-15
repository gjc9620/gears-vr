// Included inside capture.cpp's private namespace. Observation only; all original
// arguments and the uint32 return value are forwarded. GPU coverage is limited to
// same-thread direct draws; counts never establish valid scene pixels.
using GeometryFn=unsigned(__fastcall*)(void*,void*,unsigned,unsigned);
GeometryFn original_geometry{};void* geometry_target{};
constexpr unsigned char geometry_signature[]={0x44,0x89,0x4c,0x24,0x20,0x48,0x89,0x4c,0x24,0x08,0x55,0x53,0x56,0x57};
struct GeometryTrace {
    unsigned eye{},stage{},mode{},valid{},result{};
    uintptr_t view{},list{},visibility{};
    int count{},capacity{};std::array<int,4> rectangle{};
    uint64_t matrix_hash{},visibility_hash{},calls{},draws{},indexed{},elements{};
    uint64_t empty_lists{},invalid_lists{},min_count{},max_count{},zero_draw_calls{};
    gears_viewport::Target first_target{};
};
std::array<GeometryTrace,32> geometry_traces{};
std::mutex geometry_mutex;uint64_t geometry_overflow{},geometry_rejected{};
thread_local GeometryTrace* current_geometry{};
struct ViewDrawTrace {unsigned eye{},indexed{},viewport{},final_quad{},stack_count{};uint64_t calls{},elements{};
    uintptr_t view{};std::array<void*,16> stack{};};
std::array<ViewDrawTrace,32> view_draw_traces{};std::mutex view_draw_mutex;
uint64_t view_draw_unassigned{},view_draw_overflow{};
thread_local bool in_final_quad{};
void observe_engine_view_draw(ID3D12GraphicsCommandList* list,bool indexed,UINT elements,UINT instances){
    if(!recording.load()||!trial_draw_scope||!consuming_renderer)return;
    gears_render_cost::Scope cost(gears_render_cost::engine_view);
    uintptr_t tls{},context{},view{},views{};int count{};
    // Runtime assembly in 173170 and its callers: GS:[58] -> slot 0,
    // slot+20 -> render context, context+14c0 -> current view. All reads bounded.
    bool readable=safe_copy(&tls,reinterpret_cast<void*>(__readgsqword(0x58)),8)&&tls&&
        safe_copy(&context,reinterpret_cast<void*>(tls+0x20),8)&&context&&
        safe_copy(&view,reinterpret_cast<void*>(context+0x14c0),8)&&
        safe_copy(&views,reinterpret_cast<char*>(consuming_renderer)+0x68,8)&&
        safe_copy(&count,reinterpret_cast<char*>(consuming_renderer)+0x70,4);
    if(!readable||!views||count<1||count>2||view<views||(view-views)%0xd30||(view-views)/0xd30>=unsigned(count)){
        std::lock_guard<std::mutex> lock(view_draw_mutex);++view_draw_unassigned;return;
    }
    unsigned eye=unsigned((view-views)/0xd30),viewport=viewport_stats.binding(reinterpret_cast<uintptr_t>(list)).viewport;
    std::lock_guard<std::mutex> lock(view_draw_mutex);
    for(auto& entry:view_draw_traces){
        if(!entry.calls){entry.eye=eye;entry.indexed=indexed;entry.viewport=viewport;entry.final_quad=in_final_quad;entry.view=view;
            entry.stack_count=CaptureStackBackTrace(0,static_cast<DWORD>(entry.stack.size()),entry.stack.data(),nullptr);}
        if(entry.eye==eye&&entry.indexed==unsigned(indexed)&&entry.viewport==viewport&&entry.final_quad==unsigned(in_final_quad)){
            ++entry.calls;entry.elements+=uint64_t(elements)*instances;return;
        }
    }
    ++view_draw_overflow;
}
uint64_t geometry_hash(uintptr_t address,size_t size,bool& valid) {
    std::array<unsigned char,128> bytes{};valid=size<=bytes.size()&&safe_copy(bytes.data(),reinterpret_cast<void*>(address),size);
    if(!valid)return 0;uint64_t hash=14695981039346656037ULL;
    for(size_t i=0;i<size;++i)hash=(hash^bytes[i])*1099511628211ULL;return hash;
}
void geometry_draw(ID3D12GraphicsCommandList* list,bool indexed,UINT elements,UINT instances) {
    if(!current_geometry||!recording.load())return;
    auto& value=*current_geometry;++(indexed?value.indexed:value.draws);
    value.elements+=uint64_t(elements)*instances;
    if(!value.first_target.resource)value.first_target=viewport_stats.binding(reinterpret_cast<uintptr_t>(list)).target;
}
unsigned __fastcall hook_geometry(void* renderer,void* view,unsigned stage,unsigned mode) {
    Scope scope;if(!scope.world())return original_geometry(renderer,view,stage,mode);GeometryTrace value{};bool observe=false;
    if(recording.load()&&viewport_diagnostics&&trial_draw_scope&&renderer==consuming_renderer&&stage<4&&mode<2){
        struct Header{uintptr_t data;int count,capacity;} header{};
        auto address=reinterpret_cast<uintptr_t>(view);
        if(safe_copy(&header,reinterpret_cast<char*>(renderer)+0x68,sizeof(header))&&header.data&&
           header.count>=1&&header.count<=2&&header.capacity>=header.count&&header.capacity<=1024&&
           address>=header.data&&(address-header.data)%0xd30==0&&(address-header.data)/0xd30<unsigned(header.count)){
            observe=true;value.eye=unsigned((address-header.data)/0xd30);value.stage=stage;value.mode=mode;
            value.view=address;value.calls=1;
            Header list{};
            if(safe_copy(&list,reinterpret_cast<void*>(address+0x570+stage*0x10),sizeof(list))&&
               list.count>=0&&list.capacity>=list.count&&list.capacity<=1000000&&(list.data||!list.count)){
                value.valid|=1;value.list=list.data;value.count=list.count;value.capacity=list.capacity;
                value.min_count=value.max_count=list.count;value.empty_lists=list.count==0;
            }else value.invalid_lists=1;
            if(safe_copy(value.rectangle.data(),reinterpret_cast<void*>(address+0x58),16))value.valid|=2;
            bool valid{};value.matrix_hash=geometry_hash(address+0x1a0,64,valid);if(valid)value.valid|=4;
            uintptr_t visibility{};
            if(safe_copy(&visibility,reinterpret_cast<void*>(address+0x450),8)){
                value.visibility=visibility?visibility:address+0x460;
                // Inline fallback is only a bounded 16-byte preview. Do not
                // assume it describes the full visibility set.
                value.visibility_hash=geometry_hash(value.visibility,16,valid);if(valid)value.valid|=8;
            }
        }else {std::lock_guard<std::mutex> lock(geometry_mutex);++geometry_rejected;}
    }
    auto previous=current_geometry;current_geometry=observe?&value:nullptr;
    unsigned result=original_geometry(renderer,view,stage,mode);
    current_geometry=previous;
    if(observe){
        value.result=result;value.zero_draw_calls=(value.draws+value.indexed)==0;
        bool saved=false;std::lock_guard<std::mutex> lock(geometry_mutex);
        for(auto& entry:geometry_traces){
            if(!entry.calls){entry=value;saved=true;break;}
            if(entry.eye==value.eye&&entry.stage==stage&&entry.mode==mode){
                ++entry.calls;entry.draws+=value.draws;entry.indexed+=value.indexed;entry.elements+=value.elements;
                entry.empty_lists+=value.empty_lists;entry.invalid_lists+=value.invalid_lists;entry.zero_draw_calls+=value.zero_draw_calls;
                if(value.valid&1){entry.min_count=std::min(entry.min_count,value.min_count);entry.max_count=std::max(entry.max_count,value.max_count);}
                if(!entry.first_target.resource)entry.first_target=value.first_target;
                saved=true;break;
            }
        }
        if(!saved)++geometry_overflow;
    }
    return result;
}
void print_geometry(FILE* status){
    fprintf(status,"engine_view_draw_summary unassigned=%llu overflow=%llu slots=32 direct_only=1 current_view_not_pixel_proof=1\n",view_draw_unassigned,view_draw_overflow);
    for(const auto& e:view_draw_traces)if(e.calls){
        fprintf(status,"engine_view_draw eye=%u indexed=%u viewport=%u final_quad=%u calls=%llu elements=%llu first_view=%llx stack=",
            e.eye,e.indexed,e.viewport,e.final_quad,e.calls,e.elements,static_cast<unsigned long long>(e.view));
        for(unsigned i=0;i<e.stack_count;++i){auto address=reinterpret_cast<uintptr_t>(e.stack[i]);
            fprintf(status,"%s%llx",i?",":"",static_cast<unsigned long long>(address>=base&&address<base+0x5000000?address-base:address));}
        fprintf(status,"\n");
    }
    fprintf(status,"geometry_summary overflow=%llu rejected=%llu slots=32 same_thread_direct_only=1 pixels_verified=0\n",geometry_overflow,geometry_rejected);
    for(const auto& e:geometry_traces)if(e.calls)fprintf(status,
        "geometry_view eye=%u stage=%u mode=%u calls=%llu draws=%llu indexed=%llu elements=%llu zero_draw_calls=%llu valid=%u view=%llx list=%llx first_count=%d capacity=%d min_count=%llu max_count=%llu empty_lists=%llu invalid_lists=%llu rectangle=%d,%d,%d,%d matrix_hash=%llx visibility=%llx visibility_hash=%llx first_result=%u target=%llx width=%llu height=%u format=%u\n",
        e.eye,e.stage,e.mode,e.calls,e.draws,e.indexed,e.elements,e.zero_draw_calls,e.valid,
        static_cast<unsigned long long>(e.view),static_cast<unsigned long long>(e.list),e.count,e.capacity,e.min_count,e.max_count,e.empty_lists,e.invalid_lists,
        e.rectangle[0],e.rectangle[1],e.rectangle[2],e.rectangle[3],e.matrix_hash,static_cast<unsigned long long>(e.visibility),e.visibility_hash,e.result,
        static_cast<unsigned long long>(e.first_target.resource),e.first_target.width,e.first_target.height,e.first_target.format);
}
