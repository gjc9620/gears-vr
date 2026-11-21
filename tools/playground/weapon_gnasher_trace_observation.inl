// Bounded observation through the EXISTING CallFunction subscriber. No new
// hook, game call, parameter/result write, input, or fire permission. The
// generic InstantFire trace reader is deliberately inapplicable to Gnasher.
namespace gnasher_trace_observation {
void begin(gears_weapon_call::Invocation const& call,gears_weapon_shot::Token const& token) noexcept {
    static unsigned rows{};
    if(rows>=2||token.entered||!log_file)return;
    auto* scope=gnasher_muzzle::active();
    if(!scope||!playground::named(call.callee,L"CalcWeaponFire"))return;
    ++rows;
    namespace meta=gripdamage_metadata;namespace frame=gears_ui_call_boundary;
    auto const& source=scope->source;
    gears_weapon_shot::Packet packet{};
    const auto reject=[&](unsigned stage)noexcept {
        fwprintf(log_file,L"gnasher_trace_observation rejected=%u role=%u shot=%llu callee=%llx caller=%llx frame=%llu sequence=%llu\n",
            stage,unsigned(source.binding.role),scope->admission.shot,call.callee,call.caller_frame,
            source.sample.source.key.frame,source.sample.source.controllers.sequence);fflush(log_file);
    };
    if(!gnasher_muzzle::packet(call,*scope,packet)){reject(1);return;}
    frame::Frame before{},after{};
    if(!frame::frame(weaponshot::read,call.caller_frame,before)||
       before.object!=source.sample.source.weapon.address){reject(2);return;}
    // An unexpected helper body is evidence to inspect, never a guessed local
    // layout. Only the exact currently authenticated Gnasher scope is read.
    if(before.node!=source.binding.identities[0].address||
       !gears_weapon_call_trace::script_pc(before.code,source.binding.script,source.binding.bytes)){
        fwprintf(log_file,L"gnasher_trace_observation rejected=3 role=%u shot=%llu actual_node=%llx expected_node=%llx code=%llx\n",
            unsigned(source.binding.role),packet.shot,before.node,source.binding.identities[0].address,before.code);fflush(log_file);return;
    }
    meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
    meta::Structure body{};uptr locals{},again{};unsigned calc{},kind{},owner{},package{};
    if(!meta::tables(base,schema.objects,schema.names)||!reader.path(call.callee,calc)||
       !reader.named(calc,L"CalcWeaponFire")||!reader.path(schema.identities[calc].klass,kind)||
       !weaponhand::core_type(reader,kind,L"Function")||!reader.path(schema.identities[calc].outer,owner)||
       !reader.named(owner,L"Weapon")||!reader.path(schema.identities[owner].outer,package)||
       !reader.named(package,L"Engine")||schema.identities[package].outer||!reader.structure(before.node,body)||
       !body.extent||body.extent>4096||!weaponshot::read(&locals,reinterpret_cast<void const*>(call.caller_frame+0x30),8)||
       !locals||(locals&3)||locals>UINTPTR_MAX-body.extent){reject(4);return;}
    struct VectorLocal {meta::Property field{};std::array<wchar_t,96> name{};std::array<float,3> value{};};
    std::array<VectorLocal,16> values{};unsigned count{},visited_count{};std::array<uptr,64> visited{};
    for(auto address=body.children;address;){
        if(visited_count==visited.size()){reject(5);return;}
        for(unsigned i=0;i<visited_count;++i)if(visited[i]==address){reject(5);return;}
        visited[visited_count++]=address;meta::Property field{};
        if(!reader.property(address,field)||schema.identities[field.identity].outer!=before.node){reject(6);return;}
        if(field.dimension==1&&field.element_bytes==12&&meta::property_fits(field,body.extent)&&
           weaponhand::core_type(reader,field.type,L"StructProperty")){
            if(unified_weapon_fire::core_struct(reader,field,L"Vector")){
                if(count==values.size()){reject(7);return;}
                auto& value=values[count++];value.field=field;
                value.name=schema.name_text[schema.identities[field.identity].name_slot].units;
                if(!weaponshot::read(value.value.data(),reinterpret_cast<void const*>(locals+field.offset),12)){reject(8);return;}
                for(float x:value.value)if(!std::isfinite(x)||std::abs(x)>1e8f){reject(8);return;}
            }else if(schema.failure.reason!=meta::Reason::none){reject(8);return;}
        }
        address=field.next;
    }
    const auto relative=unsigned(before.code-source.binding.script),start=relative>48?relative-48:0;
    const auto code_bytes=(std::min)(128u,source.binding.bytes-start);std::array<unsigned char,128> code{};
    if(!weaponshot::read(code.data(),reinterpret_cast<void const*>(source.binding.script+start),code_bytes)){reject(9);return;}
    for(unsigned i=0;i<count;++i){auto const& value=values[i];meta::Property fresh{};std::array<float,3> reread{};
        if(!reader.property(schema.identities[value.field.identity].address,fresh)||!meta::same_property(fresh,value.field)||
           !weaponshot::read(reread.data(),reinterpret_cast<void const*>(locals+value.field.offset),12)||reread!=value.value){reject(10);return;}}
    if(!unified_weapon_fire::retained_metadata(reader)||!frame::frame(weaponshot::read,call.caller_frame,after)||
       !weaponshot::read(&again,reinterpret_cast<void const*>(call.caller_frame+0x30),8)||again!=locals||
       before.node!=after.node||before.object!=after.object||before.code!=after.code||before.previous!=after.previous){reject(11);return;}
    fwprintf(log_file,L"gnasher_trace_observation rejected=0 role=%u shot=%llu callee=%llx function=%llx locals_size=%u code_offset=%u code_window_start=%u code_window_bytes=%u script_hash=%llu frame=%llu sequence=%llu vector_fields=%u observation_only=1\n",
        unsigned(source.binding.role),packet.shot,call.callee,before.node,body.extent,relative,start,code_bytes,source.binding.script_hash,
        source.sample.source.key.frame,source.sample.source.controllers.sequence,count);
    fwprintf(log_file,L"gnasher_trace_code shot=%llu hex=",packet.shot);
    for(unsigned i=0;i<code_bytes;++i)fwprintf(log_file,L"%02x",unsigned(code[i]));fwprintf(log_file,L"\n");
    for(unsigned i=0;i<count;++i){auto const& value=values[i];
        fwprintf(log_file,L"gnasher_trace_vector shot=%llu name=%ls property=%llx offset=%u value=%.9g,%.9g,%.9g\n",
            packet.shot,value.name.data(),schema.identities[value.field.identity].address,value.field.offset,
            double(value.value[0]),double(value.value[1]),double(value.value[2]));}
    fflush(log_file);
}
}
