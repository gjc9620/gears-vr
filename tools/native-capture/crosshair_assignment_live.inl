// Included after Scope. Diagnostic ownership stays with the existing VM owner.
namespace crosshair_live {
bool enabled{};
#if defined(GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER) && GEARS_CROSSHAIR_ASSIGNMENT_OBSERVER
using ExecLet=std::uintptr_t(__fastcall*)(void*,void*,void*);
ExecLet original{};void* target{};bool reserved{};
bool reserve();
gears_crosshair_assignment::OwnedConfig config;
struct ThreadState {gears_crosshair_tick::Evidence evidence;uint64_t token{};uintptr_t frame{};
    gears_crosshair_tick::Binding binding{};long long begin{},assignment{};std::array<float,3> point{};
    gears_crosshair_assignment::OwnedConfig owned{};bool prepared{};uint64_t generation{};};
thread_local ThreadState state;
std::atomic<uint64_t> config_generation{};
const gears_crosshair_assignment::OwnedConfig& current(){
    if(state.generation!=config_generation.load()){state=ThreadState{};state.generation=config_generation.load();}
    return state.prepared&&state.generation==config_generation.load()?state.owned:config;}
std::atomic<uint64_t> weapon_rebinds{},weapon_rejected{};
std::atomic<uint64_t> world_certificates{};
std::atomic<uint64_t> roots{},root_rejected{},lets{},candidates{},snapshot_rejected{},completed{},completion_rejected{},sealed{},consumed{},scene_calls{},scene_binding_rejected{};
std::atomic<uint64_t> overhead_qpc{},maximum_overhead_qpc{},row_count{};
std::atomic<bool> budget_exceeded{};
struct Row {unsigned kind{},thread{};uint64_t world{},tick{};long long begin{},assignment{},finish{},scene{};uintptr_t controller{},pawn{},weapon{};std::array<float,3> point{};};
std::array<Row,32> rows{};
std::atomic<uint64_t> anchor_associated{},anchor_association_rejected{},ticket_registered{},association_row_count{};
std::array<std::atomic<uint64_t>,6> finite_reasons{};
std::atomic<uint64_t> finite_registered{};
struct DiagnosticPose {gears_vr::StereoMatrix view{},projection{};uint64_t ticket{},world{},tick{};unsigned thread{};};
struct AssociationRow {gears_crosshair_scene::Metadata data{};gears_vr::StereoMatrix view{},projection{};
    DiagnosticPose previous{};bool previous_valid{},finite{};};
std::array<AssociationRow,32> association_rows{};
void cost(long long elapsed){if(elapsed<0)return;overhead_qpc.fetch_add(uint64_t(elapsed));
    if(qpc_frequency>0&&elapsed>qpc_frequency/200)budget_exceeded=true;
    auto previous=maximum_overhead_qpc.load();while(uint64_t(elapsed)>previous&&!maximum_overhead_qpc.compare_exchange_weak(previous,uint64_t(elapsed))){} }
uintptr_t address(unsigned index){return current().identities.identities[index].address;}
bool root_binding(gears_crosshair_assignment::OwnedConfig const& config,unsigned index){
    if(index>=config.tick_root_count||index>=config.tick_roots.size())return false;
    auto const& root=config.tick_roots[index];gears_ui::Registry script{};uintptr_t entry{};
    if(root.function>=config.identities.identity_count||root.function>=config.identities.identities.size())return false;
    const auto function=config.identities.identities[root.function].address;
    return safe_copy(&script,reinterpret_cast<void*>(function+0x8c),sizeof(script))&&
        script.data==root.script&&script.count==int(root.size)&&script.count<=script.capacity&&script.capacity<=131072&&
        safe_copy(&entry,reinterpret_cast<void*>(function+0x114),8)&&entry==base+0x1d6ca0;
}
bool root_binding(unsigned index){return root_binding(current(),index);}
bool recovery_fresh(gears_crosshair_assignment::OwnedConfig const&,gears_anchor::Config const&,gears_ui::Config const&,gears_anchor::GuardDiagnostic* diagnostic=nullptr);
bool load(const wchar_t* ini,FILE* status=nullptr){
    auto reject=[&](const char* stage,const char* reason="none"){
        if(status)fprintf(status,"crosshair_assignment_activation accepted=0 stage=%s reason=%s\n",stage,reason);
        return false;};
    auto read=[&](const wchar_t* key,wchar_t* out,unsigned capacity){return GetPrivateProfileStringW(L"CrosshairAssignment",key,L"",out,capacity,ini);};
    gears_crosshair_assignment::OwnedConfig candidate;
    if(!gears_crosshair_assignment::parse_config(read,GetCurrentProcessId(),base,candidate))return reject("parse_config");
    if(candidate.mounted_suspended){if(!recovery_fresh(candidate,anchor_config,ui_config))return reject("mounted_recovery");
        config=candidate;config_generation=committed_world_generation.load();return reserve()||reject("reserve");}
    if(candidate.tick_root_count!=2)return reject("tick_root_count");
    if(candidate.identities.identities[candidate.assignment.controller].address!=expected_pc)return reject("controller");
    gears_crosshair_assignment::ValidationFailure failure{};
    gears_crosshair_assignment::OwnedConfig admitted;
    const auto started=GetTickCount64();unsigned attempts{};
    for(;;){
        ++attempts;
        if(gears_crosshair_assignment::prepare_activation(candidate,safe_copy,admitted,&failure))break;
        // Read-only acquisition only: no hook is reserved/enabled in this loop.
        // Every attempt starts from the immutable certificate and revalidates
        // script/code/identity proofs, including the original execLet bytes.
        if(GetTickCount64()-started>=1000){
            if(status)fprintf(status,"crosshair_assignment_startup accepted=0 attempts=%u elapsed_ms=%llu\n",attempts,GetTickCount64()-started);
            return reject("validate_bindings",gears_crosshair_assignment::validation_failure_name(failure));
        }
        Sleep(10);
    }
    if(status)fprintf(status,"crosshair_assignment_startup accepted=1 attempts=%u elapsed_ms=%llu weapon_rebound=%u\n",
        attempts,GetTickCount64()-started,
        !gears_anchor::equal_identity(candidate.identities.identities[candidate.assignment.weapon],admitted.identities.identities[admitted.assignment.weapon]));
    candidate=std::move(admitted);
    config=std::move(candidate);
    config_generation=committed_world_generation.load();
    // Reserve only this explicitly enabled observer. The ordinary UI reserve
    // path must not acquire a dependency on an unused global opcode handler.
    if(!root_binding(0))return reject("root0");
    if(!root_binding(1))return reject("root1");
    return reserve()||reject("reserve");
}
void append(unsigned kind,gears_crosshair_tick::Sealed const& value,long long scene){
    const auto index=row_count.fetch_add(1);if(index>=rows.size())return;
    rows[index]={kind,GetCurrentThreadId(),value.binding.world,value.tick,value.begin_qpc,value.assignment_qpc,value.finish_qpc,scene,
        value.binding.controller,value.binding.pawn,value.binding.weapon,value.point};
}
bool prepare_weapon(){
    if(state.prepared&&state.generation==config_generation.load()&&gears_crosshair_assignment::validate_bindings(current().assignment,safe_copy,false))return true;
    gears_crosshair_assignment::OwnedConfig candidate;
    if(!gears_crosshair_assignment::rebind_weapon(config,safe_copy,candidate)){++weapon_rejected;return false;}
    state.owned=std::move(candidate);state.generation=config_generation.load();state.prepared=true;++weapon_rebinds;return true;
}
bool same_object(gears_ui::Identity const& a,gears_ui::Identity const& b){
    return a.address==b.address&&a.index==b.index&&a.name_index==b.name_index&&a.number==b.number&&
        a.cls==b.cls&&a.outer==b.outer&&a.name==b.name;
}
bool recovery_fresh(gears_crosshair_assignment::OwnedConfig const& candidate,gears_anchor::Config const& anchor,gears_ui::Config const& ui,gears_anchor::GuardDiagnostic* diagnostic){
    if(diagnostic)*diagnostic={};
    if(candidate.mounted_suspended){
        return gears_crosshair_assignment::mounted_recovery_fresh(candidate,anchor,ui,GetCurrentProcessId(),base,safe_copy,diagnostic);
    }
    if(anchor.mounted_enabled||ui.mounted)return false;
    auto const& c=candidate.assignment;auto const& ids=candidate.identities;
    if(candidate.pid!=GetCurrentProcessId()||ids.base!=base||candidate.tick_root_count!=2||
       c.controller>=ids.identity_count||c.pawn>=ids.identity_count||c.weapon>=ids.identity_count||
       anchor.role_controller>=anchor.identity_count||anchor.role_pawn>=anchor.identity_count||anchor.role_camera>=anchor.identity_count||
       ui.role_controller>=ui.identity_count||ui.role_pawn>=ui.identity_count||
       !same_object(ids.identities[c.controller],anchor.identities[anchor.role_controller])||
       !same_object(ids.identities[c.controller],ui.identities[ui.role_controller])||
       !same_object(ids.identities[c.pawn],anchor.identities[anchor.role_pawn])||
       !same_object(ids.identities[c.pawn],ui.identities[ui.role_pawn]))return false;
    auto camera=gears_ui::known(ids,anchor.identities[anchor.role_camera].address);
    if(!camera||!same_object(*camera,anchor.identities[anchor.role_camera])||
       !root_binding(candidate,0)||!root_binding(candidate,1)||
       !gears_crosshair_assignment::validate_bindings(c,safe_copy,false))return false;
    std::array<unsigned char,32> original_entry{};
    if(!read_attested_hook_entry(original_entry.data(),reinterpret_cast<void*>(base+0x1d7ad0),original_entry.size())||
       original_entry!=gears_crosshair_assignment::execlet_entry)return false;
    gears_crosshair_assignment::OwnedConfig checked;
    return gears_crosshair_assignment::rebind_weapon(candidate,safe_copy,checked)&&
        same_object(checked.identities.identities[checked.assignment.weapon],ids.identities[c.weapon])&&
        gears_crosshair_assignment::validate_bindings(c,safe_copy,false);
}
bool prepare_recovery(const wchar_t* ini,gears_anchor::Config const& anchor,gears_ui::Config const& ui,
    gears_crosshair_assignment::OwnedConfig& output){
    auto read=[&](const wchar_t* key,wchar_t* buffer,unsigned capacity){return GetPrivateProfileStringW(L"CrosshairAssignment",key,L"",buffer,capacity,ini);};
    gears_crosshair_assignment::OwnedConfig candidate;
    if(!gears_crosshair_assignment::parse_config(read,GetCurrentProcessId(),base,candidate)||!recovery_fresh(candidate,anchor,ui))return false;
    output=std::move(candidate);return true;
}
// Caller owns the closed world gate and has drained every prior native lease.
void publish_recovery(gears_crosshair_assignment::OwnedConfig const& candidate,uint64_t generation){
    config=candidate;config_generation=generation;++world_certificates;
}
struct TickScope {
    bool entered{};uint64_t token{},previous_token{},world{};uintptr_t previous_frame{};int exceptions{};
    TickScope(void* self,void* frame){
        if(!enabled||config.mounted_suspended||budget_exceeded.load()||!recording.load()||!frame)return;
        auto const& before_config=current();
        if(reinterpret_cast<uintptr_t>(self)!=address(before_config.assignment.controller))return;
        uintptr_t node{},object{},code{};
        if(!safe_copy(&node,static_cast<unsigned char*>(frame)+0x18,8))return;
        unsigned index=0;while(index<before_config.tick_root_count&&node!=address(before_config.tick_roots[index].function))++index;
        if(index==before_config.tick_root_count)return;
        state.evidence.invalidate(); // Also poison nested roots and failed rebind attempts.
        ++roots;const auto started=now();auto const& root=before_config.tick_roots[index];
        const bool root_valid=safe_copy(&object,static_cast<unsigned char*>(frame)+0x20,8)&&object==reinterpret_cast<uintptr_t>(self)&&
            safe_copy(&code,static_cast<unsigned char*>(frame)+0x28,8)&&code>=root.script&&code-root.script<root.size&&
            root_binding(index);
        const bool valid=root_valid&&(state.frame?
            gears_crosshair_assignment::validate_bindings(current().assignment,safe_copy,false):prepare_weapon());
        cost(now()-started);if(!valid){++root_rejected;state.evidence.invalidate();return;}
        auto const& config=current();
        previous_token=state.token;previous_frame=state.frame;
        world=committed_world_generation.load();
        state.binding={reinterpret_cast<uintptr_t>(self),address(config.assignment.pawn),address(config.assignment.weapon),world,reinterpret_cast<uintptr_t>(frame)};
        state.begin=now();state.assignment=0;state.point={};token=state.evidence.begin(state.binding,state.begin);
        state.token=token;state.frame=reinterpret_cast<uintptr_t>(frame);entered=true;exceptions=std::uncaught_exceptions();
    }
    ~TickScope(){if(!entered)return;const auto started=now();
        auto const& config=current();
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
    auto const& config=current();
    // Every path invokes the original exactly once and returns its opaque RAX.
    Scope scope;if(!scope.world()||!enabled||config.mounted_suspended||budget_exceeded.load()||!recording.load()||!state.token)return original(self,frame,result);
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
        if(receipt.view.valid){gears_anchor::Anchor anchor{};std::uint32_t stamp{};
            if(gears_anchor::guard(anchor_config,safe_copy,anchor,stamp)&&anchor.controller==receipt.controller&&anchor.pawn==receipt.pawn){
                receipt.view.camera_binding_valid=true;receipt.view.camera=anchor.camera;
                receipt.view.local=anchor_config.identities[anchor_config.role_local].address;
                receipt.view.level=anchor.level;receipt.view.assignment_stamp=stamp;}}
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
std::optional<gears_crosshair_scene::Metadata> scene(void* local){
    auto const& config=current();
    if(!enabled||config.mounted_suspended||budget_exceeded.load()||!recording.load()||reinterpret_cast<uintptr_t>(local)!=expected_local)return {};
    ++scene_calls;const auto stamp=now();auto value=state.evidence.consume(committed_world_generation.load(),stamp);
    if(value){const auto before=now();const bool valid=gears_crosshair_assignment::validate_bindings(config.assignment,safe_copy,false);cost(now()-before);
        if(valid){++consumed;append(1,*value,stamp);
            gears_crosshair_scene::Metadata result{};result.controller=value->binding.controller;
            result.pawn=value->binding.pawn;result.weapon=value->binding.weapon;result.root_frame=value->binding.root_frame;
            result.world=value->binding.world;result.tick=value->tick;result.thread=GetCurrentThreadId();
            result.begin_qpc=value->begin_qpc;result.assignment_qpc=value->assignment_qpc;
            result.finish_qpc=value->finish_qpc;result.scene_qpc=stamp;result.point=value->point;result.writer_view=value->view;return result;
        }else ++scene_binding_rejected;}
    return {};
}
void attach(std::optional<gears_crosshair_scene::Metadata> value,gears_anchor::Receipt& receipt){
    auto const& config=current();
    if(!value||config.mounted_suspended||receipt.anchor.unpossessed)return;
    const auto started=now();
    const bool valid=enabled&&!budget_exceeded.load()&&recording.load()&&
        value->controller==address(config.assignment.controller)&&value->pawn==address(config.assignment.pawn)&&
        value->weapon==address(config.assignment.weapon)&&
        gears_crosshair_assignment::validate_bindings(config.assignment,safe_copy,false)&&
        gears_crosshair_scene::associate(*value,receipt.key,receipt.invocation,receipt.thread,
            committed_world_generation.load(),receipt.anchor.controller,receipt.anchor.pawn);
    cost(now()-started);
    if(valid)receipt.crosshair=*value;else ++anchor_association_rejected;
}
void accepted(gears_anchor::Receipt const& receipt){if(receipt.crosshair)++anchor_associated;}
void freeze(gears_submitted::FrameTicket& frame,gears_anchor::Receipt const* anchor){
    auto const& config=current();
#if defined(GEARS_UI_RETICLE_FINITE_QUERY) && GEARS_UI_RETICLE_FINITE_QUERY
    if(!enabled||config.mounted_suspended||!frame.enabled||!frame.crosshair||!anchor||anchor->anchor.unpossessed||!recording.load()||budget_exceeded.load())return;
    const auto started=now();
    gears_crosshair_finite_query::Authority authority{
        anchor->anchor.controller,anchor->anchor.pawn,address(config.assignment.weapon),anchor->anchor.camera,
        anchor->key.local,anchor->anchor.level,anchor->key.family,anchor->key.source,anchor->key.owner,
        committed_world_generation.load(),anchor->invocation,GetCurrentThreadId(),anchor->key.stamp};
    const auto reason=gears_crosshair_assignment::validate_bindings(config.assignment,safe_copy,false)?
        gears_crosshair_finite_query::authorize(*frame.crosshair,authority,frame.source_view,now(),qpc_frequency,frame.finite_aim,&frame.aiming):
        gears_crosshair_finite_query::Reason::binding;
    ++finite_reasons[static_cast<unsigned>(reason)];cost(now()-started);
#else
    (void)frame;(void)anchor;
#endif
}
void registered(gears_submitted::FrameTicket const& frame){
    if(!frame.crosshair||!frame.id||frame.crosshair->ticket!=frame.id)return;
    if(frame.finite_aim.valid)++finite_registered;
    // Previous pose is retained for offline diagnostics only, never aim admission.
    thread_local DiagnosticPose previous{};
    const DiagnosticPose current{frame.source_view,frame.source_projection,frame.id,frame.crosshair->world,frame.crosshair->tick,GetCurrentThreadId()};
    const auto prior=previous;previous=current;
    const auto ordinal=ticket_registered.fetch_add(1);if(ordinal%8)return;
    const auto index=association_row_count.fetch_add(1);
    if(index<association_rows.size())association_rows[index]={*frame.crosshair,frame.source_view,frame.source_projection,prior,
        prior.ticket&&prior.ticket<current.ticket&&prior.world==current.world&&prior.thread==current.thread,frame.finite_aim.valid};
}
void print(FILE* out){
    fprintf(out,"crosshair_assignment_observer enabled=%u compiled=1 roots=%llu root_rejected=%llu lets_in_tick=%llu candidates=%llu snapshot_rejected=%llu completed=%llu completion_rejected=%llu sealed=%llu scene_calls=%llu scene_consumed=%llu overhead_qpc=%llu maximum_overhead_qpc=%llu qpc_frequency=%lld rows=%llu camera_equivalence_proven=0 ticket_association_proven=0 observer_query_coordinates_modified=0\n",
        enabled?1:0,roots.load(),root_rejected.load(),lets.load(),candidates.load(),snapshot_rejected.load(),completed.load(),completion_rejected.load(),
        sealed.load(),scene_calls.load(),consumed.load(),overhead_qpc.load(),maximum_overhead_qpc.load(),qpc_frequency,(std::min)(row_count.load(),uint64_t(rows.size())));
    fprintf(out,"crosshair_assignment_mounted_suspended=%u human_weapon_aim_used=0\n",config.mounted_suspended?1u:0u);
    fprintf(out,"crosshair_assignment_scene_binding_rejected=%llu\n",scene_binding_rejected.load());
    fprintf(out,"crosshair_assignment_budget_exceeded=%u per_observation_budget_ms=5\n",budget_exceeded.load()?1:0);
    fprintf(out,"crosshair_weapon_binding thread_local=1 rebinds=%llu rejected=%llu same_pawn_only=1 universal_object_aba_proven=0\n",weapon_rebinds.load(),weapon_rejected.load());
    fprintf(out,"crosshair_world_certificate generation=%llu commits=%llu\n",config_generation.load(),world_certificates.load());
    fprintf(out,"crosshair_finite_hud_query eligible=%llu binding_rejected=%llu clock_rejected=%llu age_rejected=%llu geometry_rejected=%llu registered=%llu camera_rejected=%llu source_camera_freshness_guard=1 max_assignment_and_tick_age_ms=50 camera_equivalence_proven=0 current_camera_aim_proven=0 bullet_impact_proven=0\n",
        finite_reasons[0].load(),finite_reasons[1].load(),finite_reasons[2].load(),finite_reasons[3].load(),finite_reasons[4].load(),finite_registered.load(),finite_reasons[5].load());
    if(active.load())return;
    fprintf(out,"crosshair_assignment_association anchors=%llu anchor_rejected=%llu tickets_registered=%llu rows=%llu camera_equivalence_proven=0 finite_aim_certified=0\n",
        anchor_associated.load(),anchor_association_rejected.load(),ticket_registered.load(),
        (std::min)(association_row_count.load(),uint64_t(association_rows.size())));
    for(unsigned i=0;i<(std::min)(association_row_count.load(),uint64_t(association_rows.size()));++i){auto const& r=association_rows[i].data;
        fprintf(out,"crosshair_assignment_ticket_row index=%u thread=%u world=%llu tick=%llu anchor_invocation=%llu ticket=%llu local=%p family=%p source=%p owner=%p stamp=%u controller=%p pawn=%p weapon=%p scene_qpc=%lld point=%.9g,%.9g,%.9g finite_query=%u\n",
            i,r.thread,r.world,r.tick,r.invocation,r.ticket,reinterpret_cast<void*>(r.local),reinterpret_cast<void*>(r.family),
            reinterpret_cast<void*>(r.source),reinterpret_cast<void*>(r.owner),r.stamp,reinterpret_cast<void*>(r.controller),
            reinterpret_cast<void*>(r.pawn),reinterpret_cast<void*>(r.weapon),r.scene_qpc,r.point[0],r.point[1],r.point[2],association_rows[i].finite?1:0);
        auto const& v=r.writer_view;
        fprintf(out,"crosshair_writer_view_row index=%u valid=%u cam_loc=%.9g,%.9g,%.9g cam_rot=%d,%d,%d start=%.9g,%.9g,%.9g direction=%.9g,%.9g,%.9g source_view=",
            i,v.valid?1:0,v.location[0],v.location[1],v.location[2],v.rotation[0],v.rotation[1],v.rotation[2],
            v.start[0],v.start[1],v.start[2],v.direction[0],v.direction[1],v.direction[2]);
        for(unsigned n=0;n<16;++n)fprintf(out,"%s%.9g",n?",":"",association_rows[i].view[n]);
        fprintf(out," source_projection=");
        for(unsigned n=0;n<16;++n)fprintf(out,"%s%.9g",n?",":"",association_rows[i].projection[n]);
        fprintf(out," camera_binding_valid=%u camera=%p level=%p assignment_stamp=%u scene_stamp=%u sample_stride=8",
            v.camera_binding_valid?1:0,reinterpret_cast<void*>(v.camera),reinterpret_cast<void*>(v.level),v.assignment_stamp,r.stamp);
        auto const& prior=association_rows[i].previous;
        fprintf(out," previous_valid=%u previous_ticket=%llu previous_world=%llu previous_tick=%llu previous_source_view=",
            association_rows[i].previous_valid?1:0,prior.ticket,prior.world,prior.tick);
        for(unsigned n=0;n<16;++n)fprintf(out,"%s%.9g",n?",":"",prior.view[n]);
        fprintf(out," previous_source_projection=");
        for(unsigned n=0;n<16;++n)fprintf(out,"%s%.9g",n?",":"",prior.projection[n]);
        fprintf(out,"\n");
    }
    for(unsigned i=0;i<(std::min)(row_count.load(),uint64_t(rows.size()));++i){auto const& r=rows[i];
        fprintf(out,"crosshair_assignment_row index=%u kind=%u thread=%u world=%llu tick=%llu tick_begin_qpc=%lld assignment_qpc=%lld tick_finish_qpc=%lld scene_qpc=%lld controller=%p pawn=%p weapon=%p point=%.9g,%.9g,%.9g\n",
            i,r.kind,r.thread,r.world,r.tick,r.begin,r.assignment,r.finish,r.scene,reinterpret_cast<void*>(r.controller),reinterpret_cast<void*>(r.pawn),reinterpret_cast<void*>(r.weapon),r.point[0],r.point[1],r.point[2]);}
}
#else
bool load(const wchar_t*,FILE* status=nullptr){
    if(status)fprintf(status,"crosshair_assignment_activation accepted=0 stage=compile_disabled reason=none\n");
    return false;}
struct TickScope {TickScope(void*,void*){}};
std::optional<gears_crosshair_scene::Metadata> scene(void*){return {};}
void attach(std::optional<gears_crosshair_scene::Metadata>,gears_anchor::Receipt&){}
void accepted(gears_anchor::Receipt const&){}
void registered(gears_submitted::FrameTicket const&){}
void freeze(gears_submitted::FrameTicket&,gears_anchor::Receipt const*){}
void print(FILE* out){fprintf(out,"crosshair_assignment_observer enabled=%u compiled=0 observer_query_coordinates_modified=0\n",enabled?1:0);}
#endif
}
