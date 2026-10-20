// Included by ui_observer_hooks.inl after payload state. Read-only range capture.
using UiSubtitle=uintptr_t(__fastcall*)(void*,void*,int32_t*);
UiSubtitle ui_original_subtitle{};void* ui_subtitle_target{};
constexpr std::array<unsigned char,32> ui_subtitle_bytes{
    0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x48,
    0x8b,0x05,0xca,0xbf,0x97,0x01,0x49,0x8b,0xd8,0x48,0x8b,0xfa,0x48,0x8b,0xf1,0x48};
thread_local unsigned ui_text_collect_depth{};
struct UiTextCollectScope {UiTextCollectScope(){++ui_text_collect_depth;}~UiTextCollectScope(){--ui_text_collect_depth;}};
struct UiTextCounts {
    std::atomic<uint64_t> calls{},eligible{},before_rejected{},delta_rejected{},empty{},
        published_calls{},published_ranges{},published_vertices{},publish_rejected{};
};
std::array<UiTextCounts,2> ui_text_counts{};

void publish_ui_text_delta(gears_ui_text_batch::Delta const& delta,unsigned kind,uint64_t invocation,
        gears_ui_receipts::Source source,unsigned bucket){
    auto& counts=ui_text_counts[bucket];
    if(delta.ranges.empty()){++counts.empty;return;}
    // Consumers take the same outer mutex. A multi-atlas call either publishes
    // every copied range, or discards all of its payload candidates before any
    // consumer can claim a partial string. The original text is never changed.
    // Multi-atlas native subtitles retain one frozen producer-call geometry.
    // Every fragment still needs its own current bytes and actual GPU indices.
    std::lock_guard<std::mutex> lock(ui_payload_mutex);bool accepted=true;
    const auto text_group=source==gears_ui_receipts::Source::NativeSubtitle&&delta.ranges.size()>1?
        gears_ui_text_group::capture(delta,invocation,safe_copy):gears_ui_receipts::TextGroup{};
    for(auto const& range:delta.ranges){
        const auto length=range.end-range.begin;
        gears_ui_receipts::RangeMetadata metadata{delta.canvas,delta.fcanvas,range.item,range.payload,
            delta.sort_key,range.begin,range.end};
        if(range.vertex_offset>delta.vertices.size()||length>delta.vertices.size()-range.vertex_offset||
           !ui_payload_store.publish_range(metadata,delta.vertices.data()+range.vertex_offset,length,kind,invocation,source,
               (source==gears_ui_receipts::Source::CanvasText||source==gears_ui_receipts::Source::NativeSubtitle)&&delta.ranges.size()==1,
               text_group,text_group?range.vertex_offset:0)){
            accepted=false;break;
        }
    }
    if(!accepted){
        for(auto const& range:delta.ranges)ui_payload_store.retire(range.payload);
        ++counts.publish_rejected;return;
    }
    ++counts.published_calls;counts.published_ranges+=delta.ranges.size();counts.published_vertices+=delta.vertices.size();
}

uintptr_t __fastcall hook_ui_subtitle(void* manager,void* canvas,int32_t* rectangle){
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());Scope active_scope;if(!active_scope.world())return ui_original_subtitle(manager,canvas,rectangle);
    auto& counts=ui_text_counts[1];
    if(ui_observer_enabled&&recording.load())++counts.calls;
    std::array<int32_t,4> rect{};gears_ui_text_batch::Snapshot before{};
    const bool eligible=ui_observer_enabled&&recording.load()&&!ui_text_collect_depth&&
        caller==base+0x2c9375&&manager&&canvas&&rectangle&&safe_copy(rect.data(),rectangle,sizeof(rect))&&
        rect[2]>rect[0]&&rect[3]>rect[1]&&rect[0]>=0&&rect[1]>=0&&rect[2]<=16384&&rect[3]<=16384;
    if(!eligible)return ui_original_subtitle(manager,canvas,rectangle);
    ++counts.eligible;const auto invocation=++ui_invocation_counter;
    gears_ui_binding::Binding binding{};const bool bound=ui_capture_binding(binding);
    if(bound)ui_binding_begin(invocation,binding);
    const bool captured=gears_ui_text_batch::snapshot_fcanvas({base},safe_copy,reinterpret_cast<uintptr_t>(canvas),before);
    if(!captured)++counts.before_rejected;
    UiTextCollectScope collecting;
    const auto result=ui_original_subtitle(manager,canvas,rectangle);
    gears_ui_binding::Binding after{};const bool valid=bound&&ui_capture_binding(after)&&gears_ui_binding::same(binding,after);
    if(bound)ui_binding_finish(invocation,valid);
    if(captured&&recording.load()&&valid){
        gears_ui_text_batch::Delta delta{};
        if(gears_ui_text_batch::delta_fcanvas({base},safe_copy,before,reinterpret_cast<uintptr_t>(canvas),delta))
            publish_ui_text_delta(delta,3,invocation,gears_ui_receipts::Source::NativeSubtitle,1);
        else ++counts.delta_rejected;
    }
    return result;
}

bool reserve_ui_subtitle(){
    ui_subtitle_target=reinterpret_cast<void*>(base+0x2a4370);
    std::array<unsigned char,32> before{},after{};
    if(!safe_copy(before.data(),ui_subtitle_target,before.size())||before!=ui_subtitle_bytes||
       MH_CreateHook(ui_subtitle_target,reinterpret_cast<void*>(hook_ui_subtitle),reinterpret_cast<void**>(&ui_original_subtitle))!=MH_OK||
       !safe_copy(after.data(),ui_subtitle_target,after.size())||after!=before)return false;
    return true;
}
void print_ui_text_observer(FILE* status){
    for(unsigned i=0;i<ui_text_counts.size();++i){auto const& c=ui_text_counts[i];
        fprintf(status,"ui_text_ranges source=%u calls=%llu eligible=%llu before_rejected=%llu delta_rejected=%llu empty=%llu published_calls=%llu published_ranges=%llu published_vertices=%llu publish_rejected=%llu source_vertices_modified=0 subtitle_pixels_verified=0\n",
            i,c.calls.load(),c.eligible.load(),c.before_rejected.load(),c.delta_rejected.load(),c.empty.load(),
            c.published_calls.load(),c.published_ranges.load(),c.published_vertices.load(),c.publish_rejected.load());
    }
}
