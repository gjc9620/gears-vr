#define GEARS_PLAYGROUND_NATIVE_MAIN_MOUNT_PROVIDER_TRIAL 1
#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#include "weapon_metadata_parse.h"
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
    const auto entry=m.names[index];const unsigned units=unsigned(m.labels[index-1000].size()+1);unsigned calls{};
    std::function<void()> step;step=[&]{++calls;if(calls<units){mutate_read=entry+20+2*calls;mutation=step;}};
    mutate_read=entry+20;mutation=step;CHECK(meta::fresh_name(name_table(m),index,text));
    mutation={};mutate_read=0;return calls;
}
void names(){
    {auto m=std::make_unique<Memory>();const auto index=Memory::get<meta::RawName>(m->pawn,0x48).index;
        meta::NameText a{},b{};Query q;const auto miss=character_calls(*m,index,a),hit=character_calls(*m,index,b);
        CHECK(miss==a.length+1&&hit==1&&meta::same_name_text(a,b));
        std::printf("Actual fresh_name character calls: miss=%u hit=%u; all characters still fresh, no timing claim\n",miss,hit);
        {Query scalar(false);CHECK(character_calls(*m,index,b)==miss&&meta::same_name_text(a,b));}
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
}
int main(){walks();scanners();names();CHECK(parse::active==nullptr);
    std::printf("WeaponMetadataParseTest: %u checks passed; actual scanners/name reader, owned memory only\n",checks);return 0;}
