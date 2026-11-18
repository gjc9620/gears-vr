// Read-only mounted Scene binding. Include after weapon_hand_scene_binding.
// Caller owns the original admitted Scene and stereo_mutex. This is not an
// ordinary WarPawn Binding, a public hand Frame, a C0 grant or an input reader.
namespace troika_scene_binding {
namespace original=weapon_hand_scene_binding;
struct Binding {
    gears_anchor::Identity world{},controller{},turret{},driver{},driver_mesh{},turret_mesh{},weapon{};
    uint64_t world_generation{},scene_invocation{};
    int64_t scene_qpc{},family_qpc{},renderer_qpc{};
    unsigned thread{};
};
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
original::Proof parts_proof{},weapon_proof{};
bool same(Binding const& a,Binding const& b){
    return a.world_generation==b.world_generation&&a.scene_invocation==b.scene_invocation&&
        a.scene_qpc==b.scene_qpc&&a.family_qpc==b.family_qpc&&a.renderer_qpc==b.renderer_qpc&&a.thread==b.thread&&
        original::same(a.world,b.world)&&original::same(a.controller,b.controller)&&original::same(a.turret,b.turret)&&
        original::same(a.driver,b.driver)&&original::same(a.driver_mesh,b.driver_mesh)&&
        original::same(a.turret_mesh,b.turret_mesh)&&original::same(a.weapon,b.weapon);
}
bool parts(gears_anchor::Identity const& turret,gears_anchor::Identity const& driver){
    gears_anchor::Identity tc{},dc{};
    if(!original::identity(turret.cls,tc)||!original::identity(driver.cls,dc))return false;
    original::keys(parts_proof,tc,dc);
    auto& p=parts_proof;if(!original::begin_prepare(p))return false;if(p.ready)return true;
    if(!original::property(p,tc.address,L"Mesh",L"MeshPointer",8,L"SkeletalMeshComponent",p.fields[0])||
       !original::property(p,dc.address,L"Mesh",L"MeshPointer",8,L"SkeletalMeshComponent",p.fields[1])||
       !original::property(p,tc.address,L"Weapon",L"ObjectProperty",8,L"Weapon",p.fields[2])||!original::current(p))return false;
    p.ready=true;return true;
}
bool instigator(gears_anchor::Identity const& weapon){
    gears_anchor::Identity cls{};if(!original::identity(weapon.cls,cls))return false;
    original::keys(weapon_proof,cls);auto& p=weapon_proof;
    if(!original::begin_prepare(p))return false;if(p.ready)return true;
    if(!original::property(p,cls.address,L"Instigator",L"ObjectProperty",8,L"Pawn",p.fields[0])||!original::current(p))return false;
    p.ready=true;return true;
}
bool root(gears_anchor::Receipt const& scene,Binding& out){
    auto const& config=original::metadata;
    if(!config.mounted_enabled||config.mounted_role_driver>=config.identity_count||
       config.mounted_role_world_info>=config.identity_count||scene.anchor.cinematic||scene.anchor.unpossessed)return false;
    gears_anchor::Anchor live{};uint32_t stamp{};
    if(!gears_anchor::guard(config,safe_copy,live,stamp)||!gears_anchor::equal_anchor(live,scene.anchor))return false;
    Binding next{};next.world_generation=original::metadata_generation;next.scene_invocation=scene.invocation;
    next.scene_qpc=scene.scene_qpc;next.family_qpc=scene.family_qpc;next.renderer_qpc=scene.renderer_qpc;next.thread=scene.thread;
    if(!original::identity(live.controller,next.controller,true)||!original::identity(live.pawn,next.turret,true)||
       !original::identity(config.identities[config.mounted_role_driver].address,next.driver,true)||
       !original::identity(config.identities[config.mounted_role_world_info].address,next.world,true)||
       !original::same(next.driver,config.identities[config.mounted_role_driver])||
       !original::same(next.world,config.identities[config.mounted_role_world_info]))return false;
    original::ClassPath path{};
    if(!original::derives(next.turret.cls,L"Turret_TroikaCabal_base",0,path)||
       !original::derives(next.driver.cls,L"WarPawn",0,path)||!parts(next.turret,next.driver))return false;
    uintptr_t tm{},dm{},weapon{},owner{};
    auto const& p=parts_proof;
    if(!original::field(tm,next.turret.address,p.fields[0].offset)||!tm||
       !original::field(dm,next.driver.address,p.fields[1].offset)||!dm||tm==dm||
       !original::field(weapon,next.turret.address,p.fields[2].offset)||!weapon||
       !original::identity(tm,next.turret_mesh,true)||!original::identity(dm,next.driver_mesh,true)||
       !original::identity(weapon,next.weapon,true)||
       !original::derives(next.turret_mesh.cls,L"SkeletalMeshComponent",p.fields[0].reference.address,path)||
       !original::derives(next.driver_mesh.cls,L"SkeletalMeshComponent",p.fields[1].reference.address,path)||
       !original::derives(next.weapon.cls,L"Weap_Troika_base",p.fields[2].reference.address,path)||
       !instigator(next.weapon)||!original::derives(next.turret.cls,L"Pawn",weapon_proof.fields[0].reference.address,path)||
       !original::field(owner,weapon,weapon_proof.fields[0].offset)||owner!=next.turret.address)return false;
    out=next;return true;
}
bool sample(gears_anchor::Receipt const* scene,Binding& out){
    try{
        std::lock_guard<std::mutex> lock(original::mutex);
        if(!scene||!scene->invocation||scene->thread!=GetCurrentThreadId()||
           scene->scene_qpc<=0||scene->family_qpc<scene->scene_qpc||scene->renderer_qpc<scene->family_qpc||
           !original::synchronize_metadata()||!original::context_ready())return false;
        std::array<unsigned char,sizeof(gears_anchor::Receipt)> receipt{};
        std::memcpy(receipt.data(),scene,receipt.size());Binding first{},closing{};
        if(!root(*scene,first)||!original::current(parts_proof)||!original::current(weapon_proof)||
           !root(*scene,closing)||!same(first,closing)||!original::current(parts_proof)||!original::current(weapon_proof)||
           !original::context_ready()||first.world_generation!=committed_world_generation.load()||
           std::memcmp(receipt.data(),scene,receipt.size()))return false;
        // The original ordinary binder's game-thread latch is observed, never
        // changed by this distinct mounted path. No ordinary Frame is made.
        out=first;return true;
    }catch(...){return false;}
}
#else
bool sample(gears_anchor::Receipt const*,Binding&){return false;}
#endif
} // namespace troika_scene_binding
