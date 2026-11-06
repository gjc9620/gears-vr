#define GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL 1
#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#include "weapon_metadata_parse.h"
#include "../native-capture/weapon_hud_query_input.h"
#include "../vr-common/weapon_main_mount_provider.h"
#include "weapon_hand_offline_fixture.h"
#include <algorithm>
#include <vector>
namespace {
namespace parse=weapon_metadata_parse;
unsigned checks{};
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}} while(false)
struct Query {
    std::unique_ptr<parse::Workspace> workspace=std::make_unique<parse::Workspace>();
    parse::Workspace* previous{};
    explicit Query(bool fast=true){if(fast)previous=parse::activate(*workspace);else {previous=parse::active;parse::active=nullptr;}}
    ~Query(){parse::restore(previous);}
};
void walks(){
    std::vector<uptr> previous(4096);std::uint64_t scalar_comparisons{};
    {Query q(false);parse::Walk walk;for(unsigned i=0;i<4096;++i){const uptr p=8ull*(i+1);
        CHECK(!walk.seen(p,previous.data(),i));previous[i]=p;}
        CHECK(walk.seen(previous[4095],previous.data(),4096));scalar_comparisons=walk.comparisons();
        CHECK(scalar_comparisons==std::uint64_t(4096)*4095/2+4096&&walk.probes()==0);}
    {Query q;parse::Walk walk;for(unsigned i=0;i<4096;++i){const uptr p=8ull*(i+1);
        CHECK(!walk.seen(p,previous.data(),i));previous[i]=p;}
        CHECK(walk.seen(previous[4095],previous.data(),4096));CHECK(walk.comparisons()==0&&walk.probes()<scalar_comparisons);
        const auto epoch=q.workspace->epoch;{parse::Walk inner;CHECK(inner.seen(previous[2],previous.data(),4096));
            CHECK(inner.probes()==0&&inner.comparisons()==3&&q.workspace->epoch==epoch);}
        CHECK(q.workspace->walk_owned&&walk.seen(previous[0],previous.data(),4096));
        std::printf("Walk membership only: linear comparisons=%llu hashed probes=%llu, no timing claim\n",scalar_comparisons,walk.probes());}
    {Query q;uptr seen[3]{};std::vector<uptr> collision;
        const auto bucket=parse::bucket(8,parse::visit_buckets-1);
        for(uptr p=8;collision.size()<3;p+=8)if(parse::bucket(p,parse::visit_buckets-1)==bucket)collision.push_back(p);
        {parse::Walk walk;for(unsigned i=0;i<3;++i){CHECK(!walk.seen(collision[i],seen,i));seen[i]=collision[i];}
            CHECK(walk.seen(collision[1],seen,3)&&walk.probes()>=8);}
        CHECK(!q.workspace->walk_owned);{parse::Walk next;CHECK(!next.seen(collision[0],seen,0));}
        q.workspace->epoch=~parse::U32(0);{parse::Walk wrapped;CHECK(!wrapped.seen(collision[0],seen,0)&&q.workspace->epoch==1);}}
}
struct Prefix {
    std::vector<Block> nodes;std::vector<uptr> registry;
    Prefix(Memory& m,unsigned count,bool cycle=false):nodes(count),registry(1001+m.node_count+count){
        CHECK(count>0);std::copy(m.objects.begin(),m.objects.begin()+1001+m.node_count,registry.begin());
        const auto label=m.text(L"IgnoredParserField");m.tables();const auto original=Memory::get<uptr>(m.pawn_class,0x80);
        for(unsigned i=0;i<count;++i){const auto p=reinterpret_cast<uptr>(nodes[i].bytes.data());const auto index=1001+m.node_count+i;
            Memory::put(p,8,index);Memory::put(p,0x40,m.pawn_class);Memory::put(p,0x48,label);Memory::put(p,0x50,m.object_type);
            auto next=i+1<count?reinterpret_cast<uptr>(nodes[i+1].bytes.data()):original;
            if(cycle&&i+1==count)next=reinterpret_cast<uptr>(nodes[0].bytes.data());
            Memory::put(p,0x68,next);registry[index]=p;}
        Memory::put(m.pawn_class,0x80,reinterpret_cast<uptr>(nodes[0].bytes.data()));
        Memory::put(base,0x1c24730,meta::Table{reinterpret_cast<uptr>(registry.data()),int(registry.size()),int(registry.size())});
    }
};
struct Selection {bool accepted{};uptr property{};unsigned offset{};uptr value{};};
Selection select(Memory& m,unsigned scanner,bool fast){
    Query q(fast);meta::Snapshot schema{};meta::Reader reader{schema,meta::Stage::final_check};
    CHECK(meta::tables(base,schema.objects,schema.names));Selection result{};
    if(scanner==0){weaponhand::BoolField field{};result.accepted=weaponhand::field(reader,m.pawn,L"Controller",L"ObjectProperty",8,field);
        if(result.accepted){result.property=schema.identities[field.property.identity].address;result.offset=field.property.offset;}}
    else {meta::PointerField field{};
        if(scanner==1){weaponhand::PawnObserveFields batch;
            result.accepted=batch.begin(reader,m.pawn)&&batch.pointer(reader,weaponhand::PawnObserveFields::controller_link,m.pc,field)&&batch.finish(reader);}
        else result.accepted=reader.pointer(m.pawn,L"Controller",field);
        if(result.accepted){result.property=schema.identities[field.property.identity].address;result.offset=field.property.offset;result.value=field.value;}}
    return result;
}
void differential(Memory& m,unsigned scanner,bool accepted){
    const auto slow=select(m,scanner,false),fast=select(m,scanner,true);
    CHECK(slow.accepted==accepted&&fast.accepted==accepted);
    CHECK(slow.property==fast.property&&slow.offset==fast.offset&&slow.value==fast.value);
}
void scanners(){
    for(unsigned scanner=0;scanner<3;++scanner){
        {auto m=std::make_unique<Memory>();differential(*m,scanner,true);
            const auto original=Memory::get<uptr>(m->pawn_class,0x80);
            const auto first=m->property(m->pawn_class,L"Controller",m->bool_type,0x110,8);
            Memory::put(first,0x68,original);Memory::put(m->pawn_class,0x80,first);m->tables();
            differential(*m,scanner,false);}
        {auto m=std::make_unique<Memory>();Prefix prefix(*m,3,true);differential(*m,scanner,false);}
        const unsigned allowed=scanner==1?4091u:4095u;
        for(unsigned extra:{allowed,allowed+1}){auto m=std::make_unique<Memory>();Prefix prefix(*m,extra);
            differential(*m,scanner,extra==allowed);}
    }
}
meta::Table name_table(Memory& m){return {reinterpret_cast<uptr>(m.names.data()),int(1000+m.name_count),int(m.names.size())};}
unsigned character_calls(Memory& m,unsigned index,meta::NameText& text){
    const auto entry=m.names[index];unsigned calls{};
    read_observer=[&](uptr from,std::size_t){if(from>=entry+20&&from<entry+20+2*meta::max_name_units)++calls;};
    CHECK(meta::fresh_name(name_table(m),index,text));read_observer={};return calls;
}
void names(){
    {auto m=std::make_unique<Memory>();const auto index=Memory::get<meta::RawName>(m->pawn,0x48).index;
        meta::NameText a{},b{};Query q;const auto miss=character_calls(*m,index,a),hit=character_calls(*m,index,b);
        CHECK(miss==a.length+1&&hit==1&&meta::same_name_text(a,b));
        std::printf("Actual fresh_name character calls: miss=%u hit=%u; all characters still fresh, no timing claim\n",miss,hit);
        {Query scalar(false);CHECK(character_calls(*m,index,b)==a.length+1&&meta::same_name_text(a,b));}
        CHECK(character_calls(*m,index,b)==1);}
    for(unsigned change=0;change<7;++change){auto m=std::make_unique<Memory>();Query q;
        const auto index=Memory::get<meta::RawName>(m->pawn,0x48).index;const auto entry=m->names[index];meta::NameText a{},b{};
        CHECK(meta::fresh_name(name_table(*m),index,a));
        switch(change){
        case 0:Memory::put(entry,0,index+1);break;
        case 1:Memory::put(entry,20,wchar_t(0));break; // Earlier NUL.
        case 2:Memory::put(entry,20+2*a.length,wchar_t(L'X'));break; // Former NUL removed.
        case 3:fail_read=entry+20;break;
        case 4:fail_read=reinterpret_cast<uptr>(m->names.data())+index*8;break;
        case 5:fail_read=entry;break;
        case 6:Memory::put(entry,20,wchar_t(L'X'));break; // Same-length text must be fresh.
        }
        const auto good=meta::fresh_name(name_table(*m),index,b);
        CHECK(change==6?good&&!meta::same_name_text(a,b):!good);
    }
    {auto m=std::make_unique<Memory>();Query q;meta::Snapshot schema{};meta::Reader r{schema,meta::Stage::final_check};
        CHECK(meta::tables(base,schema.objects,schema.names));unsigned slot{};CHECK(r.identity(m->pawn,slot));
        const auto name=Memory::get<meta::RawName>(m->pawn,0x48);Memory::put(m->names[name.index],20,wchar_t(L'X'));
        CHECK(!r.identity(m->pawn,slot));} // Existing Reader witness rejects fresh changed text.
    {auto m=std::make_unique<Memory>();Query q;const auto index=Memory::get<meta::RawName>(m->pawn,0x48).index;
        meta::NameText a{},b{};CHECK(meta::fresh_name(name_table(*m),index,a));Block replacement{};
        const auto entry=reinterpret_cast<uptr>(replacement.bytes.data());Memory::put(entry,0,index);
        const wchar_t text[]=L"ReplacementName";std::memcpy(reinterpret_cast<void*>(entry+20),text,sizeof(text));m->names[index]=entry;
        CHECK(meta::fresh_name(name_table(*m),index,b)&&b.entry==entry&&meta::exact_text(b,L"ReplacementName"));}
    for(unsigned size:{0u,95u,96u}){auto m=std::make_unique<Memory>();const auto name=m->text(std::wstring(size,L'A'));m->tables();Query q;
        meta::NameText text{};const auto good=meta::fresh_name(name_table(*m),name.index,text);
        CHECK(good==(size==95));if(good){CHECK(text.length==95&&meta::fresh_name(name_table(*m),name.index,text));}}
    {auto m=std::make_unique<Memory>();const auto index=Memory::get<meta::RawName>(m->pawn,0x48).index;
        const auto entry=m->names[index],target=parse::name_bucket(entry,index);Query q;unsigned inserted{};
        for(uptr fake=8;inserted<parse::name_probes;fake+=8)if(fake!=entry&&parse::name_bucket(fake,index)==target){
            parse::remember_length(fake,index,3);CHECK(parse::length_bound(fake,index)==3);++inserted;}
        CHECK(parse::length_bound(entry,index)==0);meta::NameText text{};
        CHECK(character_calls(*m,index,text)==text.length+1&&parse::length_bound(entry,index)==0);
        CHECK(character_calls(*m,index,text)==text.length+1);}
    {auto m=std::make_unique<Memory>();const auto index=Memory::get<meta::RawName>(m->pawn,0x48).index;const auto entry=m->names[index];
        {Query q;meta::NameText text{};CHECK(meta::fresh_name(name_table(*m),index,text));CHECK(parse::length_bound(entry,index)>0);}
        Query next;CHECK(parse::length_bound(entry,index)==0);}
}
void cold_name_boundaries(){
    SYSTEM_INFO info{};GetSystemInfo(&info);const auto page=std::size_t(info.dwPageSize);
    auto memory=static_cast<unsigned char*>(VirtualAlloc(nullptr,page*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    CHECK(memory&&page>=4096);DWORD old{};CHECK(VirtualProtect(memory+page,page,PAGE_NOACCESS,&old));
    for(unsigned length:{0u,1u,15u,16u,17u,31u,32u,95u}){
        auto m=std::make_unique<Memory>();const auto name=m->text(std::wstring(length,L'Q'));m->tables();
        // Last NUL is the final readable wchar. Both the scalar cold scan and
        // retained length-bound read must accept it; removing it must reject.
        const auto bytes=20+(length+1)*sizeof(wchar_t);
        const auto entry=reinterpret_cast<uptr>(memory+page-bytes);
        Memory::put(entry,0,name.index);
        std::memcpy(reinterpret_cast<void*>(entry+20),m->labels[name.index-1000].c_str(),(length+1)*sizeof(wchar_t));
        m->names[name.index]=entry;
        meta::NameText scalar{},batch{};
        {Query q(false);CHECK(meta::fresh_name(name_table(*m),name.index,scalar)==(length!=0));}
        {Query q;CHECK(meta::fresh_name(name_table(*m),name.index,batch)==(length!=0));
            CHECK(meta::same_name_text(scalar,batch));
            if(length)CHECK(meta::fresh_name(name_table(*m),name.index,batch));}
        Memory::put(entry,20+2*length,wchar_t(L'Q'));
        {Query q(false);CHECK(!meta::fresh_name(name_table(*m),name.index,scalar));}
        {Query q;CHECK(!meta::fresh_name(name_table(*m),name.index,batch));}
    }
    CHECK(VirtualFree(memory,0,MEM_RELEASE));
    auto m=std::make_unique<Memory>();const auto name=m->text(std::wstring(95,L'Q'));m->tables();
    meta::NameText scalar{},batch{};unsigned scalar_calls{},cold_calls{},bound_calls{};
    {Query q(false);scalar_calls=character_calls(*m,name.index,scalar);}
    {Query q;cold_calls=character_calls(*m,name.index,batch);bound_calls=character_calls(*m,name.index,batch);}
    CHECK(meta::same_name_text(scalar,batch)&&scalar_calls==96&&cold_calls==96&&bound_calls==1);
    std::printf("95-character name: scalar=%u cold=%u current length-bound=%u reads; no live FPS claim\n",scalar_calls,cold_calls,bound_calls);
}

}
void tick_scope_lifetime(){
    CHECK(parse::active==nullptr);
    {
        parse::TickScope tick;auto* owner=parse::active;CHECK(owner);
        parse::remember_length(0x1000,7,4);
        {parse::TickScope nested;CHECK(parse::active==owner&&parse::length_bound(0x1000,7)==4);}
        CHECK(parse::active==owner);
        {Query provider;CHECK(parse::active!=owner);parse::TickScope borrowed;CHECK(parse::active!=owner);}
        CHECK(parse::active==owner&&parse::length_bound(0x1000,7)==4);
    }
    CHECK(parse::active==nullptr);
    try{parse::TickScope next;CHECK(parse::length_bound(0x1000,7)==0);throw 1;}catch(int){}
    CHECK(parse::active==nullptr);
}
void hud_direct_arguments(){
    namespace hud=gears_weapon_hud_query;
    std::array<unsigned char,29> bytes{};
    const std::uint32_t name=12027;const std::uintptr_t start=0x1230,end=0x4560;
    bytes[0]=0x1b;std::memcpy(bytes.data()+1,&name,4);
    std::memcpy(bytes.data()+10,&start,8);std::memcpy(bytes.data()+19,&end,8);
    bytes[27]=0x50;bytes[28]=0x16;
    const auto script=reinterpret_cast<std::uintptr_t>(bytes.data());
    hud::Layout layout{};layout.code=script+9;
    auto read=[&](void* out,void const* address,std::size_t size){
        const auto p=reinterpret_cast<std::uintptr_t>(address);
        if(p<script||p-script>bytes.size()||size>bytes.size()-(p-script))return false;
        std::memcpy(out,address,size);return true;
    };
    auto accepted=[&](unsigned size=29){return hud::direct_arguments(read,layout,script,size,name,start,end);};
    CHECK(accepted());CHECK(!accepted(28));
    bytes[27]=0x4a;CHECK(!accepted());bytes[27]=0x50;
    bytes[28]=0x15;CHECK(!accepted());bytes[28]=0x16;
    bytes[10]^=1;CHECK(!accepted());bytes[10]^=1;
    bytes[1]^=1;CHECK(!accepted());bytes[1]^=1;
    bytes[27]=0x16;CHECK(accepted(28));
}
void query_local_identities(){
    {
        auto m=std::make_unique<Memory>();meta::Snapshot schema{};
        meta::Reader r{schema,meta::Stage::final_check};
        CHECK(!r.reuse_initial_identities&&meta::tables(base,schema.objects,schema.names));
        unsigned reads{};
        read_observer=[&](uptr address,std::size_t bytes){if(address==m->weapon+0x40&&bytes==24)++reads;};
        r.reuse_initial_identities=true;meta::U32 first{};CHECK(r.identity(m->weapon,first));
        for(unsigned i=0;i<32;++i){meta::U32 slot{};CHECK(r.identity(m->weapon,slot)&&slot==first);}
        CHECK(reads==1);
        auto name=Memory::get<meta::RawName>(m->weapon,0x48);++name.number;Memory::put(m->weapon,0x48,name);
        meta::U32 slot{};CHECK(r.identity(m->weapon,slot)); // Still discovery, no result authority.
        r.reuse_initial_identities=false;CHECK(!r.identity(m->weapon,slot)&&reads==2);
        read_observer={};
    }
    // Mutate only when the world is freshly read a SECOND time, at the
    // closing pass. A different object's old discovery identity must never
    // satisfy publication after that mutation, in either observer route.
    for(bool source_only:{false,true})for(unsigned field=0;field<4;++field){
        auto m=std::make_unique<Memory>();unsigned world_reads{};bool changed{};
        read_observer=[&](uptr address,std::size_t bytes){
            if(address!=m->world+0x40||bytes!=24||++world_reads!=2)return;
            changed=true;
            if(field==0)Memory::put(m->weapon,0x50,m->pawn_class);
            if(field==1)Memory::put(m->weapon,0x40,m->core);
            if(field==2){auto name=Memory::get<meta::RawName>(m->weapon,0x48);++name.number;Memory::put(m->weapon,0x48,name);}
            if(field==3)m->objects[Memory::get<unsigned>(m->weapon,8)]=m->pawn;
        };
        abi::GripGameState out{};CHECK(!m->read(out,source_only)&&!out.known&&changed&&world_reads>=2);
        read_observer={};
    }
    // An ordinary Reader has no discovery reuse enabled, including on its
    // second access. Do not alter other metadata consumers' contracts.
    auto m=std::make_unique<Memory>();meta::Snapshot schema{};meta::Reader r{schema,meta::Stage::final_check};
    CHECK(meta::tables(base,schema.objects,schema.names));meta::U32 slot{};CHECK(r.identity(m->weapon,slot));
    Memory::put(m->weapon,0x50,m->pawn_class);CHECK(!r.identity(m->weapon,slot));
}
void contiguous_metadata_fields(){
    // Faults at ANY byte of the existing contiguous fields must reject both
    // complete and source-only observations, not just the first byte of a copy.
    for(bool source_only:{false,true})for(unsigned region=0;region<2;++region)
    for(unsigned byte=0;byte<24;++byte){
        auto m=std::make_unique<Memory>();
        fail_read=(region?m->switch_property+0x68:m->weapon+0x40)+byte;
        abi::GripGameState out{};CHECK(!m->read(out,source_only)&&!out.known);
    }
    // A coherent prefix read does not license retaining it through the query:
    // the ORIGINAL closing identity/property rereads must find these changes.
    for(bool source_only:{false,true})for(unsigned field=0;field<7;++field){
        auto m=std::make_unique<Memory>();
        const unsigned offsets[]={0x40,0x48,0x50,0x68,0x70,0x74,0x78};
        const auto object=field<3?m->weapon:m->switch_property;
        mutate_read=object+offsets[field]+1;
        mutation=[&]{
            if(field==0)Memory::put(m->weapon,0x40,m->core);
            if(field==1){auto name=Memory::get<meta::RawName>(m->weapon,0x48);++name.number;Memory::put(m->weapon,0x48,name);}
            if(field==2)Memory::put(m->weapon,0x50,m->pawn_class);
            if(field==3)Memory::put(m->switch_property,0x68,uptr(0));
            if(field==4)Memory::put(m->switch_property,0x70,2u);
            if(field==5)Memory::put(m->switch_property,0x74,8u);
            if(field==6)Memory::put(m->switch_property,0x78,std::uint64_t(0x80));
        };
        abi::GripGameState out{};CHECK(!m->read(out,source_only)&&!out.known);
    }
    fail_read=mutate_read=0;mutation={};
    auto allocation=static_cast<unsigned char*>(VirtualAlloc(nullptr,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    CHECK(allocation);DWORD prior{};CHECK(VirtualProtect(allocation+4096,4096,PAGE_NOACCESS,&prior));
    const meta::PropertyPrefix source{0x12345678,1,4,0x1122334455667788};
    auto address=reinterpret_cast<uptr>(allocation+4096-sizeof(source));
    std::memcpy(reinterpret_cast<void*>(address),&source,sizeof(source));
    meta::PropertyPrefix actual{};CHECK(meta::read_at(address-0x68,0x68,actual));
    CHECK(actual.next==source.next&&actual.dimension==source.dimension&&actual.element_bytes==source.element_bytes&&actual.flags==source.flags);
    CHECK(!meta::read_at(address+1-0x68,0x68,actual)); // No overread may cross the inaccessible page.
    meta::IdentityHeader header{};CHECK(meta::read_at(address-0x40,0x40,header));
    CHECK(std::memcmp(&header,&source,24)==0);CHECK(!meta::read_at(address+1-0x40,0x40,header));
    CHECK(VirtualFree(allocation,0,MEM_RELEASE));
}
void aimed_fields(){
    using F=weaponhand::AimObserveFields;
    for(unsigned scenario=0;scenario<6;++scenario){
        auto m=std::make_unique<Memory>();
        const auto held=m->property(m->pc_class,L"bHoldingButton_R2",m->bool_type,0x180,4,1);
        m->property(m->pc_class,L"bPressingTargetingKey",m->bool_type,0x184,4,2);
        m->property(m->pc_class,L"bToggleTargeting",m->bool_type,0x188,4,4);m->tables();
        if(scenario==1)Memory::put(held,0x50,m->object_type);
        if(scenario==2)Memory::put(held,0x74,8u);
        if(scenario==3)Memory::put(held,0x40,m->pawn_class);
        meta::Snapshot schema{};meta::Reader r{schema,meta::Stage::final_check};
        CHECK(meta::tables(base,schema.objects,schema.names));F fields;
        if(scenario>=1&&scenario<=3){CHECK(!fields.begin(r,m->pc));continue;}
        CHECK(fields.begin(r,m->pc));weaponhand::BoolField value{};
        CHECK(fields.boolean(r,F::held_flag,value)&&value.mask==1);
        if(scenario==4){Memory::put(held,0xd0,3u);CHECK(!fields.boolean(r,F::held_flag,value));continue;}
        if(scenario==5){Memory::put(held,0x94,0x190u);CHECK(!fields.boolean(r,F::held_flag,value));continue;}
        Memory::put(m->pc,0x180,1u);CHECK(fields.boolean(r,F::held_flag,value)&&(value.bits&value.mask));
        CHECK(fields.boolean(r,F::target_flag,value)&&value.mask==2);
        CHECK(fields.boolean(r,F::toggle_flag,value)&&value.mask==4);
        CHECK(fields.finish(r));CHECK(!fields.boolean(r,F::held_flag,value));
    }
}int main(){query_local_identities();contiguous_metadata_fields();aimed_fields();hud_direct_arguments();tick_scope_lifetime();walks();scanners();names();cold_name_boundaries();CHECK(parse::active==nullptr);
    std::printf("WeaponMetadataParseTest: %u checks passed; actual scanners/name reader, owned memory only\n",checks);return 0;}
