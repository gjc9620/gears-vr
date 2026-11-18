// Reserve only a hidden transient COMPONENT around an already existing static
// asset. Original mesh construction, root and RHI ownership are never adopted.
// Caller authenticates its exact source asset/material/scale on every use.
Creation create_existing_static(hand::Frame const& f,std::uint64_t owner_generation,Identity const& source_mesh,
    std::vector<Identity> const& source_materials,playground::Vector const& source_scale,Handle& handle) noexcept {
    handle={};failure=0;
    if(!gt()||inside_create||faulted||stop_requested.load(std::memory_order_acquire)||!owner_generation||
       !object(source_mesh)||!playground::instance(source_mesh.address,L"StaticMesh")||
       source_materials.empty()||source_materials.size()>32||!next_generation||next_generation==UINT_MAX)return Creation::rejected;
    for(float part_scale:{source_scale.x,source_scale.y,source_scale.z})
        if(!std::isfinite(part_scale)||part_scale<=.1f||part_scale>=10.f)return Creation::rejected;
    for(auto const& source_material:source_materials)
        if(!object(source_material)||!playground::instance(source_material.address,L"MaterialInterface"))return Creation::rejected;
    if(!nativeequipment::current(nullptr,f))return Creation::rejected;
    struct Scope {Scope(){inside_create=true;}~Scope(){inside_create=false;}} scope;
    Resource* slot=nullptr;unsigned index{};
    for(unsigned i=0;i<resources.size();++i)if(resources[i].stage==Stage::empty){slot=&resources[i];index=i;break;}
    if(!slot||!object(source_mesh)||!nativeequipment::current(nullptr,f))return Creation::rejected;
    auto& r=*slot;r.owner={f.world,f.pawn,f.key.game_start,f.key.native_started_qpc,
        f.key.registration_cookie,f.key.world_generation,owner_generation};
    r.generation=next_generation++;r.stage=Stage::reserved;r.borrowed_mesh=true;
    r.mesh=source_mesh;r.original_scale=source_scale;outstanding.fetch_add(1,std::memory_order_acq_rel);
    try {
        r.materials=source_materials;r.stage=Stage::original_mesh_ready;
        handle={index,r.generation};return Creation::pending;
    }catch(...){r.stage=Stage::retiring;cleanup(r);failure=82;return Creation::rejected;}
}
