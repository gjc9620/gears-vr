// Included after Scope. Diagnostic ownership stays with the existing VM owner.
namespace crosshair_live {
bool enabled{};
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
using ExecLet=std::uintptr_t(__fastcall*)(void*,void*,void*);
ExecLet original{};void* target{};bool reserved{};
bool reserve();
gears_crosshair_assignment::OwnedConfig config;
struct ThreadState {gears_crosshair_tick::Evidence evidence;uint64_t token{};uintptr_t frame{};
    gears_crosshair_tick::Binding binding{};long long begin{},assignment{};std::array<float,3> point{};};
thread_local ThreadState state;
std::atomic<uint64_t> roots{},root_rejected{},lets{},candidates{},snapshot_rejected{},completed{},completion_rejected{},sealed{},consumed{},scene_calls{},scene_binding_rejected{};
std::atomic<uint64_t> overhead_qpc{},maximum_overhead_qpc{},row_count{};
std::atomic<bool> budget_exceeded{};
struct Row {unsigned kind{},thread{};uint64_t world{},tick{};long long begin{},assignment{},finish{},scene{};uintptr_t controller{},pawn{},weapon{};std::array<float,3> point{};};
std::array<Row,32> rows{};
void cost(long long elapsed){if(elapsed<0)return;overhead_qpc.fetch_add(uint64_t(elapsed));
    if(qpc_frequency>0&&elapsed>qpc_frequency/200)budget_exceeded=true;
    auto previous=maximum_overhead_qpc.load();while(uint64_t(elapsed)>previous&&!maximum_overhead_qpc.compare_exchange_weak(previous,uint64_t(elapsed))){} }
uintptr_t address(unsigned index){return config.identities.identities[index].address;}
bool root_binding(unsigned index){
    if(index>=config.tick_root_count)return false;
    auto const& root=config.tick_roots[index];gears_ui::Registry script{};uintptr_t entry{};
    return safe_copy(&script,reinterpret_cast<void*>(address(root.function)+0x8c),sizeof(script))&&
        script.data==root.script&&script.count==int(root.size)&&script.count<=script.capacity&&script.capacity<=131072&&
        safe_copy(&entry,reinterpret_cast<void*>(address(root.function)+0x114),8)&&entry==base+0x1d6ca0;
}
bool load(const wchar_t* ini){
    auto read=[&](const wchar_t* key,wchar_t* out,unsigned capacity){return GetPrivateProfileStringW(L"CrosshairAssignment",key,L"",out,capacity,ini);};
    gears_crosshair_assignment::OwnedConfig candidate;
    if(!gears_crosshair_assignment::parse_config(read,GetCurrentProcessId(),base,candidate)||candidate.tick_root_count!=2||
       candidate.identities.identities[candidate.assignment.controller].address!=expected_pc||
       !gears_crosshair_assignment::validate_bindings(candidate.assignment,safe_copy,true))return false;
    config=std::move(candidate);
    // Reserve only this explicitly enabled observer. The ordinary UI reserve
    // path must not acquire a dependency on an unused global opcode handler.
    return root_binding(0)&&root_binding(1)&&reserve();
}
void append(unsigned kind,gears_crosshair_tick::Sealed const& value,long long scene){
    const auto index=row_count.fetch_add(1);if(index>=rows.size())return;
    rows[index]={kind,GetCurrentThreadId(),value.binding.world,value.tick,value.begin_qpc,value.assignment_qpc,value.finish_qpc,scene,
        value.binding.controller,value.binding.pawn,value.binding.weapon,value.point};
}
struct TickScope {
    bool entered{};uint64_t token{},previous_token{},world{};uintptr_t previous_frame{};int exceptions{};
    TickScope(void* self,void* frame){
        if(!enabled||budget_exceeded.load()||!recording.load()||reinterpret_cast<uintptr_t>(self)!=address(config.assignment.controller)||!frame)return;
        uintptr_t node{},object{},code{};
        if(!safe_copy(&node,static_cast<unsigned char*>(frame)+0x18,8))return;
        unsigned index=0;while(index<config.tick_root_count&&node!=address(config.tick_roots[index].function))++index;
        if(index==config.tick_root_count)return;
        ++roots;const auto started=now();auto const& root=config.tick_roots[index];
        const bool valid=safe_copy(&object,static_cast<unsigned char*>(frame)+0x20,8)&&object==reinterpret_cast<uintptr_t>(self)&&
            safe_copy(&code,static_cast<unsigned char*>(frame)+0x28,8)&&code>=root.script&&code-root.script<root.size&&
            root_binding(index)&&gears_crosshair_assignment::validate_bindings(config.assignment,safe_copy,false);
        cost(now()-started);if(!valid){++root_rejected;state.evidence.invalidate();return;}
        previous_token=state.token;previous_frame=state.frame;
        world=committed_world_generation.load();
        state.binding={reinterpret_cast<uintptr_t>(self),address(config.assignment.pawn),address(config.assignment.weapon),world,reinterpret_cast<uintptr_t>(frame)};
        state.begin=now();state.assignment=0;state.point={};token=state.evidence.begin(state.binding,state.begin);
        state.token=token;state.frame=reinterpret_cast<uintptr_t>(frame);entered=true;exceptions=std::uncaught_exceptions();
    }
    ~TickScope(){if(!entered)return;const auto started=now();
        const bool valid=!budget_exceeded.load()&&std::uncaught_exceptions()==exceptions&&recording.load()&&world==committed_world_generation.load()&&
            gears_crosshair_assignment::validate_bindings(config.assignment,safe_copy,false);
        const auto finished=now();
        if(state.evidence.finish(token,valid,finished)){++sealed;
            append(0,{state.binding,state.point,token,state.begin,state.assignment,finished},0);}
        cost(now()-started);state.token=previous_token;state.frame=previous_frame;
    }
};
bool ancestry(uintptr_t frame){
    std::array<uintptr_t,12> seen{};unsigned count=0;
    while(frame&&count<seen.size()){
        for(unsigned i=0;i<count;++i)if(seen[i]==frame)return false;
        seen[count++]=frame;if(frame==state.frame)return true;
        if(frame>(std::numeric_limits<uintptr_t>::max)()-0x38)return false;
        if(!safe_copy(&frame,reinterpret_cast<void*>(frame+0x38),8))return false;
    }return false;
}
std::uintptr_t __fastcall hook(void* self,void* frame,void* result){
    // Every path invokes the original exactly once and returns its opaque RAX.
    Scope scope;if(!scope.world()||!enabled||budget_exceeded.load()||!recording.load()||!state.token)return original(self,frame,result);
    ++lets;if(!frame)return original(self,frame,result);
    // execLet's incoming RCX is opaque/unused; the authenticated caller object
    // comes from FFrame.Object. Preserve RCX unchanged for the original call.
    uintptr_t node{},code{},object{};
    if(!safe_copy(&node,static_cast<unsigned char*>(frame)+0x18,8)||node!=address(config.assignment.writer)||
       !safe_copy(&code,static_cast<unsigned char*>(frame)+0x28,8)||code!=config.assignment.script+0x35a||
       !safe_copy(&object,static_cast<unsigned char*>(frame)+0x20,8)||object!=address(config.assignment.controller))return original(self,frame,result);
    ++candidates;const auto before=now();gears_crosshair_assignment::Snapshot snapshot{};
    const bool valid=ancestry(reinterpret_cast<uintptr_t>(frame))&&
        gears_crosshair_assignment::snapshot(config.assignment,safe_copy,object,reinterpret_cast<uintptr_t>(frame),snapshot);
    cost(now()-before);
    if(!valid){++snapshot_rejected;state.evidence.invalidate();return original(self,frame,result);}
    const auto value=original(self,frame,result);const auto after=now();
    gears_crosshair_assignment::CompletedAssignmentReceipt receipt{};
    if(gears_crosshair_assignment::completed(config.assignment,safe_copy,snapshot,receipt)){
        ++completed;state.assignment=now();state.point=receipt.point;state.evidence.assignment(state.token,receipt,state.assignment);
    }else {++completion_rejected;state.evidence.invalidate();}
    cost(now()-after);return value;
}
bool reserve(){
    if(reserved)return false;target=reinterpret_cast<void*>(base+0x1d7ad0);
    std::array<unsigned char,32> before{},after{};
    if(!safe_copy(before.data(),target,before.size())||before!=gears_crosshair_assignment::execlet_entry||
       MH_CreateHook(target,reinterpret_cast<void*>(hook),reinterpret_cast<void**>(&original))!=MH_OK||
       !safe_copy(after.data(),target,after.size())||after!=before)return false;
    reserved=true;return true;
}
void scene(void* local){
    if(!enabled||budget_exceeded.load()||!recording.load()||reinterpret_cast<uintptr_t>(local)!=expected_local)return;
    ++scene_calls;const auto stamp=now();auto value=state.evidence.consume(committed_world_generation.load(),stamp);
    if(value){const auto before=now();const bool valid=gears_crosshair_assignment::validate_bindings(config.assignment,safe_copy,false);cost(now()-before);
        if(valid){++consumed;append(1,*value,stamp);}else ++scene_binding_rejected;}
}
void print(FILE* out){
    fprintf(out,"crosshair_assignment_observer enabled=%u compiled=1 roots=%llu root_rejected=%llu lets_in_tick=%llu candidates=%llu snapshot_rejected=%llu completed=%llu completion_rejected=%llu sealed=%llu scene_calls=%llu scene_consumed=%llu overhead_qpc=%llu maximum_overhead_qpc=%llu qpc_frequency=%lld rows=%llu camera_equivalence_proven=0 ticket_association_proven=0 coordinates_modified=0\n",
        enabled?1:0,roots.load(),root_rejected.load(),lets.load(),candidates.load(),snapshot_rejected.load(),completed.load(),completion_rejected.load(),
        sealed.load(),scene_calls.load(),consumed.load(),overhead_qpc.load(),maximum_overhead_qpc.load(),qpc_frequency,(std::min)(row_count.load(),uint64_t(rows.size())));
    fprintf(out,"crosshair_assignment_scene_binding_rejected=%llu\n",scene_binding_rejected.load());
    fprintf(out,"crosshair_assignment_budget_exceeded=%u per_observation_budget_ms=5\n",budget_exceeded.load()?1:0);
    if(active.load())return;
    for(unsigned i=0;i<(std::min)(row_count.load(),uint64_t(rows.size()));++i){auto const& r=rows[i];
        fprintf(out,"crosshair_assignment_row index=%u kind=%u thread=%u world=%llu tick=%llu tick_begin_qpc=%lld assignment_qpc=%lld tick_finish_qpc=%lld scene_qpc=%lld controller=%p pawn=%p weapon=%p point=%.9g,%.9g,%.9g\n",
            i,r.kind,r.thread,r.world,r.tick,r.begin,r.assignment,r.finish,r.scene,reinterpret_cast<void*>(r.controller),reinterpret_cast<void*>(r.pawn),reinterpret_cast<void*>(r.weapon),r.point[0],r.point[1],r.point[2]);}
}
#else
bool load(const wchar_t*){return false;}
struct TickScope {TickScope(void*,void*){}};
void scene(void*){}
void print(FILE* out){fprintf(out,"crosshair_assignment_observer enabled=%u compiled=0 coordinates_modified=0\n",enabled?1:0);}
#endif
}
