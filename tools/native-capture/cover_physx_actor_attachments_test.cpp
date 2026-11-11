#define GEARS_COMPONENTS_PHYSX_TEST_MAIN PriorActorEdgesComponentsMain
#include "cover_physx_components_test.cpp"
#undef GEARS_COMPONENTS_PHYSX_TEST_MAIN
static Memory actor_edges_fixture(p::Source& fields){
    auto m=parts_fixture(fields);fields.root_count=3;fields.component_count=3;
    for(unsigned n=0;n<3;++n){auto& root=fields.roots[n];if(n){root.actor=0x600000+(n-1)*0x10000;root.offsets={0x300,0x310};
            root.base_offset=0x2e0;root.attached_offset=0x2f0;root.base=n==1?0x1b0000:0x1e0000;m.block(root.actor,0x320);}
        root.attached={0x620000+n*0x10000,n==0?4:n==1?2:1,4};m.block(root.attached.data,32);}
    fields.roots[0].attached_members={0x600000,0,0x600000,0x610000};
    fields.roots[1].attached_members={0x610000,0x1b0000};fields.roots[2].attached_members={0x600000};
    fields.roots[1].lists[0]={0x650000,1,1};fields.roots[1].members[0][0]=0x660000;
    fields.roots[2].lists[1]=fields.roots[1].lists[0];fields.roots[2].members[1][0]=0x660000;
    m.block(0x650000,8);m.put<std::uintptr_t>(0x650000,0x660000);m.block(0x660000,0x188);m.block(0x670000,0x98);
    m.block(0x680000,0x48);m.block(0x690000,0x18);
    fields.components[2]={0x660000,0x610000,0x670000,0x660000,0x610000,4,4,0x180,true,false,p::State::body_data_nonnull,-9,0x680000,0};
    m.put<std::uintptr_t>(0x660180,0x670000);m.put<std::uintptr_t>(0x670088,0x680000);
    m.put<std::uintptr_t>(0x680000,base+0x2330a8);m.actor_owner(0x680000,base);m.put<std::uintptr_t>(0x680038,0x170000);
    m.put<std::uintptr_t>(0x680040,0x690000);m.put<std::uintptr_t>(0x690010,0x680000);
    for(unsigned n=0;n<3;++n){auto const& root=fields.roots[n];m.put(root.actor+root.base_offset,root.base);m.put(root.actor+root.attached_offset,root.attached);
        for(unsigned k=0;k<4;++k)m.put(root.attached.data+k*8,root.attached_members[k]);
        for(unsigned l=0;l<2;++l)m.put(root.actor+root.offsets[l],root.lists[l]);}
    return m;
}
int main(){
    auto fields=std::make_unique<p::Source>();auto memory=actor_edges_fixture(*fields);const auto initial=memory;auto original=std::make_unique<p::Source>(*fields);
    auto read=[&](void* dst,void const* at,std::size_t bytes){return memory.read(dst,at,bytes);};s::Snapshot scope{};
    check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"root query source");
    auto work=std::make_unique<p::Workspace>();auto output=std::make_unique<p::Snapshot>();output->actor_count=99;auto saved=std::make_unique<p::Snapshot>(*output);
    unsigned guards{};memory.calls=0;
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards},*work,*output)==p::Code::observed,"recursive actor components qualified on the same Scene");
    const auto reads=memory.calls,guard_count=guards;
    check(output->actor_count==2&&output->actors[1]==0x680000&&output->fields.roots[2].base==0x1e0000&&
        output->fields.roots[0].attached_members[1]==0&&output->fields.components[2].owner_roots==4,"graph membership, actual Base and physical actors remain separate facts");
    for(unsigned n=1;n<=reads;++n){*output=*saved;memory.calls=0;memory.fail_at=n;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)!=p::Code::observed&&p::same(*output,*saved),"every recursive graph read failure preserves output");}
    memory.fail_at=0;
    for(unsigned n=1;n<=guard_count;++n){*output=*saved;guards=0;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards,n},*work,*output)==p::Code::guard&&p::same(*output,*saved),"every typed graph guard rejects without publishing");}
    for(unsigned bad=0;bad<8;++bad){memory=initial;*fields=*original;*output=*saved;
        if(bad==0)memory.put<std::uintptr_t>(0x620000,0);
        if(bad==1)memory.put<std::uintptr_t>(0x6002e0,0x610000);
        if(bad==2)memory.put(0x6102f0,s::pose::ArrayHeader{0x640000,0,4});
        if(bad==3)memory.put<std::uintptr_t>(0x650000,0x190000);
        if(bad==4)memory.put<std::uintptr_t>(0x680038,0x180000);
        if(bad==5)memory.put<std::uintptr_t>(0x680030,0x160000);
        if(bad==6)memory.put<std::uintptr_t>(0x680000,base+0x2330b0);
        if(bad==7)memory.put<std::uintptr_t>(0x670088,0x160000);
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)!=p::Code::observed&&p::same(*output,*saved),"edge Base child member or child actor change rejects the whole packet");}
    for(unsigned bad=0;bad<12;++bad){memory=initial;*fields=*original;*output=*saved;
        if(bad==0)fields->seed_count=0;
        if(bad==1)fields->seed_count=3;
        if(bad==2)fields->roots[0].attached_members[4]=0x600000;
        if(bad==3)fields->roots[1].actor=0x610000;
        if(bad==4)fields->root_count=2;
        if(bad==5)fields->roots[3].actor=0x600000;
        if(bad==6){fields->roots[0].attached_members[3]=0x1b0000;fields->roots[1].attached_members[0]=0x1b0000;}
        if(bad==7)fields->roots[1].base_offset=0x300;
        if(bad==8)fields->roots[1].attached_offset=0x2e0;
        if(bad==9)fields->roots[2].attached.capacity=0;
        if(bad==10)fields->components[2].owner_roots=2;
        if(bad==11)fields->roots[1].attached_members[0]=0x1d0000;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::request&&p::same(*output,*saved),"orphan reordered hidden partial or malformed graph rejects");}
    memory=initial;*fields=*original;
    struct ChangeEdge {Memory& memory;unsigned typed{};bool operator()(){return true;}
        bool operator()(p::Snapshot const&){if(++typed==1)memory.put<std::uintptr_t>(0x630000,0);return true;}} change{memory};
    check(p::observe(base,size,scope,*fields,read,change,*work,*output)==p::Code::changed&&p::same(*output,*saved),"recursive edge closes over both passes");
    memory=initial;
    struct ChangeKey {p::Source& fields;unsigned typed{};bool operator()(){return true;}
        bool operator()(p::Snapshot const&){if(++typed==2)fields.roots[2].base=0;return true;}} mutate{*fields};
    check(p::observe(base,size,scope,*fields,read,mutate,*work,*output)==p::Code::source_changed&&p::same(*output,*saved),"Base source closes after the final guard");
    // Full node capacity, including a cycle from the final node to the seed.
    memory=parts_fixture(*fields);fields->root_count=16;
    for(unsigned n=0;n<16;++n){auto& root=fields->roots[n];if(n){root.actor=0x700000+n*0x1000;root.offsets={0x300,0x310};
            root.base_offset=0x2e0;root.attached_offset=0x2f0;root.base=n==1?0x1b0000:0x700000+(n-1)*0x1000;memory.block(root.actor,0x320);}
        root.attached={0x900000+n*0x1000,1,1};root.attached_members={};root.attached_members[0]=n==15?0x1b0000:0x700000+(n+1)*0x1000;
        memory.block(root.attached.data,8);memory.put(root.attached.data,root.attached_members[0]);memory.put(root.actor+root.base_offset,root.base);memory.put(root.actor+root.attached_offset,root.attached);}
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::observed&&output->fields.roots[15].attached_members[0]==0x1b0000,"all sixteen recursive actor nodes close including final cycle edge");
    printf("CoverPhysXActorAttachments checks=%u failed=%u synthetic recursive graph and same-Scene bodies only; no complete self or world fence\n",checks,failed);return failed?1:0;
}
