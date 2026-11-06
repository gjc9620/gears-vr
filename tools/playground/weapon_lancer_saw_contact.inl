// Actual held blade -> current original pawn collision -> original validators
// -> one original AcquiredVictim. No head-facing target, synthetic victim,
// direct damage, teleport or stock forward melee-impact query is substituted.
namespace lancersaw_contact {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
using V=gears_vr::Vector;
struct Target {hand::ObjectIdentity pawn{},controller{},collision{};V a{},b{};double radius{};};
hand::ObjectIdentity cursor{},cursor_world{};
std::int64_t claimed_start{};unsigned gate{},reported_gate{~0u},rows{};
template<class T,std::size_t N,std::size_t K>
bool scalar(meta::Reader& r,uptr object,wchar_t const (&label)[N],wchar_t const (&kind)[K],T& out) noexcept {
    weaponhand::BoolField field{};
    return weaponhand::field(r,object,label,kind,unsigned(sizeof(T)),field)&&r.at(object,field.property.offset,out);
}
bool identify(meta::Reader& r,uptr address,hand::ObjectIdentity& out) noexcept {
    return address&&lancermain_asset::identify(r,address,out);
}
bool target(hand::Frame const& f,hand::ObjectIdentity const& pawn,Target& out) noexcept {
    out={};meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,pawn)||
       !weaponhand::derives(r,pawn.address,L"WarPawn",L"WarfareGame"))return false;
    weaponhand::BoolField deleted{},dead{};int health{};
    meta::PointerField world{},controller{},collision{},owner{};
    Target value{};value.pawn=pawn;
    if(!weaponhand::pointer(r,pawn.address,L"WorldInfo",f.world.address,world)||
       !weaponhand::boolean(r,pawn.address,L"bDeleteMe",deleted)||(deleted.bits&deleted.mask)||
       !weaponhand::boolean(r,pawn.address,L"bPlayedDeath",dead)||(dead.bits&dead.mask)||
       !scalar(r,pawn.address,L"Health",L"IntProperty",health)||health<=0||
       !weaponhand::read_pointer(r,pawn.address,L"Controller",controller)||!identify(r,controller.value,value.controller)||
       !weaponhand::derives(r,controller.value,L"Controller",L"Engine")||
       !weaponhand::read_pointer(r,pawn.address,L"CollisionComponent",collision,true)||!identify(r,collision.value,value.collision)||
       !weaponhand::derives(r,collision.value,L"CylinderComponent",L"Engine")||
       !weaponhand::pointer(r,collision.value,L"Owner",pawn.address,owner))return false;
    float radius{},height{};std::array<float,16> matrix{},again{};
    if(!scalar(r,collision.value,L"CollisionRadius",L"FloatProperty",radius)||
       !scalar(r,collision.value,L"CollisionHeight",L"FloatProperty",height)||
       !scalar(r,collision.value,L"LocalToWorld",L"StructProperty",matrix)||
       !std::isfinite(radius)||!std::isfinite(height)||radius<=0||height<=0)return false;
    gears_vr::Viewpoint pose{};V scale{};
    if(!gears_weapon::lancer_starter_draw_pose(matrix,pose,scale))return false;
    value.radius=radius*(std::max)(scale.x,scale.y);
    const double half_height=height*scale.z;
    if(value.radius>200||half_height>1000)return false;
    const double segment=(std::max)(0.,half_height-value.radius);
    const V axis{matrix[8]/scale.z,matrix[9]/scale.z,matrix[10]/scale.z};
    value.a={pose.position.x-axis.x*segment,pose.position.y-axis.y*segment,pose.position.z-axis.z*segment};
    value.b={pose.position.x+axis.x*segment,pose.position.y+axis.y*segment,pose.position.z+axis.z*segment};
    if(!scalar(r,collision.value,L"LocalToWorld",L"StructProperty",again)||again!=matrix||
       !unified_weapon_fire::retained_metadata(r))return false;
    out=value;return true;
}
bool blade(hand::Frame const& f,gears_weapon::LancerBladeOutline& out) noexcept {
    if(!lancersaw_original::acquisition_allowed(f))return false;
    lancermain_asset::Evidence asset{};vrequip::MeshPoseProbe gun{};
    if(lancermain_asset::read(f,asset,lancermain_asset::AssetScope::basic_hold)!=lancermain_asset::Status::ready||
       !hand::same(asset.component,nativelancerfollow::bound_key.component)||!hand::same(asset.asset,nativelancerfollow::bound_key.asset)||
       !nativeequipment::attached_frame(f,asset.component.address)||!vrequip::mesh_pose_probe(f.weapon.address,gun)||gun.hidden||
       gun.mesh!=asset.component.address||gun.asset!=asset.asset.address)return false;
    std::array<float,16> matrix{};std::memcpy(matrix.data(),gun.local_to_world,sizeof(gun.local_to_world));
    return gears_weapon::lancer_chain_outline_from_mesh(matrix,true,out)&&lancersaw_original::acquisition_allowed(f);
}
// Resolve the retail function's actual formal pawn/bool fields, rather than
// borrowing the decompiler's unresolved parameter type/name. Reject extra or
// overlapping parameters; locals do not become call arguments.
bool pawn_arguments(playground::Args& call,uptr pawn,unsigned input_bools,bool returns_bool,unsigned& result_mask) noexcept {
    result_mask=0;if(!call.valid)return false;
    std::array<uptr,64> seen{};unsigned count{},objects{},booleans{},returns{};
    struct Span {unsigned offset{},size{},mask{};};std::array<Span,4> spans{};unsigned spans_count{};
    for(auto field=get<uptr>(call.r.function+0x80);field;field=get<uptr>(field+0x68)){
        if(count==seen.size()||!registered(field)||get<uptr>(field+0x40)!=call.r.function)return false;
        for(unsigned i=0;i<count;++i)if(seen[i]==field)return false;seen[count++]=field;
        const auto flags=get<unsigned long long>(field+0x78);if(!(flags&0x80))continue;
        if(get<unsigned>(field+0x70)!=1||spans_count==spans.size())return false;
        const auto offset=get<unsigned>(field+0x94),size=get<unsigned>(field+0x74);
        if(offset>call.r.size||size>call.r.size-offset)return false;
        wchar_t label[64]{};if(get<Name>(field+0x48).number||!name(field,label,64))return false;
        const auto type=get<uptr>(field+0x50);unsigned mask{};
        if(flags&0x400){
            if(!returns_bool||std::wcscmp(label,L"ReturnValue")||size!=4||!playground::named(type,L"BoolProperty"))return false;
            mask=get<unsigned>(field+0xd0);if(!mask||(mask&(mask-1)))return false;
            result_mask=mask;++returns;
        }else if(playground::named(type,L"ObjectProperty")&&size==8&&!(flags&0x100)){
            const auto klass=get<uptr>(field+0xd0);
            if(!registered(klass)||!playground::named(get<uptr>(klass+0x50),L"Class")||
               (!playground::named(klass,L"WarPawn")&&!playground::named(klass,L"Pawn"))||!call.set(label,pawn).valid)return false;
            ++objects;
        }else if(playground::named(type,L"BoolProperty")&&size==4&&!(flags&0x100)){
            mask=get<unsigned>(field+0xd0);if(!mask||(mask&(mask-1))||!call.flag(label,true).valid)return false;++booleans;
        }else return false;
        for(unsigned i=0;i<spans_count;++i){const auto& prior=spans[i];
            if(offset<prior.offset+prior.size&&prior.offset<offset+size&&
               !(offset==prior.offset&&size==4&&prior.size==4&&mask&&prior.mask&&!(mask&prior.mask)))return false;}
        spans[spans_count++]={offset,size,mask};
    }
    return objects==1&&booleans==input_bools&&returns==unsigned(returns_bool);
}
bool original_validators(hand::Frame const& f,Target const& victim,bool& pawn_ok,bool& weapon_ok) noexcept {
    pawn_ok=weapon_ok=false;
    playground::Args pawn(f.pawn.address,L"IsValidMeleeTarget"),weapon(f.weapon.address,L"IsTargetValidForSpecialMeleeAttack");
    unsigned pawn_mask{},weapon_mask{};
    if(!lancerreload_ui::original_function(pawn.r.function,L"IsValidMeleeTarget",L"WarPawn",false)||
       !lancerreload_ui::original_function(weapon.r.function,L"IsTargetValidForSpecialMeleeAttack",L"Weap_AssaultRifleBase",false)||
       !pawn_arguments(pawn,victim.pawn.address,1,true,pawn_mask)||!pawn_arguments(weapon,victim.pawn.address,0,true,weapon_mask)||
       !lancersaw_original::acquisition_allowed(f)||!pawn.run())return false;
    pawn_ok=(pawn.output<unsigned>()&pawn_mask)!=0;
    if(!pawn_ok)return true;
    if(!lancersaw_original::acquisition_allowed(f)||!weapon.run())return false;
    weapon_ok=(weapon.output<unsigned>()&weapon_mask)!=0;return lancersaw_original::acquisition_allowed(f);
}
bool adhesion(hand::Frame const& f,Target const& victim) noexcept {
    playground::Args call(f.pawn.address,L"GetAdhesionTarget");
    if(!lancerreload_ui::original_function(call.r.function,L"GetAdhesionTarget",L"WarPawn",false)||
       call.r.size!=8||!vrequip::result_property(call,L"ObjectProperty",8)||!lancersaw_original::acquisition_allowed(f)||!call.run())return false;
    const auto target_address=call.output<uptr>();
    return (!target_address||target_address==victim.pawn.address)&&lancersaw_original::acquisition_allowed(f);
}
bool trace_clear(hand::Frame const& f,uptr victim,V start,V end) noexcept {
    const double distance=gears_weapon::length(gears_weapon::difference(start,end));
    if(distance<.01)return true;
    playground::Args call(f.pawn.address,L"Trace");
    const auto vector_arg=[&](wchar_t const* label,playground::Vector const& value){
        const auto field=call.prop(label);const auto structure=field?get<uptr>(field+0xd0):0;
        return hammerburst_animation::input_field(call,field,L"StructProperty",12)&&registered(structure)&&
            playground::named(structure,L"Vector")&&call.set(label,value).valid;};
    const playground::Vector from{float(start.x),float(start.y),float(start.z)},to{float(end.x),float(end.y),float(end.z)},extent{};
    if(!weapon_part_assets::native_function(call,L"Trace",L"Actor")||!vrequip::result_property(call,L"ObjectProperty",8)||
       !vector_arg(L"TraceStart",from)||!vector_arg(L"TraceEnd",to)||!vector_arg(L"Extent",extent)||
       !nativelancerfollow::bool_arg(call,L"bTraceActors",true)||!lancersaw_original::acquisition_allowed(f)||!call.run())return false;
    const auto hit=call.output<uptr>();return (!hit||hit==victim)&&lancersaw_original::acquisition_allowed(f);
}
bool wall_clear(hand::Frame const& f,Target const& victim,gears_weapon::LancerBladeOutline const& edge) noexcept {
    V contact{},axis{};double best=std::numeric_limits<double>::infinity();
    for(unsigned i=0;i<edge.world_raw.size();++i){V p{},q{};
        const auto d=gears_weapon::closest_segment_points(edge.world_raw[i],edge.world_raw[(i+1)%edge.world_raw.size()],victim.a,victim.b,p,q);
        if(d<best){best=d;contact=p;axis=q;}}
    return std::isfinite(best)&&trace_clear(f,victim.pawn.address,edge.world_raw.front(),contact)&&
        trace_clear(f,victim.pawn.address,contact,axis);
}
bool attacker(hand::Frame const& f) noexcept {
    gears_lancer_saw_input::Observation observation{};
    if(!lancersaw_original::context(lancersaw_original::button.owner(),observation)||!observation.actors_current||
       !observation.button_held||observation.special_move!=33||!observation.motor_running||!observation.doing_melee||observation.doing_special)return false;
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    unsigned char mode{};int health{};weaponhand::BoolField dead{};meta::PointerField current_victim{};
    return meta::tables(base,schema.objects,schema.names)&&weaponhand::object(r,f.world)&&weaponhand::object(r,f.pawn)&&weaponhand::object(r,f.weapon)&&
        scalar(r,f.world.address,L"NetMode",L"ByteProperty",mode)&&mode<3&& // shipped ENetMode0/1/2 authoritative,3 client
        scalar(r,f.pawn.address,L"Health",L"IntProperty",health)&&health>0&&
        weaponhand::boolean(r,f.pawn.address,L"bPlayedDeath",dead)&&!(dead.bits&dead.mask)&&
        weaponhand::read_pointer(r,f.weapon.address,L"SpecialMeleeAttackVictim",current_victim)&&!current_victim.value&&
        unified_weapon_fire::retained_metadata(r)&&lancersaw_original::acquisition_allowed(f);
}
bool try_victim(hand::Frame const& f,Target const& victim,gears_weapon::LancerBladeOutline const& current_blade) noexcept {
    if(victim.pawn.address==f.pawn.address)return false;
    gears_weapon::LancerSawContact contact{};contact.victim=victim.pawn.address;contact.target_a_raw=victim.a;
    contact.target_b_raw=victim.b;contact.target_radius_raw=victim.radius;
    contact.blade=current_blade;
    gate=10;if(!gears_weapon::lancer_saw_blade_geometry(contact)||!attacker(f))return false;
    bool pawn_ok{},weapon_ok{};gate=11;
    if(!original_validators(f,victim,pawn_ok,weapon_ok)||!pawn_ok||!weapon_ok)return false;
    gate=12;if(!adhesion(f,victim)||!wall_clear(f,victim,contact.blade))return false;
    Target current{};gate=13;
    if(!target(f,victim.pawn,current)||!hand::same(current.controller,victim.controller)||
       !hand::same(current.collision,victim.collision)||!attacker(f)||!blade(f,contact.blade))return false;
    contact.target_a_raw=current.a;contact.target_b_raw=current.b;contact.target_radius_raw=current.radius;
    contact.original={true,true,true,true,true,true,true,pawn_ok,weapon_ok,true,true};
    if(!gears_weapon::lancer_saw_blade_contact(contact))return false;
    playground::Args call(f.weapon.address,L"AcquiredVictim");unsigned unused{};
    gate=14;if(!lancerreload_ui::original_function(call.r.function,L"AcquiredVictim",L"Weap_AssaultRifleBase",false)||
       !pawn_arguments(call,victim.pawn.address,0,false,unused)||!lancersaw_original::acquisition_allowed(f))return false;
    // Journal before dispatch. An exception/uncertain result cannot replay the
    // victim request while this same original B press remains owned.
    claimed_start=lancersaw_original::started_ns;
    bool succeeded{};{lancersaw_original::Mutation mutation;succeeded=call.run();}
    if(!succeeded)lancersaw_original::stop_pending=true;
    if(log_file){fwprintf(log_file,L"lancer_saw_victim_requested frame=%llu sequence=%llu victim=%llx original_call=%u actual_execution_pending=1\n",
        f.key.frame,f.controllers.sequence,victim.pawn.address,unsigned(succeeded));fflush(log_file);}
    gate=succeeded?0:15;return true;
}
void tick(hand::Frame const& f) noexcept {
    if(!lancersaw_original::active()){cursor={};cursor_world={};claimed_start=0;return;}
    if(claimed_start==lancersaw_original::started_ns||!lancersaw_original::acquisition_allowed(f))return;
    gate=1;if(!attacker(f))return;
    gears_weapon::LancerBladeOutline current_blade{};if(!blade(f,current_blade))return;
    const auto began=weaponhand::clock_ns(nullptr);
    if(!hand::same(cursor_world,f.world)){cursor={};cursor_world=f.world;}
    if(!cursor.address){meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};meta::PointerField first{};
        gate=2;if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,f.world)||
           !weaponhand::read_pointer(r,f.world.address,L"PawnList",first)||!first.value||!identify(r,first.value,cursor)||
           !unified_weapon_fire::retained_metadata(r))return;}
    std::array<uptr,16> visited{};unsigned count{};
    while(cursor.address&&count<visited.size()&&weaponhand::clock_ns(nullptr)-began<6000000){
        for(unsigned i=0;i<count;++i)if(visited[i]==cursor.address){cursor={};return;}visited[count++]=cursor.address;
        const auto candidate=cursor;meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};meta::PointerField next{};
        gate=3;if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(r,candidate)||
           !weaponhand::read_pointer(r,candidate.address,L"NextPawn",next)||
           (next.value&&!identify(r,next.value,cursor))||!unified_weapon_fire::retained_metadata(r)){cursor={};return;}
        if(!next.value)cursor={};
        Target victim{};
        if(candidate.address!=f.pawn.address&&target(f,candidate,victim)&&try_victim(f,victim,current_blade))return;
        if(!lancersaw_original::acquisition_allowed(f))return;
    }
    if(gate!=reported_gate&&rows++<32&&log_file){
        fwprintf(log_file,L"lancer_saw_contact gate=%u scanned=%u claimed=%u\n",gate,count,unsigned(claimed_start==lancersaw_original::started_ns));fflush(log_file);}
    reported_gate=gate;
}
} // namespace lancersaw_contact
