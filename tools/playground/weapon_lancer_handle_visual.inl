// Two actual original-engine transient MorphTargets. Their immutable native
// deltas affect only the64 original FRONT-handle vertices. Native owns instance render
// input and its FIFO fence; this owner releases roots only after that fence.
namespace lancerhandle_visual {
namespace api=gears_lancer_handle_morph;namespace hand=gears_weapon_hand;
namespace asset=weapon_part_assets;
struct Part {asset::Resource resource{};unsigned stage{};bool unsafe_lod{};};
std::array<Part,2> parts{};std::array<std::vector<std::uint8_t>,2> input{};
asset::WeaponPoseOwner weapon{};api::Control control{};
std::atomic<unsigned> outstanding{};std::atomic<bool> stop_requested{};
std::uint64_t generation{},next_generation{1};unsigned cursor{},failure{};
bool revoking{},neutral_prepared{};
double wanted_distance_raw{};
template<class Fn>Fn function(const char* label) noexcept {
    const auto address=weaponhand::native_module?GetProcAddress(weaponhand::native_module,label):nullptr;
    return address&&weaponhand::executable(address,weaponhand::native_module)?reinterpret_cast<Fn>(address):nullptr;
}
bool live() noexcept {return outstanding.load(std::memory_order_acquire)!=0;}
void request_stop() noexcept {stop_requested.store(true,std::memory_order_release);}
bool reset_after_drain() noexcept {
    if(live())return false;stop_requested.store(false,std::memory_order_release);return true;
}
void report(api::Receipt const* r=nullptr) noexcept {
    static unsigned last=~0u,rows{};
    if(log_file&&rows<32&&(last!=failure||r)){
        fwprintf(log_file,L"lancer_handle_visual gate=%u generation=%llu handle=%llu pending=%u revoking=%u submitted=%llu rendered_payloads=%llu distance_raw=%.9g fenced=%u matched=%llu angle_rejects=%llu admission_rejects=%llu regular_factory=%llu regular_secondary=%llu display_only=1\n",
            failure,generation,control.handle,outstanding.load(),unsigned(revoking),r?r->submitted:0,r?r->rendered:0,r?r->last_distance_raw:0,r?unsigned(r->fenced):0,
            r?r->matched_updates:0,r?r->angle_rejections:0,r?r->admission_rejections:0,
            r?r->regular_factory_routes:0,r?r->regular_secondary_routes:0);
        fflush(log_file);++rows;last=failure;
    }
}
bool restore() noexcept {
    if(!asset::gt())return false;
    if(!live())return true;
    wanted_distance_raw=0;
    revoking=true;failure=80;
    if(control.handle){
        const auto call=function<api::Revoke>(api::revoke_export);if(!call)return false;
        api::Receipt result{};const auto code=call(&control,&result);
        if(code!=api::Result::retired||result.size!=sizeof result||result.version!=2||
           result.result!=code||result.handle!=control.handle||!result.fenced){report();return false;}
        report(&result);control={};
    }
    // A partially initialized native LOD must remain rooted/quarantined.
    // Nothing is submitted from a failed preparation; never let engine GC
    // traverse an uninitialized array element after an ambiguous write.
    for(auto& p:parts){
        if(p.unsafe_lod){failure=81;report();return false;}
        if(p.resource.mesh_root&&!asset::unroot(p.resource.mesh,p.resource.mesh_root)){failure=82;report();return false;}
    }
    parts={};input={};weapon={};generation=0;cursor=0;revoking=neutral_prepared=false;
    outstanding.store(0,std::memory_order_release);failure=0;return true;
}
bool load() noexcept try {
    const wchar_t* files[]={L"pullA.delta20",L"pullB.delta20"};
    const char* hashes[]={gears_weapon::lancer_front::delta_sha256,gears_weapon::lancer_front::delta_sha256};
    std::wstring folder;
    if(!weapon_part_inputs::folder(folder,L"/analysis/weapon-part-cache/lancer-front-handle-df356ff3-p3"))return false;
    const auto nib=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(unsigned i=0;i<2;++i){std::array<std::uint8_t,32> wanted{};
        for(unsigned j=0;j<wanted.size();++j)wanted[j]=std::uint8_t((nib(hashes[i][j*2])<<4)|nib(hashes[i][j*2+1]));
        if(!weapon_part_inputs::read(folder+L"/"+files[i],api::vertices*sizeof(api::Delta),input[i])||
           !weapon_part_inputs::hash(input[i],wanted))return false;
    }return true;
}catch(...){return false;}
bool prepare(hand::Frame const& f) noexcept try {
    if(revoking||stop_requested.load())return false;
    if(control.handle){if(asset::same_weapon(weapon,f))return true;restore();return false;}
    asset::WeaponPoseOwner selected{};failure=1;
    if(!asset::weapon_owner(f,selected)||!nativeequipment::current(nullptr,f))return false;
    if(generation&&!asset::same_weapon(weapon,f)){restore();return false;}
    if(control.handle){failure=0;return true;}
    if(!generation){
        failure=2;if(!load()||!asset::certify()||!next_generation||next_generation==UINT64_MAX)return false;
        generation=next_generation++;weapon=selected;
        for(auto& p:parts)p.resource.owner={f.world,f.pawn,f.key.game_start,f.key.native_started_qpc,
            f.key.registration_cookie,f.key.world_generation,generation};
        outstanding.store(1,std::memory_order_release);return false;
    }
    auto& p=parts[cursor];auto& r=p.resource;
    if(p.stage==0){
        asset::Identity klass{};failure=10+cursor;
        if(!asset::find_class(L"MorphTarget",klass)||get<unsigned>(klass.address+0x88)!=112)return false;
        if(!asset::construct(f,r,klass,r.mesh,r.mesh_root)){
            if(r.mesh_root)p.stage=1;
            return false;
        }
        p.stage=1;return false;
    }
    if(p.stage==1){
        failure=20+cursor;
        if(!asset::current(r.owner,f)||!r.mesh_root||!asset::object(r.mesh))return false;
        const api::Lod empty{{},12721};
        p.unsafe_lod=true; // Journal before raw array append/initialization.
        if(!asset::native_array(r.mesh.address+96,1,sizeof(api::Lod),&empty))return false;
        api::Array lods{};api::Lod actual{};
        if(!copy(&lods,r.mesh.address+96,sizeof lods)||lods.count!=1||!lods.data||
           !copy(&actual,lods.data,sizeof actual)||actual.vertices.data||actual.vertices.count||
           actual.vertices.capacity||actual.base_vertices!=12721)return false;
        p.unsafe_lod=false;p.stage=2;return false;
    }
    if(p.stage==2){
        failure=30+cursor;api::Array lods{};
        if(!asset::current(r.owner,f)||!asset::object(r.mesh)||
           !copy(&lods,r.mesh.address+96,sizeof lods)||lods.count!=1||!lods.data||
           !asset::native_array(lods.data,api::vertices,sizeof(api::Delta),input[cursor].data())){revoking=true;return false;}
        p.stage=3;cursor=(cursor+1)%2;return false;
    }
    for(unsigned i=0;i<2;++i)if(parts[i].stage!=3){cursor=i;return false;}
    failure=40;const auto bind=function<api::Begin>(api::begin_export);if(!bind)return false;
    api::Request q{};q.source=f;q.component=weapon.component;q.asset=weapon.asset;
    for(unsigned i=0;i<2;++i)q.targets[i]=parts[i].resource.mesh;
    if(!nativeequipment::current(nullptr,f))return false;
    api::Receipt result{};const auto code=bind(&q,&result);
    if(code!=api::Result::ready||result.size!=sizeof result||result.version!=2||
       result.result!=code||!result.handle){failure=100+result.binding_gate;return false;}
    control={};control.handle=result.handle;control.cookie=f.key.registration_cookie;failure=0;report(&result);return true;
}catch(...){revoking=true;return false;}
bool ready_for(hand::Frame const& f) noexcept {
    return asset::gt()&&!revoking&&!stop_requested.load()&&control.handle&&
        control.cookie==f.key.registration_cookie&&asset::same_weapon(weapon,f);
}
bool observe_existing_owner() noexcept {
    if(!control.handle)return false;
    const auto poll=function<api::Revoke>(api::poll_export);api::Receipt state{};
    if(!poll)return false;
    const auto status=poll(&control,&state);
    if(state.size!=sizeof state||state.version!=2||state.handle!=control.handle||state.result!=status)return false;
    if(status==api::Result::revoking||status==api::Result::retired)revoking=true;
    return status==api::Result::ready;
}
bool set_distance(hand::Frame const& f,double distance,api::Receipt& out) noexcept {
    out={};if(!ready_for(f)||!std::isfinite(distance)||distance<0||
       distance>gears_weapon::lancer_front::travel_raw)return false;
    const auto update=function<api::Revoke>(api::update_export);if(!update||!nativeequipment::current(nullptr,f))return false;
    auto request=control;request.source=f;request.distance_raw=distance;
    const auto code=update(&request,&out);
    if(out.size!=sizeof out||out.version!=2||out.handle!=control.handle||out.result!=code||code!=api::Result::ready){
        // Native can retire a replaced GPU generation without accepting a new
        // display update. Query only that existing handle/cookie, then use the
        // normal restore/fence/unroot path before preparing its replacement.
        // A rejected input alone never grants a rebind or clears ownership.
        observe_existing_owner();
        return false;
    }
    neutral_prepared=out.neutral_prepared;
    wanted_distance_raw=distance;return true;
}
void tick(uptr pc) noexcept {
    if(!asset::gt())return;
    // Reject before constructing/rooting targets. Native independently rejects
    // old callers too. Existing withdrawal still drains retained ownership.
    if(!api::retail_material_route_available){if(live())restore();return;}
    if(stop_requested.load()||revoking){restore();return;}
    hand::Frame f{};hand::GripGameState state{};
    if(!weaponhand::client.copy_frame_on_gt(f,state)||f.controller.address!=pc)return;
    if(!state.known||!state.main_hand_held||!nativelancerfollow::bound_is_lancer){if(live())restore();return;}
    if(control.handle&&!asset::same_weapon(weapon,f)){restore();return;}
    if(!control.handle)prepare(f);report();
    if(control.handle){
        // Charging step already submits this frame's authenticated motion.
        // An idle zero target needs no repeated Native metadata transaction
        // after its observed neutral preparation. Neither branch grants input.
        if(lancer_charging::ownership.active)return;
        if(neutral_prepared&&wanted_distance_raw==0){
            // A neutral upload can outlive its GPU instance. Observe only the
            // existing handle here so its withdrawal/rebind finishes while
            // idle, rather than consuming the player's first pinch deadline.
            // This read grants no input or part travel.
            if(!observe_existing_owner()&&revoking)restore();
            return;
        }
        api::Receipt submitted{};
        if(!set_distance(f,wanted_distance_raw,submitted))return;
        const auto poll=function<api::Revoke>(api::poll_export);api::Receipt result{};
        if(poll&&poll(&control,&result)==api::Result::ready&&result.size==sizeof result&&result.handle==control.handle){
            static unsigned bucket=~0u;const auto current=unsigned(std::lround(result.last_distance_raw*20));
            if(current!=bucket){bucket=current;report(&result);}
        }
    }
}
}
