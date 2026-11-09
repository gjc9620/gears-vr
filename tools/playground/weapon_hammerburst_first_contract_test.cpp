#include "../vr-common/weapon_hammerburst_first.h"
#include "../vr-common/weapon_hammerburst_property_cache.h"
#include "weapon_hand_offline_fixture.h"
#include <map>
namespace {
using Array=meta::Table;
std::map<std::wstring,uptr> virtual_functions;
namespace playground {
struct Args {
    struct {uptr function{};}r;bool valid{};
    Args(uptr,wchar_t const* name){auto i=virtual_functions.find(name);if(i!=virtual_functions.end()){r.function=i->second;valid=true;}}
};
}
namespace lancerrefire {
struct Script {Array array{};std::array<unsigned char,512> bytes{};unsigned bound{256};};
bool script(meta::Reader& r,uptr function,Script& out,unsigned bound=256){
    if(!bound||bound>out.bytes.size()||!r.at(function,0x8c,out.array)||!out.array.data||out.array.count<1||
       out.array.count>int(bound)||out.array.capacity<out.array.count||out.array.capacity>4096)return false;
    out.bound=bound;return copy(out.bytes.data(),out.array.data,std::size_t(out.array.count));
}
bool unchanged(meta::Reader& r,uptr function,Script const& before){Script after{};
    return script(r,function,after,before.bound)&&!std::memcmp(&before.array,&after.array,sizeof(Array))&&
        !std::memcmp(before.bytes.data(),after.bytes.data(),std::size_t(before.array.count));}
template<std::size_t N>bool weapon_function(meta::Reader& r,uptr function,wchar_t const(&label)[N],unsigned& slot,
                                          meta::Structure& structure,bool allow_war=false){
    unsigned kind{},owner{},owner_kind{},package{};
    return r.path(function,slot)&&r.named(slot,label)&&r.path(r.result.identities[slot].klass,kind)&&
        weaponhand::core_type(r,kind,L"Function")&&r.path(r.result.identities[slot].outer,owner)&&
        r.path(r.result.identities[owner].klass,owner_kind)&&weaponhand::core_type(r,owner_kind,L"Class")&&
        r.path(r.result.identities[owner].outer,package)&&!r.result.identities[package].outer&&
        ((r.named(owner,L"Weapon")&&r.named(package,L"Engine"))||
         (allow_war&&r.named(owner,L"WarWeapon")&&r.named(package,L"WarfareGame")))&&r.structure(function,structure);
}
namespace hammerburst_refire {
bool vm_shape(meta::Reader& r,uptr function,unsigned bytes,unsigned output){
    std::uint16_t parameters{},result{},native{};unsigned flags{};uptr exec{};
    return r.at(function,0x106,parameters)&&parameters==bytes&&r.at(function,0x108,result)&&result==output&&
        r.at(function,0xf8,native)&&!native&&r.at(function,0xf4,flags)&&!(flags&0x400)&&
        r.at(function,0x114,exec)&&exec==base+0x1d6ca0;
}
}
}
namespace hammerburst_burst_contract {
bool retain(meta::Reader& r,meta::Property const& p){if(r.result.field_count==meta::max_fields)return false;
    r.result.fields[r.result.field_count++]=p;return true;}
}
namespace unified_weapon_fire {
bool retained_metadata(meta::Reader& r){
    auto& schema=r.result;const auto ids=schema.identity_count,structures=schema.structure_count,fields=schema.field_count;
    for(unsigned i=0;i<ids;++i){unsigned slot{};if(!r.identity(schema.identities[i].address,slot)||slot!=i)return false;}
    for(unsigned i=0;i<structures;++i){meta::Structure s{};
        if(!r.structure(schema.identities[schema.structures[i].identity].address,s)||!meta::same_structure(s,schema.structures[i]))return false;}
    for(unsigned i=0;i<fields;++i){meta::Property p{};
        if(!r.property(schema.identities[schema.fields[i].identity].address,p)||!meta::same_property(p,schema.fields[i]))return false;}
    for(unsigned i=0;i<schema.name_count;++i){meta::NameText text{};
        if(!meta::fresh_name(schema.names,schema.name_text[i].index,text)||!meta::same_name_text(text,schema.name_text[i]))return false;}
    meta::Table objects{},names{};
    return base==schema.module_base&&meta::tables(base,objects,names)&&meta::same_table(objects,schema.objects)&&
        meta::same_table(names,schema.names)&&schema.failure.reason==meta::Reason::none;
}
}
#include "weapon_hammerburst_first_contract.inl"
struct FirstMemory:Memory {
    uptr function_type{},state_type{},name_type{},byte_type{},hb_state{},outer{},previous{},cue{};
    std::array<uptr,13> references{};std::array<unsigned char,271> code{};
    uptr function(wchar_t const* name,uptr owner){auto p=node(name,function_type,owner);put(p,0x88,8u);
        put(p,0x108,std::uint16_t(0xffff));put(p,0x114,base+0x1d6ca0);return p;}
    FirstMemory(){
        virtual_functions.clear();function_type=cls(L"Function",core);state_type=cls(L"State",core);
        name_type=cls(L"NameProperty",core);byte_type=cls(L"ByteProperty",core);
        const auto engine_pawn=cls(L"Pawn",engine,actor_class),engine_weapon=cls(L"Weapon",engine,actor_class);
        put(concrete,0x48,text(L"Weap_LocustAssaultRifle"));frame.weapon=identity(weapon);
        hb_state=node(L"WeaponFiring",state_type,concrete);outer=function(L"BeginState",hb_state);
        previous=property(outer,L"PreviousStateName",name_type,0,8);
        const auto engine_state=node(L"WeaponFiring",state_type,engine_weapon);
        references[0]=property(engine_pawn,L"Controller",object_type,0x110,8);
        references[1]=property(actor_class,L"Instigator",object_type,0x170,8);
        references[2]=function(L"BeginState",engine_state);
        references[3]=function(L"ClientPlayForceFeedbackWaveform",cls(L"PlayerController",engine));
        references[4]=function(L"IsLocallyControlled",engine_pawn);
        references[5]=property(pawn_class,L"bIsTargeting",bool_type,0x170,4,2);
        references[6]=pawn_class;
        references[7]=property(weapon_class,L"WeaponFireWaveForm",object_type,0x170,8);
        references[8]=pc_class;
        references[9]=property(concrete,L"bWasTargeting",bool_type,0x174,4,4);
        references[10]=property(concrete,L"BurstCount",byte_type,0x178,1);
        references[11]=previous;
        const auto audio=node(L"Weapon_AssaultRifle",package_type),group=node(L"Firing",package_type,audio);
        cue=node(L"LocustRifleFireCompositeCue",cls(L"SoundCue",engine),group);references[12]=cue;
        const wchar_t* function_labels[]={L"WeaponPlaySound",L"PlayPS5ControllerVibration",L"SetPS5TriggerFeedbackRight"};
        std::array<meta::RawName,3> virtual_names{};
        for(unsigned i=0;i<3;++i){const auto fn=function(function_labels[i],weapon_class);virtual_functions[function_labels[i]]=fn;
            virtual_names[i]=get<meta::RawName>(fn,0x48);}
        constexpr char serialized[]="142d013294000082772e1e0e000001c5f6ffff2a16182100192e1e0e000001c5f6ffff0a00042d01f7040000160f013594000024001b7a7f00000000000020115a01001e0000803f1607f90082772e596900001901c5f6ffff0900080184f6ffff2a161818001901c5f6ffff0a00041c81f9ffff161607ef00811b505b0000000000001616192e596900001901c5f6ffff0900080184f6ffff1300001c2ff9ffff01c9590000161b026a000000000000161c86f7ffff005794000016040b53";
        const std::array<unsigned,20> at{{3,10,15,26,31,40,47,63,79,85,93,104,112,135,141,149,157,162,178,183}};
        const std::array<unsigned,20> slots{{9,6,1,6,1,5,10,12,8,1,0,1,4,8,1,0,3,7,2,11}};
        const std::array<unsigned,3> name_at{{54,123,168}};
        const auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
        unsigned in{},out{},ref{},name{};
        while(in<191){
            if(ref<20&&in==at[ref]){std::memcpy(code.data()+out,&references[slots[ref]],8);out+=8;in+=4;++ref;}
            else if(name<3&&in==name_at[name]){std::memcpy(code.data()+out,&virtual_names[name],8);out+=8;in+=8;++name;}
            else{code[out++]=static_cast<unsigned char>(16*digit(serialized[2*in])+digit(serialized[2*in+1]));++in;}
        }
        assert(out==271&&ref==20&&name==3);
        put(outer,0x8c,Array{reinterpret_cast<uptr>(code.data()),271,271});tables();
    }
    bool prove(){weapon_metadata_parse::TickScope parsing;meta::Snapshot snapshot{};snapshot.module_base=base;
        meta::Reader reader{snapshot,meta::Stage::final_check};lancerrefire::Script body{};
        return meta::tables(base,snapshot.objects,snapshot.names)&&hammerburst_first_contract::prove(reader,weapon,outer,body);}
};
}
int main(){
    unsigned checks{};
    {auto m=std::make_unique<FirstMemory>();assert(m->prove());++checks;
        meta::Snapshot snapshot{};snapshot.module_base=base;meta::Reader r{snapshot,meta::Stage::final_check};
        lancerrefire::Script body{};assert(!lancerrefire::script(r,m->outer,body)); // Default remains256.
        assert(lancerrefire::script(r,m->outer,body,512)&&body.array.count==271);checks+=2;}
    const auto reject=[&](std::function<void(FirstMemory&)> change){auto m=std::make_unique<FirstMemory>();change(*m);
        assert(!m->prove());++checks;};
    for(unsigned index=0;index<13;++index){
        reject([=](auto& m){Memory::put(m.references[index],0x48,m.text(L"WrongOriginalReference"));m.tables();});
        reject([=](auto& m){Memory::put(m.references[index],0x40,m.core);});
        reject([=](auto& m){const auto old=Memory::get<uptr>(m.references[index],0x50);
            Memory::put(m.references[index],0x50,old==m.object_type?m.bool_type:m.object_type);});
    }
    reject([](auto& m){Memory::put(m.outer,0xf8,std::uint16_t(1));});
    reject([](auto& m){Memory::put(m.outer,0xf4,0x400u);});
    reject([](auto& m){Memory::put(m.outer,0x114,base+0x1d6ca1);});
    reject([](auto& m){Memory::put(m.outer,0x106,std::uint16_t(8));});
    reject([](auto& m){Memory::put(m.outer,0x108,std::uint16_t(0));});
    reject([](auto& m){Memory::put(m.outer,0x88,16u);});
    reject([](auto& m){Memory::put(m.previous,0x78,std::uint64_t(0x80));});
    reject([](auto& m){Memory::put(m.previous,0x94,4u);});
    reject([](auto& m){Memory::put(m.references[10],0x74,4u);});
    reject([](auto& m){Memory::put(m.references[9],0xd0,3u);});
    reject([](auto& m){m.code[0]^=1;});
    reject([](auto& m){Memory::put(m.outer,0x8c,Array{reinterpret_cast<uptr>(m.code.data()),270,271});});
    reject([](auto& m){fail_read=reinterpret_cast<uptr>(m.code.data());});
    reject([](auto& m){const auto fn=virtual_functions[L"WeaponPlaySound"];Memory::put(fn,0x40,m.pc_class);});
    reject([](auto&){virtual_functions.erase(L"PlayPS5ControllerVibration");});
    // Real Reader closure catches descriptor/name/mask changes after discovery.
    reject([](auto& m){mutate_read=m.references[9]+0xd0;mutation=[&m]{Memory::put(m.references[9],0xd0,8u);};});
    {auto m=std::make_unique<FirstMemory>();
        const auto native_id=[](meta::Reader& r,unsigned slot){auto const& id=r.result.identities[slot];
            return abi::ObjectIdentity{id.address,id.outer,id.klass,id.index,id.name.index,id.name.number,1};};
        meta::Snapshot a{};a.module_base=base;meta::Reader before{a,meta::Stage::final_check};
        assert(meta::tables(base,a.objects,a.names));unsigned unrelated{};assert(before.path(m->cue,unrelated));
        meta::Property saved{};assert(before.property(m->references[10],saved));
        const auto descriptor=native_id(before,saved.identity),type=native_id(before,saved.type);
        meta::Snapshot b{};b.module_base=base;meta::Reader after{b,meta::Stage::final_check};
        assert(meta::tables(base,b.objects,b.names));meta::Property current{};assert(after.property(m->references[10],current));
        assert(saved.identity!=current.identity&&!meta::same_property(saved,current));
        assert(gears_hammerburst_property_cache::same(after,current,saved,descriptor,type));
        auto stale=descriptor;++stale.registry_index;
        assert(!gears_hammerburst_property_cache::same(after,current,saved,stale,type));
        stale=type;++stale.registry_index;assert(!gears_hammerburst_property_cache::same(after,current,saved,descriptor,stale));
        auto changed=current;++changed.offset;assert(!gears_hammerburst_property_cache::same(after,changed,saved,descriptor,type));
        checks+=10;
    }
    std::printf("HB actual outer-first contract: %u real-Reader owned-memory path/layout/closure cases passed; virtual resolver and VM getter helpers are fixture adapters, no game proof\n",checks);
}
