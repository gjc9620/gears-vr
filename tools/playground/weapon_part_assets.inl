// Original-engine transient resources only. Included in the single GT owner.
// Geometry preparation and native resource lifetime never transfer ammunition.
namespace weapon_part_assets {
namespace hand=gears_weapon_hand;
namespace geometry=gears_weapon::part_geometry;
namespace contract=gears_weapon::part_native_contract;
using Identity=hand::ObjectIdentity;
struct Owner {
    Identity world{},pawn{};
    std::uint64_t game{},native{},registration{},world_generation{},generation{};
};
struct Handle {unsigned slot{},generation{};};
struct WeaponPoseRequest {
    Handle handle{};std::uint64_t owner_generation{},frame{};
    gears_vr::Viewpoint pose{};gears_vr::Vector scale{};
    bool visible{},translation_changed{},rotation_changed{},scale_changed{};
    gears_vr::Matrix engine_basis{};
};
struct WeaponPoseOwner {
    Identity weapon{},component{},asset{};std::uint64_t hold_generation{};
};
enum class Stage {empty,reserved,mesh_created,mesh_rooted,lod_created,prepared,
    resources_queued,component_created,ready,visible,retiring,failed};
enum class Creation {rejected,pending,ready};
struct Resource {
    Owner owner{};Identity mesh{},component{};
    unsigned generation{};Stage stage{};
    std::uint64_t last_creation_frame{};
    geometry::Prepared input{};std::vector<Identity> materials{};
    hand::Lease pose_lease{};bool visibility_revoke{},weapon_pose_pending{},lighting_bound{},lighting_assigned{};
    WeaponPoseOwner weapon_pose{};
    gears_vr::Viewpoint last_weapon_pose{};gears_vr::Vector last_weapon_scale{};bool weapon_pose_known{};
    gears_vr::Matrix last_weapon_basis{};
    uptr lod{};bool mesh_root{},component_root{},mesh_bound{},attached{},lod_constructed{},lod_allocation_dispatched{};
    bool resources_queued{},publication_withdrawn{};
};
template<class Current>
bool read_scene_matrix(hand::Frame const&,Handle,std::uint64_t,std::array<float,16>&,Current&&) noexcept;
std::array<Resource,16> resources{};
std::atomic<unsigned> outstanding{};
std::atomic<bool> stop_requested{};
unsigned next_generation{1};
bool inside_create{};
bool faulted{};
unsigned failure{};
bool gt() noexcept {return game_thread.load(std::memory_order_acquire)==GetCurrentThreadId();}
bool object(Identity const& id) noexcept {
    Identity actual{};
    return hand::identity_valid(id)&&nativeequipment::identify(nullptr,id.address,actual)&&hand::same(id,actual);
}
bool write(uptr target,const void* source,std::size_t count) noexcept {
    if(!target||!source||!count)return false;
    __try{std::memcpy(reinterpret_cast<void*>(target),source,count);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
template<class T>bool put(uptr target,T const& value) noexcept {return write(target,&value,sizeof(value));}
bool same(Owner const& owner,hand::Frame const& f) noexcept {
    return gt()&&owner.generation&&hand::same(owner.world,f.world)&&hand::same(owner.pawn,f.pawn)&&
        owner.game==f.key.game_start&&owner.native==f.key.native_started_qpc&&
        owner.registration==f.key.registration_cookie&&owner.world_generation==f.key.world_generation;
}
// Totals are consumed only as deltas by the existing bounded phase receipt;
// never a cached currentness result or an input/action witness.
struct SourceCost {std::uint64_t calls{};std::int64_t ns{};};
SourceCost source_cost{};
bool current(Owner const& owner,hand::Frame const& f) noexcept {
    if(!same(owner,f))return false;
    const auto begin=weaponhand::clock_ns(nullptr);
    const bool result=nativeequipment::current(nullptr,f);
    const auto end=weaponhand::clock_ns(nullptr);++source_cost.calls;
    if(end>=begin)source_cost.ns+=end-begin;
    return result;
}
bool weapon_owner(hand::Frame const& f,WeaponPoseOwner& out) noexcept {
    if(!nativelancerfollow::bound_key_known||!nativelancerfollow::bound_is_lancer||!nativelancerfollow::mount.bound()||
       !hand::same(f.weapon,nativelancerfollow::bound_key.weapon)||
       !vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)||
       !vrequip::model.hold_generation()||vrequip::model.hold_generation()!=nativelancerfollow::bound_key.hold_generation||
       !gears_vr::controller_pose_usable(f.controllers.hands[1].grip,f.controllers.hands[1].grip_flags,f.controllers.hands[1].grip_active)||
       nativelancerfollow::bound_session!=f.stereo.session||nativelancerfollow::bound_space!=f.stereo.space_epoch||
       nativelancerfollow::bound_input!=f.controllers.input_epoch||nativelancerfollow::bound_world!=f.key.world_generation||
       nativelancerfollow::bound_reference!=f.reference_generation)return false;
    out={nativelancerfollow::bound_key.weapon,nativelancerfollow::bound_key.component,
        nativelancerfollow::bound_key.asset,nativelancerfollow::bound_key.hold_generation};return true;
}
bool same_weapon(WeaponPoseOwner const& owner,hand::Frame const& f) noexcept {
    WeaponPoseOwner now{};
    return owner.hold_generation&&weapon_owner(f,now)&&now.hold_generation==owner.hold_generation&&
        hand::same(owner.weapon,now.weapon)&&hand::same(owner.component,now.component)&&hand::same(owner.asset,now.asset);
}
Resource* find(Handle h) noexcept {
    if(!h.generation||h.slot>=resources.size())return nullptr;
    auto& r=resources[h.slot];
    return r.generation==h.generation&&r.stage!=Stage::empty?&r:nullptr;
}
uptr entry(char const* role) noexcept {
    for(const auto& e:contract::entries)if(!std::strcmp(e.role,role))return base+e.rva;
    return 0;
}
bool executable(uptr address,std::size_t size) noexcept {
    if(!size||size>100000||address>UINTPTR_MAX-size)return false;
    for(auto at=address;at<address+size;){
        MEMORY_BASIC_INFORMATION info{};
        if(!VirtualQuery(reinterpret_cast<void*>(at),&info,sizeof(info))||
           info.State!=MEM_COMMIT||info.Type!=MEM_IMAGE||
           info.AllocationBase!=reinterpret_cast<void*>(base)||
           (info.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
           !(info.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return false;
        const auto end=reinterpret_cast<uptr>(info.BaseAddress)+info.RegionSize;
        if(end<=at)return false;at=(std::min)(end,address+size);
    }
    return true;
}
// Validate current image chunks, not just a matching historical RVA. The
// historical fingerprints do NOT certify the current calling convention/GPU.
bool certify() {
    if(!gt()||base!=reinterpret_cast<uptr>(GetModuleHandleW(nullptr)))return false;
    BCRYPT_ALG_HANDLE algorithm{};
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return false;
    struct Close {BCRYPT_ALG_HANDLE p;~Close(){BCryptCloseAlgorithmProvider(p,0);}} close{algorithm};
    for(const auto& e:contract::entries){
        std::size_t size=0;for(std::size_t i=0;i<e.count;++i){
            if(e.chunks[i].rva>UINTPTR_MAX-base||!executable(base+e.chunks[i].rva,e.chunks[i].bytes))return false;
            size+=e.chunks[i].bytes;
        }
        if(!size||size>100000)return false;
        std::vector<unsigned char> bytes(size);std::size_t at=0;
        for(std::size_t i=0;i<e.count;++i){
            const auto& chunk=e.chunks[i];
            if(!copy(bytes.data()+at,base+chunk.rva,chunk.bytes))return false;at+=chunk.bytes;
        }
        std::array<unsigned char,32> actual{};
        if(BCryptHash(algorithm,nullptr,0,bytes.data(),ULONG(bytes.size()),actual.data(),ULONG(actual.size()))<0||
           actual!=e.sha256){
            faulted=true;
            if(log_file){fwprintf(log_file,L"weapon_part_asset_contract_mismatch role=%hs rva=%llx\n",e.role,e.rva);fflush(log_file);}
            return false;
        }
    }
    return true;
}
bool invoke_construct(uptr function,uptr klass,uptr& result) noexcept {
    // Verified wrapper preserves RCX=class and R8=FName, supplies the real
    // transient package/error device and zero flags/template/graph itself.
    using F=uptr(__fastcall*)(uptr,uptr,std::uint64_t);
    __try{result=reinterpret_cast<F>(function)(klass,0,0);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool invoke_two(uptr function,uptr a,uptr b,uptr& result) noexcept {
    using F=uptr(__fastcall*)(uptr,uptr);
    __try{result=reinterpret_cast<F>(function)(a,b);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool invoke_three(uptr function,uptr a,uptr b,unsigned c) noexcept {
    using F=void(__fastcall*)(uptr,uptr,unsigned);
    __try{reinterpret_cast<F>(function)(a,b,c);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool invoke_array(uptr function,uptr array,unsigned count,unsigned stride,unsigned alignment,int& result) noexcept {
    using F=int(__fastcall*)(uptr,unsigned,unsigned,unsigned);
    __try{result=reinterpret_cast<F>(function)(array,count,stride,alignment);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool invoke_one(uptr function,uptr a) noexcept {
    using F=void(__fastcall*)(uptr);
    __try{reinterpret_cast<F>(function)(a);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool invoke_void_two(uptr function,uptr a,uptr b) noexcept {
    using F=void(__fastcall*)(uptr,uptr);
    __try{reinterpret_cast<F>(function)(a,b);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool root(Identity const& id,bool& owned) noexcept {
    if(owned||!gt()||!object(id))return false;
    std::uint64_t bits{};
    if(!copy(&bits,id.address+12,8))return false;
    if(bits&0x4000){faulted=true;return false;} // Never adopt/clear another root owner.
    const auto next=bits|0x4000;
    if(!put(id.address+12,next))return false;
    // Journal ownership immediately after a successful write, even if the
    // following verification fails; cleanup must not lose a new root lease.
    owned=true;
    return object(id)&&get<std::uint64_t>(id.address+12)==next;
}
bool unroot(Identity const& id,bool& owned) noexcept {
    if(!owned)return true;
    if(!gt()||!object(id))return false;
    std::uint64_t bits{};if(!copy(&bits,id.address+12,8))return false;
    const auto next=bits&~std::uint64_t(0x4000);
    if(!put(id.address+12,next))return false;
    owned=false;return true; // Never dereference this weak identity afterwards.
}
bool find_class(wchar_t const* name,Identity& out) noexcept {
#if defined(GEARS_PLAYGROUND_HAND_WEAPON_MESH_PHASE1_TRIAL)
    component_names::Scope bulk_names(true);
#endif
    static std::array<Identity,3> selected{};
    const unsigned slot=!std::wcscmp(name,L"StaticMesh")?0:!std::wcscmp(name,L"StaticMeshComponent")?1:
        !std::wcscmp(name,L"MorphTarget")?2:3;
    if(slot>=selected.size())return false;
    const auto valid=[&](Identity const& id) noexcept {
        return object(id)&&playground::named(id.address,name)&&
            playground::named(id.klass,L"Class")&&playground::named(id.outer,L"Engine");
    };
    // Class identities only, never a source frame, grant or transform. If the
    // first scan spends this frame, its next use still revalidates the object
    // and the creation phase separately requires a fresh current scene.
    if(valid(selected[slot])){out=selected[slot];return true;}
    const auto objects=get<Array>(base+0x1c24730);
    if(!gt()||!objects.data||objects.count<1000||objects.count>1000000||objects.count>objects.capacity)return false;
    for(int i=0;i<objects.count;++i){
        const auto p=get<uptr>(objects.data+uptr(i)*8);
        if(!p||!registered(p)||!playground::named(p,name)||
           !playground::named(get<uptr>(p+0x50),L"Class")||
           !playground::named(get<uptr>(p+0x40),L"Engine"))continue;
        if(!nativeequipment::identify(nullptr,p,out)||!valid(out))return false;
        selected[slot]=out;return true;
    }
    return false;
}
bool construct(hand::Frame const& frame,Resource& resource,Identity const& klass,
    Identity& result,bool& rooted) {
    if(!current(resource.owner,frame)||!object(klass))return false;
    const auto table=get<Array>(base+0x1c24730);
    if(!table.data||table.count<1000||table.count>1000000||table.count>table.capacity)return false;
    // The constructor may reuse a vacant index. Snapshot original slot values
    // to prove its return was not an already existing asset/component.
    std::vector<uptr> previous(table.count);
    if(!copy(previous.data(),table.data,previous.size()*sizeof(uptr))||!current(resource.owner,frame))return false;
    uptr value{};
    if(!invoke_construct(entry("transient_construct_wrapper"),klass.address,value)||!value)return false;
    Identity created{};
    if(!nativeequipment::identify(nullptr,value,created)||!object(created)||created.klass!=klass.address||
       (created.registry_index<previous.size()&&previous[created.registry_index]==value)||
       !playground::named(created.outer,L"Transient"))return false;
    result=created; // Even an acquisition failure retains its exact identity.
    // New UObject registration may retire this scene publication. Root the
    // object immediately, then wait for another CURRENT frame before any
    // further preparation. Root ownership is not a hand/action grant.
    return root(result,rooted)&&object(resource.owner.world)&&object(resource.owner.pawn)&&
        playground::pointer(resource.owner.pawn.address,L"WorldInfo")==resource.owner.world.address;
}
bool native_array(uptr address,std::size_t count,unsigned stride,const void* data) noexcept {
    Array before{};if(!copy(&before,address,sizeof(before))||before.data||before.count||before.capacity||
        !count||count>196602||!stride||stride>60||!data)return false;
    int prior=-1;
    if(!invoke_array(entry("native_array_add"),address,unsigned(count),stride,8,prior)||prior!=0)return false;
    Array after{};
    return copy(&after,address,sizeof(after))&&after.data&&after.count==int(count)&&after.capacity>=after.count&&
        after.capacity<=400000&&write(after.data,data,count*stride);
}
bool prepare_mesh(hand::Frame const& f,Resource& r,geometry::Prepared const& data,
    std::vector<Identity> const& materials) noexcept {
    if(!current(r.owner,f)||!r.mesh_root||!object(r.mesh)||data.vertices.empty()||data.vertices.size()>32767||
       data.indices.empty()||data.indices.size()>196602||materials.empty()||materials.size()>32)return false;
    for(const auto& material:materials)
        if(!object(material)||!playground::instance(material.address,L"MaterialInterface"))return false;
    const auto mesh=r.mesh.address;
    Array lods{};if(!copy(&lods,mesh+96,sizeof(lods))||lods.data||lods.count||lods.capacity)return false;
    uptr lod{};
    // The allocator appends unconstructed memory. A failure after append is
    // quarantined until its ctor succeeds; never release a malformed LOD.
    r.lod_allocation_dispatched=true;
    if(!invoke_two(entry("allocate_append_lod"),0,mesh+96,lod)||!lod)return false;
    r.lod=lod;r.stage=Stage::lod_created;
    if(!invoke_one(entry("lod_constructor"),lod))return false;
    r.lod_constructed=true;
    Array build{reinterpret_cast<uptr>(data.vertices.data()),int(data.vertices.size()),int(data.vertices.size())};
    if(!current(r.owner,f)||!invoke_three(entry("init_tangent_uv"),lod,reinterpret_cast<uptr>(&build),1)||
       !invoke_void_two(entry("init_position"),lod+76,reinterpret_cast<uptr>(&build))||
       !invoke_void_two(entry("init_color"),lod+144,reinterpret_cast<uptr>(&build)))return false;
    if(get<unsigned>(lod+68)!=data.vertices.size()*2||get<unsigned>(lod+76+64)!=data.vertices.size()*2||
       build.data!=reinterpret_cast<uptr>(data.vertices.data())||build.count!=int(data.vertices.size())||
       build.capacity!=int(data.vertices.size())||!current(r.owner,f))return false;
    if(!put(lod+212,unsigned(data.vertices.size()))||
       !native_array(lod+268,data.indices.size(),2,data.indices.data()))return false;
    for(const auto& section:data.sections){
        if(section.material>=materials.size())return false;
        uptr slot{};
        if(!current(r.owner,f)||!invoke_two(entry("append_section"),0,lod+352,slot)||!slot)return false;
        std::array<std::uint8_t,56> bytes{};
        const auto material=materials[section.material].address;
        const unsigned one=1;
        std::memcpy(bytes.data(),&material,8);std::memcpy(bytes.data()+32,&one,4);
        std::memcpy(bytes.data()+36,&section.first_index,4);std::memcpy(bytes.data()+40,&section.triangles,4);
        std::memcpy(bytes.data()+44,&section.min_vertex,4);std::memcpy(bytes.data()+48,&section.max_vertex,4);
        std::memcpy(bytes.data()+52,&section.material,4);
        if(!write(slot,bytes.data(),bytes.size()))return false;
    }
    const auto sections=get<Array>(lod+352);
    if(sections.count!=int(data.sections.size())||!sections.data||sections.capacity<sections.count||
       !write(mesh+256,data.bounds.centre.data(),12)||!write(mesh+268,data.bounds.extent.data(),12)||
       !put(mesh+280,data.bounds.radius)||!current(r.owner,f))return false;
    r.stage=Stage::prepared;
    // Mesh init queues every LOD exactly once. Do not separately initialise
    // the same vertex factory/resources or pretend queueing proves GPU use.
    if(!invoke_one(entry("mesh_init_resources"),mesh))return false;
    r.resources_queued=true;r.stage=Stage::resources_queued;
    return current(r.owner,f);
}
bool native_function(playground::Args const& call,const wchar_t* label,const wchar_t* owner_name,
    const wchar_t* package_name=L"Engine") noexcept {
    if(!call.valid||!registered(call.r.function)||!playground::named(call.r.function,label))return false;
    const auto owner=get<uptr>(call.r.function+0x40),package=owner?get<uptr>(owner+0x40):0;
    return registered(owner)&&registered(package)&&playground::named(owner,owner_name)&&
        playground::named(package,package_name)&&get<uptr>(package+0x40)==0&&
        (get<unsigned>(call.r.function+0xf4)&0x400)!=0&&
        weaponhand::executable(reinterpret_cast<FARPROC>(get<uptr>(call.r.function+0x114)),GetModuleHandleW(nullptr));
}
bool pointer_field(Identity const& id,const wchar_t* label,uptr& value,bool component=false) noexcept {
    value=0;if(!object(id))return false;
    const auto p=playground::property(id.address,label);
    const auto extent=get<unsigned>(id.klass+0x88);
    if(!registered(p)||!playground::named(get<uptr>(p+0x50),component?L"ComponentProperty":L"ObjectProperty")||
       get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=8)return false;
    const auto offset=get<unsigned>(p+0x94);
    return offset<=extent&&8<=extent-offset&&copy(&value,id.address+offset,8)&&object(id);
}
bool boolean(Identity const& id,const wchar_t* label,bool& value) noexcept {
    value=false;if(!object(id))return false;
    const auto p=playground::property(id.address,label);
    const auto extent=get<unsigned>(id.klass+0x88);
    if(!registered(p)||!playground::named(get<uptr>(p+0x50),L"BoolProperty")||
       get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=4)return false;
    const auto offset=get<unsigned>(p+0x94),mask=get<unsigned>(p+0xd0);unsigned bits{};
    if(!mask||(mask&(mask-1))||offset>extent||4>extent-offset||!copy(&bits,id.address+offset,4))return false;
    value=(bits&mask)!=0;return object(id);
}
// Only newly constructed, privately rooted components reach this setter.
bool configure_bool(Resource& r,const wchar_t* label,bool desired) noexcept {
    if(!gt()||!r.component_root||!object(r.component))return false;
    const auto p=playground::property(r.component.address,label);
    const auto extent=get<unsigned>(r.component.klass+0x88);
    if(!registered(p)||!playground::named(get<uptr>(p+0x50),L"BoolProperty")||
       get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=4)return false;
    const auto offset=get<unsigned>(p+0x94),mask=get<unsigned>(p+0xd0);unsigned bits{};
    if(!mask||(mask&(mask-1))||offset>extent||4>extent-offset||!copy(&bits,r.component.address+offset,4))return false;
    const auto wanted=desired?bits|mask:bits&~mask;
    return put(r.component.address+offset,wanted)&&object(r.component)&&
        get<unsigned>(r.component.address+offset)==wanted;
}
bool object_call(Identity const& id,const wchar_t* label,const wchar_t* owner,
    const wchar_t* parameter,const wchar_t* kind,uptr value) noexcept {
    if(!gt()||!object(id))return false;
    playground::Args call(id.address,label);
    return call.r.size==8&&native_function(call,label,owner)&&
        hammerburst_animation::input_field(call,call.prop(parameter),kind,8)&&
        call.set(parameter,value).valid&&call.run()&&object(id);
}
bool bind_light_environment(hand::Frame const& f,Handle handle,std::uint64_t owner_generation,
    Identity const& environment,bool known_empty) noexcept {
    auto r=find(handle);
    if(!r||r->owner.generation!=owner_generation||r->stage!=Stage::ready||!current(r->owner,f)||
       (known_empty?environment.address!=0:(!object(environment)||!playground::instance(environment.address,L"LightEnvironmentComponent"))))return false;
    uptr before{};
    if(!pointer_field(r->component,L"LightEnvironment",before,true))return false;
    if(before!=environment.address){r->lighting_assigned=true;
        if(!object_call(r->component,L"SetLightEnvironment",L"PrimitiveComponent",
            L"NewLightEnvironment",L"ComponentProperty",environment.address))return false;}
    uptr after{};
    if(!pointer_field(r->component,L"LightEnvironment",after,true)||after!=environment.address||
       (!known_empty&&!object(environment)))return false;
    r->lighting_bound=true;return true;
}
bool hidden(Resource& r,bool value) noexcept {
    if(!gt()||!r.component_root||!object(r.component))return false;
    bool already{};
    if(!boolean(r.component,L"HiddenGame",already))return false;
    if(already==value)return true;
    playground::Args call(r.component.address,L"SetHidden");
    if(call.r.size!=4||!native_function(call,L"SetHidden",L"PrimitiveComponent")||
       !hammerburst_animation::input_field(call,call.prop(L"NewHidden"),L"BoolProperty",4)||
       !nativelancerfollow::bool_arg(call,L"NewHidden",value)||!call.run())return false;
    bool actual{};return boolean(r.component,L"HiddenGame",actual)&&actual==value;
}
bool bind_mesh(Resource& r,uptr mesh) noexcept {
    if(!r.component_root||!object(r.component))return false;
    if(mesh&&(!r.mesh_root||!object(r.mesh)||mesh!=r.mesh.address))return false;
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
bool attachment(Resource& r,bool desired) noexcept {
    if(!gt()||!r.component_root||!object(r.component))return false;
    bool attached{};
    if(!boolean(r.component,L"bAttached",attached))return false;
    uptr owner{};if(!pointer_field(r.component,L"Owner",owner))return false;
    if(attached&&owner!=r.owner.pawn.address)return false;
    if(attached==desired){r.attached=attached;return true;}
    if(!object(r.owner.pawn))return false;
    if(desired)r.attached=true; // A dispatched attachment may need cleanup.
    if(!object_call(r.owner.pawn,desired?L"AttachComponent":L"DetachComponent",L"Actor",
       desired?L"NewComponent":L"ExComponent",L"ComponentProperty",r.component.address))return false;
    if(!boolean(r.component,L"bAttached",attached)||attached!=desired||
       !pointer_field(r.component,L"Owner",owner)||(desired&&owner!=r.owner.pawn.address))return false;
    r.attached=attached;return true;
}
bool set_absolute(Resource& r) noexcept {
    playground::Args call(r.component.address,L"SetAbsolute");
    return object(r.component)&&native_function(call,L"SetAbsolute",L"PrimitiveComponent")&&
        nativelancerfollow::bool_arg(call,L"NewAbsoluteTranslation",true)&&
        nativelancerfollow::bool_arg(call,L"NewAbsoluteRotation",true)&&
        nativelancerfollow::bool_arg(call,L"NewAbsoluteScale",true)&&call.run()&&object(r.component);
}
bool transform_call(Resource& r,const wchar_t* label,const wchar_t* parameter,
    const wchar_t* type,const void* value) noexcept {
    if(!object(r.component))return false;
    playground::Args call(r.component.address,label);const auto field=call.prop(parameter);
    if(call.r.size!=12||!native_function(call,label,L"PrimitiveComponent")||
       !hammerburst_animation::input_field(call,field,L"StructProperty",12))return false;
    const auto structure=get<uptr>(field+0xd0);
    return registered(structure)&&playground::named(structure,type)&&call.bytes(parameter,value,12)&&
        call.run()&&object(r.component);
}
bool engine_rotation_axes(hand::Frame const& f,Resource const& resource,
    playground::Rotation const& rotation,gears_vr::Matrix& out) noexcept {
    struct Cached {Owner owner{};Identity function{};playground::Rotation rotation{};gears_vr::Matrix basis{};bool known{};};
    static std::array<Cached,4> cache{};static unsigned cursor{};
    for(auto const& entry:cache)if(entry.known&&same(entry.owner,f)&&
       entry.rotation.pitch==rotation.pitch&&entry.rotation.yaw==rotation.yaw&&entry.rotation.roll==rotation.roll&&object(entry.function)){
          // Identity/basis cache only. The sole caller brackets the following
          // actual matrix read with its full current-source/weapon checks.
          // There is no engine dispatch on this cache-hit path.
          out=entry.basis;return true;
    }
    failure=112;playground::Args call(f.pawn.address,L"GetAxes");Identity function{};
    if(call.r.size!=48||!native_function(call,L"GetAxes",L"Object",L"Core")||
       !nativeequipment::identify(nullptr,call.r.function,function))return false;
    const wchar_t* names[]={L"A",L"X",L"Y",L"Z"};unsigned offsets[4]{};
    for(unsigned i=0;i<4;++i){
        const auto field=call.prop(names[i]);failure=113+i;
        if(!registered(field)||get<uptr>(field+0x40)!=call.r.function||
           !playground::named(get<uptr>(field+0x50),L"StructProperty")||get<unsigned>(field+0x70)!=1||
           get<unsigned>(field+0x74)!=12||(get<unsigned long long>(field+0x78)&0x580)!=(i?0x180ull:0x80ull))return false;
        const auto structure=get<uptr>(field+0xd0);
        if(!registered(structure)||!playground::named(structure,i?L"Vector":L"Rotator"))return false;
        offsets[i]=get<unsigned>(field+0x94);
        if(offsets[i]>call.r.size||12>call.r.size-offsets[i])return false;
        for(unsigned j=0;j<i;++j)if(!(offsets[i]+12<=offsets[j]||offsets[j]+12<=offsets[i]))return false;
    }
    failure=117;if(!current(resource.owner,f)||!call.bytes(L"A",&rotation,12)||!call.run()||
       !object(function)||!current(resource.owner,f))return false;
    const auto x=call.output<playground::Vector>(L"X"),y=call.output<playground::Vector>(L"Y"),z=call.output<playground::Vector>(L"Z");
    const gears_vr::Matrix result{{{x.x,y.x,z.x},{x.y,y.y,z.y},{x.z,y.z,z.z}}};
    failure=118;if(!gears_weapon::lancer_starter_rotation_basis_valid(result))return false;
    cache[cursor++%cache.size()]={resource.owner,function,rotation,result,true};out=result;return true;
}
bool cleanup(Resource& r) noexcept {
    if(!gt())return false;
    r.publication_withdrawn=true;r.stage=Stage::retiring;
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
bool drain() noexcept {
    if(!gt())return false;
    bool clean=true;
    for(auto& r:resources){
        if(r.stage==Stage::empty)continue;
        // The ordinary consumers have now had their one current-frame turn.
        // A skipped/expired/unacknowledged weapon pose must be hidden before
        // render; an actual ACK below clears this obligation, not its age.
        if(r.weapon_pose_pending){r.weapon_pose_pending=false;r.visibility_revoke=true;}
        if(r.visibility_revoke&&r.component_root&&!r.publication_withdrawn){
            if(hidden(r,true)){r.visibility_revoke=false;if(r.stage==Stage::visible)r.stage=Stage::ready;}
            else{r.publication_withdrawn=true;r.stage=Stage::retiring;}
        }
        if(stop_requested.load(std::memory_order_acquire)||r.stage==Stage::retiring||r.stage==Stage::failed)
            clean=cleanup(r)&&clean;
    }
    return clean&&outstanding.load(std::memory_order_acquire)==0;
}
// Fresh-frame work and revoked visibility are different states. In particular,
// a full source observation during equipment/fire must not lose a visible
// starter just because its normal-GT pose consumer has not run yet this frame.
// Missing source/changed identity still withdraws immediately; drain hides a
// skipped pose update after all ordinary consumers and before render.
void on_frame(hand::Frame const& f) noexcept {
    if(!gt())return;
    for(auto& r:resources){
        if(r.stage==Stage::empty)continue;
        if(!same(r.owner,f)){r.publication_withdrawn=true;r.stage=Stage::retiring;continue;}
        if(r.weapon_pose.hold_generation){
            // Every new frame needs a current gun pose. If its consumer is
            // skipped/expired, normal drain hides it rather than leaving a
            // detached starter floating at a previous world position.
            r.weapon_pose_pending=true;
            continue;
        }
        if(r.stage==Stage::visible&&(!f.left_tracked||!f.left_grip_known||!f.left_grip_down||
           r.pose_lease.lifecycle!=f.key.lifecycle||r.pose_lease.id!=f.press_token))r.visibility_revoke=true;
    }
}
void on_frame_unavailable() noexcept {
    if(!gt())return;
    for(auto& r:resources)if(r.stage==Stage::visible)r.visibility_revoke=true;
}
bool destroy(Handle handle,std::uint64_t owner_generation) noexcept {
    auto r=find(handle);return r&&r->owner.generation==owner_generation&&cleanup(*r);
}
Creation create(hand::Frame const& f,std::uint64_t owner_generation,geometry::Prepared const& data,
    std::vector<Identity> const& materials,Handle& handle) noexcept {
    handle={};failure=0;
    if(!gt()||inside_create||faulted||stop_requested.load(std::memory_order_acquire)||!owner_generation||
       !data.calibration_generation||!nativeequipment::current(nullptr,f)||!next_generation||
       next_generation==UINT_MAX)return Creation::rejected;
    struct Scope {Scope(){inside_create=true;}~Scope(){inside_create=false;}} scope;
    Resource* slot=nullptr;unsigned index=0;
    for(unsigned i=0;i<resources.size();++i)if(resources[i].stage==Stage::empty){slot=&resources[i];index=i;break;}
    if(!slot)return Creation::rejected;
    auto& r=*slot;r.owner={f.world,f.pawn,f.key.game_start,f.key.native_started_qpc,
        f.key.registration_cookie,f.key.world_generation,owner_generation};
    r.generation=next_generation++;r.stage=Stage::reserved;
    outstanding.fetch_add(1,std::memory_order_acq_rel);
    try {
        // CPU ownership only. Preparation is advanced by new GT frames; never
        // carry a hand pose or an expired write grant across these phases.
        r.input=data;r.materials=materials;
        handle={index,r.generation};failure=0;return Creation::pending;
    }catch(...){
        const auto saved_failure=failure;r.stage=Stage::retiring;
        const bool restored=cleanup(r);failure=saved_failure;
        if(log_file){fwprintf(log_file,L"weapon_part_asset_create_failed gate=%u cleaned=%u outstanding=%u\n",
            failure,unsigned(restored),outstanding.load());fflush(log_file);}
        return Creation::rejected;
    }
}
Creation advance(hand::Frame const& f,Handle handle,std::uint64_t owner_generation) noexcept {
    auto r=find(handle);
    if(!gt()||inside_create||!r||r->owner.generation!=owner_generation)return Creation::rejected;
    if(r->publication_withdrawn||r->stage==Stage::retiring||r->stage==Stage::failed||
       stop_requested.load(std::memory_order_acquire)||!same(r->owner,f)){
        cleanup(*r);return Creation::rejected;
    }
    if(r->stage==Stage::ready||r->stage==Stage::visible)return Creation::ready;
    // Publication expiry alone is not a failed constructor and never starts
    // another object. The SAME handle waits for a new authenticated scene.
    if(!current(r->owner,f)||f.key.frame<=r->last_creation_frame)return Creation::pending;
    struct Scope {Scope(){inside_create=true;}~Scope(){inside_create=false;}} scope;
    r->last_creation_frame=f.key.frame;
    try {
        failure=1;
        if(!certify())throw 1;
        if(!current(r->owner,f))return Creation::pending;
        switch(r->stage){
        case Stage::reserved:{
            Identity klass{};
            if(!find_class(L"StaticMesh",klass))throw 2;
            if(!current(r->owner,f))return Creation::pending;
            failure=2;if(!construct(f,*r,klass,r->mesh,r->mesh_root))throw 2;
            r->stage=Stage::mesh_rooted;return Creation::pending;
        }
        case Stage::mesh_rooted:
            failure=3;if(!prepare_mesh(f,*r,r->input,r->materials))throw 3;
            r->input={};r->materials.clear();return Creation::pending;
        case Stage::resources_queued:{
            Identity klass{};
            if(!find_class(L"StaticMeshComponent",klass))throw 4;
            if(!current(r->owner,f))return Creation::pending;
            failure=4;if(!construct(f,*r,klass,r->component,r->component_root))throw 4;
            r->stage=Stage::component_created;return Creation::pending;
        }
        case Stage::component_created:{
            failure=5;
            // Still unattached until a current physical pose is provided.
            if(!hidden(*r,true))throw 5;
            for(auto property:{L"CollideActors",L"BlockActors",L"BlockZeroExtent",L"BlockNonZeroExtent",
                              L"BlockRigidBody",L"bUsePrecomputedShadows"})
                if(!configure_bool(*r,property,false))throw 5;
            if(!current(r->owner,f))return Creation::pending;
            const playground::Vector unit{1,1,1};failure=6;
            if(!set_absolute(*r)||!transform_call(*r,L"SetScale3D",L"NewScale3D",L"Vector",&unit)||
               !bind_mesh(*r,r->mesh.address))throw 6;
            // A setter may expire the frame after succeeding. Its exact
            // binding remains journaled; readiness itself grants no pose.
            r->stage=Stage::ready;failure=0;return Creation::ready;
        }
        default:throw 7;
        }
    }catch(...){
        const auto saved_failure=failure;r->stage=Stage::retiring;
        const bool restored=cleanup(*r);failure=saved_failure;
        if(log_file){fwprintf(log_file,L"weapon_part_asset_prepare_failed gate=%u cleaned=%u outstanding=%u\n",
            failure,unsigned(restored),outstanding.load());fflush(log_file);}
        return Creation::rejected;
    }
}
Creation create_weapon_part(hand::Frame const& f,std::uint64_t owner_generation,geometry::Prepared const& data,
    std::vector<Identity> const& materials,Handle& handle) noexcept {
    handle={};WeaponPoseOwner owner{};
    if(!weapon_owner(f,owner)||!current(Owner{f.world,f.pawn,f.key.game_start,f.key.native_started_qpc,
       f.key.registration_cookie,f.key.world_generation,owner_generation},f))return Creation::rejected;
    const auto result=create(f,owner_generation,data,materials,handle);
    if(result!=Creation::rejected){auto r=find(handle);if(!r)return Creation::rejected;r->weapon_pose=owner;}
    return result;
}
// Display-only gun attachment, using the same resource pool, original engine
// component owner and normal cleanup. It cannot create a left-hand grant or
// transfer any ammunition. A magazine resource cannot enter this path.
bool stage_weapon_pose(hand::Frame const& f,Handle handle,std::uint64_t owner_generation,
    gears_vr::Viewpoint const& pose,gears_vr::Vector const& scale,bool visible,WeaponPoseRequest& request) noexcept {
    request={};
    auto r=find(handle);if(!gt()||!r)return false;
    const auto rejected=[&]() noexcept {
        if(!r->publication_withdrawn&&(r->stage==Stage::ready||r->stage==Stage::visible)){
            if(hidden(*r,true))r->stage=Stage::ready;
            else{r->publication_withdrawn=true;r->stage=Stage::retiring;}
        }
        r->weapon_pose_known=false;return false;
    };
    const auto bound=[&]() noexcept {return same(r->owner,f)&&same_weapon(r->weapon_pose,f)&&
        object(r->weapon_pose.weapon)&&object(r->weapon_pose.component)&&object(r->weapon_pose.asset);};
    const auto checked=[&]() noexcept {return current(r->owner,f)&&bound();};
    if(r->owner.generation!=owner_generation||!r->lighting_bound||r->publication_withdrawn||
       (r->stage!=Stage::ready&&r->stage!=Stage::visible)||!r->weapon_pose.hold_generation||
       !bound()||!gears_vr::finite(pose.position)||std::abs(pose.position.x)>1e8||std::abs(pose.position.y)>1e8||
       std::abs(pose.position.z)>1e8||!std::isfinite(pose.rotation.pitch)||!std::isfinite(pose.rotation.yaw)||
       !std::isfinite(pose.rotation.roll)||!gears_vr::finite(scale)||scale.x<.01||scale.y<.01||scale.z<.01||
       scale.x>200||scale.y>200||scale.z>200)return rejected();
    const playground::Rotation rotation{gears_vr::to_rotator_units(pose.rotation.pitch),
        gears_vr::to_rotator_units(pose.rotation.yaw),gears_vr::to_rotator_units(pose.rotation.roll)};
    gears_vr::Matrix engine_basis{};
    // No mutation in the checks above. A cache miss authenticates before its
    // original GetAxes dispatch; either path then fully brackets the matrix
    // read below. Individually guarded transform setters remain unchanged.
    if(!engine_rotation_axes(f,*r,rotation,engine_basis))return rejected();
    std::array<float,16> before{};
    if(r->attached&&read_scene_matrix(f,handle,owner_generation,before,checked)){
        if(gears_weapon::lancer_starter_matches_draw(before,pose,scale,&engine_basis)){
            // It already satisfies the SAME actual-matrix ACK used below.
            // Do not dispatch setters/owner updates for float decomposition
            // noise while holding still. This is display work avoidance, not
            // a wider tolerance, cached input sample or visibility permit.
            request={handle,owner_generation,f.key.frame,pose,scale,visible,false,false,false,engine_basis};return true;
        }
        if(r->weapon_pose_known&&!gears_weapon::lancer_starter_matches_draw(before,r->last_weapon_pose,r->last_weapon_scale,&r->last_weapon_basis))
            r->weapon_pose_known=false;
    }
    // An absent/unmatched actual matrix is not a transform write permit.
    // Reauthenticate here before the existing individually guarded setters.
    if(!checked())return rejected();
    const auto& prior=r->last_weapon_pose;const auto& prior_scale=r->last_weapon_scale;
    const playground::Vector position{float(pose.position.x),float(pose.position.y),float(pose.position.z)};
    const playground::Vector scaling{float(scale.x),float(scale.y),float(scale.z)};
    // Compare the EXACT original-engine argument representation. Tiny double
    // decomposition changes must not resend all three setters for every part.
    // Every actual matrix is still read and acknowledged below on this frame.
    const bool translation_changed=!r->weapon_pose_known||float(prior.position.x)!=position.x||float(prior.position.y)!=position.y||float(prior.position.z)!=position.z;
    const bool rotation_changed=!r->weapon_pose_known||gears_vr::to_rotator_units(prior.rotation.pitch)!=rotation.pitch||
        gears_vr::to_rotator_units(prior.rotation.yaw)!=rotation.yaw||gears_vr::to_rotator_units(prior.rotation.roll)!=rotation.roll;
    const bool scale_changed=!r->weapon_pose_known||float(prior_scale.x)!=scaling.x||float(prior_scale.y)!=scaling.y||float(prior_scale.z)!=scaling.z;
    failure=101;if(translation_changed&&(!transform_call(*r,L"SetTranslation",L"NewTranslation",L"Vector",&position)||!checked()))return rejected();
    failure=102;if(rotation_changed&&(!transform_call(*r,L"SetRotation",L"NewRotation",L"Rotator",&rotation)||!checked()))return rejected();
    failure=103;if(scale_changed&&(!transform_call(*r,L"SetScale3D",L"NewScale3D",L"Vector",&scaling)||!checked()))return rejected();
    failure=104;if(!attachment(*r,true)||!checked())return rejected();
    // Original setters defer attached-component transforms. Stage the whole
    // three-part batch, request the original owner's component update once,
    // then acknowledge actual matrices. Staging never publishes visibility
    // or contact/action authority and never advances the acknowledged cache.
    request={handle,owner_generation,f.key.frame,pose,scale,visible,
        translation_changed,rotation_changed,scale_changed,engine_basis};return true;
}
template<std::size_t N>
bool flush_weapon_poses(hand::Frame const& f,std::array<WeaponPoseRequest,N> const& requests) noexcept {
    static_assert(N==1||N==3);
    if(!gt())return false;
    const auto checked=[&]() noexcept {
        for(unsigned i=0;i<requests.size();++i){
            const auto& request=requests[i];auto r=find(request.handle);
            if(!r||!request.frame||request.frame!=f.key.frame||
               request.owner_generation!=requests[0].owner_generation||r->owner.generation!=request.owner_generation||
               r->publication_withdrawn||!r->attached||!r->lighting_bound||
               (r->stage!=Stage::ready&&r->stage!=Stage::visible)||!r->weapon_pose.hold_generation||
               !current(r->owner,f)||!same_weapon(r->weapon_pose,f)||!object(r->component)||
               !object(r->weapon_pose.weapon)||!object(r->weapon_pose.component)||!object(r->weapon_pose.asset))return false;
            for(unsigned j=0;j<i;++j)if(request.handle.slot==requests[j].handle.slot)return false;
        }
        return true;
    };
    failure=109;if(!checked())return false;
    bool changed=false;for(auto const& request:requests)
        changed=changed||request.translation_changed||request.rotation_changed||request.scale_changed;
    if(!changed)return true;
    // Use the supported original Actor interface, on the same current owner,
    // rather than writing LocalToWorld or pretending an old matrix is current.
    // All selected setters precede this one update. Collision updates are not
    // requested, and all current-input/object checks still apply afterwards.
    const auto update_began_ns=weaponhand::clock_ns(nullptr);
    playground::Args call(f.pawn.address,L"ForceUpdateComponents");
    const auto collision=call.prop(L"bCollisionUpdate"),transform=call.prop(L"bTransformOnly");
    // The shipped Engine object inventory declares BOTH optional booleans.
    // Validate their actual layout (including disjoint packed bools), rather
    // than assuming a one-boolean parameter block from the partial decompile.
    failure=110;if(!native_function(call,L"ForceUpdateComponents",L"Actor")||
       !hammerburst_animation::input_field(call,collision,L"BoolProperty",4)||
       !hammerburst_animation::input_field(call,transform,L"BoolProperty",4))return false;
    const auto collision_offset=get<unsigned>(collision+0x94),transform_offset=get<unsigned>(transform+0x94);
    const auto collision_mask=get<unsigned>(collision+0xd0),transform_mask=get<unsigned>(transform+0xd0);
    if(call.r.size!=(std::max)(collision_offset+4,transform_offset+4)||
       (collision_offset==transform_offset&&(collision_mask&transform_mask))||
       !nativelancerfollow::bool_arg(call,L"bCollisionUpdate",false)||
       !nativelancerfollow::bool_arg(call,L"bTransformOnly",true)||!call.run())return false;
    failure=111;const bool acknowledged=checked();
    static unsigned update_rows{};
    if(log_file&&update_rows++<8){const auto ended=weaponhand::clock_ns(nullptr);
        fwprintf(log_file,L"weapon_part_component_update frame=%llu parts=%u collision_update_requested=0 transform_only_requested=1 current_after=%u elapsed_ns=%lld input_age_ns=%lld native_rva=%llx\n",
            f.key.frame,unsigned(N),unsigned(acknowledged),ended-update_began_ns,ended-f.controllers.sync_ns,get<uptr>(call.r.function+0x114)-base);fflush(log_file);}
    return acknowledged;
}
bool acknowledge_weapon_pose(hand::Frame const& f,WeaponPoseRequest const& request) noexcept {
    const auto handle=request.handle;const auto owner_generation=request.owner_generation;
    const auto& pose=request.pose;const auto& scale=request.scale;const bool visible=request.visible;
    auto r=find(handle);if(!gt()||!r)return false;
    const auto rejected=[&]() noexcept {
        if(!r->publication_withdrawn&&(r->stage==Stage::ready||r->stage==Stage::visible)){
            if(hidden(*r,true))r->stage=Stage::ready;
            else{r->publication_withdrawn=true;r->stage=Stage::retiring;}
        }
        r->weapon_pose_known=false;return false;
    };
    const auto checked=[&]() noexcept {return request.frame&&request.frame==f.key.frame&&
        r->owner.generation==owner_generation&&!r->publication_withdrawn&&r->attached&&r->lighting_bound&&
        (r->stage==Stage::ready||r->stage==Stage::visible)&&current(r->owner,f)&&same_weapon(r->weapon_pose,f)&&
        object(r->component)&&object(r->weapon_pose.weapon)&&object(r->weapon_pose.component)&&object(r->weapon_pose.asset);};
    const auto& prior=r->last_weapon_pose;
    const bool translation_changed=request.translation_changed,rotation_changed=request.rotation_changed,scale_changed=request.scale_changed;
    const playground::Vector position{float(pose.position.x),float(pose.position.y),float(pose.position.z)};
    const playground::Rotation rotation{gears_vr::to_rotator_units(pose.rotation.pitch),
        gears_vr::to_rotator_units(pose.rotation.yaw),gears_vr::to_rotator_units(pose.rotation.roll)};
    std::array<float,16> actual{};
    failure=105;if(!read_scene_matrix(f,handle,owner_generation,actual,checked))return rejected();
    failure=106;if(!gears_weapon::lancer_starter_matches_draw(actual,pose,scale,&request.engine_basis)){
        // Bounded evidence for the actual106 failure. A reflected setter's
        // success is not acknowledgement of the rendered component matrix.
        // Preserve the rejection while distinguishing a deferred position
        // from a rotation/non-uniform-scale mismatch; no input permit changes.
        static unsigned mismatch_rows{};
        if(log_file&&mismatch_rows++<12){
            const auto& basis=request.engine_basis;
            const double sizes[]={scale.x,scale.y,scale.z};double basis_error{};
            for(unsigned axis=0;axis<3;++axis)for(unsigned row=0;row<3;++row)
                basis_error=(std::max)(basis_error,std::abs(actual[axis*4+row]-basis[row][axis]*sizes[axis]));
            fwprintf(log_file,L"weapon_part_matrix_mismatch frame=%llu slot=%u generation=%u changed=%u,%u,%u prior_known=%u actual_position=%.9g,%.9g,%.9g requested_position=%.9g,%.9g,%.9g prior_position=%.9g,%.9g,%.9g rotation_arg=%d,%d,%d scale=%.9g,%.9g,%.9g basis_error=%.9g actual_basis=%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g requested_angles=%.12g,%.12g,%.12g rejected=1\n",
                f.key.frame,handle.slot,handle.generation,unsigned(translation_changed),unsigned(rotation_changed),unsigned(scale_changed),unsigned(r->weapon_pose_known),
                double(actual[12]),double(actual[13]),double(actual[14]),double(position.x),double(position.y),double(position.z),
                prior.position.x,prior.position.y,prior.position.z,rotation.pitch,rotation.yaw,rotation.roll,scale.x,scale.y,scale.z,basis_error,
                double(actual[0]),double(actual[1]),double(actual[2]),double(actual[4]),double(actual[5]),double(actual[6]),double(actual[8]),double(actual[9]),double(actual[10]),
                pose.rotation.pitch,pose.rotation.yaw,pose.rotation.roll);
            fflush(log_file);
        }
        return rejected();
    }
    failure=107;if((((r->stage==Stage::visible)!=visible)||r->visibility_revoke)&&!hidden(*r,!visible))return rejected();
    failure=108;if(!checked())return rejected();
    r->last_weapon_pose=pose;r->last_weapon_scale=scale;r->last_weapon_basis=request.engine_basis;r->weapon_pose_known=true;
    r->stage=visible?Stage::visible:Stage::ready;r->visibility_revoke=false;r->weapon_pose_pending=false;failure=0;return true;
}
bool set_pose(hand::Frame const& f,Handle handle,std::uint64_t owner_generation,hand::Decision const& decision,
    gears_vr::Viewpoint const& pose,bool visible) noexcept {
    auto r=find(handle);
    if(!gt()||!r)return false;
    const auto rejected=[&]() noexcept {
        if(!r->publication_withdrawn&&(r->stage==Stage::ready||r->stage==Stage::visible)){
            if(hidden(*r,true))r->stage=Stage::ready;
            else{r->publication_withdrawn=true;r->stage=Stage::retiring;}
        }
        return false;
    };
    if(r->weapon_pose.hold_generation||r->owner.generation!=owner_generation||!r->lighting_bound||r->publication_withdrawn||(r->stage!=Stage::ready&&r->stage!=Stage::visible)||
       !current(r->owner,f)||!hand::same(decision.key,f.key)||decision.release_required||
       (decision.status!=hand::DecisionStatus::granted&&decision.status!=hand::DecisionStatus::retained)||
       !hand::lease_valid(decision.lease,f.key.lifecycle)||!decision.lease.id||
       (decision.lease.kind!=hand::LeaseKind::ammo&&decision.lease.kind!=hand::LeaseKind::magazine)||
       !f.left_tracked||!f.left_grip_known||!f.left_grip_down||decision.press_token!=f.press_token||
       decision.lease.id!=f.press_token||!std::isfinite(pose.position.x)||!std::isfinite(pose.position.y)||
       !std::isfinite(pose.position.z)||std::abs(pose.position.x)>1e8||std::abs(pose.position.y)>1e8||
       std::abs(pose.position.z)>1e8||!std::isfinite(pose.rotation.pitch)||
       !std::isfinite(pose.rotation.yaw)||!std::isfinite(pose.rotation.roll))return rejected();
    const playground::Vector position{float(pose.position.x),float(pose.position.y),float(pose.position.z)};
    // gears_vr::Viewpoint uses RADIANS, including ue_angles(). Convert once
    // to the original engine's65536 units/turn, never as degrees.
    const playground::Rotation rotation{gears_vr::to_rotator_units(pose.rotation.pitch),
        gears_vr::to_rotator_units(pose.rotation.yaw),gears_vr::to_rotator_units(pose.rotation.roll)};
    if(!transform_call(*r,L"SetTranslation",L"NewTranslation",L"Vector",&position)||!current(r->owner,f)||
       !transform_call(*r,L"SetRotation",L"NewRotation",L"Rotator",&rotation)||
       !current(r->owner,f)||!attachment(*r,true)||!hidden(*r,!visible)||!current(r->owner,f))return rejected();
    r->stage=visible?Stage::visible:Stage::ready;r->pose_lease=decision.lease;r->visibility_revoke=false;return true;
}
bool set_visible(Handle handle,std::uint64_t owner_generation,bool visible) noexcept {
    auto r=find(handle);
    // Visibility without a current pose is a withdrawal only. Showing an old
    // cached pose after tracking/lease loss must go through set_pose again.
    if(!gt()||visible||!r||r->owner.generation!=owner_generation||r->publication_withdrawn||(r->stage!=Stage::ready&&r->stage!=Stage::visible)||
       !hidden(*r,true))return false;
    r->stage=visible?Stage::visible:Stage::ready;return true;
}
// Observation only. A completed world matrix is separate from successful
// setters, queued GPU work, actual stereo visibility and headset acceptance.
// The caller's complete source AND weapon/resource predicate surrounds this
// metadata-only read. Previously each caller also performed the same source
// query immediately outside both boundaries. Keep two real observations,
// including after the read; do not retain a currentness result across calls.
template<class Current>
bool read_scene_matrix(hand::Frame const& f,Handle handle,std::uint64_t owner_generation,
    std::array<float,16>& out,Current&& checked) noexcept {
    out={};auto r=find(handle);
    if(!r||r->owner.generation!=owner_generation||r->publication_withdrawn||!r->attached||
       !same(r->owner,f)||!checked()||!object(r->component))return false;
    const auto property=playground::property(r->component.address,L"LocalToWorld");
    const auto extent=get<unsigned>(r->component.klass+0x88);
    if(!registered(property)||!playground::named(get<uptr>(property+0x50),L"StructProperty")||
       get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=64||
       !playground::named(get<uptr>(property+0xd0),L"Matrix"))return false;
    const auto offset=get<unsigned>(property+0x94);std::array<float,16> actual{};
    if(offset>extent||64>extent-offset||!copy(actual.data(),r->component.address+offset,64)||
       !object(r->component)||!checked())return false;
    for(auto v:actual)if(!std::isfinite(v))return false;
    if(std::abs(actual[3])>1e-4||std::abs(actual[7])>1e-4||std::abs(actual[11])>1e-4||
       std::abs(actual[15]-1)>1e-4)return false;
    out=actual;return true;
}
// Pure GT metadata observation for Native's synchronous proposal callback.
// Do not call the hand client/current() recursively under that callback gate.
// This observes an already visible, still owned weapon part, not a setter or
// a gameplay permit; the normal GT consumer reauthenticates before actions.
bool observe_weapon_matrix(hand::Frame const& f,Handle handle,std::uint64_t owner_generation,
    std::array<float,16>& out,unsigned* failure_stage=nullptr) noexcept {
    out={};auto r=find(handle);
    // Reuse the existing bounded caller receipt; no new reads, mutation or
    // runtime gate. A missing matrix must distinguish revoked visibility from
    // wrong actors/components and an unreadable or unstable actual matrix.
    const auto failed=[&](unsigned stage) noexcept {if(failure_stage)*failure_stage=stage;return false;};
    const auto checked=[&]() noexcept {
        if(!r)return failed(1);
        if(r->owner.generation!=owner_generation)return failed(2);
        if(r->publication_withdrawn)return failed(3);
        if(r->visibility_revoke)return failed(4);
        if(r->stage!=Stage::visible)return failed(5);
        if(!r->attached)return failed(6);
        if(!same(r->owner,f))return failed(7);
        if(!same_weapon(r->weapon_pose,f))return failed(8);
        return object(r->component)||failed(9);
    };
    if(!checked())return false;
    const auto property=playground::property(r->component.address,L"LocalToWorld");
    const auto extent=get<unsigned>(r->component.klass+0x88);
    if(!registered(property)||!playground::named(get<uptr>(property+0x50),L"StructProperty")||
       get<unsigned>(property+0x70)!=1||get<unsigned>(property+0x74)!=64||
       !playground::named(get<uptr>(property+0xd0),L"Matrix"))return failed(10);
    const auto offset=get<unsigned>(property+0x94);std::array<float,16> first{},last{};
    if(offset>extent||64>extent-offset||!copy(first.data(),r->component.address+offset,64))return failed(11);
    if(!checked())return false;
    if(!copy(last.data(),r->component.address+offset,64)||first!=last)return failed(12);
    if(!checked())return false;
    for(float v:first)if(!std::isfinite(v))return failed(13);
    if(std::abs(first[3])>1e-4||std::abs(first[7])>1e-4||std::abs(first[11])>1e-4||std::abs(first[15]-1)>1e-4)return failed(14);
    out=first;if(failure_stage)*failure_stage=0;return true;
}
} // namespace weapon_part_assets
