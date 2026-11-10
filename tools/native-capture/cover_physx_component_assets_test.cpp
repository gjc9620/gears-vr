#define GEARS_COMPONENTS_PHYSX_TEST_MAIN PriorComponentActorsMain
#include "cover_physx_components_test.cpp"
#undef GEARS_COMPONENTS_PHYSX_TEST_MAIN
static Memory asset_fixture(p::Source& fields){
    auto m=parts_fixture(fields);m.block(0x190000,0x280);m.block(0x300000,0x80);m.block(0x310000,32);m.block(0x320000,0x98);
    fields.components[0].skeletal=true;auto& asset=fields.assets[0];asset.sampled=true;asset.asset=0x110000;asset.instance=0x300000;
    asset.owner=0x1e0000;asset.offsets={0x60,0x6c,0x7c};asset.array={0x310000,4,4};asset.root=-1;fields.asset_body_count=4;
    fields.attachments[0]={true,0x330000,0x260,56,0,0,{}};
    fields.asset_bodies[0]={0x1a0000,0x190000,0x1b0000,1,-1,0x160000,0x220000};
    fields.asset_bodies[1]={0x320000,0x190000,0x1b0000,1,-7,0x1f0000,0};fields.asset_bodies[3]=fields.asset_bodies[1];
    m.put<std::uintptr_t>(0x190248,asset.asset);m.put<std::uintptr_t>(0x190250,asset.instance);m.put<std::uintptr_t>(0x300060,asset.owner);
    m.put(0x30006c,asset.array);m.put(0x30007c,asset.root);
    for(unsigned n=0;n<4;++n)m.put<std::uintptr_t>(0x310000+n*8,fields.asset_bodies[n].body);
    m.put<std::uintptr_t>(0x320088,0x1f0000);return m;
}
#ifndef GEARS_COMPONENT_ASSETS_PHYSX_TEST_MAIN
#define GEARS_COMPONENT_ASSETS_PHYSX_TEST_MAIN main
#endif
int GEARS_COMPONENT_ASSETS_PHYSX_TEST_MAIN(){
    auto fields=std::make_unique<p::Source>();auto memory=asset_fixture(*fields);auto original=std::make_unique<p::Source>(*fields);const auto initial=memory;
    auto read=[&](void* dst,void const* at,std::size_t bytes){return memory.read(dst,at,bytes);};s::Snapshot scope{};
    check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"root source");
    auto work=std::make_unique<p::Workspace>();auto output=std::make_unique<p::Snapshot>();output->actor_count=99;auto saved=std::make_unique<p::Snapshot>(*output);
    unsigned guards{};memory.calls=0;
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards},*work,*output)==p::Code::observed,"optional PAA actors qualified with component bodies");
    const auto reads=memory.calls,guard_count=guards;
    check(output->actor_count==2&&output->actors[0]==0x160000&&output->actors[1]==0x1f0000&&
        output->asset_observations[2].presence==p::Presence::body_field_null&&output->asset_observations[3].presence==p::Presence::actor_observed&&
        output->fields.assets[0].owner==0x1e0000&&output->fields.asset_bodies[1].index==-7,"shared bodies dedup actors only; null rows and external owner remain");
    for(unsigned n=1;n<=reads;++n){*output=*saved;memory.calls=0;memory.fail_at=n;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)!=p::Code::observed&&p::same(*output,*saved),"every optional source read failure rejects whole packet");}
    memory.fail_at=0;
    for(unsigned n=1;n<=guard_count;++n){*output=*saved;guards=0;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards,n},*work,*output)==p::Code::guard&&p::same(*output,*saved),"every typed owner boundary rejects whole packet");}
    for(unsigned bad=0;bad<9;++bad){memory=initial;*fields=*original;*output=*saved;
        if(bad==0)memory.put<std::uintptr_t>(0x1f0000,base+0x2330b0);
        if(bad==1)memory.put<std::uintptr_t>(0x1f0038,0x180000);
        if(bad==2)memory.put<std::uintptr_t>(0x210010,0x160000);
        if(bad==3)memory.put<std::uintptr_t>(0x310008,0x1a0000);
        if(bad==4)memory.put<std::uintptr_t>(0x300060,0x1b0000);
        if(bad==5)memory.put<std::uintptr_t>(0x190248,0);
        if(bad==6)memory.put<std::int32_t>(0x30007c,1);
        if(bad==7)memory.put<std::uintptr_t>(0x320090,0x220000);
        if(bad==8)memory.put(0x30006c,s::pose::ArrayHeader{0x310000,3,4});
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)!=p::Code::observed&&p::same(*output,*saved),"PAA table Scene reverse membership owner asset root or body change rejects");}
    for(unsigned bad=0;bad<13;++bad){memory=initial;*fields=*original;*output=*saved;
        if(bad==0)fields->assets[0].sampled=false;
        if(bad==1)fields->assets[0].first_body=1;
        if(bad==2)fields->asset_body_count=257;
        if(bad==3)fields->asset_body_count=3;
        if(bad==4)fields->asset_bodies[4]=fields->asset_bodies[0];
        if(bad==5)fields->asset_bodies[2].data=0x160000;
        if(bad==6)fields->asset_bodies[0].owner_roots=0;
        if(bad==7)fields->assets[0].offsets[2]=0x70;
        if(bad==8)fields->assets[0].root=4;
        if(bad==9)fields->assets[0].array.capacity=3;
        if(bad==10)fields->assets[0].instance=0;
        if(bad==11)fields->assets[1]=fields->assets[0];
        if(bad==12)fields->components[0].skeletal=false;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::request&&p::same(*output,*saved),"malformed optional source never truncates or hides rows");}
    memory=initial;*fields=*original;
    struct ChangeAsset {Memory& memory;unsigned typed{};bool operator()(){return true;}
        bool operator()(p::Snapshot const&){if(++typed==1)memory.put<std::uintptr_t>(0x310010,0x1a0000);return true;}} change{memory};
    check(p::observe(base,size,scope,*fields,read,change,*work,*output)==p::Code::changed&&p::same(*output,*saved),"PAA membership closes over both passes");
    memory=initial;
    struct ChangeInput {p::Source& fields;unsigned typed{};bool operator()(){return true;}
        bool operator()(p::Snapshot const&){if(++typed==2)fields.assets[0].owner=0;return true;}} mutate{*fields};
    check(p::observe(base,size,scope,*fields,read,mutate,*work,*output)==p::Code::source_changed&&p::same(*output,*saved),"caller PAA source closes after last typed guard");
    memory=initial;*fields=*original;
    fields->asset_bodies[1].data=fields->asset_bodies[3].data=0;memory.put<std::uintptr_t>(0x320088,0);
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&output->actor_count==1&&
        output->asset_observations[1].presence==p::Presence::body_data_field_null,"null BodyData is distinct from a null body slot");
    fields->asset_body_count=0;fields->asset_bodies={};fields->assets[0].array={0x310000,0,4};fields->assets[0].root=-1;
    memory.put(0x30006c,fields->assets[0].array);
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&
        output->asset_observations[0].presence==p::Presence::not_sampled,"empty reserved PAA array clears previous observations");
    fields->assets[0]={};fields->assets[0].sampled=true;fields->assets[0].asset=0x110000;memory.put<std::uintptr_t>(0x190250,0);
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed,"asset exists without PAA instance");
    // Four shared arrays fill the bounded pool. Each source row is retained.
    *fields=*original;memory=initial;fields->component_count=4;fields->asset_body_count=256;fields->components={};fields->assets={};
    fields->roots[0].members={};fields->roots[0].lists[0]={0x280000,4,4};fields->roots[0].lists[1]={0x290000,0,4};
    for(unsigned n=0;n<4;++n){const auto at=std::uintptr_t(0x400000+n*0x400);memory.block(at,0x280);auto& row=fields->components[n];
        row.component=at;row.owner=0x1b0000;row.owner_roots=1;row.body_instance_offset=0x180;row.primitive=row.skeletal=true;row.state=p::State::body_instance_field_null;
        fields->roots[0].members[0][n]=at;fields->assets[n]=original->assets[0];fields->assets[n].array={0x310000,64,64};fields->assets[n].first_body=n*64;
        fields->attachments[n]={true,0x330000,0x260,56,0,0,{}};
        memory.put<std::uintptr_t>(at+0x248,0x110000);memory.put<std::uintptr_t>(at+0x250,0x300000);}
    memory.block(0x310000,512);memory.put(0x30006c,fields->assets[0].array);
    for(unsigned n=0;n<256;++n)fields->asset_bodies[n]=original->asset_bodies[0];
    for(unsigned n=0;n<64;++n)memory.put<std::uintptr_t>(0x310000+n*8,0x1a0000);
    for(unsigned n=0;n<2;++n){memory.put(0x1b0000+fields->roots[0].offsets[n],fields->roots[0].lists[n]);
        for(unsigned k=0;k<4;++k)memory.put<std::uintptr_t>(fields->roots[0].lists[n].data+k*8,fields->roots[0].members[n][k]);}
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&output->actor_count==1&&
        output->asset_observations[255].presence==p::Presence::actor_observed,"all 256 optional PAA rows close without truncating shared sources");
    printf("CoverPhysXComponentAssets checks=%u failed=%u synthetic optional PAA fields and same-Scene actors; no complete self or world proof\n",checks,failed);return failed?1:0;
}
