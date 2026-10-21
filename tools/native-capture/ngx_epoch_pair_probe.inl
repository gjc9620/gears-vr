// Capture-local diagnostic; included after the global probe header and before
// ngx_stereo_host_live.inl. Never exposes an admission or resource-state API.
namespace ngx_epoch_pair {
#if defined(GEARS_NGX_EPOCH_PAIR_OBSERVER) && GEARS_NGX_EPOCH_PAIR_OBSERVER
#if !defined(GEARS_NGX_STEREO_HOST_OBSERVER) || !GEARS_NGX_STEREO_HOST_OBSERVER
#error NGX epoch pair probe requires the authenticated native host observer
#endif
using namespace gears_ngx_epoch;
Probe probe;TypedLists typed_lists;bool enabled{},printed{},cleanup_pending{};
// Post-Create / accepted Eval observations only. These are the four globals
// consumed by authenticated be65cc, NOT a captured GetOptimal callback result.
struct BoundsSnapshot {
    uint64_t revision{},world{},ticket{};uintptr_t renderer{},scope{},handle{},parameters{};
    std::array<uint32_t,4> first{},last{}; // max W,H; min W,H at RVA1ad1960..6c.
    bool readable{},identity{},stable{};
};
std::array<BoundsSnapshot,4> create_bounds{};std::array<BoundsSnapshot,2> eval_bounds{};
BoundsSnapshot sample_bounds(uint64_t ticket=0,uintptr_t renderer=0,uintptr_t scope=0){
    BoundsSnapshot out;out.revision=probe.revision;out.world=probe.world;out.ticket=ticket;
    out.renderer=renderer;out.scope=scope;out.handle=probe.handle;out.parameters=probe.parameters;
    uintptr_t h0{},p0{},h1{},p1{};const auto world0=committed_world_generation.load();
    const bool before=safe_copy(&h0,reinterpret_cast<void const*>(base+gears_ngx_route::handle_cell_rva),8)&&
        safe_copy(&p0,reinterpret_cast<void const*>(base+gears_ngx_route::parameter_cell_rva),8);
    const bool first=safe_copy(out.first.data(),reinterpret_cast<void const*>(base+0x1ad1960),16);
    const bool last=safe_copy(out.last.data(),reinterpret_cast<void const*>(base+0x1ad1960),16);
    const bool after=safe_copy(&h1,reinterpret_cast<void const*>(base+gears_ngx_route::handle_cell_rva),8)&&
        safe_copy(&p1,reinterpret_cast<void const*>(base+gears_ngx_route::parameter_cell_rva),8);
    out.readable=first&&last;
    out.identity=before&&after&&out.revision&&world0==out.world&&committed_world_generation.load()==out.world&&
        h0==out.handle&&h1==out.handle&&p0==out.parameters&&p1==out.parameters;
    out.stable=out.readable&&out.identity&&out.first==out.last;return out;
}
void print_bounds(FILE* file,char const* stage,unsigned ordinal,BoundsSnapshot const& b){
    if(!b.revision)return;
    fprintf(file,"ngx_epoch_bounds stage=%s ordinal=%u revision=%llu world=%llu ticket=%llu renderer=%llx scope=%llx handle=%llx parameters=%llx readable=%u identity=%u stable=%u order=maxW,maxH,minW,minH first=%u,%u,%u,%u last=%u,%u,%u,%u rva=1ad1960 getoptimal_callback_observed=0 gpu_domain_verified=0\n",
        stage,ordinal,b.revision,b.world,b.ticket,b.renderer,b.scope,b.handle,b.parameters,unsigned(b.readable),unsigned(b.identity),unsigned(b.stable),
        b.first[0],b.first[1],b.first[2],b.first[3],b.last[0],b.last[1],b.last[2],b.last[3]);
}

List list(ID3D12CommandList* p){if(!p)return {};const auto id=canonical_identity(p);auto ref=frame_link.current(id);return {reinterpret_cast<uintptr_t>(p),id,ref.identity==id?ref.generation:0};}
bool consumption(uint64_t& world,uint64_t& ticket,uintptr_t& renderer){
    renderer=reinterpret_cast<uintptr_t>(consuming_renderer);gears_submitted::FrameTicket live;
    if(!renderer||!consuming_frozen_frame.enabled||!submitted_lifetime.frame(renderer,live)||!live.enabled||
       live.id!=consuming_frame_ticket||live.id!=consuming_frozen_frame.id||live.world_generation!=consuming_frozen_frame.world_generation||
       live.world_generation!=committed_world_generation.load())return false;
    world=live.world_generation;ticket=live.id;return true;
}
bool selected(){if(enabled&&probe.armed&&probe.world!=committed_world_generation.load())probe.changed(2);uint64_t w{},t{};uintptr_t r{};return enabled&&recording.load()&&consumption(w,t,r)&&probe.matches(w,t,r);}
Geometry geometry(void* renderer,gears_submitted::FrameTicket const& frame,Record const& record){
    Geometry g;g.world=frame.world_generation;g.ticket=frame.id;g.renderer=reinterpret_cast<uintptr_t>(renderer);
    g.views={record.family_views[0],record.family_views[1]};
    if(frame.enabled&&record.family_count==2&&record.renderer_count==2&&g.views[0]&&g.views[1]&&g.views[0]!=g.views[1])g.valid|=1;
    if(safe_copy(g.extent.data(),reinterpret_cast<void*>(g.renderer+0xd0),8))g.valid|=2;
    if(safe_copy(&g.buffer[0],reinterpret_cast<void*>(base+0x1c3b3b8),4)&&safe_copy(&g.buffer[1],reinterpret_cast<void*>(base+0x1c3b3bc),4))g.valid|=4;
    for(unsigned i=0;i<2;++i)if(g.views[i]&&safe_copy(g.source[i].data(),reinterpret_cast<void*>(g.views[i]+0x48),16)&&
        safe_copy(g.target[i].data(),reinterpret_cast<void*>(g.views[i]+0x58),16)&&safe_copy(g.sampling[i].data(),reinterpret_cast<void*>(g.views[i]+0x3d0),16))g.valid|=8u<<i;
    return g;
}
void built(void* renderer,gears_submitted::FrameTicket const& frame,Record const& record,bool verified,void const* left,void const* right){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!enabled||!probe.armed||probe.selected||probe.finished)return;
    auto g=geometry(renderer,frame,record);
    if(left&&safe_copy(g.constructor[0].data(),left,16))g.constructor_valid|=1;
    if(right&&safe_copy(g.constructor[1].data(),right,16))g.constructor_valid|=2;probe.built(g,verified);
}
void prepare(unsigned stage,void* renderer,Record const& record){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!selected()||renderer!=consuming_renderer)return;
    probe.prepared(stage,geometry(renderer,consuming_frozen_frame,record));
}
bool list_collection(){return enabled&&!cleanup_pending&&recording.load()&&probe.armed&&!probe.finished&&probe.world==committed_world_generation.load();}
void created_list(ID3D12CommandList* command,bool open,bool tracked){
    if(!list_collection()||!command)return;
    typed_lists.boundary(command,list(command),committed_world_generation.load(),!tracked?TypedLists::generation_missing:!open?TypedLists::closed:TypedLists::ok);
}
void creation_failed(){if(list_collection())++typed_lists.create_failures;}
void reset_capture(bool ready){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
    // Existing observations may still be in callbacks. Disable collection but
    // preserve their audit and leases until a drained print/reset can release.
    bool held=typed_lists.held()!=0;for(auto const& r:probe.resources)held=held||r.lease.retained();
    if(held&&(recording.load()||active.load())){probe.changed(4);cleanup_pending=true;printed=false;return;}
    typed_lists={};probe.clear();create_bounds={};eval_bounds={};enabled=ready;printed=cleanup_pending=false;
}
void created(ngx_stereo_host::NativeCreateSource const& source){if(enabled&&!cleanup_pending){const auto old=probe.revision;
    probe.created(source.revision,source.world,source.handle,source.parameters,source.fields);
    // Pure empty epoch rearm never carries candidate identities into the next
    // Create. Accepted Eval leases make the old epoch nonempty and immutable.
    if(old!=probe.revision){typed_lists={};create_bounds[probe.empty_epoch_count]=sample_bounds();}}}
void changed(){if(enabled)probe.changed();}
void released(uintptr_t handle){if(enabled&&handle==probe.handle)probe.changed(1);}
void eval_enter(uintptr_t scope,bool owned,uintptr_t caller,uintptr_t command,void const* structure,bool concurrent){
    if(enabled&&probe.armed&&probe.world!=committed_world_generation.load())probe.changed(2);
    if(!enabled||!probe.armed||probe.finished)return;EvalInput in;in.scope=scope;in.thread=GetCurrentThreadId();
    in.associated=owned&&caller==base+0x9a35d3&&consumption(in.world,in.ticket,in.renderer);
    if(!in.associated){if(probe.selected)probe.gap();else ++probe.skipped;return;}
    if(!probe.selected){++probe.skipped;return;}if(!probe.matches(in.world,in.ticket,in.renderer))return;
    // Join only through retained typed Create/Reset boundaries. Old event rows
    // cannot resurrect a failed Reset, evicted candidate, or stale generation.
    auto found=typed_lists.lookup(command,in.world,[](List saved){auto live=frame_link.current(saved.canonical);return List{saved.raw,live.identity,live.generation};});
    in.command=found.reason?List{command,0,0}:found.saved;
    Record record{};inspect_completed_renderer(record,consuming_renderer);in.geometry=geometry(consuming_renderer,consuming_frozen_frame,record);
    constexpr unsigned offsets[]{0,8,0x18,0x20};constexpr unsigned values[]{0x30,0x34,0x38,0x28,0x2c,0x3c,0x40,0x60,0x64,0x68,0x6c,0x70,0x74,0x88,0x8c};
    in.readable=safe_copy(&in.handle,reinterpret_cast<void*>(base+gears_ngx_route::handle_cell_rva),8)&&safe_copy(&in.parameters,reinterpret_cast<void*>(base+gears_ngx_route::parameter_cell_rva),8);
    const auto address=reinterpret_cast<uintptr_t>(structure);
    for(unsigned i=0;i<4;++i)in.readable=safe_copy(&in.resources[i],reinterpret_cast<void const*>(address+offsets[i]),8)&&in.readable;
    for(unsigned i=0;i<15;++i)in.readable=safe_copy(&in.values[i],reinterpret_cast<void const*>(address+values[i]),4)&&in.readable;
    unsigned extra=found.reason?Probe::typed_lookup_failed:0u;
    if(!extra&&!Probe::input_rejection(in,concurrent)&&in.handle==probe.handle&&in.parameters==probe.parameters&&probe.eval_count<2&&!typed_lists.pin(found))extra|=Probe::pinning_failed;
    const bool had_rejection=probe.rejection.present;const auto prior_evals=probe.eval_count;probe.enter(in,concurrent,extra);
    if(probe.eval_count==prior_evals+1){eval_bounds[prior_evals]=sample_bounds(in.ticket,in.renderer,in.scope);
#if defined(GEARS_NGX_MV_DESCRIPTOR_OBSERVER) && GEARS_NGX_MV_DESCRIPTOR_OBSERVER
        // Match the integer only to an existing typed D3D boundary lease. A
        // missing MV stays missing; descriptor provenance never supplies it.
        gears_ngx_stereo::TypedResourceLease const* mv{};
        for(unsigned i=0;i<probe.resource_count;++i)if(in.resources[3]&&probe.resources[i].lease.raw==in.resources[3]&&probe.resources[i].lease.retained()){
            mv=&probe.resources[i].lease;break;}
        ngx_mv_descriptor_capture::accepted_eval(prior_evals,probe.revision,in.world,in.ticket,in.renderer,in.handle,in.parameters,mv);
#endif
    }
    if(!had_rejection&&probe.rejection.present){auto& r=probe.rejection;r.lookup_reason=found.reason;r.saved=found.saved;r.current=found.current;
        const auto tracker=frame_link.snapshot();r.tracker_overflow=tracker.list_overflow;r.tracker_capacity_bad=tracker.capacity_bad;}

}
void eval_leave(uintptr_t scope,bool returned,uint64_t result){if(enabled)probe.leave(scope,returned,result);}
int dispatch(uintptr_t c,uintptr_t h,uintptr_t p,uintptr_t cb){return enabled?probe.dispatch(c,h,p,cb,GetCurrentThreadId()):-1;}
void dispatched(int index,uint32_t result){if(enabled)probe.dispatched(index,result);}
struct BarrierScope {
    uint64_t from{},to{};List before{};bool active{};
    BarrierScope(ID3D12GraphicsCommandList* command,UINT count,D3D12_RESOURCE_BARRIER const* values,uintptr_t caller){
        if(!selected())return;active=true;before=list(command);from=probe.serial;
        if(count>256||(count&&!values)){probe.gap();return;}
        for(unsigned i=0;i<count;++i){D3D12_RESOURCE_BARRIER b;if(!safe_copy(&b,values+i,sizeof(b))){probe.gap();break;}probe.barrier(before,b,caller);}to=probe.serial;
    }
    void returned(ID3D12GraphicsCommandList* command){if(active)probe.barrier_return(from,to,list(command));}
};
struct CopyScope {
    int index=-1;D3D12_TEXTURE_COPY_LOCATION src{},dst{};D3D12_BOX box{};bool has_box{};
    CopyScope(ID3D12GraphicsCommandList* c,D3D12_TEXTURE_COPY_LOCATION const* d,UINT x,UINT y,UINT z,D3D12_TEXTURE_COPY_LOCATION const* s,D3D12_BOX const* b,uintptr_t caller){
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);
        // Copy passthrough supplies _ReturnAddress()-base, unlike Eval's absolute caller.
        if(!selected()||caller!=0x1c6093)return;
        if(!safe_copy(&src,s,sizeof(src))||!safe_copy(&dst,d,sizeof(dst))||(b&&!safe_copy(&box,b,sizeof(box)))){probe.gap();return;}
        has_box=b!=nullptr;index=probe.copy(list(c),dst,{x,y,z},src,b?&box:nullptr,caller);
    }
    void returned(ID3D12GraphicsCommandList* c,D3D12_TEXTURE_COPY_LOCATION const* d,D3D12_TEXTURE_COPY_LOCATION const* s,D3D12_BOX const* b){
        if(index<0)return; // An unselected copy owns no observation or typed list lease.
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);D3D12_TEXTURE_COPY_LOCATION sd{},dd{};bool valid=safe_copy(&sd,s,sizeof(sd))&&safe_copy(&dd,d,sizeof(dd))&&
            sd.pResource==src.pResource&&dd.pResource==dst.pResource&&sd.Type==src.Type&&dd.Type==dst.Type;
        if(valid&&src.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)valid=sd.SubresourceIndex==src.SubresourceIndex;
        if(valid&&dst.Type==D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX)valid=dd.SubresourceIndex==dst.SubresourceIndex;
        D3D12_BOX after{};valid=valid&&(has_box==(b!=nullptr))&&(!b||(safe_copy(&after,b,sizeof(after))&&memcmp(&after,&box,sizeof(box))==0));
        probe.copied(index,list(c),valid);
    }
};
struct ExecuteScope {
    uint64_t order{};uintptr_t queue{};unsigned count{};std::array<List,64> before{};std::array<ID3D12CommandList*,64> raw{};
    ExecuteScope(ID3D12CommandQueue* q,UINT n,ID3D12CommandList* const* commands){
        std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(enabled&&probe.armed&&probe.world!=committed_world_generation.load())probe.changed(2);
        if(!enabled||!probe.observing())return;if(n>raw.size()||(n&&!safe_copy(raw.data(),commands,n*sizeof(raw[0])))){probe.gap();return;}
        queue=canonical_identity(q);count=n;for(unsigned i=0;i<n;++i)before[i]=list(raw[i]);order=probe.execute_begin(queue,before.data(),count);
    }
    void returned(){std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!order)return;std::array<List,64> after{};for(unsigned i=0;i<count;++i)if(probe.executing(order,i,before[i]))after[i]=list(raw[i]);probe.execute_end(order,queue,before.data(),after.data(),count);}
};
void reset(List before,ID3D12CommandList* command,HRESULT result){
    if(!enabled)return;const bool collecting=list_collection();if(!collecting&&!probe.observing())return;
    const auto after=list(command);
    if(collecting&&command&&command->GetType()==D3D12_COMMAND_LIST_TYPE_DIRECT)
        typed_lists.boundary(command,SUCCEEDED(result)?after:before,committed_world_generation.load(),SUCCEEDED(result)?TypedLists::ok:TypedLists::boundary_failed);
    if(probe.observing())probe.reset(before,after,SUCCEEDED(result));
}
void print_geometry(FILE* file,char const* stage,Geometry const& g){
    fprintf(file,"ngx_epoch_geometry stage=%s world=%llu ticket=%llu renderer=%llx valid=%x constructor_valid=%x extent=%u,%u buffer=%u,%u\n",stage,g.world,g.ticket,g.renderer,g.valid,g.constructor_valid,g.extent[0],g.extent[1],g.buffer[0],g.buffer[1]);
    for(unsigned i=0;i<2;++i)fprintf(file,"ngx_epoch_view stage=%s ordinal=%u view=%llx source=%.9g,%.9g,%.9g,%.9g target=%d,%d,%d,%d sampling=%.9g,%.9g,%.9g,%.9g constructor=%.9g,%.9g,%.9g,%.9g\n",stage,i,g.views[i],
        g.source[i][0],g.source[i][1],g.source[i][2],g.source[i][3],g.target[i][0],g.target[i][1],g.target[i][2],g.target[i][3],g.sampling[i][0],g.sampling[i][1],g.sampling[i][2],g.sampling[i][3],g.constructor[i][0],g.constructor[i][1],g.constructor[i][2],g.constructor[i][3]);
}
void print(FILE* file){
    std::lock_guard<FrameHookMutex> lock(frame_hook_mutex);if(!enabled||printed)return;if(probe.armed&&probe.world!=committed_world_generation.load())probe.changed(2);const bool drained=!recording.load()&&active.load()==0;printed=drained;
    fprintf(file,"ngx_epoch_pair enabled=1 armed=%u selected=%u finished=%u stop=%u end_reason=%u empty_epochs=%u candidate_complete=%u drained=%u revision=%llu world=%llu ticket=%llu renderer=%llx handle=%llx parameters=%llx dropped=%u unknown=%u skipped_initial=%u resources=%u events=%u evals=%u gpu_completion_verified=0 producer_domain_verified=0 sdk_mutation=0 gpu_commands_added=0\n",unsigned(probe.armed),unsigned(probe.selected),unsigned(probe.finished),probe.stop,probe.end_reason,probe.empty_epoch_count,unsigned(drained&&probe.complete()),unsigned(drained),probe.revision,probe.world,probe.ticket,probe.renderer,probe.handle,probe.parameters,probe.dropped,probe.unknown,probe.skipped,probe.resource_count,probe.event_count,probe.eval_count);
    fprintf(file,"ngx_epoch_scope execute_members_excluded=%llu resets_excluded=%llu filter=selected_barrier_eval_copy_recordings membership_frozen_at_execute_begin=1 earlier_history_complete=0\n",probe.excluded_execute_members,probe.excluded_resets);
    fprintf(file,"ngx_epoch_lists candidates_limit=64 pinned_limit=2 held=%u pinned=%u evicted=%llu create_failures=%llu lookup_source=typed_create_reset gpu_completion_verified=0\n",typed_lists.held(),typed_lists.pinned,typed_lists.evicted,typed_lists.create_failures);
    fprintf(file,"ngx_epoch_reject_schema mask_bits=1:concurrent,2:unassociated,4:unreadable,8:handle,16:parameters,32:raw,64:canonical,128:generation,256:scope,512:typed_lookup,1024:pin lookup=0:ok,1:miss,2:closed,3:boundary_failed,4:generation_missing,5:world,6:generation_changed,7:pin_limit\n");
    if(probe.rejection.present){auto const& r=probe.rejection;auto const& in=r.input;
        fprintf(file,"ngx_epoch_reject mask=%x lookup=%u world=%llu ticket=%llu renderer=%llx scope=%llx command=%llx saved=%llx,%llx,%llu current=%llx,%llx,%llu handle=%llx parameters=%llx tracker_list_overflow=%llu tracker_capacity_bad=%u\n",r.mask,r.lookup_reason,in.world,in.ticket,in.renderer,in.scope,in.command.raw,r.saved.raw,r.saved.canonical,r.saved.generation,r.current.raw,r.current.canonical,r.current.generation,in.handle,in.parameters,r.tracker_overflow,unsigned(r.tracker_capacity_bad));}
    if(!drained)return;
    fprintf(file,"ngx_epoch_schema version=1 resources_order=Color,Output,Depth,MotionVectors event_kinds=1:transition,2:uav,3:alias,4:eval_begin,5:eval_dispatch,6:eval_return,7:copy_begin,8:copy_return,9:execute_begin,10:execute_return,11:reset stops=0:open,1:captured,2:epoch_end,3:input_gap,4:extra_eval,5:unwind,6:capacity,8:empty_epoch_budget end_reasons=1:release,2:world,3:recreate,4:invalid_create logical_eye_verified=0 state_certified=0 barrier_state_is_request=1 execute_order_is_cpu_call_order=1 queue_dependency_verified=0\n");
    for(unsigned i=0;i<probe.empty_epoch_count;++i){auto const& e=probe.empty_epochs[i];fprintf(file,"ngx_epoch_empty index=%u revision=%llu world=%llu handle=%llx parameters=%llx reason=%u skipped=%u fields=%u,%u,%u,%u,%u,%u,%u pair_evidence=0\n",i,e.revision,e.world,e.handle,e.parameters,e.reason,e.skipped,e.fields[0],e.fields[1],e.fields[2],e.fields[3],e.fields[4],e.fields[5],e.fields[6]);}
    fprintf(file,"ngx_epoch_create fields=%u,%u,%u,%u,%u,%u,%u\n",probe.creation[0],probe.creation[1],probe.creation[2],probe.creation[3],probe.creation[4],probe.creation[5],probe.creation[6]);
    for(unsigned i=0;i<create_bounds.size();++i)print_bounds(file,"post_create",i,create_bounds[i]);
    for(unsigned i=0;i<probe.eval_count;++i)print_bounds(file,"eval_enter",i,eval_bounds[i]);
    print_geometry(file,"built",probe.geometry[0]);print_geometry(file,"prepare_before",probe.geometry[1]);print_geometry(file,"prepare_after",probe.geometry[2]);
    for(unsigned i=0;i<probe.resource_count;++i){auto const& t=probe.resources[i];auto const& r=t.lease;auto const& d=r.desc;
        fprintf(file,"ngx_epoch_resource index=%u raw=%llx canonical=%llx device=%llx retained=%u dimension=%u width=%llu height=%u format=%u mips=%u layers=%u samples=%u quality=%u flags=%u layout=%u alignment=%llu first=%llu last=%llu observations=%u\n",i,r.raw,r.canonical,r.device,unsigned(r.retained()),unsigned(d.Dimension),d.Width,d.Height,unsigned(d.Format),d.MipLevels,d.DepthOrArraySize,d.SampleDesc.Count,d.SampleDesc.Quality,unsigned(d.Flags),unsigned(d.Layout),d.Alignment,t.first,t.last,t.observations);}
    for(unsigned i=0;i<probe.eval_count;++i){auto const& e=probe.evals[i];auto const& in=e.input;
        fprintf(file,"ngx_epoch_eval ordinal=%u scope=%llx thread=%u begin=%llu end=%llu returned=%u result=%llx dispatches=%u dispatch_returned=%u dispatch_match=%u dispatch_result=%x command=%llx,%llx,%llu raw_CODM=%llx,%llx,%llx,%llx typed_before=%x typed_final=%x values=",i,in.scope,in.thread,e.begin,e.end,unsigned(e.returned),e.result,e.dispatches,unsigned(e.dispatch_returned),unsigned(e.dispatch_match),e.dispatch_result,in.command.raw,in.command.canonical,in.command.generation,in.resources[0],in.resources[1],in.resources[2],in.resources[3],e.typed_mask,probe.mask(in.resources));
        for(unsigned j=0;j<15;++j)fprintf(file,"%s%u",j?",":"",in.values[j]);fprintf(file,"\n");print_geometry(file,i?"eval1":"eval0",in.geometry);
        fprintf(file,"ngx_epoch_copy ordinal=%u copies=%u returned=%u list=%llx,%llx,%llu dst=%u,%u,%u boxed=%u box=%u,%u,%u,%u,%u,%u types=%u,%u subresources=%u,%u eval_executes=%u eval_generation=%u eval_queue=%llx eval_order=%llu eval_member=%u copy_executes=%u copy_generation=%u copy_queue=%llx copy_order=%llu copy_member=%u\n",i,e.copies,unsigned(e.copy_returned),e.copy_list.raw,e.copy_list.canonical,e.copy_list.generation,e.destination[0],e.destination[1],e.destination[2],unsigned(e.boxed),e.box.left,e.box.top,e.box.front,e.box.right,e.box.bottom,e.box.back,e.source_type,e.target_type,e.source_subresource,e.target_subresource,e.eval_executes,unsigned(e.eval_generation),e.eval_queue,e.eval_order,e.eval_member,e.copy_executes,unsigned(e.copy_generation),e.copy_queue,e.copy_order,e.copy_member);
    }
    for(unsigned i=0;i<probe.event_count;++i){auto const& e=probe.events[i];fprintf(file,"ngx_epoch_event serial=%llu kind=%u list=%llx,%llx,%llu resource=%llx other=%llx queue=%llx caller=%llx order=%llu before=%u after=%u sub=%u flags=%u member=%u count=%u returned=%u generation_match=%u\n",e.serial,unsigned(e.kind),e.list.raw,e.list.canonical,e.list.generation,e.resource,e.other,e.queue,e.caller,e.order,e.before,e.after,e.subresource,e.flags,e.member,e.count,unsigned(e.returned),unsigned(e.current));}
    probe.release_leases();typed_lists.release();
}
#else
inline void reset_capture(bool){}inline void created(ngx_stereo_host::NativeCreateSource const&){}inline void changed(){}inline void released(uintptr_t){}
inline void eval_enter(uintptr_t,bool,uintptr_t,uintptr_t,void const*,bool){}inline void eval_leave(uintptr_t,bool,uint64_t){}
inline int dispatch(uintptr_t,uintptr_t,uintptr_t,uintptr_t){return -1;}inline void dispatched(int,uint32_t){}
inline void built(void*,gears_submitted::FrameTicket const&,Record const&,bool,void const*,void const*){}inline void prepare(unsigned,void*,Record const&){}
inline void print(FILE*){}
#endif
}
