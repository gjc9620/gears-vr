// Optional structural observation plus a separately gated source transform.
// Never publishes semantic/UI GPU receipts.
#include "ui_scene_root_candidate.h"
#include "ui_scene_batch.h"
#include "ui_scene_transform_trial_live.inl"
#include "path_choice_position_trial_live.inl"
#include "path_choice_layout_observer.inl"
#if defined(GEARS_PATH_CHOICE_LAYOUT_OBSERVER) && GEARS_PATH_CHOICE_LAYOUT_OBSERVER
bool install(void*,void*,void**);
namespace path_choice_layout_observer {
bool install_child_hook(){return install(child_target,reinterpret_cast<void*>(child_hook),reinterpret_cast<void**>(&child_original));}
}
#endif
namespace ui_scene_root_live {
#if defined(GEARS_UI_SCENE_ROOT_OBSERVER) && GEARS_UI_SCENE_ROOT_OBSERVER
using Exec=uintptr_t(__fastcall*)(void*,void*,void*);
Exec original{};void* target{};bool reserved{};
#include "startup_ui_draw_observer.inl"
inline constexpr unsigned row_limit=8;
struct Row {
    uintptr_t client{},fcanvas{},scene{},caller{};uint64_t invocation{};
    long long enter{},exit{};unsigned thread{},simple_executes{},material_executes{};
    unsigned before_keys{},before_items{},before_vertices{},simple_ranges{},added_vertices{},materials{};
    bool before_valid{},after_candidate{},delta_valid{},before_matrix{},after_matrix{},matrix_restored{};
};
std::array<Row,row_limit> rows{};std::atomic<unsigned> slots{};
std::atomic<uint64_t> calls{},candidates{},rejected{},dropped{};
// One small read-only layout record per distinct root/matrix. The original
// first-eight batch inventories are usually consumed by HUD scenes before a
// pause menu opens; do not repeat those expensive inventories per frame.
struct LayoutRow {
    uintptr_t client{},canvas{},scene{};uint64_t world{},invocation{};long long at{};
    unsigned depth{};std::array<unsigned char,64> client_matrix{},canvas_matrix{};
};
SRWLOCK layout_lock=SRWLOCK_INIT;
std::array<LayoutRow,24> layout_rows{};unsigned layout_count{};
void observe_layout(gears_ui_scene_root::Candidate const& c,uint64_t invocation){
    gears_ui_scene_root::MatrixSnapshot canvas{};
    LayoutRow r{};r.client=c.client;r.canvas=c.fcanvas;r.scene=c.scene;
    r.world=committed_world_generation.load();r.invocation=invocation;r.at=gears_vr::monotonic_ns();
    std::array<unsigned char,64> again{};
    if(!gears_ui_scene_root::matrix_stack(safe_copy,c.fcanvas,canvas)||
       !safe_copy(r.client_matrix.data(),reinterpret_cast<void const*>(c.client+0xb0),r.client_matrix.size())||
       !safe_copy(again.data(),reinterpret_cast<void const*>(c.client+0xb0),again.size())||again!=r.client_matrix)return;
    r.depth=unsigned(canvas.stack.count);std::copy_n(canvas.top.begin(),64,r.canvas_matrix.begin());
    if(!TryAcquireSRWLockExclusive(&layout_lock))return;
    struct Unlock {~Unlock(){ReleaseSRWLockExclusive(&layout_lock);}} unlock;
    for(unsigned i=0;i<layout_count;++i){auto const& old=layout_rows[i];
        if(old.client==r.client&&old.scene==r.scene&&old.world==r.world&&old.depth==r.depth&&
           old.client_matrix==r.client_matrix&&old.canvas_matrix==r.canvas_matrix)return;}
    if(layout_count<layout_rows.size())layout_rows[layout_count++]=r;
}
thread_local Row* current{};
void executing(unsigned kind){ui_logical_group_live::executing(kind);
    if(current){if(kind==1)++current->simple_executes;else if(kind==2)++current->material_executes;}}
void reset_tls(){current=nullptr;}
uintptr_t __fastcall hook(void* client,void* fcanvas,void* scene){
    ui_scene_trial_live::entering();
    if(stereo_publish_bypass)return original(client,fcanvas,scene);
    // Preserve original RAX bits: return semantics have not been established.
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    struct Restore {Row* previous=current;~Restore(){current=previous;}} restore;
    current=nullptr;Scope scope;
    if(startup_draw::armed.load(std::memory_order_acquire)&&prime_recording.load()&&!recording.load())
        return startup_draw::invoke(client,fcanvas,scene,caller);
    if(!scope.world()||!ui_observer_enabled||!recording.load())return original(client,fcanvas,scene);
    ++calls;gears_ui_scene_root::Candidate selected{};
    if(!gears_ui_scene_root::candidate({base},safe_copy,reinterpret_cast<uintptr_t>(client),
        reinterpret_cast<uintptr_t>(fcanvas),reinterpret_cast<uintptr_t>(scene),caller,selected)){
        ++rejected;return original(client,fcanvas,scene);}
    const auto invocation=++candidates;
    observe_layout(selected,invocation);
    ui_logical_group_live::Observe group(2,invocation,reinterpret_cast<uintptr_t>(scene),
        base+gears_ui_scene_root::scene_render_rva,0,reinterpret_cast<uintptr_t>(fcanvas),0);
    auto render=[&]{return path_choice_position_live::invoke(selected,[&]{
        return ui_scene_trial_live::invoke(selected,[&]{return original(client,fcanvas,scene);});});};
    auto observed_render=[&]{const auto result=render();if(group.observing()){gears_ui_scene_root::Candidate after{};
        group.finish(gears_ui_scene_root::candidate({base},safe_copy,reinterpret_cast<uintptr_t>(client),
            reinterpret_cast<uintptr_t>(fcanvas),reinterpret_cast<uintptr_t>(scene),caller,after));}return result;};
    // Bound expensive inventories across the whole session, not per frame.
    unsigned slot=slots.load();
    while(slot<row_limit&&!slots.compare_exchange_weak(slot,slot+1)){}
    if(slot>=row_limit){++dropped;return observed_render();}
    Row row{};row.client=reinterpret_cast<uintptr_t>(client);row.fcanvas=reinterpret_cast<uintptr_t>(fcanvas);
    row.scene=reinterpret_cast<uintptr_t>(scene);row.caller=caller;row.invocation=invocation;
    row.thread=GetCurrentThreadId();row.enter=now();
    gears_ui_scene_root::MatrixSnapshot before_matrix{},after_matrix{};
    row.before_matrix=gears_ui_scene_root::matrix_stack(safe_copy,row.fcanvas,before_matrix);
    gears_ui_scene_batch::Inventory before{};
    row.before_valid=gears_ui_scene_batch::inventory({base},safe_copy,row.fcanvas,before);
    if(row.before_valid){row.before_keys=unsigned(before.keys.size());
        for(auto const& key:before.keys)for(auto const& item:key.items){++row.before_items;row.before_vertices+=unsigned(item.vertices.size());}}
    current=&row;
    const uintptr_t result=observed_render(); // Exactly once, including either private trial.
    current=nullptr;row.exit=now();
    gears_ui_scene_root::Candidate after{};
    row.after_candidate=gears_ui_scene_root::candidate({base},safe_copy,row.client,row.fcanvas,row.scene,caller,after);
    row.after_matrix=gears_ui_scene_root::matrix_stack(safe_copy,row.fcanvas,after_matrix);
    row.matrix_restored=row.before_matrix&&row.after_matrix&&
        gears_ui_batch::detail::same(before_matrix.stack,after_matrix.stack)&&before_matrix.top==after_matrix.top;
    if(row.before_valid&&row.after_candidate){gears_ui_scene_batch::Delta delta{};
        row.delta_valid=gears_ui_scene_batch::delta({base},safe_copy,before,row.fcanvas,delta);
        if(row.delta_valid){row.simple_ranges=unsigned(delta.simple.size());row.materials=unsigned(delta.material.size());
            for(auto const& range:delta.simple)row.added_vertices+=unsigned(range.vertices.size());}}
    rows[slot]=row;return result;
}
bool reserve(){
    if(reserved||!gears_ui_scene_root::validate_adapter({base},safe_copy))return false;
    target=reinterpret_cast<void*>(base+gears_ui_scene_root::adapter_rva);
    if(!safe_copy(startup_draw::original_bytes.data(),target,startup_draw::original_bytes.size()))return false;
    if(MH_CreateHook(target,reinterpret_cast<void*>(hook),reinterpret_cast<void**>(&original))!=MH_OK||
       !gears_ui_scene_root::validate_adapter({base},safe_copy))return false;
    reserved=true;return true;
}
void print(FILE* status){
    const bool modified=ui_scene_trial_live::modified()||path_choice_position_live::modified();
    fprintf(status,"ui_scene_root compiled=1 observation_only=%u calls=%llu structural_candidates=%llu rejected=%llu rows=%u dropped=%llu semantic_identity_proven=0 geometry_published=0 source_modified=%u same_thread_execute_counts_only=1 flush_coverage_proven=0\n",
        modified?0u:1u,calls.load(),candidates.load(),rejected.load(),(std::min)(slots.load(),row_limit),dropped.load(),modified?1u:0u);
    for(unsigned i=0;i<(std::min)(slots.load(),row_limit);++i){auto const& r=rows[i];
        fprintf(status,"ui_scene_root_row row=%u invocation=%llu client=%p fcanvas=%p scene=%p caller=%p thread=%u enter=%lld exit=%lld before_valid=%u after_candidate=%u delta_valid=%u before_keys=%u before_items=%u before_vertices=%u simple_ranges=%u added_vertices=%u materials=%u simple_executes=%u material_executes=%u before_matrix=%u after_matrix=%u matrix_restored=%u\n",
            i,r.invocation,reinterpret_cast<void*>(r.client),reinterpret_cast<void*>(r.fcanvas),reinterpret_cast<void*>(r.scene),
            reinterpret_cast<void*>(r.caller),r.thread,r.enter,r.exit,r.before_valid,r.after_candidate,r.delta_valid,r.before_keys,r.before_items,
            r.before_vertices,r.simple_ranges,r.added_vertices,r.materials,r.simple_executes,r.material_executes,r.before_matrix,r.after_matrix,r.matrix_restored);}
    std::array<LayoutRow,24> layouts{};unsigned count{};
    if(TryAcquireSRWLockExclusive(&layout_lock)){count=layout_count;layouts=layout_rows;ReleaseSRWLockExclusive(&layout_lock);}
    for(unsigned i=0;i<count;++i){auto const& r=layouts[i];
        fprintf(status,"ui_scene_root_layout row=%u at_ns=%lld client=%p canvas=%p scene=%p world=%llu invocation=%llu depth=%u client_matrix=",
            i,r.at,reinterpret_cast<void*>(r.client),reinterpret_cast<void*>(r.canvas),reinterpret_cast<void*>(r.scene),r.world,r.invocation,r.depth);
        for(auto b:r.client_matrix)fprintf(status,"%02x",unsigned(b));fprintf(status," canvas_matrix=");
        for(auto b:r.canvas_matrix)fprintf(status,"%02x",unsigned(b));fprintf(status," semantic_identity_proven=0 source_modified=0\n");}
}
#else
namespace startup_draw {
bool remember(void*){return true;}
bool observing(){return false;}
bool reserved_target(void*){return false;}
struct ExecuteScope {};
uint64_t list_generation(uintptr_t,bool=false,bool=true){return 0;}
void gpu_draw(ID3D12GraphicsCommandList*,uintptr_t,bool,unsigned,unsigned){}
struct Submission {};
Submission submission_begin(ID3D12CommandQueue*,unsigned,ID3D12CommandList* const*){return {};}
void submitted(Submission const&){}
unsigned execute_begin(void*,uintptr_t){return 0;}
void execute_end(unsigned,uint32_t){}
unsigned flush_begin(void*,uintptr_t){return 0;}
void flush_end(unsigned,uintptr_t){}
bool queue(bool){return true;}
bool primed_bytes(void*,std::array<unsigned char,32>&){return false;}
bool disable(){return true;}
bool restored(){return true;}
void print(FILE*,bool=false){}
}
bool reserve(){return true;}
void executing(unsigned kind){ui_logical_group_live::executing(kind);}
void reset_tls(){}
void print(FILE* status){fprintf(status,"ui_scene_root compiled=0 observation_only=1\n");}
#endif
}
