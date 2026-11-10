"""Exact single-runtime-file delta on the actual r149 source."""
def instrument(text):
    old='struct BodyLayout {binding::Proof proof{};gears_anchor::Identity cls{};std::array<binding::Property,4> fields{};};'
    new='''struct BodyLayout {
    binding::Proof proof{};gears_anchor::Identity cls{};std::array<binding::Property,4> fields{};
    std::uintptr_t mesh_role_class{},mesh_role_reference{};unsigned mesh_role_pass{};
};'''
    assert text.count(old)==1;text=text.replace(old,new)
    marker='// Close every sibling, including those AFTER the selected property.'
    helper='''inline bool body_mesh_role(BodyLayout& layout,std::uintptr_t mesh_class,unsigned pass){
    const auto reference=layout.fields[1].reference.address;
    // One successful certification per exact layout/key and values pass.
    // sample() clears ALL layouts before pass1; pass2 independently certifies
    // again. Every class path/source remains in proof for layouts_current().
    // This skips only repeated same-pass derivation for sibling body instances,
    // never their identity, owner, pointers, indices or the closing proof.
    if(pass&&layout.mesh_role_pass==pass&&layout.mesh_role_class==mesh_class&&
       layout.mesh_role_reference==reference)return true;
    if(!class_role(layout.proof,mesh_class,L"SkeletalMeshComponent",reference))return false;
    layout.mesh_role_class=mesh_class;layout.mesh_role_reference=reference;layout.mesh_role_pass=pass;
    return true;
}
'''
    assert text.count(marker)==1;text=text.replace(marker,helper+marker)
    old='if(!class_role(l->proof,next.mesh.cls,L"SkeletalMeshComponent",l->fields[1].reference.address))return Code::layout;'
    new='if(!body_mesh_role(*l,next.mesh.cls,w.witness.pass))return Code::layout;'
    assert text.count(old)==1;return text.replace(old,new)
