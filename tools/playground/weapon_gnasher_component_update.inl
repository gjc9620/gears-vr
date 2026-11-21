// Complete the existing component lease's setters through the original Actor
// transform update. No independent mesh matrix or controller writer is added.
namespace nativelancerfollow {
bool update_gnasher_component(hand::Frame const& f,Key const& key) noexcept {
    namespace pool=weapon_part_assets;
    const auto current=[&]() noexcept {
        return weaponhand::current_gt(nullptr)&&bound_key_known&&bound_key==key&&
            hand::same(key.weapon,f.weapon)&&key.hold_generation==vrequip::model.hold_generation()&&
            vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address)&&
            lease.has_snapshot()&&!lease.faulted()&&!lease.revoked_mask()&&lease.key()==key&&
            nativeequipment::current(nullptr,f)&&
            playground::instance(f.weapon.address,L"Weap_Shotgun");
    };
    if(!current())return false;
    State before{};vrequip::MeshPoseProbe first{};
    if(!value_of(key,before,&first))return false;
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
       !bool_arg(call,L"bCollisionUpdate",false)||!bool_arg(call,L"bTransformOnly",true)||!current())return false;
    const auto began=weaponhand::clock_ns(nullptr);
    if(!call.run())return false;
    State after{};vrequip::MeshPoseProbe last{};
    const bool accepted=current()&&value_of(key,after,&last)&&gears_weapon::mesh_component_exact(before.value,after.value);
    static unsigned rows{};
    if(log_file&&rows++<12){
        const auto ended=weaponhand::clock_ns(nullptr);
        fwprintf(log_file,L"gnasher_component_transform_update frame=%llu sequence=%llu current_after=%u collision_update=0 transform_only=1 elapsed_ns=%lld input_age_ns=%lld before=%.9g,%.9g,%.9g after=%.9g,%.9g,%.9g native_rva=%llx\n",
            f.key.frame,f.controllers.sequence,unsigned(accepted),ended-began,ended-f.controllers.sync_ns,
            double(first.local_to_world[12]),double(first.local_to_world[13]),double(first.local_to_world[14]),
            double(last.local_to_world[12]),double(last.local_to_world[13]),double(last.local_to_world[14]),get<uptr>(call.r.function+0x114)-base);
        fflush(log_file);
    }
    return accepted;
}
}
