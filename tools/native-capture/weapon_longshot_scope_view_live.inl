// Final display-only selection. Reuses the admitted Scene, current Broker and
// read-only object metadata; no hook, camera/rig writer or engine invocation.
namespace longshot_scope_view_live {
namespace api=gears_weapon_longshot_scope_view;namespace hand=gears_weapon_hand;
namespace binding=weapon_hand_scene_binding;
struct Bound {api::Request request{};binding::Proof proof{};std::array<binding::Property,5> fields{};DWORD thread{};bool ready{};};
Bound bound;
template<class T>bool read(std::uintptr_t at,T& out) noexcept {
    return at&&at<=UINTPTR_MAX-sizeof(T)&&safe_copy(&out,reinterpret_cast<void const*>(at),sizeof(T));
}
bool object(hand::ObjectIdentity const& wanted) {
    gears_anchor::Identity id{};
    return hand::identity_valid(wanted)&&binding::identity(wanted.address,id,true)&&id.address==wanted.address&&
        id.outer==wanted.outer&&id.cls==wanted.klass&&id.index==wanted.registry_index&&
        id.name_index==wanted.name_index&&id.number==wanted.name_number;
}
bool derives(hand::ObjectIdentity const& id,wchar_t const* type,wchar_t const* package) {
    binding::ClassPath path{};if(!binding::derives(id.klass,type,0,path))return false;
    for(unsigned i=0;i<path.count;++i)if(binding::named(path.ids[i],type)){
        gears_anchor::Identity outer{};return binding::identity(path.ids[i].outer,outer)&&!outer.outer&&binding::named(outer,package);
    }return false;
}
bool layout(Bound& b) {
    const auto& q=b.request;auto& p=b.proof;auto& fields=b.fields;
    if(!derives(q.source.weapon,L"Weap_SniperRifle",L"WarfareGame")||
       !derives(q.component,L"SkeletalMeshComponent",L"Engine")||!derives(q.asset,L"SkeletalMesh",L"Engine"))return false;
    gears_anchor::Identity asset{};
    if(!binding::identity(q.asset.address,asset,true)||!binding::named(asset,L"LongShot")||
       !binding::property(p,q.source.pawn.klass,L"Weapon",L"MeshPointer",8,nullptr,fields[0])||
       !binding::property(p,q.source.weapon.klass,L"Mesh",L"MeshPointer",8,nullptr,fields[1])||
       !binding::property(p,q.source.weapon.klass,L"Instigator",L"ObjectProperty",8,nullptr,fields[2])||
       !binding::property(p,q.component.klass,L"SkeletalMesh",L"ObjectProperty",8,nullptr,fields[3])||
       !binding::property(p,q.component.klass,L"LocalToWorld",L"StructProperty",64,nullptr,fields[4]))return false;
    std::uintptr_t structure{};gears_anchor::Identity matrix{};unsigned size{};
    return binding::source_field(p,structure,fields[4].id.address,0xd0)&&binding::certified(p,structure,matrix)&&
        binding::named(matrix,L"Matrix")&&binding::source_field(p,size,structure,0x88)&&size==64&&binding::current(p);
}
bool snapshot(Bound const& b,std::array<float,16>& matrix) {
    const auto& q=b.request;const auto& f=q.source;
    if(!b.ready||GetCurrentThreadId()!=b.thread||!binding::metadata_current()||!binding::current(b.proof))return false;
    for(auto const& id:{f.world,f.controller,f.pawn,f.weapon,q.component,q.asset})if(!object(id))return false;
    const std::array<std::uintptr_t,4> owners{f.pawn.address,f.weapon.address,f.weapon.address,q.component.address};
    const std::array<std::uintptr_t,4> values{f.weapon.address,q.component.address,f.pawn.address,q.asset.address};
    for(unsigned i=0;i<4;++i){std::uintptr_t value{};
        if(!read(owners[i]+b.fields[i].offset,value)||value!=values[i])return false;}
    std::array<float,16> again{};
    if(!read(q.component.address+b.fields[4].offset,matrix)||!read(q.component.address+b.fields[4].offset,again)||matrix!=again)return false;
    for(auto const& id:{f.world,f.controller,f.pawn,f.weapon,q.component,q.asset})if(!object(id))return false;
    return binding::current(b.proof);
}
std::uint32_t publish(api::Request const* input) noexcept try {
    api::Request q{};auto* broker=weapon_hand_runtime::current();
    if(!input||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=2||q.active>1||q.eye>1||
       !broker||!broker->admit_weapon_render(q.source))return unsigned(api::Result::rejected);
    if(!q.active){bound.request={};bound.ready=false;return unsigned(api::Result::applied);}
    if(!gears_vr::stereo_pose_valid(q.model_world)||!std::isfinite(q.model_scale)||q.model_scale<.1||q.model_scale>10)return 0;
    if(!std::isfinite(q.original_targeting_fov_degrees)||q.original_targeting_fov_degrees<2||q.original_targeting_fov_degrees>90)return 0;
    std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
    if(!lock.owns_lock()||!binding::metadata_current())return 0;
    const bool same=bound.ready&&bound.thread==GetCurrentThreadId()&&hand::same(bound.request.component,q.component)&&
        hand::same(bound.request.asset,q.asset)&&hand::same(bound.request.source.weapon,q.source.weapon)&&
        bound.request.source.key.world_generation==q.source.key.world_generation&&binding::current(bound.proof);
    if(!same){bound.ready=false;bound.proof={};bound.fields={};bound.request=q;bound.thread=GetCurrentThreadId();
        if(!layout(bound))return 0;bound.ready=true;}
    else bound.request=q;
    std::array<float,16> matrix{};
    if(!snapshot(bound,matrix)||!broker->admit_weapon_render(q.source)){bound.ready=false;return 0;}
    return unsigned(api::Result::applied);
}catch(...){bound.ready=false;return 0;}
bool select(std::uint64_t world,std::uint64_t reference,gears_vr::StereoSample const& sample,
            double units,gears_vr::StereoMatrices& matrices) noexcept try {
    if(!bound.ready||!bound.request.active||!std::isfinite(units)||units<10||units>500)return false;
    const auto& q=bound.request;const auto& f=q.source;auto* broker=weapon_hand_runtime::current();
    if(f.key.world_generation!=world||f.reference_generation!=reference||f.stereo.session!=sample.session||
       f.stereo.space_epoch!=sample.space_epoch||f.stereo.source!=sample.source||!broker||!broker->admit_weapon_render(f))return false;
    std::array<float,16> m{};
    {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        if(!lock.owns_lock()||!snapshot(bound,m))return false;}
    for(float v:m)if(!std::isfinite(v))return false;
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return false;
    const double scale=std::hypot(std::hypot(double(m[4]),double(m[5])),double(m[6]));
    if(!std::isfinite(scale)||scale<.1||scale>10||std::abs(scale-q.model_scale)>.001)return false;
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
        double product{};for(unsigned k=0;k<3;++k)product+=double(m[4*i+k])*m[4*j+k]/(scale*scale);
        if(std::abs(product-(i==j?1.:0.))>1e-4)return false;}
    const auto center=gears_weapon::longshot_eye_contact::ocular_center_raw;
    const auto axes=gears_vr::rotation_matrix(q.model_world.orientation);
    const auto offset=gears_vr::transform(axes,center);
    const auto position=q.model_world.position;
    const gears_vr::Vector ocular{(position.x+offset.x*scale)/units,
        (position.y+offset.y*scale)/units,(position.z+offset.z*scale)/units};
    const auto bore=gears_vr::transform(axes,{0,1,0});
    const auto& view=matrices.view[q.eye];const auto eye=gears_vr::stereo_camera_position(view);
    const gears_vr::Vector delta{eye.x/units-ocular.x,eye.y/units-ocular.y,eye.z/units-ocular.z};
    const double distance=-(delta.x*bore.x+delta.y*bore.y+delta.z*bore.z);
    const gears_vr::Vector lateral{delta.x+distance*bore.x,delta.y+distance*bore.y,delta.z+distance*bore.z};
    const double cosine=std::clamp(bore.x*view[2]+bore.y*view[6]+bore.z*view[10],-1.,1.);
    const gears_weapon::longshot_scope_control::EyeContact contact{distance,std::hypot(std::hypot(lateral.x,lateral.y),lateral.z),
        std::acos(cosine)*180/3.14159265358979323846,true};
    if(!gears_weapon::longshot_scope_control::in_eye_box(contact,true)||!broker->admit_weapon_render(f))return false;
    return gears_weapon::longshot_scope_projection::select(matrices,bore,q.original_targeting_fov_degrees,matrices);
}catch(...){return false;}
}
