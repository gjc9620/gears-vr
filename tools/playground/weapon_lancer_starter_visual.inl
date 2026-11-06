// Three authored starter parts, one original-engine resource owner. Current
// gun transforms supply display only; contact/action authority remains Native's
// existing hand lease. No independent camera, gun or skeletal writer is added.
namespace lancersaw_visual {
namespace hand=gears_weapon_hand;namespace asset=weapon_part_assets;
namespace input=weapon_part_inputs;namespace geometry=gears_weapon::part_geometry;
namespace inv=lancerreload_inventory;namespace meta=gripdamage_metadata;
// Moving socket/grip/cord plus an exact rigid union for the common rest pose.
std::array<asset::Handle,4> handles{};
std::uint64_t generation{},next_generation{1};inv::Identity identity{};
asset::WeaponPoseOwner owner{};hand::ObjectIdentity surface{},light{};
bool light_empty{},ready{},visible{},resting{};unsigned failure{},prepare_cursor{};
std::array<geometry::Prepared,4> prepared{};
PhaseCost last_cost{};
asset::Handle socket_handle() noexcept {return handles[resting?3:0];}
asset::Handle grip_handle() noexcept {return handles[resting?3:1];}

bool same_binding(hand::Frame const& f) noexcept {
    return generation&&identity==inv::identity(f)&&asset::same_weapon(owner,f);
}
void hide() noexcept {
    ready=visible=resting=false;
    for(auto h:handles)if(auto r=asset::find(h);r&&(r->stage==asset::Stage::visible||r->stage==asset::Stage::ready))
        asset::set_visible(h,generation,false);
}
bool restore() noexcept {
    hide();bool clean=true;
    for(auto& h:handles){
        if(h.generation&&asset::find(h)&&!asset::destroy(h,generation)){clean=false;continue;}
        h={};
    }
    if(!clean)return false;
    prepared={};generation=0;identity={};owner={};surface={};light={};light_empty=false;prepare_cursor=0;return true;
}
bool gun_matrix(hand::Frame const& f,std::array<float,16>& raw) noexcept {
    raw={};vrequip::MeshPoseProbe gun{};
    // Sole caller: immediately after prepare(f)'s closing full source check,
    // with no intervening engine dispatch. Keep the closing check below too.
    if(!vrequip::mesh_pose_probe(f.weapon.address,gun)||gun.hidden||
       gun.mesh!=nativelancerfollow::bound_key.component.address||gun.asset!=nativelancerfollow::bound_key.asset.address)return false;
    std::memcpy(raw.data(),gun.local_to_world,sizeof(gun.local_to_world));
    gears_weapon::LancerStarterWorld world{};
    return gears_weapon::lancer_starter_world(raw,world)&&nativeequipment::current(nullptr,f);
}
bool material(hand::Frame const& f,hand::ObjectIdentity& out) noexcept {
    out={};const auto component=nativelancerfollow::bound_key.component.address;
    playground::Args call(component,L"GetMaterial");const auto in=call.prop(L"ElementIndex"),result=call.prop(L"ReturnValue");
    if(!asset::native_function(call,L"GetMaterial",L"MeshComponent")||
       !hammerburst_animation::input_field(call,in,L"IntProperty",4)||!vrequip::result_property(call,L"ObjectProperty",8)||
       !(get<unsigned>(in+0x94)+4<=get<unsigned>(result+0x94)||get<unsigned>(result+0x94)+8<=get<unsigned>(in+0x94))||
       !call.set(L"ElementIndex",0).run()||!nativeequipment::current(nullptr,f))return false;
    const auto address=call.output<uptr>();
    return playground::instance(address,L"MaterialInterface")&&nativeequipment::identify(nullptr,address,out)&&nativeequipment::current(nullptr,f);
}
bool light_environment(hand::Frame const& f) noexcept {
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    meta::PointerField value{};hand::ObjectIdentity found{};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,owner.component)||
       !weaponhand::read_pointer(r,owner.component.address,L"LightEnvironment",value,true)||
       (value.value&&(!lancermain_asset::identify(r,value.value,found)||!playground::instance(value.value,L"LightEnvironmentComponent")))||
       !unified_weapon_fire::retained_metadata(r)||!nativeequipment::current(nullptr,f))return false;
    light=found;light_empty=!value.value;return true;
}
bool prepare(hand::Frame const& f) noexcept try {
    failure=1;asset::WeaponPoseOwner current{};
    if(asset::stop_requested.load(std::memory_order_acquire)||!unified_weapon_fire::manual_profile()||!asset::weapon_owner(f,current)){
        restore();return false;
    }
    if(generation&&!same_binding(f)){restore();return false;}
    bool all_ready=generation!=0;
    if(all_ready)for(auto h:handles){auto r=asset::find(h);
        if(!r||!r->lighting_bound||(r->stage!=asset::Stage::ready&&r->stage!=asset::Stage::visible)){all_ready=false;break;}}
    if(all_ready){
        // Only owner keys/resource records were read. No constructor, setter,
        // pose or engine call occurred; one closing observation replaces the
        // duplicate entry+exit queries for already prepared resources.
        failure=0;return same_binding(f)&&nativeequipment::current(nullptr,f);
    }
    if(!nativeequipment::current(nullptr,f))return false;
    if(lancerreload_runtime::needs_priority()){
        // Keep already-created gun parts visible through reload. Defer new
        // constructors until idle, rather than hiding a mounted starter just
        // because the left hand currently owns a magazine/receiver.
        if(!generation)return false;
        for(auto h:handles){auto r=asset::find(h);
            if(!r||!r->lighting_bound||(r->stage!=asset::Stage::ready&&r->stage!=asset::Stage::visible))return false;}
    }
    if(!generation){
        if(!next_generation||next_generation==UINT64_MAX)return false;
        hand::ObjectIdentity original_surface{};failure=2;if(!material(f,original_surface))return false;
        std::array<geometry::Prepared,3> data{};
        const geometry::Conversion conversion{{{{1,0,0},{0,1,0},{0,0,1}}},100.,f.reference_generation,true};
        const char* ids[]={"lancer-starter-StarterSocket","lancer-starter-StarterPullGrip","lancer-starter-StarterCord"};
        for(unsigned i=0;i<3;++i){failure=3+i;if(input::load(ids[i],conversion,data[i])!=input::Result::ready)return false;}
        geometry::Prepared rigid{};failure=7;
        if(geometry::merge_rigid(data,rigid)!=geometry::Result::ready)return false;
        if(!nativeequipment::current(nullptr,f))return false;
        generation=next_generation++;identity=inv::identity(f);owner=current;surface=original_surface;
        for(unsigned i=0;i<3;++i)prepared[i]=std::move(data[i]);
        prepared[3]=std::move(rigid);
        failure=6;if(!light_environment(f)){restore();return false;}
    }
    // Advance only one part per new scene so construction does not consume a
    // complete frame's input lifetime. Never recreate a pending handle.
    for(unsigned n=0;n<4;++n){const unsigned i=(prepare_cursor+n)%4;
        auto r=asset::find(handles[i]);
        if(!handles[i].generation){
            failure=10+i;
            if(asset::create_weapon_part(f,generation,prepared[i],{surface,surface,surface,surface},handles[i])==asset::Creation::rejected)return false;
            prepared[i]={};prepare_cursor=(i+1)%4;return false;
        }
        if(!r){restore();return false;}
        if(r->stage!=asset::Stage::ready&&r->stage!=asset::Stage::visible){
            failure=20+i;const auto state=asset::advance(f,handles[i],generation);prepare_cursor=(i+1)%4;
            if(state==asset::Creation::rejected)restore();return false;
        }
        if(!r->lighting_bound){failure=30+i;
            if(!asset::bind_light_environment(f,handles[i],generation,light,light_empty))return false;
            prepare_cursor=(i+1)%4;return false;
        }
    }
    failure=0;return same_binding(f)&&nativeequipment::current(nullptr,f);
}catch(...){hide();return false;}

void report() noexcept {
    static unsigned prior_failure{~0u},prior_state{~0u},rows{};
    const auto state=unsigned(ready)|(unsigned(visible)<<1);
    if(rows<32&&(prior_failure!=failure||prior_state!=state)&&log_file){
        fwprintf(log_file,L"lancer_starter_visual gate=%u ready=%u visible=%u generation=%llu asset_gate=%u display_only=1\n",
            failure,unsigned(ready),unsigned(visible),generation,asset::failure);fflush(log_file);++rows;}
    prior_failure=failure;prior_state=state;
}
template<std::size_t N>
bool draw(hand::Frame const& f,std::array<unsigned,N> const& selected,
          std::array<std::array<float,16> const*,N> const& matrices) noexcept {
    const auto stage_begin=weaponhand::clock_ns(nullptr);
    std::array<asset::WeaponPoseRequest,N> requests{};
    for(unsigned i=0;i<N;++i){gears_vr::Viewpoint pose{};gears_vr::Vector scale{};
        failure=41+i;
        if(!gears_weapon::lancer_starter_draw_pose(*matrices[i],pose,scale)||
           !asset::stage_weapon_pose(f,handles[selected[i]],generation,pose,scale,true,requests[i]))return false;
    }
    last_cost.stage_ns=weaponhand::clock_ns(nullptr)-stage_begin;
    const auto flush_begin=weaponhand::clock_ns(nullptr);
    failure=47;if(!asset::flush_weapon_poses(f,requests))return false;
    last_cost.flush_ns=weaponhand::clock_ns(nullptr)-flush_begin;
    const auto ack_begin=weaponhand::clock_ns(nullptr);
    // Withdraw the other representation before publishing the selected one.
    // Never show an unacknowledged rest pose or leave overlapping duplicates.
    for(unsigned i=0;i<handles.size();++i){
        if(std::find(selected.begin(),selected.end(),i)!=selected.end())continue;
        auto r=asset::find(handles[i]);
        if(!r||(r->stage==asset::Stage::visible&&!asset::set_visible(handles[i],generation,false)))return false;
    }
    for(unsigned i=0;i<N;++i){failure=44+i;
        if(!asset::acknowledge_weapon_pose(f,requests[i]))return false;}
    last_cost.ack_ns=weaponhand::clock_ns(nullptr)-ack_begin;
    return true;
}
void tick(hand::Frame const& f,double pull_metres) noexcept try {
    struct Review {~Review(){report();}} review;
    last_cost={};const auto cost_before=asset::source_cost;
    struct SourceReview {
        asset::SourceCost before;
        ~SourceReview(){last_cost.asset_source_calls=asset::source_cost.calls-before.calls;
            last_cost.asset_source_ns=asset::source_cost.ns-before.ns;}
    } source_review{cost_before};
    ready=false;
    const auto prepare_begin=weaponhand::clock_ns(nullptr);const bool prepared_now=prepare(f);
    last_cost.prepare_ns=weaponhand::clock_ns(nullptr)-prepare_begin;
    if(!prepared_now){hide();return;}
    std::array<float,16> gun{};gears_weapon::LancerStarterParts parts{};
    const auto gun_begin=weaponhand::clock_ns(nullptr);
    failure=40;if(!gun_matrix(f,gun)||!gears_weapon::lancer_starter_parts(gun,pull_metres,nativelancerfollow::bound_units,parts)){
        hide();return;}
    last_cost.gun_ns=weaponhand::clock_ns(nullptr)-gun_begin;
    const bool at_rest=pull_metres==0.;
    // All three authored local transforms must be identical for the union.
    // Do not approximate a small pull or discard the actual cord animation.
    if(at_rest&&(parts.socket!=parts.grip||parts.socket!=parts.cord)){failure=48;hide();return;}
    const bool drawn=at_rest?
        draw(f,std::array<unsigned,1>{3},std::array<std::array<float,16> const*,1>{&parts.socket}):
        draw(f,std::array<unsigned,3>{0,1,2},std::array<std::array<float,16> const*,3>{&parts.socket,&parts.grip,&parts.cord});
    if(!drawn){hide();return;}
    // Readback proves current original component matrices, not actual pixels
    // or material quality. The combined paired recording must inspect those.
    resting=at_rest;visible=ready=true;failure=0;
}catch(...){hide();}
} // namespace lancersaw_visual
