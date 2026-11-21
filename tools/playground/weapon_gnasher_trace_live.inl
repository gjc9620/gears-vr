#include "weapon_gnasher_trace_input.h"
// Existing GN subscriber's read-only trajectory observation. No parameters,
// results, spread, collision, input or original shot execution are changed.
namespace gnasher_trace_observation {
namespace f=unified_weapon_fire;namespace meta=gripdamage_metadata;
bool input(gears_weapon_call::Invocation const& call,f::SourceScope const& source,
           gears_weapon_call_trace::FireInput& out,unsigned& code_offset,unsigned& failure_site) noexcept {
    const auto reject=[&](unsigned site) noexcept {failure_site=site;return false;};
    if(source.binding.role!=gears_weapon_shot::Role::shotgun_pellets||source.binding.bytes<2548)return reject(1);
    gears_ui_call_boundary::Frame caller{};
    if(!gears_ui_call_boundary::frame(weaponshot::read,call.caller_frame,caller)||
       caller.node!=source.binding.identities[0].address||caller.object!=source.sample.source.weapon.address||
       caller.previous!=source.call.caller_frame||caller.code<source.binding.script||
       caller.code-source.binding.script>=source.binding.bytes)return reject(2);
    code_offset=unsigned(caller.code-source.binding.script);
    if(code_offset!=2471&&code_offset!=2520)return reject(3);
    meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};
    r.reuse_initial_identities=true;
    unsigned calc{},kind{},owner{},package{},fn{},fn_owner{},fn_package{};meta::Structure body{};
    if(!meta::tables(base,schema.objects,schema.names)||!r.path(call.callee,calc)||!r.named(calc,L"CalcWeaponFire")||
       !r.path(schema.identities[calc].klass,kind)||!weaponhand::core_type(r,kind,L"Function")||
       !r.path(schema.identities[calc].outer,owner)||!r.named(owner,L"Weapon")||
       !r.path(schema.identities[owner].outer,package)||!r.named(package,L"Engine")||schema.identities[package].outer||
       !r.path(caller.node,fn)||!r.named(fn,L"FireAllPellets")||
       !r.path(schema.identities[fn].outer,fn_owner)||!r.named(fn_owner,L"Weap_Shotgun")||
       !r.path(schema.identities[fn_owner].outer,fn_package)||!r.named(fn_package,L"WarfareGame")||schema.identities[fn_package].outer||
       !weaponhand::object(r,source.binding.identities[0])||!weaponhand::object(r,source.binding.identities[1])||
       !weaponhand::object(r,source.binding.identities[2])||!r.structure(caller.node,body)||body.extent!=320)return reject(4);
    std::array<meta::Property,4> fields{};std::array<uptr,4> addresses{};
    std::array<uptr,64> seen{};unsigned visited{};
    for(auto address=body.children;address;){
        if(visited==seen.size())return reject(5);
        for(unsigned i=0;i<visited;++i)if(seen[i]==address)return reject(6);seen[visited++]=address;
        meta::Property p{};if(!r.property(address,p)||schema.identities[p.identity].outer!=caller.node)return reject(7);
        unsigned index=3;
        if(r.named(p.identity,L"StartLocation"))index=0;
        else if(r.named(p.identity,L"EndTrace"))index=1;
        else if(r.named(p.identity,L"PushedForwardStartLocation"))index=2;
        if(index<3){constexpr unsigned offsets[]{0,112,160};
            if(addresses[index]||p.offset!=offsets[index]||p.dimension!=1||p.element_bytes!=12||
               !meta::property_fits(p,320)||!f::core_struct(r,p,L"Vector"))return reject(8);
            fields[index]=p;addresses[index]=address;
        }
        address=p.next;
    }
    if(!addresses[0]||!addresses[1]||!addresses[2]||
       !r.at(source.binding.script,code_offset+19,addresses[3])||!r.property(addresses[3],fields[3])||
       schema.identities[fields[3].identity].outer!=caller.node||fields[3].dimension!=1||fields[3].element_bytes!=16||
       !weaponhand::core_type(r,fields[3].type,L"ArrayProperty")||!meta::property_fits(fields[3],320))return reject(9);
    for(unsigned i=0;i<3;++i)if(!(fields[3].offset+16<=fields[i].offset||fields[i].offset+12<=fields[3].offset))return reject(10);
    gears_gnasher_trace_input::Layout layout{caller.node,source.sample.source.weapon.address,source.call.caller_frame,
        source.binding.script,source.binding.bytes,body.extent,code_offset,schema.identities[calc].name.index,
        {addresses[2],addresses[1],addresses[3]}};
    if(!gears_gnasher_trace_input::direct(weaponshot::read,call.caller_frame,layout,out))return reject(11);
    r.reuse_initial_identities=false;
    for(unsigned i=0;i<fields.size();++i){meta::Property fresh{};
        if(!r.property(addresses[i],fresh)||!meta::same_property(fresh,fields[i]))return reject(12);}
    if(!f::retained_metadata(r))return reject(13);
    return true;
}
void begin(gears_weapon_call::Invocation const& call,gears_weapon_shot::Token const& token) noexcept {
    if(token.entered)return;
    auto* scope=gnasher_muzzle::active();if(!scope||!playground::named(call.callee,L"CalcWeaponFire"))return;
    gears_weapon_shot::Packet packet{};gears_weapon_call_trace::FireInput actual{};unsigned offset{},failure_site{};
    const auto rejected=[&](unsigned stage) noexcept {
        static unsigned rows{};if(!log_file||rows>=12)return;++rows;
        const auto& trace_source_frame=scope->source.sample.source;
        fwprintf(log_file,L"gnasher_trace_rejected stage=%u input_site=%u frame=%llu sequence=%llu sync_ns=%lld observed_ns=%lld code_offset=%u caller=%llx callee=%llx observation_only=1\n",
            stage,failure_site,trace_source_frame.key.frame,trace_source_frame.controllers.sequence,trace_source_frame.controllers.sync_ns,
            weaponhand::clock_ns(nullptr),offset,call.caller_frame,call.callee);fflush(log_file);
    };
    if(!gnasher_muzzle::packet(call,*scope,packet)){rejected(1);return;}
    if(!input(call,scope->source,actual,offset,failure_site)){rejected(2);return;}
    const auto& source=scope->source.sample;
    const auto epoch=f::control_epoch.load(std::memory_order_acquire);
    if(epoch!=source.epoch||!weaponshot::stack.current(packet)){rejected(3);return;}
    if(f::fire_trace.epoch!=epoch){f::fire_trace={};f::fire_trace.epoch=epoch;}
    if(f::fire_trace.count==(std::numeric_limits<std::uint64_t>::max)())return;
    const auto count=f::fire_trace.count;f::fire_trace={};
    auto& trace=f::fire_trace;trace.epoch=epoch;trace.count=count+1;trace.shot=packet.shot;
    trace.frame=source.source.key.frame;trace.input_sequence=source.source.controllers.sequence;
    trace.input_sync_ns=source.source.controllers.sync_ns;trace.observed_ns=weaponhand::clock_ns(nullptr);
    std::memcpy(trace.start,actual.start.data(),12);std::memcpy(trace.end,actual.end.data(),12);
    std::memcpy(trace.pushed,actual.pushed.data(),12);std::memcpy(trace.muzzle_origin,&source.origin,12);std::memcpy(trace.muzzle_aim,&source.aim,12);
    if(auto* slot=f::trajectory_slot(packet.shot,epoch)){
        if(slot->record.trace_valid&&(slot->record.trace.frame!=trace.frame||
           slot->record.trace.input_sequence!=trace.input_sequence||slot->record.trace.input_sync_ns!=trace.input_sync_ns))
            slot->record.trace_source_consistent=false;
        slot->record.trace=trace;std::memcpy(slot->record.writes_at_trace,slot->record.writes,sizeof(slot->record.writes));
        slot->record.trace_valid=true;++slot->record.revision;
    }
    static unsigned logs{};
    if(log_file&&logs<8){++logs;fwprintf(log_file,L"gnasher_actual_trace shot=%llu frame=%llu sequence=%llu code_offset=%u count=%llu start=%.9g,%.9g,%.9g end=%.9g,%.9g,%.9g pushed=%.9g,%.9g,%.9g observation_only=1\n",
        packet.shot,trace.frame,trace.input_sequence,offset,trace.count,double(actual.start[0]),double(actual.start[1]),double(actual.start[2]),
        double(actual.end[0]),double(actual.end[1]),double(actual.end[2]),double(actual.pushed[0]),double(actual.pushed[1]),double(actual.pushed[2]));fflush(log_file);}
}
}
