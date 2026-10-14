// Included inside capture.cpp's anonymous namespace, after Scope/safe_copy.
// Observation only: original VM/Canvas arguments and draw counts are preserved.
bool ui_observer_enabled{},ui_reserved{};
gears_ui::Config ui_config;
using UiExec=void(__fastcall*)(void*,void*,void*);
std::array<UiExec,4> ui_original{};
std::array<void*,4> ui_targets{};
struct UiInvocation {gears_ui::ScopeContext context{};uintptr_t frame{};uint64_t sequence{};long long entered{};};
thread_local UiInvocation ui_invocation;
struct UiInvocationRestore {
    UiInvocation saved=ui_invocation;
    ~UiInvocationRestore(){ui_invocation=saved;}
};
struct UiRow {
    gears_ui::CanvasSample sample{};
    uint64_t invocation{},calls{},latest_built_ticket{};
    uintptr_t root_frame{},matched_frame{};unsigned matched_depth{};
    long long scope_qpc{},qpc{};unsigned thread{},draw{};
};
std::array<std::array<UiRow,48>,9> ui_rows{};
std::array<unsigned,9> ui_row_counts{};
std::array<long long,9> ui_next_sample{};
std::array<std::atomic<uint64_t>,9> ui_calls{},ui_rejected{},ui_dropped{};
std::array<std::atomic<uint64_t>,8> ui_scopes{},ui_scope_rejected{};
std::array<std::atomic<uint64_t>,8> ui_canvas_scopes{};
std::atomic<uint64_t> ui_chain_calls{},ui_chain_rejected{};
struct UiChainReject {uintptr_t root_frame{},frame{},node{},object{},code{},previous{};long long qpc{};unsigned draw{};};
UiChainReject ui_first_chain_reject;
struct UiScopeReject {uintptr_t self{},frame{},object{},code{};long long qpc{};unsigned thread{};};
std::array<UiScopeReject,8> ui_first_scope_reject{};
std::atomic<uint64_t> ui_vm_calls{},ui_invocation_counter{};
std::mutex ui_rows_mutex;

void observe_ui_canvas(unsigned draw,void* canvas,void* frame){
    if(!ui_observer_enabled||!recording.load()||!ui_invocation.context.kind)return;
    if(draw<1||draw>3)return;
    ++ui_chain_calls;
    gears_ui::ScopeContext selected{};uintptr_t matched_frame{};unsigned matched_depth{};
    if(!gears_ui::classify_canvas_scope(ui_config,safe_copy,ui_invocation.context,
            ui_invocation.frame,reinterpret_cast<uintptr_t>(frame),selected,matched_frame,&matched_depth)){
        if(ui_chain_rejected.fetch_add(1)==0){auto& r=ui_first_chain_reject;
            r.root_frame=ui_invocation.frame;r.frame=reinterpret_cast<uintptr_t>(frame);r.qpc=now();r.draw=draw;
            if(frame){safe_copy(&r.node,static_cast<unsigned char*>(frame)+0x18,8);
                safe_copy(&r.object,static_cast<unsigned char*>(frame)+0x20,8);
                safe_copy(&r.code,static_cast<unsigned char*>(frame)+0x28,8);
                safe_copy(&r.previous,static_cast<unsigned char*>(frame)+0x38,8);}
        }return;
    }
    const auto kind=selected.kind;if(kind<1||kind>3||selected.scope_index>=ui_canvas_scopes.size())return;
    ++ui_canvas_scopes[selected.scope_index];
    const unsigned bucket=(kind-1)*3+draw-1;
    const auto calls=++ui_calls[bucket];const auto timestamp=now();
    {
        std::lock_guard<std::mutex> lock(ui_rows_mutex);
        if(ui_row_counts[bucket]>=ui_rows[bucket].size()){++ui_dropped[bucket];return;}
        if(calls>12&&timestamp<ui_next_sample[bucket])return;
        ui_next_sample[bucket]=timestamp+qpc_frequency/2;
    }
    UiRow row{};
    if(!gears_ui::sample_canvas(ui_config,safe_copy,selected,
            reinterpret_cast<uintptr_t>(canvas),reinterpret_cast<uintptr_t>(frame),row.sample)){
        ++ui_rejected[bucket];return;
    }
    row.invocation=ui_invocation.sequence;row.scope_qpc=ui_invocation.entered;
    row.root_frame=ui_invocation.frame;row.matched_frame=matched_frame;row.matched_depth=matched_depth;
    row.qpc=timestamp;row.thread=GetCurrentThreadId();row.draw=draw;row.calls=calls;
    // A temporal hint only. This does not associate a Canvas draw with a GPU ticket.
    row.latest_built_ticket=stereo_ticket_counter.load();
    std::lock_guard<std::mutex> lock(ui_rows_mutex);
    if(ui_row_counts[bucket]<ui_rows[bucket].size())ui_rows[bucket][ui_row_counts[bucket]++]=row;
    else ++ui_dropped[bucket];
}
void __fastcall hook_ui_vm(void* self,void* frame,void* result){
    Scope active_scope;
    if(ui_observer_enabled&&recording.load()){
        ++ui_vm_calls;uintptr_t node{};
        if(frame&&safe_copy(&node,static_cast<unsigned char*>(frame)+0x18,sizeof(node))){
            for(unsigned i=0;i<ui_config.scope_count;++i){
                if(node!=ui_config.identities[ui_config.scopes[i].function].address)continue;
                gears_ui::ScopeContext context{};
                if(gears_ui::match_scope(ui_config,safe_copy,reinterpret_cast<uintptr_t>(self),reinterpret_cast<uintptr_t>(frame),context)){
                    ++ui_scopes[i];UiInvocationRestore restore;
                    ui_invocation={context,reinterpret_cast<uintptr_t>(frame),++ui_invocation_counter,now()};
                    ui_original[0](self,frame,result);return;
                }
                if(ui_scope_rejected[i].fetch_add(1)==0){auto& row=ui_first_scope_reject[i];
                    row.self=reinterpret_cast<uintptr_t>(self);row.frame=reinterpret_cast<uintptr_t>(frame);row.qpc=now();row.thread=GetCurrentThreadId();
                    safe_copy(&row.object,static_cast<unsigned char*>(frame)+0x20,8);
                    safe_copy(&row.code,static_cast<unsigned char*>(frame)+0x28,8);
                }
                // A rejected recognized child must not be mislabeled as its parent HUD.
                UiInvocationRestore restore;ui_invocation={};ui_original[0](self,frame,result);return;
            }
        }
    }
    ui_original[0](self,frame,result);
}
void __fastcall hook_ui_tile(void* self,void* frame,void* result){Scope active_scope;observe_ui_canvas(1,self,frame);ui_original[1](self,frame,result);}
void __fastcall hook_ui_material(void* self,void* frame,void* result){Scope active_scope;observe_ui_canvas(2,self,frame);ui_original[2](self,frame,result);}
void __fastcall hook_ui_text(void* self,void* frame,void* result){Scope active_scope;observe_ui_canvas(3,self,frame);ui_original[3](self,frame,result);}
const std::array<UiExec,4> ui_detours{hook_ui_vm,hook_ui_tile,hook_ui_material,hook_ui_text};

bool reserve_ui_hooks(){
    if(ui_reserved)return false;
    for(unsigned i=0;i<4;++i){
        ui_targets[i]=reinterpret_cast<void*>(base+gears_ui::binding_rvas[i]);
        std::array<unsigned char,16> before{},after{};
        if(!safe_copy(before.data(),ui_targets[i],before.size())||before!=gears_ui::binding_bytes[i]||
           MH_CreateHook(ui_targets[i],reinterpret_cast<void*>(ui_detours[i]),reinterpret_cast<void**>(&ui_original[i]))!=MH_OK||
           !safe_copy(after.data(),ui_targets[i],after.size())||after!=before)return false;
    }
    ui_reserved=true;return true;
}

bool load_ui_config(const wchar_t* ini){
    auto load=[&](const wchar_t* key,auto& out){
        wchar_t text[64]{};GetPrivateProfileStringW(L"UIObserver",key,L"",text,64,ini);
        if(!*text||*text==L'-'||*text==L'+')return false;
        wchar_t* end{};auto value=_wcstoui64(text,&end,0);
        using T=std::remove_reference_t<decltype(out)>;
        if(!end||*end||value>static_cast<unsigned long long>((std::numeric_limits<T>::max)()))return false;
        out=static_cast<T>(value);return true;
    };
    unsigned schema{},pid{};wchar_t sha[66]{};
    GetPrivateProfileStringW(L"UIObserver",L"sha256",L"",sha,66,ini);
    if(!load(L"schema",schema)||schema!=1||!load(L"pid",pid)||pid!=GetCurrentProcessId()||
       wcscmp(sha,L"bdc6705d18ddb335b767df0b38e256ba5da0f49449c8970b302118bb7a4a8fa4"))return false;
    const std::pair<const wchar_t*,unsigned> layout[]={{L"frame_node",0x18},{L"frame_object",0x20},
        {L"frame_code",0x28},{L"frame_locals",0x30},{L"frame_previous",0x38},{L"function_script",0x8c},{L"function_entry",0x114}};
    for(auto const& value:layout){unsigned actual{};if(!load(value.first,actual)||actual!=value.second)return false;}
    auto& c=ui_config;
#define UI_WIDEN_(s) L##s
#define UI_WIDEN(s) UI_WIDEN_(s)
#define UI_LOAD(field) if(!load(UI_WIDEN(#field),c.field))return false;
    UI_LOAD(base) UI_LOAD(objects_header) UI_LOAD(names_header)
    UI_LOAD(object_internal_index) UI_LOAD(object_name) UI_LOAD(object_class) UI_LOAD(object_outer)
    UI_LOAD(field_super) UI_LOAD(name_entry_index) UI_LOAD(name_entry_text) UI_LOAD(property_offset_internal)
    UI_LOAD(identity_count) UI_LOAD(super_count) UI_LOAD(scope_count)
    UI_LOAD(role_controller) UI_LOAD(role_camera) UI_LOAD(role_pawn) UI_LOAD(role_hud) UI_LOAD(role_inventory) UI_LOAD(role_canvas_class)
#define UI_FIELD(field) UI_LOAD(field) UI_LOAD(field##_property)
    GEARS_UI_FIELDS(UI_FIELD)
#undef UI_FIELD
#undef UI_LOAD
#undef UI_WIDEN
#undef UI_WIDEN_
    if(c.base!=base||!c.identity_count||c.identity_count>c.identities.size()||c.super_count>c.supers.size()||
       !c.scope_count||c.scope_count>c.scopes.size())return false;
    for(unsigned i=0;i<c.identity_count;++i){auto& id=c.identities[i];wchar_t key[64]{};
        auto field=[&](const wchar_t* label,auto& value){swprintf_s(key,L"id%u_%s",i,label);return load(key,value);};
        if(!field(L"address",id.address)||!field(L"index",id.index)||!field(L"name_index",id.name_index)||
           !field(L"number",id.number)||!field(L"cls",id.cls)||!field(L"outer",id.outer))return false;
        swprintf_s(key,L"id%u_name",i);wchar_t name[65]{};
        auto n=GetPrivateProfileStringW(L"UIObserver",key,L"",name,65,ini);
        if(!n||n>=64)return false;std::copy(name,name+n+1,id.name.begin());
    }
    for(unsigned i=0;i<c.super_count;++i){wchar_t key[64]{};auto& value=c.supers[i];
        swprintf_s(key,L"super%u_identity",i);if(!load(key,value.identity)||value.identity>=c.identity_count)return false;
        swprintf_s(key,L"super%u_parent",i);if(!load(key,value.parent))return false;
        c.identities[value.identity].check_super=1;c.identities[value.identity].super=value.parent;
    }
    for(unsigned i=0;i<c.scope_count;++i){wchar_t key[64]{};auto& s=c.scopes[i];
        auto field=[&](const wchar_t* name,auto& value){swprintf_s(key,L"scope%u_%s",i,name);return load(key,value);};
        if(!field(L"kind",s.kind)||!field(L"owner",s.owner)||!field(L"function",s.function)||!field(L"script",s.script)||!field(L"size",s.size))return false;
    }
    const wchar_t* names[]={L"vm",L"tile",L"material",L"text"};
    for(unsigned i=0;i<4;++i){auto& b=c.bindings[i];wchar_t key[64]{},hex[34]{};
        swprintf_s(key,L"binding_%s_rva",names[i]);if(!load(key,b.rva)||b.rva!=gears_ui::binding_rvas[i])return false;
        if(i){swprintf_s(key,L"binding_%s_function",names[i]);if(!load(key,b.function))return false;}
        swprintf_s(key,L"binding_%s_bytes",names[i]);if(GetPrivateProfileStringW(L"UIObserver",key,L"",hex,34,ini)!=32)return false;
        for(unsigned j=0;j<16;++j){wchar_t expected[3]{};swprintf_s(expected,L"%02x",gears_ui::binding_bytes[i][j]);
            if(hex[j*2]!=expected[0]||hex[j*2+1]!=expected[1])return false;b.bytes[j]=gears_ui::binding_bytes[i][j];}
    }
    return c.role_controller<c.identity_count&&c.role_camera<c.identity_count&&
        c.identities[c.role_controller].address==expected_pc&&c.identities[c.role_camera].address==expected_camera&&
        gears_ui::validate(c,safe_copy);
}
void print_ui_observer(FILE* status,bool disabled){
    fprintf(status,"ui_observer enabled=%u available=%u coordinates_modified=0 vm_calls=%llu qpc_frequency=%lld\n",
        ui_observer_enabled?1:0,disabled&&!active.load()?1:0,ui_vm_calls.load(),qpc_frequency);
    if(!ui_observer_enabled||!disabled||active.load())return;
    fprintf(status,"ui_canvas_chain calls=%llu rejected=%llu previous_offset=56 maximum_frames=%u root_identity_required=1\n",
        ui_chain_calls.load(),ui_chain_rejected.load(),gears_ui::canvas_frame_limit);
    if(ui_chain_rejected.load()){auto const& r=ui_first_chain_reject;
        fprintf(status,"ui_canvas_chain_first_reject root_frame=%p frame=%p node=%p object=%p code=%p previous=%p qpc=%lld draw=%u\n",
            reinterpret_cast<void*>(r.root_frame),reinterpret_cast<void*>(r.frame),reinterpret_cast<void*>(r.node),reinterpret_cast<void*>(r.object),
            reinterpret_cast<void*>(r.code),reinterpret_cast<void*>(r.previous),r.qpc,r.draw);}
    for(unsigned i=0;i<ui_config.scope_count;++i)fprintf(status,"ui_canvas_scope index=%u kind=%u calls=%llu calls_are_not_script_invocations=1\n",
        i,ui_config.scopes[i].kind,ui_canvas_scopes[i].load());
    for(unsigned i=0;i<ui_config.scope_count;++i)fprintf(status,"ui_scope index=%u kind=%u function=%p accepted=%llu rejected=%llu\n",
        i,ui_config.scopes[i].kind,reinterpret_cast<void*>(ui_config.identities[ui_config.scopes[i].function].address),ui_scopes[i].load(),ui_scope_rejected[i].load());
    for(unsigned i=0;i<ui_config.scope_count;++i)if(ui_scope_rejected[i].load()){auto const& r=ui_first_scope_reject[i];
        fprintf(status,"ui_scope_first_reject index=%u self=%p frame=%p object=%p code=%p qpc=%lld thread=%u\n",i,
            reinterpret_cast<void*>(r.self),reinterpret_cast<void*>(r.frame),reinterpret_cast<void*>(r.object),reinterpret_cast<void*>(r.code),r.qpc,r.thread);}
    for(unsigned i=0;i<9;++i){
        fprintf(status,"ui_canvas bucket=%u kind=%u draw=%u calls=%llu rejected=%llu rows=%u dropped=%llu\n",i,i/3+1,i%3+1,ui_calls[i].load(),ui_rejected[i].load(),ui_row_counts[i],ui_dropped[i].load());
        for(unsigned j=0;j<ui_row_counts[i];++j){auto const& r=ui_rows[i][j];auto const& s=r.sample;
            fprintf(status,"ui_canvas_row bucket=%u row=%u root_invocation=%llu scope=%u function=%p object=%p canvas=%p frame_node=%p frame_object=%p frame_code=%p controller=%p pawn=%p hud=%p inventory=%p hud_canvas=%p scope_qpc=%lld qpc=%lld thread=%u draw=%u calls=%llu latest_built_ticket_hint=%llu ticket_association_verified=0 clip_x=%.9g clip_y=%.9g cur_x=%.9g cur_y=%.9g org_x=%.9g org_y=%.9g size_x=%d size_y=%d root_frame=%p matched_frame=%p matched_depth=%u\n",
                i,j,r.invocation,s.scope.scope_index,reinterpret_cast<void*>(s.scope.function),reinterpret_cast<void*>(s.scope.object),reinterpret_cast<void*>(s.canvas),
                reinterpret_cast<void*>(s.frame_node),reinterpret_cast<void*>(s.frame_object),reinterpret_cast<void*>(s.frame_code),reinterpret_cast<void*>(s.controller),reinterpret_cast<void*>(s.pawn),
                reinterpret_cast<void*>(s.hud),reinterpret_cast<void*>(s.inventory),reinterpret_cast<void*>(s.hud_canvas),r.scope_qpc,r.qpc,r.thread,r.draw,r.calls,r.latest_built_ticket,
                s.clip_x,s.clip_y,s.cur_x,s.cur_y,s.org_x,s.org_y,s.size_x,s.size_y,
                reinterpret_cast<void*>(r.root_frame),reinterpret_cast<void*>(r.matched_frame),r.matched_depth);
        }
    }
}
