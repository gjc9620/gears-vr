namespace dlss_output_control {
#if defined(GEARS_DLSS_OUTPUT_CONTROL) && GEARS_DLSS_OUTPUT_CONTROL
#if !defined(GEARS_DLSS_ROUTE_OBSERVER) || !GEARS_DLSS_ROUTE_OBSERVER
#error DLSS output control requires its independently attested route observer
#endif
#if (defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK) || (defined(GEARS_GRAPHICS_STARTUP_LOCK) && GEARS_GRAPHICS_STARTUP_LOCK) || (defined(GEARS_GRAPHICS_MENU_LOCK) && GEARS_GRAPHICS_MENU_LOCK)
#error DLSS output experiment cannot run with graphics locks
#endif
bool attested{};Store store;thread_local Key active_key{};thread_local Output* current_output{};thread_local QuadScope* current_quad{};
uint64_t forwarded{},draw_proved{},output_proved{},rejected{},missing_source{},source_queries{},other_setters{},list_rejected{};
std::array<uint64_t,3> non_target_setters{}; // d8388e, d8397d, other; never target-proof counts.
std::array<uint64_t,static_cast<unsigned>(Decision::count)> decisions{};
struct MissingSource {uintptr_t context{},object{},native{},canonical{};unsigned slots{},width{},height{},dimension{},format{};long result{};};
MissingSource last_source;
struct Receipt {bool valid{},complete{},site{};Key key{};unsigned decision{},failure{},setters{},quads{};Proof proof{};};
Receipt last_allowed{},last_rejected{};
struct ListPath {uintptr_t owner{},raw{},table{},method{},canonical{};uint64_t generation{};bool valid{};};
ListPath last_list_before{},last_list_after{};uintptr_t last_list_draw{};gears_frame_link::ListRef last_list_ref{};
ListPath list_path(uintptr_t context){
    ListPath out;
    if(!dlss_route_observer::read(out.owner,context,0x127c)||!out.owner||!dlss_route_observer::read(out.raw,out.owner,0x14)||!out.raw||
       !dlss_route_observer::read(out.table,out.raw)||!out.table||!dlss_route_observer::read(out.method,out.table,12*8)||
       !system_resource_methods(out.raw)||!viewport_targets[3]||out.method!=reinterpret_cast<uintptr_t>(viewport_targets[3]))return out;
    out.canonical=canonical_identity(reinterpret_cast<IUnknown*>(out.raw));if(!out.canonical)return out;
    const auto ref=frame_link.current(out.canonical);out.generation=ref.generation;out.valid=ref.identity==out.canonical&&ref.generation;return out;
}
bool list_matches(ListPath const& path,uintptr_t raw,gears_frame_link::ListRef ref){
    return path.valid&&path.raw==raw&&path.canonical==ref.identity&&path.generation==ref.generation;
}
bool other_controls(){return owned_bindings_enabled||owned_xr_probe||quad_source_control_enabled()||first_view_mask_control||submit_duplicate_control||
    submit_mono_control||submit_mono_independent_state||projection_control||origin_trial_enabled;}
bool current(Key key){return attested&&recording.load()&&!other_controls()&&key&&active_key==key&&consuming_renderer&&consuming_frozen_frame.enabled&&
    consuming_frame_ticket==key.id&&consuming_frozen_frame.id==key.id&&consuming_frozen_frame.world_generation==key.world&&committed_world_generation.load()==key.world;}
bool stable(Output const& o){
    uint32_t flags{};return current(o.key)&&o.renderer==reinterpret_cast<uintptr_t>(consuming_renderer)&&o.thread==GetCurrentThreadId()&&
        dlss_route_observer::read(flags,o.pass,8)&&flags==o.flags&&(flags&8)&&same(o.before,dlss_route_observer::view(o.view));
}
bool descriptor_stable(Output const& o){Descriptor d{};return o.descriptor&&dlss_route_observer::read(d,o.descriptor)&&d==o.expected&&noop(d,0);}
void fail(Output& o,Failure why){o.failed=true;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
    if(auto* e=store.find(o.key))e->fail(why);}
void participate(Output& o){o.site=true;std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
    if(!store.participate(o.key))o.failed=true;}
ConsumeScope::ConsumeScope(uint64_t world,uint64_t ticket,bool enabled):previous(active_key),exceptions(std::uncaught_exceptions()){
    active_key={};std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
    const Key next{world,ticket};if(enabled&&attested&&recording.load()&&world&&world==committed_world_generation.load()&&ticket==consuming_frame_ticket&&ticket){
        key=next;active_key=next;store.begin(next);}
}
ConsumeScope::~ConsumeScope(){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
    if(std::uncaught_exceptions()>exceptions)if(auto* e=store.find(key))e->fail(Failure::identity_changed);
    store.finish(key);active_key=previous;
}
OutputScope::OutputScope(bool world,uintptr_t pass,uintptr_t input):previous(current_output),exceptions(std::uncaught_exceptions()){
    current_output=&value;value.key=active_key;value.pass=pass;value.view=input;value.renderer=reinterpret_cast<uintptr_t>(consuming_renderer);value.thread=GetCurrentThreadId();
    if(previous){previous->nested=true;value.nested=true;if(previous->site)fail(*previous,Failure::identity_changed);}
    if(!world||!current(value.key)||value.nested)return;
    value.before=dlss_route_observer::view(input);uint32_t flags{};
    value.valid=dlss_route_observer::read(flags,pass,8)&&(flags&8)&&valid_view(value.before);value.flags=flags;
}
OutputScope::~OutputScope(){
    if(value.site){
        const bool valid=value.valid&&!value.failed&&!value.nested&&std::uncaught_exceptions()==exceptions&&value.setters==1&&value.quads==1&&value.draws==1&&
            value.setter_proved&&value.quad_complete&&stable(value)&&descriptor_stable(value);
        std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);if(auto* e=store.find(value.key)){
            if(!valid)e->fail(Failure::identity_changed);
            else{auto& eye=e->payload.eyes[value.before.eye];eye=value.proof;eye.output_complete=true;e->proved(value.before.eye,false,true);++output_proved;}}
    }
    current_output=previous;
}
SetterScope::SetterScope(bool world,uintptr_t caller,const uint16_t* descriptor,unsigned index):output(current_output),exceptions(std::uncaught_exceptions()){
    if(!output||!output->key)return;
    if(caller!=0xd83b69){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);++other_setters;
        ++non_target_setters[caller==0xd8388e?0u:caller==0xd8397d?1u:2u];return;}
    ++output->setters;tracking=true;participate(*output);
    if(!world||!output->valid||output->nested||!stable(*output)){fail(*output,Failure::identity_changed);return;}
    {std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);auto* e=store.find(output->key);
        if(!e||!e->reserve(output->before.eye,true)){output->failed=true;return;}}
    output->descriptor=reinterpret_cast<uintptr_t>(descriptor);
    if(!dlss_route_observer::read(output->expected,output->descriptor)||!noop(output->expected,index))fail(*output,Failure::unsupported);
}
SetterScope::~SetterScope(){if(!tracking||!output)return;
    const bool valid=!output->failed&&std::uncaught_exceptions()==exceptions&&output->setters==1&&stable(*output)&&descriptor_stable(*output);
    std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);if(auto* e=store.find(output->key)){
        if(!valid)e->fail(Failure::post_setter);else{e->proved(output->before.eye,true,true);output->setter_proved=true;}}
}
QuadScope::QuadScope(bool world,uintptr_t caller,Args const&,Dimensions const& sizes):output(current_output),previous(current_quad),dimensions(sizes),exceptions(std::uncaught_exceptions()){
    current_quad=nullptr;if(previous&&previous->output){previous->output->nested=true;fail(*previous->output,Failure::duplicate_quad);}
    if(!output||!output->key)return;++output->quads;
    if(caller!=0xd83cbf)return;participate(*output);
    if(!world||!output->valid||output->failed||output->nested||output->setters!=1||output->quads!=1||!output->setter_proved||!stable(*output)||!descriptor_stable(*output)||
       !geometry(output->before,dimensions,forwarded)){fail(*output,Failure::prequad);return;}
    {
        std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);auto* e=store.find(output->key);
        if(!e||!e->reserve(output->before.eye,false)){output->failed=true;return;}
        if(e->failure!=Failure::none){output->failed=true;return;}++dlss_output_control::forwarded;
    }
    output->proof={};output->proof.renderer=output->renderer;output->proof.view=output->view;output->proof.context=output->before.context;
    output->proof.pass=output->pass;output->proof.descriptor=output->descriptor;output->proof.forwarded=forwarded;output->proof.dimensions=dimensions;
    changed=true;current_quad=this;
}
QuadScope::~QuadScope(){
    if(changed&&output){const bool valid=!output->failed&&!output->nested&&std::uncaught_exceptions()==exceptions&&output->draws==1&&
        output->proof.draw.candidate&&stable(*output)&&descriptor_stable(*output);
        if(!valid)fail(*output,Failure::prequad);else output->quad_complete=true;}
    current_quad=previous;
}
bool QuadScope::candidate()const{return changed&&current_quad==this&&output&&!output->failed&&current(output->key);}
bool scoped(){return current_quad&&current_quad->candidate();}
void QuadScope::forward(float& x,float& y,float& w,float& h,float& u,float& v,float& uw,float& vh)const{
    if(!candidate())return;x=forwarded[0];y=forwarded[1];w=forwarded[2];h=forwarded[3];u=forwarded[4];v=forwarded[5];uw=forwarded[6];vh=forwarded[7];
}
gears_final_transfer::EyeDrawEvidence draw(uintptr_t list,uintptr_t caller,unsigned kind,unsigned elements,unsigned instances,
    gears_viewport::TargetDraw const& binding,gears_frame_link::ListRef ref,gears_viewport::DrawState const& rectangles){
    gears_final_transfer::EyeDrawEvidence proof;auto* quad=current_quad;if(!quad||!quad->output)return proof;auto& o=*quad->output;++o.draws;
    if(!quad->candidate()||o.draws!=1||!stable(o)||!descriptor_stable(o)||caller!=base+0x1af92b||kind!=1||elements!=6||instances!=1||
       !list||!ref.identity||!ref.generation||frame_target_stats.descriptor_capacity_lost()||
       !late_quad_candidate.active||late_quad_candidate.caller!=0xd83cbf||late_quad_candidate.args!=quad->forwarded||late_quad_candidate.dimensions!=quad->dimensions){
        fail(o,Failure::prequad);return proof;}
    // Bind the TLS resource context to this actual list before querying its SRV.
    // This is the same owner/list/system-method chain as final_transfer::entry.
    const auto context=o.before.context;const auto before_list=list_path(context);
    {
        std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
        last_list_before=before_list;last_list_after={};last_list_draw=list;last_list_ref=ref;
        if(!list_matches(before_list,list,ref))++list_rejected;
    }
    if(!list_matches(before_list,list,ref)){fail(o,Failure::identity_changed);return proof;}
    // Query the real bound SRV through the same authenticated resource wrapper
    // used by final_transfer_live. Never infer its dimensions from view fields.
    unsigned char slots{};uintptr_t object{};ResourceIdentity source{};
    const bool bound=dlss_route_observer::read(slots,context,0x65d)&&slots==1&&dlss_route_observer::read(object,context,0x768)&&object;
    if(bound)query_bound_resource(object,source);auto const& d=source.desc;
    const auto after_list=list_path(context);
    const bool list_stable=list_matches(after_list,list,ref)&&before_list.owner==after_list.owner&&before_list.table==after_list.table&&before_list.method==after_list.method;
    {
        std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);++source_queries;
        last_list_after=after_list;if(!list_stable)++list_rejected;
        last_source={context,object,source.native,source.canonical,unsigned(slots),d.Width<=8192?unsigned(d.Width):0u,d.Height,unsigned(d.Dimension),unsigned(d.Format),source.result};
    }
    if(!list_stable){fail(o,Failure::identity_changed);return proof;}
    auto const& sizes=quad->dimensions;
    const bool resource=bound&&SUCCEEDED(source.result)&&source.canonical&&source.canonical!=binding.target.resource&&d.Dimension==3&&d.Width==sizes[2]&&d.Height==sizes[3]&&
        d.DepthOrArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count==1&&d.Format==24&&binding.targets==1&&binding.target.resource&&!binding.target.mip&&
        binding.target.format==24&&binding.target.width==sizes[0]&&binding.target.height==sizes[1];
    auto const& v=rectangles.viewport;auto const& s=rectangles.scissor;const unsigned eye=o.before.eye;
    const bool rects=rectangles.tracked&&rectangles.viewport_count==1&&rectangles.scissor_count==1&&rectangles.depth_known&&rectangles.min_depth==0&&rectangles.max_depth==1&&
        v==gears_viewport::Rect{0,0,float(sizes[0]),float(sizes[1])}&&std::isfinite(s.x)&&std::isfinite(s.y)&&std::isfinite(s.width)&&std::isfinite(s.height)&&
        s.width>0&&s.height>0&&s.x<=quad->forwarded[0]&&s.y<=0&&double(s.x)+s.width>=double(quad->forwarded[0])+sizes[0]/2&&double(s.y)+s.height>=sizes[1];
    if(!resource||!rects){if(!resource){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);++missing_source;}fail(o,Failure::read_failed);return proof;}
    proof={true,o.key.world,o.key.id,eye,ref.identity,source.canonical,binding.target.resource,ref.generation,quad->forwarded,sizes};
    o.proof.draw=proof;{std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);++draw_proved;}return proof;
}
bool enable(FILE* file,bool observer_attested){
    const bool config=!self_testing&&continuous_session&&stereo_xr&&stereo_gpu_publish&&submit_pair_enabled&&frame_link_diagnostics&&viewport_diagnostics&&!other_controls();
    attested=observer_attested&&config;store={};
    fprintf(file,"dlss_output_identity enabled=1 observer_full_ranges_attested=%u configuration=%u attested=%u generated_caller=d83cbf setter=d83b69 sentinel=ffff,ffff,0 original_setter_unchanged=1 geometry_memory_written=0 ngx_sr_verified=0\n",unsigned(observer_attested),unsigned(config),unsigned(attested));return attested;
}
bool publication(uint64_t world,uint64_t ticket){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);const Key key{world,ticket};const auto decision=store.decision(key);
    ++decisions[static_cast<unsigned>(decision)];auto* e=store.find(key);
    const bool allow=decision==Decision::no_site||(decision==Decision::zero_proved&&e&&complete(*e));
    Receipt receipt;receipt.valid=true;receipt.key=key;receipt.decision=static_cast<unsigned>(decision);
    if(e){receipt.complete=e->complete;receipt.site=e->site;receipt.failure=static_cast<unsigned>(e->failure);receipt.setters=e->setter_mask;receipt.quads=e->quad_mask;receipt.proof=e->payload;}
    (allow?last_allowed:last_rejected)=receipt;if(!allow)++rejected;return allow;
}
void retire(){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);store.retire_window();}
void progress(FILE* file){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);fprintf(file,"dlss_output_forwarded=%llu dlss_output_draw_proved=%llu dlss_output_completed=%llu dlss_output_publication_rejected=%llu ",forwarded,draw_proved,output_proved,rejected);}
void print(FILE* file){std::lock_guard<std::recursive_mutex> lock(frame_hook_mutex);
    fprintf(file,"dlss_output_control enabled=1 attested=%u forwarded=%llu actual_draw_proved=%llu output_completed=%llu publication_rejected=%llu no_site=%llu scope_proved=%llu missing=%llu incomplete=%llu rejected=%llu overflow=%llu capacity_lost=%u retired=%llu source_queries=%llu source_rejected=%llu other_setters=%llu list_rejected=%llu rows_limit_independent=1 ngx_sr_verified=0 history_verified=0\n",
        unsigned(attested),forwarded,draw_proved,output_proved,rejected,decisions[0],decisions[1],decisions[2],decisions[3],decisions[4],decisions[5],unsigned(store.capacity_lost),store.retired,source_queries,missing_source,other_setters,list_rejected);
    fprintf(file,"dlss_output_last_list draw_raw=%llx draw_canonical=%llx draw_generation=%llu before_owner=%llx before_raw=%llx before_canonical=%llx before_generation=%llu before_valid=%u after_raw=%llx after_canonical=%llx after_generation=%llu after_valid=%u path=context+127c,owner+14,vtable_slot12\n",
        static_cast<unsigned long long>(last_list_draw),static_cast<unsigned long long>(last_list_ref.identity),last_list_ref.generation,
        static_cast<unsigned long long>(last_list_before.owner),static_cast<unsigned long long>(last_list_before.raw),static_cast<unsigned long long>(last_list_before.canonical),last_list_before.generation,unsigned(last_list_before.valid),
        static_cast<unsigned long long>(last_list_after.raw),static_cast<unsigned long long>(last_list_after.canonical),last_list_after.generation,unsigned(last_list_after.valid));
    fprintf(file,"dlss_output_non_target_setters d8388e=%llu d8397d=%llu other=%llu counted_toward_target_proof=0\n",non_target_setters[0],non_target_setters[1],non_target_setters[2]);
    fprintf(file,"dlss_output_last_source context=%llx slots=%u object=%llx native=%llx canonical=%llx width=%u height=%u dimension=%u format=%u result=%ld source_offsets=context+65d,context+768 resource_query=existing_authenticated_wrapper inferred_dimensions=0\n",
        static_cast<unsigned long long>(last_source.context),last_source.slots,static_cast<unsigned long long>(last_source.object),static_cast<unsigned long long>(last_source.native),static_cast<unsigned long long>(last_source.canonical),last_source.width,last_source.height,last_source.dimension,last_source.format,last_source.result);
    const Receipt examples[]{last_allowed,last_rejected};
    for(unsigned n=0;n<2;++n){const auto& r=examples[n];
        fprintf(file,"dlss_output_publication_example allowed=%u valid=%u world=%llu ticket=%llu decision=%u failure=%u complete=%u site=%u setters=%u quads=%u\n",
            unsigned(n==0),unsigned(r.valid),r.key.world,r.key.id,r.decision,r.failure,unsigned(r.complete),unsigned(r.site),r.setters,r.quads);
        if(!r.valid||!r.site)continue;
        for(unsigned i=0;i<2;++i){const auto& e=r.proof.eyes[i];const auto& d=e.draw;
            fprintf(file,"dlss_output_eye_example allowed=%u eye=%u world=%llu ticket=%llu renderer=%llx view=%llx context=%llx pass=%llx descriptor=%llx completed_output=%u candidate_draw=%u list=%llx list_generation=%llu source=%llx target=%llx args=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g dimensions=%u,%u,%u,%u\n",
                unsigned(n==0),i,r.key.world,r.key.id,static_cast<unsigned long long>(e.renderer),static_cast<unsigned long long>(e.view),static_cast<unsigned long long>(e.context),static_cast<unsigned long long>(e.pass),static_cast<unsigned long long>(e.descriptor),unsigned(e.output_complete),unsigned(d.candidate),
                static_cast<unsigned long long>(d.list),d.generation,static_cast<unsigned long long>(d.source),static_cast<unsigned long long>(d.target),e.forwarded[0],e.forwarded[1],e.forwarded[2],e.forwarded[3],e.forwarded[4],e.forwarded[5],e.forwarded[6],e.forwarded[7],e.dimensions[0],e.dimensions[1],e.dimensions[2],e.dimensions[3]);}
    }
}
#else
ConsumeScope::ConsumeScope(uint64_t,uint64_t,bool){}ConsumeScope::~ConsumeScope(){}
OutputScope::OutputScope(bool,uintptr_t,uintptr_t){}OutputScope::~OutputScope(){}
SetterScope::SetterScope(bool,uintptr_t,const uint16_t*,unsigned){}SetterScope::~SetterScope(){}
QuadScope::QuadScope(bool,uintptr_t,Args const&,Dimensions const&){}QuadScope::~QuadScope(){}
bool QuadScope::candidate()const{return false;}
void QuadScope::forward(float&,float&,float&,float&,float&,float&,float&,float&)const{}
bool enable(FILE*,bool){return true;}bool publication(uint64_t,uint64_t){return true;}
bool scoped(){return false;}
void retire(){}void progress(FILE*){}void print(FILE*){}
gears_final_transfer::EyeDrawEvidence draw(uintptr_t,uintptr_t,unsigned,unsigned,unsigned,gears_viewport::TargetDraw const&,gears_frame_link::ListRef,gears_viewport::DrawState const&){return {};}
#endif
}
