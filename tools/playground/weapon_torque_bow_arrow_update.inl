// The same original Actor transform-only update already used by the common
// component owner, restricted here to this exact current ammo component.
namespace torquebow_arrow_visual {
namespace pool=weapon_part_assets;namespace hand=gears_weapon_hand;
bool update(hand::Frame const& f,pool::Handle handle,std::uint64_t generation,hand::Decision const& d) noexcept {
    const auto current=[&]() noexcept {
        const auto r=pool::find(handle);
        return weaponhand::current_gt(nullptr)&&r&&r->owner.generation==generation&&
            !r->publication_withdrawn&&r->component_root&&r->attached&&r->lighting_bound&&
            (r->stage==pool::Stage::ready||r->stage==pool::Stage::visible)&&pool::same(r->owner,f)&&
            pool::object(r->component)&&hand::same(r->pose_lease,d.lease)&&
            hand::same(d.key,f.key)&&!d.release_required&&
            (d.status==hand::DecisionStatus::granted||d.status==hand::DecisionStatus::retained)&&
            hand::lease_valid(d.lease,f.key.lifecycle)&&d.lease.kind==hand::LeaseKind::ammo&&
            d.press_token==f.press_token&&d.lease.id==f.press_token&&f.left_tracked&&f.left_grip_known&&f.left_grip_down&&
            nativeequipment::current(nullptr,f);
    };
    if(!current())return false;
    playground::Args call(f.pawn.address,L"ForceUpdateComponents");
    const auto collision=call.prop(L"bCollisionUpdate"),transform=call.prop(L"bTransformOnly");
    if(!pool::native_function(call,L"ForceUpdateComponents",L"Actor")||
       !hammerburst_animation::input_field(call,collision,L"BoolProperty",4)||
       !hammerburst_animation::input_field(call,transform,L"BoolProperty",4)||
       get<unsigned short>(call.r.function+0x108)!=0xffff)return false;
    unsigned count{},parameters{};std::array<uptr,32> seen{};
    for(auto child=get<uptr>(call.r.function+0x80);child;child=get<uptr>(child+0x68)){
        if(count==seen.size()||!registered(child)||get<uptr>(child+0x40)!=call.r.function)return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;seen[count++]=child;
        if(get<unsigned long long>(child+0x78)&0x80){
            if(child!=collision&&child!=transform)return false;++parameters;
        }
    }
    if(parameters!=2)return false;
    const auto a=get<unsigned>(collision+0x94),b=get<unsigned>(transform+0x94);
    const auto am=get<unsigned>(collision+0xd0),bm=get<unsigned>(transform+0xd0);
    if(!am||!bm||(am&(am-1))||(bm&(bm-1))||
       (a==b?(am&bm)!=0:!(a+4<=b||b+4<=a))||call.r.size!=(std::max)(a+4,b+4)||
       !nativelancerfollow::bool_arg(call,L"bCollisionUpdate",false)||
       !nativelancerfollow::bool_arg(call,L"bTransformOnly",true)||!current()||!call.run())return false;
    return current();
}
}
