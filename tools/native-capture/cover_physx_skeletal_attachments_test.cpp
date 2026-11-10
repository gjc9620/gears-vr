#define GEARS_COMPONENT_ASSETS_PHYSX_TEST_MAIN PriorSkeletalAssetsMain
#include "cover_physx_component_assets_test.cpp"
#undef GEARS_COMPONENT_ASSETS_PHYSX_TEST_MAIN
int main(){
    auto fields=std::make_unique<p::Source>();auto memory=asset_fixture(*fields);
    fields->component_count=3;fields->components[2]={0x400000,0x1b0000,0,0,0,1,0,0,false,false,p::State::nonprimitive};
    auto& scope_fields=fields->attachments[0];scope_fields.array={0x350000,3,3};fields->attachment_count=3;memory.block(0x350000,168);
    for(unsigned n=0;n<3;++n){auto& row=fields->attachment_rows[n];row.component=n==1?0:0x400000;
        memcpy(row.bytes.data(),&row.component,8);row.bytes[8]=static_cast<unsigned char>(7+n);row.bytes[55]=static_cast<unsigned char>(11+n);
        memcpy(memory.blocks[0x350000].data()+n*56,row.bytes.data(),56);}
    memory.put(0x190260,scope_fields.array);const auto initial=memory;auto original=std::make_unique<p::Source>(*fields);
    auto read=[&](void* d,void const* a,std::size_t n){return memory.read(d,a,n);};s::Snapshot scope{};
    check(s::capture(base,size,source(),read,[]{return true;},scope)==s::Code::current,"query scope");
    auto work=std::make_unique<p::Workspace>();auto output=std::make_unique<p::Snapshot>();output->actor_count=99;auto saved=std::make_unique<p::Snapshot>(*output);
    unsigned guards{};memory.calls=0;
    check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards},*work,*output)==p::Code::observed,"structure rows and newly discovered component close in the same Scene packet");
    const auto reads=memory.calls,guard_count=guards;
    check(output->fields.attachment_count==3&&!output->fields.attachment_rows[1].component&&output->fields.attachment_rows[1].bytes[8]==8&&
        output->fields.components[2].component==0x400000&&output->actor_count==2,"null rows, duplicate targets and raw bytes retained without new physical actors");
    for(unsigned n=1;n<=reads;++n){memory.calls=0;memory.fail_at=n;*output=*saved;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)!=p::Code::observed&&p::same(*output,*saved),"each read failure preserves entire skeletal packet");}
    memory.fail_at=0;
    for(unsigned n=1;n<=guard_count;++n){guards=0;*output=*saved;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields,&guards,n},*work,*output)==p::Code::guard&&p::same(*output,*saved),"each guard failure rejects the full packet");}
    for(unsigned bad=0;bad<10;++bad){memory=initial;*fields=*original;*output=*saved;
        if(bad==0)fields->attachments[0].stride=129;
        if(bad==1)fields->attachments[0].component_offset=55;
        if(bad==2)fields->attachments[0].first_row=1;
        if(bad==3)fields->attachment_rows[0].component=0;
        if(bad==4)fields->attachment_rows[0].bytes[56]=1;
        if(bad==5)fields->attachments[0].sampled=false;
        if(bad==6)fields->attachment_rows[3]=fields->attachment_rows[0];
        if(bad==7)fields->attachment_count=257;
        if(bad==8)fields->components[2].component=0x400008;
        if(bad==9)fields->mesh_seed=0x190000;
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::request&&p::same(*output,*saved),"malformed graph, hidden row, stride or primary seed rejects");}
    for(unsigned bad=0;bad<4;++bad){memory=initial;*fields=*original;*output=*saved;
        if(bad==0)memory.put<unsigned char>(0x350008,99);
        if(bad==1)memory.put<unsigned char>(0x350037,99);
        if(bad==2)memory.put<std::uintptr_t>(0x350000,0);
        if(bad==3)memory.put(0x190260,s::pose::ArrayHeader{0x350000,2,3});
        check(p::observe(base,size,scope,*fields,read,PartsGuard{*fields},*work,*output)==p::Code::changed&&p::same(*output,*saved),"pointer, opaque bone/padding or array source change rejects");}
    memory=initial;*fields=*original;
    struct Change {Memory& m;unsigned typed{};bool operator()(){return true;}bool operator()(p::Snapshot const&){if(++typed==1)m.put<unsigned char>(0x350008,99);return true;}} change{memory};
    check(p::observe(base,size,scope,*fields,read,change,*work,*output)==p::Code::changed&&p::same(*output,*saved),"row bytes close over both passes");
    printf("CoverPhysXSkeletalAttachments checks=%u failed=%u synthetic attachment bytes and component graph; no movement or complete self proof\n",checks,failed);return failed?1:0;
}
