namespace dlss_output_control {
#if defined(GEARS_DLSS_OUTPUT_CONTROL) && GEARS_DLSS_OUTPUT_CONTROL
#if !defined(GEARS_DLSS_ROUTE_OBSERVER) || !GEARS_DLSS_ROUTE_OBSERVER
#error DLSS output control requires its independently attested route observer
#endif
#if !defined(GEARS_DLSS_INPUT_BIND_OBSERVER) || !GEARS_DLSS_INPUT_BIND_OBSERVER
#error DLSS output control requires independently attested named input binding
#endif
#if (defined(GEARS_GRAPHICS_RUNTIME_LOCK) && GEARS_GRAPHICS_RUNTIME_LOCK) || (defined(GEARS_GRAPHICS_STARTUP_LOCK) && GEARS_GRAPHICS_STARTUP_LOCK) || (defined(GEARS_GRAPHICS_MENU_LOCK) && GEARS_GRAPHICS_MENU_LOCK)
#error DLSS output experiment cannot run with graphics locks
#endif
bool attested{};Store store;thread_local Key active_key{};thread_local Output* current_output{};thread_local QuadScope* current_quad{};
uint64_t forwarded{},draw_proved{},output_proved{},rejected{},missing_source{},source_queries{},other_setters{},list_rejected{};
std::array<uint64_t,3> non_target_setters{}; // d8388e, d8397d, other; never target-proof counts.
std::array<uint64_t,2> non_target_quads{}; // 194c93 preprocessing, other; never target-proof counts.
std::array<uint64_t,static_cast<unsigned>(Decision::count)> decisions{};
struct MissingSource {uintptr_t context{},object{},native{},canonical{};unsigned slots{},width{},height{},dimension{},format{};long result{};};
MissingSource last_source;
struct Receipt {bool valid{},complete{},site{};Key key{};unsigned decision{},failure{},setters{},quads{};Proof proof{};};
Receipt last_allowed{},last_rejected{};
OutputExitReceipt first_exit_failure;
std::atomic<bool> exit_failure_claimed{},exit_failure_ready{};
void publish_exit_failure(OutputExitReceipt const& receipt){bool empty=false;
    if(!exit_failure_claimed.compare_exchange_strong(empty,true))return;
    first_exit_failure=receipt;exit_failure_ready.store(true,std::memory_order_release);
}
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
void fail(Output& o,Failure why){o.failed=true;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(auto* e=store.find(o.key))e->fail(why);}
void participate(Output& o){o.site=true;std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    if(!store.participate(o.key))o.failed=true;}
bool named_sample(Output const& o,gears_dlss_route::InputSample const& s,uintptr_t vtable){
    auto const& r=s.resource;
    return same(o.before,s.view)&&s.mask==1023&&vtable&&s.shader_handle&&s.descriptor[1]&&s.slot<32&&s.highwater<=32&&s.slot<s.highwater&&
        s.public_object&&s.public_object==r.object&&r.queried&&SUCCEEDED(r.result)&&r.native&&r.canonical&&r.dimension==3&&r.format==26&&
        r.width&&r.width<=8192&&!(r.width%2)&&r.height&&r.height<=8192&&r.depth==1&&r.mips==1&&r.samples==1;
}
bool binding_same(Eye::BindingEvidence const& a,Eye::BindingEvidence const& b){
    auto const& x=a.resource;auto const& y=b.resource;
    return a.complete&&b.complete&&a.descriptor==b.descriptor&&a.address==b.address&&a.owner==b.owner&&a.vtable==b.vtable&&a.shader_handle==b.shader_handle&&
        a.view==b.view&&a.context==b.context&&a.public_global==b.public_global&&a.public_pointer==b.public_pointer&&a.public_wrapper==b.public_wrapper&&
        a.public_index==b.public_index&&a.public_object==b.public_object&&a.selector==b.selector&&a.slot==b.slot&&a.list==b.list&&
        a.canonical_list==b.canonical_list&&a.generation==b.generation&&x.object==y.object&&x.native==y.native&&x.canonical==y.canonical&&
        x.width==y.width&&x.height==y.height&&x.dimension==y.dimension&&x.format==y.format&&x.depth==y.depth&&x.mips==y.mips&&x.samples==y.samples;
}
bool binding_snapshot(Output const& o,Eye::BindingEvidence& out,BindingTrace* trace=nullptr,uintptr_t proved_public_global=0){
    const auto before=list_path(o.before.context);const auto s=dlss_route_observer::input_sample(o.binding_capture,true,proved_public_global);
    uintptr_t vtable{};const auto after=list_path(o.before.context);
    const bool chain=before.valid&&after.valid&&before.raw==after.raw&&before.canonical==after.canonical&&before.generation==after.generation&&
        before.owner==after.owner&&before.table==after.table&&before.method==after.method;
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++source_queries;last_list_before=before;last_list_after=after;
        if(!chain)++list_rejected;
        auto const& r=s.resource;last_source={o.before.context,r.object,r.native,r.canonical,unsigned(s.highwater),unsigned(r.width),r.height,r.dimension,r.format,r.result};}
    // Preserve partial fields/masks even when certification fails. These values
    // are diagnostic only; complete and the returned conjunction still gate use.
    if(!trace&&!chain)return false;
    const bool vtable_read=dlss_route_observer::read(vtable,o.binding_capture.shader_owner);
    const bool named=named_sample(o,s,vtable);
    if(trace){trace->sample=s;trace->before=before;trace->after=after;trace->vtable=vtable;
        trace->vtable_read=vtable_read;trace->chain=chain;trace->named=named;}
    out.complete=chain&&vtable_read&&named;out.descriptor=s.descriptor;out.address=o.binding_capture.descriptor_address;out.owner=o.binding_capture.shader_owner;out.vtable=vtable;
    out.shader_handle=s.shader_handle;out.view=s.view.address;out.context=s.view.context;out.public_global=s.public_global;out.public_pointer=s.public_pointer;
    out.public_wrapper=s.public_wrapper;out.public_index=s.public_index;out.public_object=s.public_object;out.selector=s.selector;out.slot=s.slot;out.highwater=s.highwater;
    out.list=after.raw;out.canonical_list=after.canonical;out.generation=after.generation;out.resource=s.resource;return out.complete;
}
bool binding_current(Output const& o,BindingTrace* trace=nullptr,Eye::BindingEvidence* observed=nullptr){Eye::BindingEvidence fresh;
    if(!trace)return o.binds==1&&o.binding_proved&&!o.binding_failed&&binding_snapshot(o,fresh)&&binding_same(o.binding,fresh);
    const bool snapshot=binding_snapshot(o,fresh,trace);trace->same=binding_same(o.binding,fresh);
    if(observed)*observed=fresh;
    return o.binds==1&&o.binding_proved&&!o.binding_failed&&snapshot&&trace->same;
}
bool output_tail_current(Output const& o,OutputExitReceipt& r){
    // Only the normal generated Output tail may change the view's selector.
    // Earlier Bind/quad/draw checks retain binding_current, including equality
    // of their actual selectors and public cells. This never adopts bank1.
    if(!o.site||!r.valid||r.failed||r.nested||r.exception||r.setters!=1||r.quads!=1||r.draws!=1||r.binds!=1||
       !r.setter_proved||!r.quad_complete||!r.draw_complete||!r.draw_candidate||!r.stable||!r.descriptor_stable||
       !o.binding_proved||o.binding_failed||!o.binding.complete||o.binding.selector!=0||o.binding.public_global!=base+0x1c3b410||
       o.before.marker!=1||r.fresh_view.marker!=1||!(r.trace.sample.mask&8)||r.trace.sample.selector!=1)return false;
    // The strict observation may differ only in selecting the new public bank.
    // Missing or changed original fields are not repaired by a second sample.
    auto const& strict=r.trace.sample;auto const& proved=o.binding;auto const& old=proved.resource;auto const& seen=strict.resource;
    if((strict.mask&0x23f)!=0x23f||!same(o.before,strict.view)||strict.descriptor!=proved.descriptor||strict.shader_handle!=proved.shader_handle||
       strict.slot!=proved.slot||strict.highwater!=proved.highwater||!seen.queried||!SUCCEEDED(seen.result)||seen.object!=old.object||
       seen.native!=old.native||seen.canonical!=old.canonical||seen.width!=old.width||seen.height!=old.height||seen.dimension!=old.dimension||
       seen.format!=old.format||seen.depth!=old.depth||seen.mips!=old.mips||seen.samples!=old.samples||!r.trace.vtable_read||r.trace.vtable!=proved.vtable||
       !r.trace.chain||r.trace.after.raw!=proved.list||r.trace.after.canonical!=proved.canonical_list||r.trace.after.generation!=proved.generation)return false;
    Eye::BindingEvidence fresh;if(!binding_snapshot(o,fresh,&r.tail_trace,o.binding.public_global))return false;
    auto const& a=o.binding;auto const& b=fresh;auto const& x=a.resource;auto const& y=b.resource;
    const bool equal=a.complete&&b.complete&&a.selector==0&&b.selector==1&&a.descriptor==b.descriptor&&a.address==b.address&&a.owner==b.owner&&
        a.vtable==b.vtable&&a.shader_handle==b.shader_handle&&a.view==b.view&&a.context==b.context&&a.public_global==b.public_global&&
        a.public_pointer==b.public_pointer&&a.public_wrapper==b.public_wrapper&&a.public_index==b.public_index&&a.public_object==b.public_object&&
        a.slot==b.slot&&a.highwater==b.highwater&&a.list==b.list&&a.canonical_list==b.canonical_list&&a.generation==b.generation&&
        x.object==y.object&&x.native==y.native&&x.canonical==y.canonical&&x.width==y.width&&x.height==y.height&&x.dimension==y.dimension&&
        x.format==y.format&&x.depth==y.depth&&x.mips==y.mips&&x.samples==y.samples;
    r.tail_trace.same=equal;
    uint32_t selector{},public_index{};Descriptor descriptor{};uintptr_t public_pointer{},public_object{},object{},handle{},vtable{};unsigned char highwater{};
    return equal&&dlss_route_observer::read(selector,o.view,0x3e8)&&selector==1&&
        dlss_route_observer::read(descriptor,o.binding_capture.descriptor_address)&&descriptor==a.descriptor&&
        dlss_route_observer::read(handle,a.owner,0x50)&&handle==a.shader_handle&&dlss_route_observer::read(vtable,a.owner)&&vtable==a.vtable&&
        dlss_route_observer::read(public_pointer,a.public_global)&&public_pointer==a.public_pointer&&
        dlss_route_observer::read(public_index,a.public_wrapper,0x138)&&public_index==a.public_index&&
        dlss_route_observer::read(public_object,a.public_wrapper,0x38+uintptr_t(public_index)*8)&&public_object==a.public_object&&
        dlss_route_observer::read(highwater,a.context,0x65d)&&highwater==a.highwater&&
        dlss_route_observer::read(object,a.context,0x768+uintptr_t(a.slot)*8)&&object==a.resource.object&&stable(o)&&descriptor_stable(o);
}
void binding_fail(Output& o){o.binding_failed=true;if(o.site)fail(o,Failure::identity_changed);}
BindScope::BindScope(bool world,uintptr_t caller,const uint16_t* descriptor,void* view,void* owner,uint32_t selection,int32_t alternate)
    :output(current_output),exceptions(std::uncaught_exceptions()){
    if(!output||!output->key||caller!=0xd831c8)return;
    if(++output->binds!=1){binding_fail(*output);return;}tracking=true;
    auto& b=output->binding_capture;b.present=true;b.caller=caller;b.descriptor_address=reinterpret_cast<uintptr_t>(descriptor);
    b.view_address=reinterpret_cast<uintptr_t>(view);b.shader_owner=reinterpret_cast<uintptr_t>(owner);b.selection=selection;b.alternate=alternate;
    if(!world||!output->valid||output->nested||!stable(*output)||selection||alternate||b.view_address!=output->view||!b.shader_owner||
       b.shader_owner>UINTPTR_MAX-0xb4||b.descriptor_address!=b.shader_owner+0xb4||!dlss_route_observer::read(vtable,b.shader_owner)||!vtable){binding_fail(*output);return;}
    b.before=dlss_route_observer::input_sample(b);
    if(b.before.mask!=1023||!same(output->before,b.before.view)||b.before.slot>=32){binding_fail(*output);return;}
    b.tracked_slot=b.before.slot;b.tracked_slot_known=true;
}
BindScope::~BindScope(){if(!tracking||!output)return;auto& o=*output;auto& b=o.binding_capture;Eye::BindingEvidence fresh;
    if(o.binding_failed||std::uncaught_exceptions()!=exceptions||!stable(o)||!binding_snapshot(o,fresh)||fresh.vtable!=vtable||
       fresh.descriptor!=b.before.descriptor||fresh.shader_handle!=b.before.shader_handle||fresh.public_global!=b.before.public_global||
       fresh.public_pointer!=b.before.public_pointer||fresh.public_wrapper!=b.before.public_wrapper||fresh.public_index!=b.before.public_index||
       fresh.public_object!=b.before.public_object||fresh.selector!=b.before.selector){binding_fail(o);return;}
    b.exited=true;o.binding=fresh;o.binding_proved=true;
}
ConsumeScope::ConsumeScope(uint64_t world,uint64_t ticket,bool enabled):previous(active_key),exceptions(std::uncaught_exceptions()){
    active_key={};std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    const Key next{world,ticket};if(enabled&&attested&&recording.load()&&world&&world==committed_world_generation.load()&&ticket==consuming_frame_ticket&&ticket){
        key=next;active_key=next;store.begin(next);}
}
ConsumeScope::~ConsumeScope(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
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
        OutputExitReceipt r;r.key=value.key;r.pass=value.pass;r.view=value.view;r.renderer=value.renderer;r.thread=value.thread;
        r.descriptor=value.descriptor;r.flags=value.flags;r.setters=value.setters;r.quads=value.quads;r.draws=value.draws;r.binds=value.binds;
        r.valid=value.valid;r.failed=value.failed;r.nested=value.nested;r.exception=std::uncaught_exceptions()!=exceptions;
        r.setter_proved=value.setter_proved;r.quad_complete=value.quad_complete;r.draw_candidate=value.proof.draw.candidate;
        r.draw_complete=value.draw_complete;r.binding_proved=value.binding_proved;r.binding_failed=value.binding_failed;
        r.before=value.before;r.expected=value.expected;r.binding=value.binding;r.input_before=value.binding_capture.before;
        // Evaluate the same admission functions once each, retaining their
        // original stable/descriptor/binding order. Receipt-only reads below
        // never contribute evidence to the final admission conjunction.
        r.stable=stable(value);r.descriptor_stable=descriptor_stable(value);
        Eye::BindingEvidence fresh;r.binding_current=binding_current(value,&r.trace,&fresh);
        r.binding_snapshot=fresh.complete;r.binding_equal=r.trace.same;
        r.current=current(value.key);r.thread_same=value.thread==GetCurrentThreadId();
        r.flags_read=dlss_route_observer::read(r.fresh_flags,value.pass,8);
        r.fresh_view=value.binding_capture.view_address==value.view?r.trace.sample.view:dlss_route_observer::view(value.view);
        r.descriptor_read=value.descriptor&&dlss_route_observer::read(r.fresh_descriptor,value.descriptor);
        r.tail_transition_validated=!r.binding_current&&output_tail_current(value,r);
        const bool valid=r.valid&&!r.failed&&!r.nested&&!r.exception&&r.setters==1&&r.quads==1&&r.draws==1&&
            r.setter_proved&&r.quad_complete&&r.stable&&r.descriptor_stable&&(r.binding_current||r.tail_transition_validated);
        r.output_admitted=valid;
        if(!valid||!r.binding_current)publish_exit_failure(r); // Retain strict failure even for a separately proved tail.
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(auto* e=store.find(value.key)){
            if(!valid)e->fail(Failure::identity_changed);
            else{auto& eye=e->payload.eyes[value.before.eye];eye=value.proof;eye.output_complete=true;eye.tail_transition_validated=r.tail_transition_validated;e->proved(value.before.eye,false,true);++output_proved;}}
    }
    current_output=previous;
}
SetterScope::SetterScope(bool world,uintptr_t caller,const uint16_t* descriptor,unsigned index):output(current_output),exceptions(std::uncaught_exceptions()){
    if(!output||!output->key)return;
    if(caller!=0xd83b69){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++other_setters;
        ++non_target_setters[caller==0xd8388e?0u:caller==0xd8397d?1u:2u];return;}
    ++output->setters;tracking=true;participate(*output);
    if(!world||!output->valid||output->nested||!stable(*output)){fail(*output,Failure::identity_changed);return;}
    {std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* e=store.find(output->key);
        if(!e||!e->reserve(output->before.eye,true)){output->failed=true;return;}}
    output->descriptor=reinterpret_cast<uintptr_t>(descriptor);
    if(!dlss_route_observer::read(output->expected,output->descriptor)||!noop(output->expected,index))fail(*output,Failure::unsupported);
}
SetterScope::~SetterScope(){if(!tracking||!output)return;
    const bool valid=!output->failed&&std::uncaught_exceptions()==exceptions&&output->setters==1&&stable(*output)&&descriptor_stable(*output);
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(auto* e=store.find(output->key)){
        if(!valid)e->fail(Failure::post_setter);else{e->proved(output->before.eye,true,true);output->setter_proved=true;}}
}
QuadScope::QuadScope(bool world,uintptr_t caller,Args const&,Dimensions const& sizes):output(current_output),previous(current_quad),dimensions(sizes),exceptions(std::uncaught_exceptions()){
    current_quad=nullptr;if(previous&&previous->output){previous->output->nested=true;fail(*previous->output,Failure::duplicate_quad);}
    if(!output||!output->key)return;
    if(caller!=0xd83cbf){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++non_target_quads[caller==0x194c93?0u:1u];return;}
    ++output->quads;participate(*output);
    if(!world||!output->valid||output->failed||output->nested||output->setters!=1||output->quads!=1||!output->setter_proved||!stable(*output)||!descriptor_stable(*output)||
       !geometry(output->before,dimensions,forwarded)||!binding_current(*output)||output->binding.resource.width!=dimensions[2]||output->binding.resource.height!=dimensions[3]){fail(*output,Failure::prequad);return;}
    {
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);auto* e=store.find(output->key);
        if(!e||!e->reserve(output->before.eye,false)){output->failed=true;return;}
        if(e->failure!=Failure::none){output->failed=true;return;}++dlss_output_control::forwarded;
    }
    output->proof={};output->proof.binding=output->binding;output->proof.renderer=output->renderer;output->proof.view=output->view;output->proof.context=output->before.context;
    output->proof.pass=output->pass;output->proof.descriptor=output->descriptor;output->proof.forwarded=forwarded;output->proof.dimensions=dimensions;
    changed=true;current_quad=this;
}
QuadScope::~QuadScope(){
    if(changed&&output){const bool valid=!output->failed&&!output->nested&&std::uncaught_exceptions()==exceptions&&output->draws==1&&
        output->proof.draw.candidate&&stable(*output)&&descriptor_stable(*output)&&binding_current(*output);
        if(!valid)fail(*output,Failure::prequad);else output->quad_complete=true;}
    current_quad=previous;
}
bool QuadScope::candidate()const{return changed&&current_quad==this&&output&&!output->failed&&current(output->key);}
bool scoped(){return current_quad&&current_quad->candidate();}
DrawCompletion::DrawCompletion(uintptr_t raw):list(raw),exceptions(std::uncaught_exceptions()){
    if(scoped())output=current_quad->output;
}
DrawCompletion::~DrawCompletion(){if(!output)return;auto& o=*output;
    if(std::uncaught_exceptions()!=exceptions||o.draws!=1||!o.proof.draw.candidate||o.proof.binding.list!=list||
       !stable(o)||!binding_current(o))fail(o,Failure::identity_changed);else o.draw_complete=true;
}
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
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        last_list_before=before_list;last_list_after={};last_list_draw=list;last_list_ref=ref;
        if(!list_matches(before_list,list,ref))++list_rejected;
    }
    if(!list_matches(before_list,list,ref)){fail(o,Failure::identity_changed);return proof;}
    // Re-read this invocation's descriptor-selected binding. No diagnostic row,
    // fixed slot or resource dimensions can stand in for the named binding.
    Eye::BindingEvidence named;const bool bound=o.binds==1&&o.binding_proved&&!o.binding_failed&&binding_snapshot(o,named)&&binding_same(o.binding,named);
    auto const& source=named.resource;
    const auto after_list=list_path(context);
    const bool list_stable=list_matches(after_list,list,ref)&&before_list.owner==after_list.owner&&before_list.table==after_list.table&&before_list.method==after_list.method;
    {
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        last_list_after=after_list;if(!list_stable)++list_rejected;
    }
    if(!list_stable){fail(o,Failure::identity_changed);return proof;}
    auto const& sizes=quad->dimensions;
    const bool resource=bound&&named.list==list&&named.canonical_list==ref.identity&&named.generation==ref.generation&&source.canonical!=binding.target.resource&&
        source.width==sizes[2]&&source.height==sizes[3]&&source.format==26&&binding.targets==1&&binding.target.resource&&!binding.target.mip&&
        binding.target.format==24&&binding.target.width==sizes[0]&&binding.target.height==sizes[1];
    auto const& v=rectangles.viewport;auto const& s=rectangles.scissor;const unsigned eye=o.before.eye;
    const bool rects=rectangles.tracked&&rectangles.viewport_count==1&&rectangles.scissor_count==1&&rectangles.depth_known&&rectangles.min_depth==0&&rectangles.max_depth==1&&
        v==gears_viewport::Rect{0,0,float(sizes[0]),float(sizes[1])}&&std::isfinite(s.x)&&std::isfinite(s.y)&&std::isfinite(s.width)&&std::isfinite(s.height)&&
        s.width>0&&s.height>0&&s.x<=quad->forwarded[0]&&s.y<=0&&double(s.x)+s.width>=double(quad->forwarded[0])+sizes[0]/2&&double(s.y)+s.height>=sizes[1];
    if(!resource||!rects){if(!resource){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++missing_source;}fail(o,Failure::read_failed);return proof;}
    proof={true,o.key.world,o.key.id,eye,ref.identity,source.canonical,binding.target.resource,ref.generation,quad->forwarded,sizes};
    o.proof.draw=proof;{std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);++draw_proved;}return proof;
}
bool enable(FILE* file,bool observer_attested){
    const bool config=!self_testing&&continuous_session&&stereo_xr&&stereo_gpu_publish&&submit_pair_enabled&&frame_link_diagnostics&&viewport_diagnostics&&!other_controls();
    const bool binding_attested=dlss_route_observer::verify_input_chunks(file,"566929fd4f84a21529de0ecb749d2dc75a1bf2ab3cc04876a65f65517222a339");
    attested=observer_attested&&binding_attested&&config;store={};
    exit_failure_ready.store(false);exit_failure_claimed.store(false);first_exit_failure={};
    fprintf(file,"dlss_output_identity enabled=1 observer_full_ranges_attested=%u input_full_chunks_attested=%u configuration=%u attested=%u generated_caller=d83cbf setter=d83b69 sentinel=ffff,ffff,0 original_setter_unchanged=1 geometry_memory_written=0 binding_evidence_required=1 shader_sampling_verified=0 ngx_sr_verified=0\n",unsigned(observer_attested),unsigned(binding_attested),unsigned(config),unsigned(attested));return attested;
}
bool publication(uint64_t world,uint64_t ticket){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);const Key key{world,ticket};const auto decision=store.decision(key);
    ++decisions[static_cast<unsigned>(decision)];auto* e=store.find(key);
    const bool allow=decision==Decision::no_site||(decision==Decision::zero_proved&&e&&complete(*e));
    Receipt receipt;receipt.valid=true;receipt.key=key;receipt.decision=static_cast<unsigned>(decision);
    if(e){receipt.complete=e->complete;receipt.site=e->site;receipt.failure=static_cast<unsigned>(e->failure);receipt.setters=e->setter_mask;receipt.quads=e->quad_mask;receipt.proof=e->payload;}
    (allow?last_allowed:last_rejected)=receipt;if(!allow)++rejected;return allow;
}
void retire(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);store.retire_window();}
void progress(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);fprintf(file,"dlss_output_forwarded=%llu dlss_output_draw_proved=%llu dlss_output_completed=%llu dlss_output_publication_rejected=%llu ",forwarded,draw_proved,output_proved,rejected);}
void print_exit_failure(FILE* file){
    if(!exit_failure_ready.load(std::memory_order_acquire))return;
    auto const& r=first_exit_failure;
    fprintf(file,"dlss_output_exit_failure world=%llu ticket=%llu valid=%u failed=%u nested=%u exception=%u setters=%u quads=%u draws=%u binds=%u setter_proved=%u quad_complete=%u draw_candidate=%u draw_complete=%u stable=%u descriptor_stable=%u binding_current=%u binding_snapshot=%u binding_equal=%u binding_proved=%u binding_failed=%u current=%u thread_same=%u flags_read=%u flags=%u fresh_flags=%u descriptor_read=%u descriptor=%u,%u,%u fresh_descriptor=%u,%u,%u diagnostics_authorize=0\n",
        r.key.world,r.key.id,unsigned(r.valid),unsigned(r.failed),unsigned(r.nested),unsigned(r.exception),r.setters,r.quads,r.draws,r.binds,
        unsigned(r.setter_proved),unsigned(r.quad_complete),unsigned(r.draw_candidate),unsigned(r.draw_complete),unsigned(r.stable),unsigned(r.descriptor_stable),unsigned(r.binding_current),
        unsigned(r.binding_snapshot),unsigned(r.binding_equal),unsigned(r.binding_proved),unsigned(r.binding_failed),unsigned(r.current),unsigned(r.thread_same),unsigned(r.flags_read),r.flags,r.fresh_flags,
        unsigned(r.descriptor_read),r.expected[0],r.expected[1],r.expected[2],r.fresh_descriptor[0],r.fresh_descriptor[1],r.fresh_descriptor[2]);
    fprintf(file,"dlss_output_tail_transition tail_transition_validated=%u output_admitted=%u strict_binding_current=%u actual_selector=%u sampled_sourceglobal=%llx input_mask=%u descriptor=%u,%u,%u public_pointer=%llx public_wrapper=%llx public_index=%u public_object=%llx slot_object=%llx native=%llx canonical=%llx list_raw=%llx list_canonical=%llx generation=%llu chain=%u same=%u\n",
        unsigned(r.tail_transition_validated),unsigned(r.output_admitted),unsigned(r.binding_current),r.tail_trace.sample.selector,static_cast<unsigned long long>(r.tail_trace.sample.public_global),r.tail_trace.sample.mask,
        r.tail_trace.sample.descriptor[0],r.tail_trace.sample.descriptor[1],r.tail_trace.sample.descriptor[2],static_cast<unsigned long long>(r.tail_trace.sample.public_pointer),
        static_cast<unsigned long long>(r.tail_trace.sample.public_wrapper),r.tail_trace.sample.public_index,static_cast<unsigned long long>(r.tail_trace.sample.public_object),
        static_cast<unsigned long long>(r.tail_trace.sample.resource.object),static_cast<unsigned long long>(r.tail_trace.sample.resource.native),static_cast<unsigned long long>(r.tail_trace.sample.resource.canonical),
        static_cast<unsigned long long>(r.tail_trace.after.raw),static_cast<unsigned long long>(r.tail_trace.after.canonical),r.tail_trace.after.generation,unsigned(r.tail_trace.chain),unsigned(r.tail_trace.same));
    for(unsigned i=0;i<2;++i){auto const& b=i?r.trace.sample:r.input_before;auto const& v=i?r.fresh_view:r.before;
        fprintf(file,"dlss_output_exit_sample stage=%s input_mask=%u descriptor=%u,%u,%u selector=%u slot=%u highwater=%u shader_handle=%llx public_global=%llx public_pointer=%llx public_wrapper=%llx public_index=%u public_object=%llx original_slot_object=%llx proved_slot_object=%llx actual_slot_object=%llx native=%llx canonical=%llx queried=%u result=%ld dimensions=%llu,%u format=%u dimension=%u depth=%u mips=%u samples=%u",
            i?"fresh":"before",b.mask,b.descriptor[0],b.descriptor[1],b.descriptor[2],b.selector,b.slot,unsigned(b.highwater),
            static_cast<unsigned long long>(b.shader_handle),static_cast<unsigned long long>(b.public_global),static_cast<unsigned long long>(b.public_pointer),static_cast<unsigned long long>(b.public_wrapper),b.public_index,
            static_cast<unsigned long long>(b.public_object),static_cast<unsigned long long>(r.input_before.resource.object),static_cast<unsigned long long>(r.binding.resource.object),static_cast<unsigned long long>(b.resource.object),
            static_cast<unsigned long long>(b.resource.native),static_cast<unsigned long long>(b.resource.canonical),unsigned(b.resource.queried),b.resource.result,b.resource.width,b.resource.height,b.resource.format,b.resource.dimension,b.resource.depth,b.resource.mips,b.resource.samples);
        dlss_route_observer::print_view(file,i?"fresh":"before",v);fprintf(file,"\n");
    }
    auto const& a=r.trace.before;auto const& b=r.trace.after;
    fprintf(file,"dlss_output_exit_lists proved_raw=%llx proved_canonical=%llx proved_generation=%llu before_owner=%llx before_raw=%llx before_table=%llx before_method=%llx before_canonical=%llx before_generation=%llu before_valid=%u after_owner=%llx after_raw=%llx after_table=%llx after_method=%llx after_canonical=%llx after_generation=%llu after_valid=%u chain=%u vtable_read=%u vtable=%llx named=%u\n",
        static_cast<unsigned long long>(r.binding.list),static_cast<unsigned long long>(r.binding.canonical_list),r.binding.generation,
        static_cast<unsigned long long>(a.owner),static_cast<unsigned long long>(a.raw),static_cast<unsigned long long>(a.table),static_cast<unsigned long long>(a.method),static_cast<unsigned long long>(a.canonical),a.generation,unsigned(a.valid),
        static_cast<unsigned long long>(b.owner),static_cast<unsigned long long>(b.raw),static_cast<unsigned long long>(b.table),static_cast<unsigned long long>(b.method),static_cast<unsigned long long>(b.canonical),b.generation,unsigned(b.valid),
        unsigned(r.trace.chain),unsigned(r.trace.vtable_read),static_cast<unsigned long long>(r.trace.vtable),unsigned(r.trace.named));
}
void print(FILE* file){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    print_exit_failure(file);
    fprintf(file,"dlss_output_control enabled=1 attested=%u forwarded=%llu actual_draw_proved=%llu output_completed=%llu publication_rejected=%llu no_site=%llu scope_proved=%llu missing=%llu incomplete=%llu rejected=%llu overflow=%llu capacity_lost=%u retired=%llu source_queries=%llu source_rejected=%llu other_setters=%llu list_rejected=%llu rows_limit_independent=1 ngx_sr_verified=0 history_verified=0\n",
        unsigned(attested),forwarded,draw_proved,output_proved,rejected,decisions[0],decisions[1],decisions[2],decisions[3],decisions[4],decisions[5],unsigned(store.capacity_lost),store.retired,source_queries,missing_source,other_setters,list_rejected);
    fprintf(file,"dlss_output_last_list draw_raw=%llx draw_canonical=%llx draw_generation=%llu before_owner=%llx before_raw=%llx before_canonical=%llx before_generation=%llu before_valid=%u after_raw=%llx after_canonical=%llx after_generation=%llu after_valid=%u path=context+127c,owner+14,vtable_slot12\n",
        static_cast<unsigned long long>(last_list_draw),static_cast<unsigned long long>(last_list_ref.identity),last_list_ref.generation,
        static_cast<unsigned long long>(last_list_before.owner),static_cast<unsigned long long>(last_list_before.raw),static_cast<unsigned long long>(last_list_before.canonical),last_list_before.generation,unsigned(last_list_before.valid),
        static_cast<unsigned long long>(last_list_after.raw),static_cast<unsigned long long>(last_list_after.canonical),last_list_after.generation,unsigned(last_list_after.valid));
    fprintf(file,"dlss_output_non_target_setters d8388e=%llu d8397d=%llu other=%llu counted_toward_target_proof=0\n",non_target_setters[0],non_target_setters[1],non_target_setters[2]);
    fprintf(file,"dlss_output_non_target_quads preprocessing_194c93=%llu other=%llu counted_toward_target_proof=0\n",non_target_quads[0],non_target_quads[1]);
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
                static_cast<unsigned long long>(d.list),d.generation,static_cast<unsigned long long>(d.source),static_cast<unsigned long long>(d.target),e.forwarded[0],e.forwarded[1],e.forwarded[2],e.forwarded[3],e.forwarded[4],e.forwarded[5],e.forwarded[6],e.forwarded[7],e.dimensions[0],e.dimensions[1],e.dimensions[2],e.dimensions[3]);
            auto const& b=e.binding;
            fprintf(file,"dlss_output_tail_eye allowed=%u eye=%u tail_transition_validated=%u sampled_sourceglobal=%llx\n",
                unsigned(n==0),i,unsigned(e.tail_transition_validated),static_cast<unsigned long long>(b.public_global));
            fprintf(file,"dlss_output_binding_evidence allowed=%u eye=%u complete=%u descriptor=%u,%u,%u slot=%u highwater=%u owner=%llx vtable=%llx view=%llx context=%llx public_global=%llx public_pointer=%llx wrapper=%llx index=%u public_object=%llx native=%llx canonical=%llx format=%u dimensions=%llu,%u list_raw=%llx list_canonical=%llx generation=%llu shader_sampling_verified=0\n",
                unsigned(n==0),i,unsigned(b.complete),b.descriptor[0],b.descriptor[1],b.descriptor[2],b.slot,b.highwater,
                static_cast<unsigned long long>(b.owner),static_cast<unsigned long long>(b.vtable),static_cast<unsigned long long>(b.view),static_cast<unsigned long long>(b.context),
                static_cast<unsigned long long>(b.public_global),static_cast<unsigned long long>(b.public_pointer),static_cast<unsigned long long>(b.public_wrapper),b.public_index,
                static_cast<unsigned long long>(b.public_object),static_cast<unsigned long long>(b.resource.native),static_cast<unsigned long long>(b.resource.canonical),b.resource.format,
                static_cast<unsigned long long>(b.resource.width),b.resource.height,static_cast<unsigned long long>(b.list),static_cast<unsigned long long>(b.canonical_list),b.generation);}
    }
}
#else
ConsumeScope::ConsumeScope(uint64_t,uint64_t,bool){}ConsumeScope::~ConsumeScope(){}
OutputScope::OutputScope(bool,uintptr_t,uintptr_t){}OutputScope::~OutputScope(){}
BindScope::BindScope(bool,uintptr_t,const uint16_t*,void*,void*,uint32_t,int32_t){}BindScope::~BindScope(){}
SetterScope::SetterScope(bool,uintptr_t,const uint16_t*,unsigned){}SetterScope::~SetterScope(){}
QuadScope::QuadScope(bool,uintptr_t,Args const&,Dimensions const&){}QuadScope::~QuadScope(){}
DrawCompletion::DrawCompletion(uintptr_t){}DrawCompletion::~DrawCompletion(){}
bool QuadScope::candidate()const{return false;}
void QuadScope::forward(float&,float&,float&,float&,float&,float&,float&,float&)const{}
bool enable(FILE*,bool){return true;}bool publication(uint64_t,uint64_t){return true;}
bool scoped(){return false;}
void retire(){}void progress(FILE*){}void print(FILE*){}
gears_final_transfer::EyeDrawEvidence draw(uintptr_t,uintptr_t,unsigned,unsigned,unsigned,gears_viewport::TargetDraw const&,gears_frame_link::ListRef,gears_viewport::DrawState const&){return {};}
#endif
}
