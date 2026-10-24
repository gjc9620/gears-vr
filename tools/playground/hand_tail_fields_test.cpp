#include "hand_tail_compare_write.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <utility>
#include <vector>

namespace {
using uptr=std::uintptr_t;
struct Name {unsigned index{},number{};};
unsigned case_count{},check_count{},variant{};
void require(bool passed,const char* label){
    ++check_count;
    if(!passed){
        std::fprintf(stderr,"FAIL case=%u variant=%u %s\n",case_count,variant,label);
        std::exit(1);
    }
}
void begin_case(const char* label){
    ++case_count;variant=0;std::printf("CASE %u %s\n",case_count,label);
}

// Explicit memory-service seam, not the game's UObject registry/FName ABI.
// Each registered object is a fixed aligned block. Names and field resolution
// come from this finite catalogue; the actual reader parses all raw metadata.
constexpr unsigned node_count=32,node_bytes=1024;
enum Node:unsigned {package_node,meta_class,control_class,control_node,struct_type,byte_type,
    name_type,bool_type,float_type,enum_type,wrong_type,vector_type,vector_node,enum_node,
    second_enum,first_property,first_member=first_property+11};
struct alignas(16) Block {std::array<unsigned char,node_bytes> bytes{};};
struct Binding {uptr owner{};const wchar_t* label{};uptr property{};};
struct Memory {
    std::array<Block,node_count> blocks{};
    std::array<bool,node_count> alive{},deleted{};
    std::array<const wchar_t*,node_count> names{};
    std::vector<Binding> bindings;
    std::array<uptr,3> watched_strengths{};
    unsigned strength_reads{};
    uptr denied_copy{};
    uptr rebind_on_copy{},rebind_to{};unsigned rebind_index{};
    uptr address(unsigned node)const{return reinterpret_cast<uptr>(blocks[node].bytes.data());}
    template<class T>void store(unsigned node,std::size_t offset,T const& value){
        if(offset>node_bytes||sizeof(T)>node_bytes-offset)std::abort();
        std::memcpy(blocks[node].bytes.data()+offset,&value,sizeof(value));
    }
    template<class T>T load(unsigned node,std::size_t offset)const{
        if(offset>node_bytes||sizeof(T)>node_bytes-offset)std::abort();
        T result{};std::memcpy(&result,blocks[node].bytes.data()+offset,sizeof(result));return result;
    }
};
Memory* current_memory{};
bool registered(uptr object){
    if(!current_memory)return false;
    for(unsigned node=0;node<node_count;++node)
        if(current_memory->address(node)==object)return current_memory->alive[node];
    return false;
}
bool copy(void* destination,uptr source,std::size_t bytes){
    if(!source)return false;
    if(current_memory){
        for(const auto strength_address:current_memory->watched_strengths)
            if(source==strength_address)++current_memory->strength_reads;
        if(source==current_memory->denied_copy)return false;
        if(source==current_memory->rebind_on_copy){
            current_memory->rebind_on_copy=0;
            current_memory->bindings[current_memory->rebind_index].property=current_memory->rebind_to;
        }
    }
    __try{std::memcpy(destination,reinterpret_cast<void const*>(source),bytes);return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
namespace playground {
uptr field(uptr owner,const wchar_t* label){
    if(!current_memory||!registered(owner))return 0;
    for(auto const& binding:current_memory->bindings)
        if(binding.owner==owner&&!_wcsicmp(binding.label,label))return binding.property;
    return 0;
}
// Explicit catalogue resolution seam, as with field above. The actual engine
// first-match batch implementation has its own old/new resolver fixture.
template<std::size_t N>
bool fields(uptr owner,const wchar_t* const (&labels)[N],std::array<uptr,N>& out){
    out={};std::array<uptr,N> values{};
    for(std::size_t i=0;i<N;++i)if(!(values[i]=field(owner,labels[i])))return false;
    out=values;return true;
}
bool named(uptr object,const wchar_t* label){
    if(!registered(object))return false;
    for(unsigned node=0;node<node_count;++node)
        if(current_memory->address(node)==object)
            return current_memory->names[node]&&!_wcsicmp(current_memory->names[node],label);
    return false;
}
bool deleted(uptr object){
    if(!registered(object))return true;
    for(unsigned node=0;node<node_count;++node)
        if(current_memory->address(node)==object)return current_memory->deleted[node];
    return true;
}
// Deletion is another explicit catalogue service here. The actual bounded
// live search-path implementation is exercised by its independent fixture.
struct TailDeletionLookup {bool alive(uptr object){return !deleted(object);}};
} // namespace playground

// No rewritten/extracted observer or compare-write implementation.
#include "vr_hand_tail_translation.inl"

namespace tail=gears_hand_tail_translation;
using Field=tail::Field;
using FieldValue=tail::FieldValue;
using Observed=handtail::Observed;
constexpr std::uint32_t apply_bit=0x00000200u,add_bit=0x01000000u;
constexpr std::uint32_t rotation_apply=0x00010000u,rotation_add=0x00040000u;
constexpr unsigned translation_offset=336,space_offset=375,name_offset=392,flags_offset=416;
constexpr std::array<unsigned,3> strength_offsets{{480,492,508}};
constexpr std::array<unsigned,11> offsets{{translation_offset,space_offset,name_offset,flags_offset,
    flags_offset,379,flags_offset,flags_offset,480,492,508}};

struct Fixture {
    Memory memory;
    Fixture(){reset();}
    void reset(){
        memory=Memory{};current_memory=&memory;
        for(unsigned node=0;node<node_count;++node){
            memory.alive[node]=true;memory.names[node]=L"Spare";
            memory.store(node,0,uptr(0x11110000u+node*16u));
            memory.store(node,8,node+600u);
            memory.store(node,0x40,memory.address(package_node));
            memory.store(node,0x48,Name{node+1000u,0u});
            memory.store(node,0x50,memory.address(meta_class));
        }
        memory.names[package_node]=L"TestPackage";memory.names[meta_class]=L"Class";
        memory.names[control_class]=L"SkelControlSingleBone";
        memory.names[control_node]=L"TailControl";
        memory.store(control_node,0x50,memory.address(control_class));
        memory.store(control_class,0x88,node_bytes);
        memory.names[struct_type]=L"StructProperty";memory.names[byte_type]=L"ByteProperty";
        memory.names[name_type]=L"NameProperty";memory.names[bool_type]=L"BoolProperty";
        memory.names[float_type]=L"FloatProperty";memory.names[enum_type]=L"Enum";
        memory.names[wrong_type]=L"OtherType";memory.names[vector_type]=L"ScriptStruct";
        memory.names[vector_node]=L"Vector";memory.store(vector_node,0x50,memory.address(vector_type));
        memory.store(vector_node,0x88,12u);
        for(const auto node:{unsigned(enum_node),unsigned(second_enum)}){
            memory.names[node]=L"EBoneControlSpace";memory.store(node,0x50,memory.address(enum_type));
        }
        constexpr const wchar_t* labels[]={L"BoneTranslation",L"BoneTranslationSpace",
            L"TranslationSpaceBoneName",L"bApplyTranslation",L"bAddTranslation",L"BoneRotationSpace",
            L"bApplyRotation",L"bAddRotation",L"ControlStrength",L"StrengthTarget",L"BlendTimeToGo"};
        constexpr unsigned types[]={struct_type,byte_type,name_type,bool_type,bool_type,byte_type,
            bool_type,bool_type,float_type,float_type,float_type};
        constexpr unsigned sizes[]={12,1,8,4,4,1,4,4,4,4,4};
        for(unsigned index=0;index<11;++index){
            const unsigned node=first_property+index;
            memory.names[node]=labels[index];memory.store(node,0x40,memory.address(control_class));
            memory.store(node,0x50,memory.address(types[index]));
            memory.store(node,0x70,1u);memory.store(node,0x74,sizes[index]);
            memory.store(node,0x94,offsets[index]);
            memory.bindings.push_back({memory.address(control_class),labels[index],memory.address(node)});
        }
        memory.store(first_property,0xd0,memory.address(vector_node));
        for(const auto index:{1u,5u})memory.store(first_property+index,0xd0,memory.address(enum_node));
        memory.store(first_property+3u,0xd0,apply_bit);memory.store(first_property+4u,0xd0,add_bit);
        memory.store(first_property+6u,0xd0,rotation_apply);memory.store(first_property+7u,0xd0,rotation_add);
        constexpr const wchar_t* components[]={L"X",L"Y",L"Z"};
        for(unsigned index=0;index<3;++index){
            const unsigned node=first_member+index;
            memory.names[node]=components[index];memory.store(node,0x40,memory.address(vector_node));
            memory.store(node,0x50,memory.address(float_type));memory.store(node,0x70,1u);
            memory.store(node,0x74,4u);memory.store(node,0x94,index*4u);
            memory.bindings.push_back({memory.address(vector_node),components[index],memory.address(node)});
            memory.watched_strengths[index]=memory.address(control_node)+strength_offsets[index];
        }
        for(unsigned index=0;index<12;++index)
            memory.blocks[control_node].bytes[translation_offset+index]=static_cast<unsigned char>(11u+index);
        memory.store(control_node,space_offset,std::uint8_t{0x5a});
        memory.store(control_node,name_offset,std::uint64_t{0x8877665544332211ull});
        memory.store(control_node,flags_offset,std::uint32_t{0x80000004u|rotation_apply|apply_bit});
    }
    uptr control()const{return memory.address(control_node);}
    void property_meta(unsigned index,std::size_t offset,unsigned value){
        memory.store(first_property+index,offset,value);
    }
    void strength_bits(unsigned index,std::uint32_t bits){memory.store(control_node,strength_offsets[index],bits);}
    auto snapshot()const{return memory.blocks[control_node].bytes;}
    Observed read(){Observed result{};require(handtail::observe(control(),result),"valid fixture observed");return result;}
};
bool write_field(Observed const& source,Field field,FieldValue const& expected,FieldValue const& wanted){
    return gears_hand_tail_fields::compare_write(source.addresses,source.strength_addresses,
        source.apply_mask,source.add_mask,field,expected,wanted);
}
tail::Value desired_value(){
    tail::Value value{};
    for(unsigned index=0;index<value.translation.size();++index)
        value.translation[index]=static_cast<unsigned char>(81u+index);
    for(unsigned index=0;index<value.space_bone.size();++index)
        value.space_bone[index]=static_cast<unsigned char>(151u+index);
    value.space=0x71;value.flags=add_bit;return value;
}
void require_rejected(Fixture& fixture,const char* label){
    Observed result{};result.schema.push_back(0xff);result.addresses.fill(1);
    result.strength_addresses.fill(1);result.raw_flags=1;result.apply_mask=1;result.add_mask=2;
    result.value=desired_value();
    require(!handtail::observe(fixture.control(),result),label);
    require(result.schema.empty()&&result.addresses==std::array<uptr,4>{}&&
        result.strength_addresses==std::array<uptr,3>{}&&!result.raw_flags&&!result.apply_mask&&
        !result.add_mask&&tail::exact(result.value,tail::Value{}),"failure publishes no partial/stale result");
}

void valid_observation(){
    begin_case("dynamic layout and nonhistorical masks");Fixture fixture;const auto sample=fixture.read();
    require(sample.addresses==std::array<uptr,4>{{fixture.control()+translation_offset,
        fixture.control()+space_offset,fixture.control()+name_offset,fixture.control()+flags_offset}},
        "four live addresses use metadata offsets");
    require(sample.strength_addresses==fixture.memory.watched_strengths,"three live strength addresses");
    require(sample.apply_mask==apply_bit&&sample.add_mask==add_bit,"live masks are not1/2");
    require(sample.value.flags==apply_bit&&sample.raw_flags==(0x80000004u|rotation_apply|apply_bit),
        "masked value and complete packed word remain distinct");
    require(sample.value.space==0x5a&&sample.value.translation[0]==11u&&
        sample.value.translation[11]==22u&&sample.value.space_bone[0]==0x11u&&
        sample.value.space_bone[7]==0x88u,"raw translation/space/Name bytes preserved without geometry qualification");
    require(!sample.schema.empty()&&fixture.memory.bindings.size()==14,"eleven properties and three Vector members");
    require(fixture.memory.strength_reads==0,"observer never reads strength values");
}
void fresh_resolution_rejections(){
    begin_case("fresh control and Vector field resolution rejects changed first match");Fixture fixture;
    for(const auto index:{0u,11u,13u}){
        variant=index;fixture.reset();const auto original=fixture.snapshot();
        fixture.memory.rebind_index=index;
        fixture.memory.rebind_on_copy=fixture.control()+translation_offset;
        fixture.memory.rebind_to=index==11u?0:fixture.memory.address(wrong_type);
        require_rejected(fixture,"post-descriptor first-match address changed or disappeared");
        require(fixture.snapshot()==original,"read-only rejection changes no control bytes");
    }
}
void schema_values(){
    begin_case("schema comparison ignores values but not masks/addresses");Fixture fixture;const auto before=fixture.read();
    fixture.memory.blocks[control_node].bytes[translation_offset]^=0x40u;
    fixture.memory.store(control_node,space_offset,std::uint8_t{0x13});
    fixture.memory.store(control_node,name_offset,std::uint64_t{0x1234ull});
    fixture.memory.store(control_node,flags_offset,std::uint32_t{0x40000000u|add_bit});
    const auto after=fixture.read();
    require(handtail::same_schema(before,after)&&!tail::exact(before.value,after.value),"changed values do not change schema");
    auto changed=after;changed.apply_mask^=0x10u;require(!handtail::same_schema(before,changed),"mask comparison");
    changed=after;++changed.addresses[0];require(!handtail::same_schema(before,changed),"field address comparison");
    changed=after;++changed.strength_addresses[2];require(!handtail::same_schema(before,changed),"strength address comparison");
    require(!handtail::same_schema(Observed{},Observed{}),"empty schema is not a matching observation");
}
void schema_identity_changes(){
    begin_case("valid metadata identity and strength relocation change schema");Fixture fixture;const auto before=fixture.read();
    fixture.memory.store(first_property+8u,0,uptr{0x77770000u});
    const auto identity_changed=fixture.read();
    require(!handtail::same_schema(before,identity_changed),"strength property vtable identity frozen");
    fixture.reset();fixture.memory.store(first_member+1u,0x48,Name{1027u,9u});
    const auto member_changed=fixture.read();
    require(!handtail::same_schema(before,member_changed),"Vector member full Name identity frozen");
    fixture.reset();fixture.property_meta(9u,0x94,540u);const auto relocated=fixture.read();
    require(relocated.strength_addresses[1]==fixture.control()+540u&&
        !handtail::same_schema(before,relocated),"valid strength offset relocation detected");
    fixture.reset();
    for(const auto index:{1u,5u})fixture.memory.store(first_property+index,0xd0,fixture.memory.address(second_enum));
    const auto enum_changed=fixture.read();
    require(!handtail::same_schema(before,enum_changed),"same named valid replacement Enum identity detected");
}
void enum_rejections(){
    begin_case("Enum type identity and reference relation");Fixture fixture;
    for(unsigned mode=0;mode<5;++mode){variant=mode;fixture.reset();
        if(mode==0)fixture.memory.store(enum_node,0x50,fixture.memory.address(wrong_type));
        if(mode==1)fixture.memory.store(first_property+5u,0xd0,fixture.memory.address(second_enum));
        if(mode==2)fixture.memory.names[enum_node]=L"OtherEnum";
        if(mode==3)fixture.memory.alive[enum_node]=false;
        if(mode==4)fixture.memory.store(first_property+1u,0xd0,uptr{});
        require_rejected(fixture,"bad Enum metadata rejected");
    }
}
void vector_rejections(){
    begin_case("Vector target and real member descriptors");Fixture fixture;
    for(unsigned mode=0;mode<8;++mode){variant=mode;fixture.reset();
        if(mode==0)fixture.memory.store(first_member,0x50,fixture.memory.address(byte_type));
        if(mode==1)fixture.memory.store(first_member+1u,0x70,2u);
        if(mode==2)fixture.memory.store(first_member+1u,0x74,8u);
        if(mode==3)fixture.memory.store(first_member+2u,0x94,6u);
        if(mode==4)fixture.memory.store(vector_node,0x88,16u);
        if(mode==5)fixture.memory.store(first_property,0xd0,uptr{});
        if(mode==6)fixture.memory.store(first_member,0x40,fixture.memory.address(control_class));
        if(mode==7)fixture.memory.names[vector_node]=L"Rotator";
        require_rejected(fixture,"bad Vector/member metadata rejected");
    }
}
void bool_rejections(){
    begin_case("Bool single bits same word and rotation separation");Fixture fixture;
    for(unsigned mode=0;mode<8;++mode){variant=mode;fixture.reset();
        if(mode==0)fixture.property_meta(3u,0xd0,0u);
        if(mode==1)fixture.property_meta(4u,0xd0,3u);
        if(mode==2)fixture.property_meta(4u,0xd0,apply_bit);
        if(mode==3)fixture.property_meta(6u,0xd0,apply_bit);
        if(mode==4)fixture.property_meta(7u,0xd0,add_bit);
        if(mode==5)fixture.property_meta(4u,0x94,560u);
        if(mode==6)fixture.property_meta(7u,0xd0,rotation_apply);
        if(mode==7)fixture.property_meta(6u,0xd0,0u);
        require_rejected(fixture,"bad Bool layout/mask rejected");
    }
}
void bounds_and_overlap(){
    begin_case("field bounds and illegal storage overlap");Fixture fixture;
    for(unsigned mode=0;mode<7;++mode){variant=mode;fixture.reset();
        if(mode==0)fixture.property_meta(0u,0x94,node_bytes-8u);
        if(mode==1)fixture.property_meta(2u,0x94,translation_offset+4u);
        if(mode==2)fixture.property_meta(5u,0x94,space_offset);
        if(mode==3)fixture.property_meta(6u,0x94,flags_offset+1u);
        if(mode==4)fixture.property_meta(8u,0x94,flags_offset);
        if(mode==5)fixture.property_meta(10u,0x94,strength_offsets[1]);
        if(mode==6)fixture.property_meta(1u,0x94,translation_offset+11u);
        require_rejected(fixture,"out of bounds or illegal overlap rejected");
    }
}
void strength_metadata_and_values(){
    begin_case("strength descriptors are typed but strength values are not sampled");Fixture fixture;
    for(unsigned mode=0;mode<5;++mode){variant=mode;fixture.reset();
        if(mode==0)fixture.memory.store(first_property+8u,0x50,fixture.memory.address(byte_type));
        if(mode==1)fixture.property_meta(9u,0x70,2u);
        if(mode==2)fixture.property_meta(10u,0x74,8u);
        if(mode==3)fixture.property_meta(8u,0x94,node_bytes-2u);
        if(mode==4)fixture.memory.alive[first_property+9u]=false;
        require_rejected(fixture,"bad strength descriptor rejected");
    }
    fixture.reset();fixture.strength_bits(1u,0x3f800000u);const auto sample=fixture.read();
    require(fixture.memory.strength_reads==0,"nonzero strength is not read by observer");
    const auto snapshot=fixture.snapshot();const auto wanted=desired_value();
    require(!write_field(sample,Field::translation,tail::field_value(sample.value,Field::translation),
        tail::field_value(wanted,Field::translation)),"actual helper freshly rejects observed nonzero strength");
    require(fixture.snapshot()==snapshot,"nonzero strength refusal does not mutate control");
}
void rejection_publication(){
    begin_case("wrong class size unknown and late read failure publish nothing");Fixture fixture;
    for(unsigned mode=0;mode<7;++mode){variant=mode;fixture.reset();
        if(mode==0)fixture.memory.names[control_class]=L"SkelControlOther";
        if(mode==1)fixture.memory.store(control_class,0x88,4097u);
        if(mode==2)fixture.memory.alive[control_node]=false;
        if(mode==3)fixture.memory.deleted[first_property+2u]=true;
        if(mode==4)fixture.memory.denied_copy=fixture.control()+name_offset;
        if(mode==5)fixture.memory.store(control_node,0x50,uptr{});
        if(mode==6)fixture.memory.store(first_property+1u,0x70,2u);
        require_rejected(fixture,"unknown/schema rejection clears prior output");
    }
}
void four_field_writes(){
    begin_case("actual helper writes each selected field only");Fixture fixture;auto sample=fixture.read();
    auto target=sample.value;const auto desired=desired_value();
    for(unsigned index=0;index<4;++index){variant=index;const auto field=static_cast<Field>(index);
        const auto before=fixture.snapshot();const auto raw_before=sample.raw_flags;
        const auto wanted=tail::field_value(desired,field);
        require(write_field(sample,field,tail::field_value(sample.value,field),wanted),"fresh +0 expected write succeeds");
        tail::assign_field(target,field,wanted);const auto after=fixture.read();
        require(tail::exact(after.value,target)&&handtail::same_schema(sample,after),"selected value written under same schema");
        const auto start=static_cast<std::size_t>(sample.addresses[index]-fixture.control());
        auto bytes_expected=before;
        if(field==Field::flags){const auto combined=(raw_before&~(apply_bit|add_bit))|desired.flags;
            std::memcpy(bytes_expected.data()+start,&combined,4);
        }else std::memcpy(bytes_expected.data()+start,wanted.data(),tail::field_size(field));
        require(fixture.snapshot()==bytes_expected,"no bytes outside selected field changed");sample=after;
    }
}
void fresh_zero_rejections(){
    begin_case("fresh three-strength exact +0 rejects -0 NaN nonzero");Fixture fixture;const auto sample=fixture.read();
    constexpr std::array<std::uint32_t,3> bad_bits{{0x80000000u,0x7fc01234u,0x3f800000u}};
    const auto wanted=tail::field_value(desired_value(),Field::translation);
    for(unsigned index=0;index<3;++index)for(unsigned mode=0;mode<bad_bits.size();++mode){
        variant=index*3u+mode;fixture.strength_bits(index,bad_bits[mode]);const auto before=fixture.snapshot();
        require(!write_field(sample,Field::translation,tail::field_value(sample.value,Field::translation),wanted),
            "late unhealthy strength rejects cached observation");
        require(fixture.snapshot()==before,"strength rejection leaves all control bytes unchanged");
        fixture.strength_bits(index,0u);
    }
}
void expected_rejections(){
    begin_case("fresh expected-byte mismatch in all four fields");Fixture fixture;const auto desired=desired_value();
    for(unsigned index=0;index<4;++index){variant=index;fixture.reset();const auto sample=fixture.read();
        const auto field=static_cast<Field>(index);
        const auto offset=static_cast<std::size_t>(sample.addresses[index]-fixture.control());
        if(field==Field::flags)fixture.memory.store(control_node,flags_offset,sample.raw_flags^apply_bit);
        else fixture.memory.blocks[control_node].bytes[offset]^=0x40u;
        const auto before=fixture.snapshot();
        require(!write_field(sample,field,tail::field_value(sample.value,field),tail::field_value(desired,field)),
            "fresh mismatch rejects stale expected bytes");
        require(fixture.snapshot()==before,"expected mismatch performs no mutation");
    }
}
void packed_live_bits(){
    begin_case("packed write preserves newly changed unrelated bits");Fixture fixture;const auto sample=fixture.read();
    const auto live=(sample.raw_flags^rotation_apply)|rotation_add|0x40u;
    fixture.memory.store(control_node,flags_offset,live);
    require(write_field(sample,Field::flags,tail::field_value(sample.value,Field::flags),
        tail::field_value(desired_value(),Field::flags)),"old unrelated snapshot bits do not veto masked compare");
    require(fixture.memory.load<std::uint32_t>(control_node,flags_offset)==
        ((live&~(apply_bit|add_bit))|add_bit),"LIVE unrelated and rotation bits preserved");
}
void helper_shape_rejections(){
    begin_case("malformed masks outside bits and invalid field reject");Fixture fixture;const auto sample=fixture.read();
    const auto before=fixture.snapshot();const auto old_flags=tail::field_value(sample.value,Field::flags);
    const auto wanted=tail::field_value(desired_value(),Field::flags);
    constexpr std::array<std::array<std::uint32_t,2>,5> masks{{{{0u,add_bit}},{{apply_bit,0u}},
        {{3u,add_bit}},{{apply_bit,3u}},{{apply_bit,apply_bit}}}};
    for(unsigned index=0;index<masks.size();++index){variant=index;
        require(!gears_hand_tail_fields::compare_write(sample.addresses,sample.strength_addresses,
            masks[index][0],masks[index][1],Field::flags,old_flags,wanted),"invalid masks rejected");
        require(fixture.snapshot()==before,"malformed mask does not write");
    }
    auto outside=old_flags;outside[0]|=0x01u;
    require(!write_field(sample,Field::flags,outside,wanted),"outside expected bit rejected");
    outside=wanted;outside[0]|=0x01u;
    require(!write_field(sample,Field::flags,old_flags,outside),"outside wanted bit rejected");
    require(!write_field(sample,Field::count,old_flags,wanted)&&
        !write_field(sample,static_cast<Field>(99u),old_flags,wanted),"invalid field selectors rejected");
    require(fixture.snapshot()==before,"all shape failures leave control unchanged");
}
struct NoAccessPage {
    void* address{VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS)};
    NoAccessPage(){require(address!=nullptr,"allocate this-process no-access test page");}
    ~NoAccessPage(){if(address)VirtualFree(address,0,MEM_RELEASE);}
    NoAccessPage(NoAccessPage const&)=delete;NoAccessPage& operator=(NoAccessPage const&)=delete;
};
void inaccessible_rejections(){
    begin_case("actual SEH boundary rejects inaccessible selected field or strength");Fixture fixture;
    const auto sample=fixture.read();const auto before=fixture.snapshot();const auto desired=desired_value();
    NoAccessPage page;
    for(unsigned index=0;index<4;++index){variant=index;const auto field=static_cast<Field>(index);
        auto addresses=sample.addresses;addresses[index]=reinterpret_cast<uptr>(page.address);
        require(!gears_hand_tail_fields::compare_write(addresses,sample.strength_addresses,apply_bit,add_bit,
            field,tail::field_value(sample.value,field),tail::field_value(desired,field)),"PAGE_NOACCESS field rejected by real SEH");
        require(fixture.snapshot()==before,"inaccessible field changes no control bytes");
        addresses[index]=0;
        require(!gears_hand_tail_fields::compare_write(addresses,sample.strength_addresses,apply_bit,add_bit,
            field,tail::field_value(sample.value,field),tail::field_value(desired,field)),"null selected field rejected");
    }
    for(unsigned index=0;index<3;++index){variant=4u+index;auto strengths=sample.strength_addresses;
        strengths[index]=reinterpret_cast<uptr>(page.address);
        require(!gears_hand_tail_fields::compare_write(sample.addresses,strengths,apply_bit,add_bit,Field::translation,
            tail::field_value(sample.value,Field::translation),tail::field_value(desired,Field::translation)),
            "PAGE_NOACCESS strength rejected by real SEH");
        require(fixture.snapshot()==before,"inaccessible strength changes no control bytes");strengths[index]=0;
        require(!gears_hand_tail_fields::compare_write(sample.addresses,strengths,apply_bit,add_bit,Field::translation,
            tail::field_value(sample.value,Field::translation),tail::field_value(desired,Field::translation)),
            "null strength rejected");
    }
    require(fixture.snapshot()==before,"all inaccessible/null cases preserve complete control bytes");
}
} // namespace
int main(){
    valid_observation();fresh_resolution_rejections();schema_values();schema_identity_changes();enum_rejections();vector_rejections();
    bool_rejections();bounds_and_overlap();strength_metadata_and_values();rejection_publication();
    four_field_writes();fresh_zero_rejections();expected_rejections();packed_live_bits();
    helper_shape_rejections();inaccessible_rejections();
    std::printf("passed cases=%u checks=%u\n",case_count,check_count);return 0;
}
