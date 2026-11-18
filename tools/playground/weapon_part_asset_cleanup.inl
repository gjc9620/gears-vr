// Included inside the existing resource owner; shared by production and the
// directed ownership regression. No lifetime belongs to a borrowed mesh here.
bool cleanup(Resource& r) noexcept {
    if(!gt())return false;
    r.publication_withdrawn=true;r.stage=Stage::retiring;
    if(r.borrowed_mesh&&r.mesh_root){r.stage=Stage::failed;faulted=true;failure=81;return false;}
    // Failed raw LOD construction is quarantined. Native GC must not destroy
    // unconstructed vtables. Preserve the root and stop resource reuse.
    if(r.lod_allocation_dispatched&&!r.lod_constructed){r.stage=Stage::failed;faulted=true;failure=80;return false;}
    if(r.component_root){
        if(!hidden(r,true)||!attachment(r,false)||(r.mesh_bound&&!bind_mesh(r,0)))return false;
        if(r.lighting_bound||r.lighting_assigned){
            uptr environment{};
            if(!pointer_field(r.component,L"LightEnvironment",environment,true)||
               (environment&&!object_call(r.component,L"SetLightEnvironment",L"PrimitiveComponent",L"NewLightEnvironment",L"ComponentProperty",0)))return false;
            r.lighting_bound=r.lighting_assigned=false;
        }
        if(!unroot(r.component,r.component_root))return false;
        r.component={}; // No dereference after releasing the private root.
    }
    if(r.mesh_root){
        if(!unroot(r.mesh,r.mesh_root))return false;
        r.mesh={}; // Engine GC/BeginDestroy owns its render fence/resources.
    }
    r={};outstanding.fetch_sub(1,std::memory_order_acq_rel);return true;
}
