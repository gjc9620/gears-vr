#include "../vr-common/weapon_hammerburst_first.h"
// Complete original HB OUTER BeginState. No assumption that a VM super call
// crosses ProcessEvent, and no getter or live FFrame is used during capture.
namespace hammerburst_first_contract {
namespace meta=gripdamage_metadata;
unsigned stage{};
unsigned path_node{},path_gate{},observed_extent{};
std::uint16_t observed_parameters{},observed_result{};
struct Node {wchar_t const* name;wchar_t const* type;wchar_t const* package;};
bool label(meta::Reader& r,unsigned slot,wchar_t const* name) noexcept {
    if(slot>=r.result.identity_count)return false;auto const& id=r.result.identities[slot];
    if(id.name.number||id.name_slot>=r.result.name_count)return false;
    const auto n=std::wcslen(name);auto const& text=r.result.name_text[id.name_slot];
    return n<meta::max_name_units&&text.length==n&&!std::wmemcmp(text.units.data(),name,n+1);
}
bool type(meta::Reader& r,unsigned slot,wchar_t const* name,wchar_t const* package) noexcept {
    unsigned kind{},outer{};
    path_gate=4;if(!r.path(r.result.identities[slot].klass,kind))return false;
    path_gate=5;if(!label(r,kind,name))return false;
    path_gate=6;if(!r.path(r.result.identities[kind].outer,outer))return false;
    path_gate=7;if(!label(r,outer,package))return false;
    path_gate=8;return !r.result.identities[outer].outer;
}
bool path(meta::Reader& r,uptr address,std::initializer_list<Node> nodes,unsigned* first=nullptr) noexcept {
    bool initial=true;path_node=0;
    for(auto const& node:nodes){unsigned slot{};
        path_gate=1;if(!r.path(address,slot))return false;
        path_gate=2;if(!label(r,slot,node.name))return false;
        path_gate=3;if(!type(r,slot,node.type,node.package))return false;
        if(initial&&first)*first=slot;initial=false;address=r.result.identities[slot].outer;
        ++path_node;
    }
    path_gate=9;return !address;
}
bool reference(meta::Reader& r,uptr address,unsigned index,unsigned& slot) noexcept {
    const Node engine{L"Engine",L"Package",L"Core"},warfare{L"WarfareGame",L"Package",L"Core"};
    const Node pawn{L"Pawn",L"Class",L"Core"},actor{L"Actor",L"Class",L"Core"},
        weapon{L"Weapon",L"Class",L"Core"},warweapon{L"WarWeapon",L"Class",L"Core"},
        hb{L"Weap_LocustAssaultRifle",L"Class",L"Core"},state{L"WeaponFiring",L"State",L"Core"};
    switch(index){
    case 0:return path(r,address,{{L"Controller",L"ObjectProperty",L"Core"},pawn,engine},&slot);
    case 1:return path(r,address,{{L"Instigator",L"ObjectProperty",L"Core"},actor,engine},&slot);
    case 2:return path(r,address,{{L"BeginState",L"Function",L"Core"},state,weapon,engine},&slot);
    case 3:return path(r,address,{{L"ClientPlayForceFeedbackWaveform",L"Function",L"Core"},
        {L"PlayerController",L"Class",L"Core"},engine},&slot);
    case 4:return path(r,address,{{L"IsLocallyControlled",L"Function",L"Core"},pawn,engine},&slot);
    case 5:return path(r,address,{{L"bIsTargeting",L"BoolProperty",L"Core"},{L"WarPawn",L"Class",L"Core"},warfare},&slot);
    case 6:return path(r,address,{{L"WarPawn",L"Class",L"Core"},warfare},&slot);
    case 7:return path(r,address,{{L"WeaponFireWaveForm",L"ObjectProperty",L"Core"},warweapon,warfare},&slot);
    case 8:return path(r,address,{{L"WarPC",L"Class",L"Core"},warfare},&slot);
    case 9:return path(r,address,{{L"bWasTargeting",L"BoolProperty",L"Core"},hb,warfare},&slot);
    case 10:return path(r,address,{{L"BurstCount",L"ByteProperty",L"Core"},hb,warfare},&slot);
    case 11:return path(r,address,{{L"PreviousStateName",L"NameProperty",L"Core"},
        {L"BeginState",L"Function",L"Core"},state,hb,warfare},&slot);
    case 12:return path(r,address,{{L"LocustRifleFireCompositeCue",L"SoundCue",L"Engine"},
        {L"Firing",L"Package",L"Core"},{L"Weapon_AssaultRifle",L"Package",L"Core"}},&slot);
    default:return false;
    }
}
bool prove(meta::Reader& r,uptr weapon,uptr function,lancerrefire::Script& body) noexcept {
    stage=1;path_node=path_gate=observed_extent=0;observed_parameters=observed_result=0;
    unsigned fn{},owner{};meta::Structure structure{};std::uint16_t parameters{},result{};
    // Actual65044 stopped in the former combined stage1. Preserve the exact
    // predicates/read order, but identify its first failing boundary. These
    // stages grant no eligibility and add no game-memory observations.
    if(!path(r,function,{{L"BeginState",L"Function",L"Core"},{L"WeaponFiring",L"State",L"Core"},
         {L"Weap_LocustAssaultRifle",L"Class",L"Core"},{L"WarfareGame",L"Package",L"Core"}},&fn))return false;
    stage=2;if(!r.structure(function,structure))return false;
    observed_extent=structure.extent;
    stage=3;if(!r.at(function,0x106,parameters))return false;
    observed_parameters=parameters;if(parameters!=8)return false;
    stage=4;if(!r.at(function,0x108,result))return false;
    observed_result=result;if(result!=0xffff)return false;
    stage=5;if(structure.extent!=8)return false;
    stage=6;if(!lancerrefire::hammerburst_refire::vm_shape(r,function,8,0xffff))return false;
    stage=7;if(!lancerrefire::script(r,function,body,512))return false;
    stage=8;if(body.array.count!=271)return false;
    gears_hammerburst_first::Bindings bindings{};
    std::array<unsigned,13> slots{};std::array<bool,13> seen{};
    for(unsigned i=0;i<gears_hammerburst_first::runtime_objects.size();++i){
        stage=10+i;
        uptr address{};std::memcpy(&address,body.bytes.data()+gears_hammerburst_first::runtime_objects[i],8);
        const auto index=gears_hammerburst_first::object_slots[i];
        if(seen[index]){if(bindings.objects[index]!=address)return false;continue;}
        if(!reference(r,address,index,slots[index]))return false;
        bindings.objects[index]=address;seen[index]=true;
    }
    std::array<uptr,2> bools{};std::array<unsigned,2> masks{};unsigned bool_count{};
    for(unsigned index:{0u,1u,5u,7u,9u,10u,11u}){
        stage=40+index;
        meta::Property p{};meta::Structure owned{};const auto address=bindings.objects[index];
        const unsigned bytes=index==10?1u:index==5||index==9?4u:8u;
        if(!r.property(address,p)||p.dimension!=1||p.element_bytes!=bytes||
           !r.path(r.result.identities[p.identity].outer,owner)||!r.structure(r.result.identities[owner].address,owned)||
           !meta::property_fits(p,owned.extent)||!hammerburst_burst_contract::retain(r,p))return false;
        // Original export37975 is CPF_Parm0x80, despite the old decompiler's
        // misleading local declaration. Actual25860 also reports ParmsSize8.
        if(index==11&&(p.offset||(p.flags&0x580)!=0x80||p.next||structure.children!=address||
                       !meta::property_fits(p,parameters)))return false;
        if(index==5||index==9){
            unsigned mask{};if(!r.at(address,0xd0,mask)||!mask||(mask&(mask-1)))return false;
            bools[bool_count]=address;masks[bool_count++]=mask;
        }
    }
    playground::Args sound(weapon,L"WeaponPlaySound"),vibration(weapon,L"PlayPS5ControllerVibration"),
        feedback(weapon,L"SetPS5TriggerFeedbackRight");
    unsigned sound_slot{},vibration_slot{},feedback_slot{};meta::Structure virtual_structure{};
    stage=60;
    if(!sound.valid||!vibration.valid||!feedback.valid||
       !lancerrefire::weapon_function(r,sound.r.function,L"WeaponPlaySound",sound_slot,virtual_structure,true)||
       !lancerrefire::weapon_function(r,vibration.r.function,L"PlayPS5ControllerVibration",vibration_slot,virtual_structure,true)||
       !lancerrefire::weapon_function(r,feedback.r.function,L"SetPS5TriggerFeedbackRight",feedback_slot,virtual_structure,true))return false;
    const unsigned function_slots[]={sound_slot,vibration_slot,feedback_slot};
    for(unsigned i=0;i<3;++i){const auto name=r.result.identities[function_slots[i]].name;
        bindings.names[i]={name.index,name.number};}
    stage=70;
    if(!gears_hammerburst_first::entry(body.bytes.data(),std::size_t(body.array.count),bindings))return false;
    for(unsigned i=0;i<bool_count;++i){unsigned mask{};if(!r.at(bools[i],0xd0,mask)||mask!=masks[i])return false;}
    stage=80;
    if(!unified_weapon_fire::retained_metadata(r)||!lancerrefire::unchanged(r,function,body)||
       !lancerrefire::hammerburst_refire::vm_shape(r,function,8,0xffff))return false;
    stage=0;return true;
}
}
