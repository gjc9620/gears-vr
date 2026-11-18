bool bind_mesh(Resource& r,uptr mesh) noexcept {
    if(!r.component_root||!object(r.component))return false;
    if(mesh&&((r.mesh_root==r.borrowed_mesh)||!object(r.mesh)||mesh!=r.mesh.address))return false;
    uptr before{};
    if(!pointer_field(r.component,L"StaticMesh",before))return false;
    if(before==mesh){r.mesh_bound=mesh!=0;return true;}
    // Journal before dispatch: a successful call followed by an expired read
    // must still be detached during cleanup, never repeated as a new resource.
    if(mesh)r.mesh_bound=true;
    if(!object_call(r.component,L"SetStaticMesh",L"StaticMeshComponent",L"NewMesh",L"ObjectProperty",mesh))return false;
    uptr actual{};if(!pointer_field(r.component,L"StaticMesh",actual)||actual!=mesh)return false;
    r.mesh_bound=mesh!=0;return true;
}
