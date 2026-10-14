#include "anchor_observer.h"
#include <cstdio>
#include <vector>
#include <limits>
#define CHECK(c) do{if(!(c)){std::printf("failed %d: %s\n",__LINE__,#c);return __LINE__;}}while(0)
using namespace gears_anchor;
static Receipt receipt(){Receipt r{};r.key.local=1;r.key.family=r.key.owner=2;r.key.source=3;r.key.stamp=4;
    r.anchor.pawn=5;r.anchor.location={1,2,3};r.anchor.eye_height=4;r.invocation=1;r.thread=7;r.scene_qpc=10;return r;}
static Reason offer(Context& c,Receipt r){c.begin_scene();auto why=c.offer(r);c.end_scene();return why;}
int main(){
    Context c;auto r=receipt();Receipt output{};
    CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==no_receipt);
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==accepted);
        CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==accepted);
        CHECK(output.invocation==1&&output.scene_qpc==10&&output.family_qpc==20&&output.renderer_qpc==30);
        CHECK(c.consume(r.key,r.anchor,7,31,true,1,0x3a1595,output)==already_consumed);}
    CHECK(c.consume(r.key,r.anchor,7,32,true,1,0x3a1595,output)==no_receipt);
    // Submission is bound to the current source's full128-byte local snapshot,
    // including projection changes after the previous view64 association.
    auto bound=output;auto snapshot=bound.renderer_matrices;
    auto submission=[&](Receipt const* value){return submission_matches(value,2,3,1,7,snapshot.data(),40);};
    CHECK(submission(&bound));CHECK(!submission(nullptr));
    auto bad=bound;bad.invocation=0;CHECK(!submission(&bad));
    bad=bound;bad.thread=0;CHECK(!submission(&bad));
    CHECK(!submission_matches(&bound,2,3,1,0,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,1,8,snapshot.data(),40));
    CHECK(!submission_matches(&bound,4,3,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,4,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,4,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,0,3,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,0,1,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,0,7,snapshot.data(),40));
    CHECK(!submission_matches(&bound,2,3,1,7,nullptr,40));
    bad=bound;bad.key.owner=4;CHECK(!submission(&bad));
    const std::array<std::array<std::int64_t,3>,4> bad_times{{{0,20,30},{21,20,30},{10,31,30},{10,20,41}}};
    for(auto const& times:bad_times){
        bad=bound;bad.scene_qpc=times[0];bad.family_qpc=times[1];bad.renderer_qpc=times[2];CHECK(!submission(&bad));}
    // Snapshot first, then the current source changed: old receipt must reject.
    snapshot[0]=1;CHECK(!submission(&bound));snapshot=bound.renderer_matrices;
    snapshot[64]=1;CHECK(!submission(&bound));snapshot=bound.renderer_matrices;
    float nonfinite=std::numeric_limits<float>::infinity();
    bad=bound;std::memcpy(bad.renderer_matrices.data()+64,&nonfinite,sizeof(nonfinite));
    CHECK(!submission_matches(&bad,2,3,1,7,bad.renderer_matrices.data(),40));
    bad=bound;std::memcpy(&bad.key.stamp,&nonfinite,sizeof(nonfinite));CHECK(!submission(&bad));
    bad=bound;bad.anchor.location[0]=nonfinite;CHECK(!submission(&bad));
    bad=bound;bad.anchor.eye_height=nonfinite;CHECK(!submission(&bad));
    bad=bound;bad.anchor.base_eye_height=nonfinite;CHECK(!submission(&bad));
    bad=bound;bad.anchor.location[2]=bad.anchor.eye_height=std::numeric_limits<float>::max();CHECK(!submission(&bad));
    // A rejected stale source binding has no persistent stop effect. A fresh
    // current one-use family scope supplies its own invocation/source snapshot.
    auto next=r;next.invocation=2;next.key.source=4;next.scene_qpc=50;next.key.matrices[64]=1;
    CHECK(!submission_matches(&bound,2,4,1,7,next.key.matrices.data(),80));
    CHECK(offer(c,next)==accepted);
    {Context::FamilyScope family(c,next.key,next.anchor,7,60,true,1,0x2c8909);CHECK(family.reason==accepted);
        Receipt current{};CHECK(c.consume(next.key,next.anchor,7,70,true,1,0x3a1595,current)==accepted);
        CHECK(submission_matches(&current,2,4,1,7,next.key.matrices.data(),80));
        CHECK(c.consume(next.key,next.anchor,7,71,true,1,0x3a1595,current)==already_consumed);}
    CHECK(offer(c,r)==accepted);CHECK(offer(c,r)==ambiguous);CHECK(offer(c,r)==accepted); // finite boundary recovery
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);
        {Context::FamilyScope nestedfamily(c,r.key,r.anchor,7,21,true,1,0x2c8909);CHECK(nestedfamily.reason==nested);}
        CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==nested);}
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);
        CHECK(c.consume(r.key,r.anchor,7,30,true,1,123,output)==caller_mismatch);
        CHECK(c.consume(r.key,r.anchor,7,31,true,1,0x3a1595,output)==already_consumed);}
    CHECK(offer(c,r)==accepted);auto changed=r.key;changed.stamp++;
    {Context::FamilyScope family(c,changed,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==stamp_mismatch);}
    CHECK(offer(c,r)==accepted);changed=r.key;changed.matrices[64]=1;
    {Context::FamilyScope family(c,changed,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==accepted);
        CHECK(c.consume(changed,r.anchor,7,30,true,1,0x3a1595,output)==accepted);CHECK(output.family_matrices[64]==1);}
    CHECK(offer(c,r)==accepted);changed=r.key;changed.matrices[0]=1;
    {Context::FamilyScope family(c,changed,r.anchor,7,20,true,1,0x2c8909);CHECK(family.reason==matrix_mismatch);}
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,8,20,true,1,0x2c8909);CHECK(family.reason==no_receipt);}
    CHECK(offer(c,r)==accepted);
    {Context::FamilyScope family(c,r.key,r.anchor,7,20,true,2,0x2c8909);CHECK(family.reason==multiple_views);}
    c.begin_scene();c.begin_scene();CHECK(c.offer(r)==nested);c.end_scene();CHECK(c.offer(r)==nested);c.end_scene();
    CHECK(offer(c,r)==accepted);{Context::FamilyScope family(c,r.key,r.anchor,7,20,true,1,0x2c8909);
        c.begin_scene();CHECK(c.offer(r)==nested);c.end_scene();CHECK(c.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==nested);}
    Context other;CHECK(other.consume(r.key,r.anchor,7,30,true,1,0x3a1595,output)==no_receipt);
    // Full production guard against synthetic registered objects, including
    // legal count growth between calls and corruption within a timestamp bracket.
    std::vector<unsigned char> mem(300000);
    auto put=[&](std::uintptr_t address,auto const& value){std::memcpy(mem.data()+address,&value,sizeof(value));};
    auto read=[&](void* dest,const void* ptr,std::size_t size){auto address=reinterpret_cast<std::uintptr_t>(ptr);
        if(address+size>mem.size())return false;std::memcpy(dest,mem.data()+address,size);return true;};
    Config cfg{};cfg.objects_header=100;cfg.names_header=200;cfg.identity_count=9;
    cfg.object_internal_index=8;cfg.object_name=12;cfg.object_class=24;cfg.object_outer=32;cfg.field_super=96;
    cfg.name_entry_index=0;cfg.name_entry_text=8;
    cfg.role_controller=0;cfg.role_local=1;cfg.role_camera=2;cfg.role_pawn=3;cfg.role_level=4;
    cfg.controller_pawn=120;cfg.pawn_controller=120;cfg.player_camera=128;cfg.pc_owner=120;cfg.player=136;
    cfg.camera_view_target=128;cfg.camera_timestamp=140;cfg.pawn_location=140;cfg.eye_height=152;cfg.base_eye_height=156;
    Registry objects{1000,2001,2100},names{20000,2001,2100};put(100,objects);put(200,names);
    const wchar_t* labels[]={L"PC",L"Local",L"Camera",L"Pawn",L"LevelObj",L"WarLocalPlayer",L"WarPlayerCamera",L"WarPawn",L"Level"};
    for(unsigned i=0;i<9;++i){auto& id=cfg.identities[i];id.address=40000+i*256;id.index=id.name_index=i;
        id.cls=i==1?41280:i==2?41536:i==3?41792:i==4?42048:41280;id.outer=i==3?41024:0;
        id.check_super=i>=5;std::wcsncpy(id.name.data(),labels[i],63);put(id.address+8,id.index);put(id.address+12,id.name_index);
        put(id.address+24,id.cls);put(id.address+32,id.outer);put(objects.data+i*8,id.address);
        std::uintptr_t entry=50000+i*256;put(names.data+i*8,entry);put(entry,id.name_index);
        std::memcpy(mem.data()+entry+8,id.name.data(),sizeof(id.name));}
    auto pc=cfg.identities[0].address,local=cfg.identities[1].address,camera=cfg.identities[2].address,
        pawn=cfg.identities[3].address,level=cfg.identities[4].address;
    put(pc+120,pawn);put(pawn+120,pc);put(pc+128,camera);put(camera+120,pc);put(pc+136,local);put(camera+128,pawn);
    put(camera+140,1.f);put(pawn+140,std::array<float,3>{1,2,3});put(pawn+152,4.f);put(pawn+156,5.f);
    Anchor anchor{};std::uint32_t stamp{};CHECK(guard(cfg,read,anchor,stamp)&&anchor.level==level);
    objects.count++;names.count++;put(100,objects);put(200,names);CHECK(guard(cfg,read,anchor,stamp));
    put(objects.data+3*8,std::uintptr_t(999));CHECK(!guard(cfg,read,anchor,stamp));put(objects.data+3*8,pawn);
    auto corrupt_read=[&](void* dest,const void* ptr,std::size_t size){auto ok=read(dest,ptr,size);
        if(reinterpret_cast<std::uintptr_t>(ptr)==pawn+140)put(camera+140,2.f);return ok;};
    CHECK(!guard(cfg,corrupt_read,anchor,stamp));CHECK(guard(cfg,read,anchor,stamp));
    put(pawn+140,std::array<float,3>{1,2,std::numeric_limits<float>::max()});put(pawn+152,std::numeric_limits<float>::max());
    CHECK(!guard(cfg,read,anchor,stamp));
    put(pawn+140,std::array<float,3>{1,2,3});put(pawn+152,4.f);
    PawnBinding before_scene{};CHECK(capture_pawn_binding(cfg,read,before_scene));
    CHECK(guard(cfg,read,anchor,stamp)&&scene_binding_matches(before_scene,anchor));
    // Pawn position and camera time may advance inside original_scene. Only
    // the possession/identity bracket is required to remain unchanged.
    put(pawn+140,std::array<float,3>{8,9,10});put(camera+140,3.f);
    CHECK(guard(cfg,read,anchor,stamp)&&scene_binding_matches(before_scene,anchor));
    auto original_config=cfg;
    const uintptr_t new_pawn=60000,new_name_entry=62000;
    const unsigned new_index=50,new_name_index=50,new_number=2;
    put(new_pawn+8,new_index);put(new_pawn+12,new_name_index);put(new_pawn+16,new_number);
    put(new_pawn+24,cfg.identities[3].cls);put(new_pawn+32,cfg.identities[3].outer);
    put(new_pawn+120,pc);put(new_pawn+140,std::array<float,3>{11,12,13});put(new_pawn+152,4.f);put(new_pawn+156,5.f);
    put(objects.data+new_index*8,new_pawn);put(names.data+new_name_index*8,new_name_entry);put(new_name_entry,new_name_index);
    std::array<wchar_t,64> new_name{};std::copy_n(L"Pawn_Respawn",sizeof(L"Pawn_Respawn")/sizeof(wchar_t),new_name.begin());put(new_name_entry+8,new_name);
    put(pc+120,new_pawn);put(camera+128,new_pawn);
    CHECK(guard(cfg,read,anchor,stamp)&&anchor.pawn==new_pawn&&anchor.pawn_identity.number==new_number);
    CHECK(!scene_binding_matches(before_scene,anchor));
    CHECK(equal_identity(cfg.identities[3],original_config.identities[3])); // No global mutation.
    CHECK(cfg.identities[3].address==pawn);CHECK(memcmp(&cfg,&original_config,sizeof(cfg))==0);
    // Old Pawn remains registered and readable: it is deliberately not used.
    CHECK(capture_pawn_binding(cfg,read,before_scene));CHECK(scene_binding_matches(before_scene,anchor));
    auto stable_memory=mem;
    put(objects.data+cfg.identities[3].index*8,uintptr_t(0));put(pawn+24,uintptr_t(0));
    CHECK(guard(cfg,read,anchor,stamp)); // Obsolete startup Pawn is no longer certified.
    mem=stable_memory;
    for(unsigned fault=0;fault<13;++fault){mem=stable_memory;
        if(fault==0)put(new_pawn+24,cfg.identities[2].cls);
        if(fault==1)put(new_pawn+32,uintptr_t(level+256));
        if(fault==2)put(pc+128,uintptr_t(camera+256));
        if(fault==3)put(camera+120,uintptr_t(pc+256));
        if(fault==4)put(level+32,uintptr_t(123));
        if(fault==5)put(new_pawn+8,cfg.identities[0].index); // Aliased registered slot.
        if(fault==6)put(pc+120,pc); // Aliased certificate object.
        if(fault==7)put(new_pawn+8,unsigned(objects.count));
        if(fault==8)put(new_pawn+12,unsigned(names.count));
        if(fault==9)put(new_name_entry,new_name_index+1);
        if(fault==10)put(new_pawn+120,uintptr_t(pc+256));
        if(fault==11)put(camera+128,pawn);
        if(fault==12)put(pc+24,uintptr_t(123));
        CHECK(!guard(cfg,read,anchor,stamp));}
    for(unsigned fault=0;fault<3;++fault){mem=stable_memory;auto text=new_name;
        if(fault==0)std::copy_n(L"Default__WarPawn",sizeof(L"Default__WarPawn")/sizeof(wchar_t),text.begin());
        if(fault==1)text.fill(L'x');if(fault==2)text.fill(0);
        put(new_name_entry+8,text);CHECK(!guard(cfg,read,anchor,stamp));}
    mem=stable_memory;
    unsigned pawn_header_reads=0;
    auto changing_identity=[&](void* dest,const void* ptr,std::size_t size){auto ok=read(dest,ptr,size);
        if(reinterpret_cast<uintptr_t>(ptr)==new_pawn&&size==96&&++pawn_header_reads==1)put(new_pawn+16,new_number+1);
        return ok;};
    CHECK(!guard(cfg,changing_identity,anchor,stamp));
    mem=stable_memory;CHECK(guard(cfg,read,anchor,stamp));
    auto changed_identity=anchor;changed_identity.pawn_identity.number++;
    CHECK(!equal_anchor(anchor,changed_identity));CHECK(!scene_binding_matches(before_scene,changed_identity));
    auto observed=r;observed.anchor=anchor;Context reassociated;
    CHECK(offer(reassociated,observed)==accepted);
    {Context::FamilyScope family(reassociated,observed.key,changed_identity,7,20,true,1,0x2c8909);
        CHECK(family.reason==identity_failed);}
    CHECK(offer(reassociated,observed)==accepted);
    {Context::FamilyScope family(reassociated,observed.key,anchor,7,20,true,1,0x2c8909);CHECK(family.reason==accepted);
        CHECK(reassociated.consume(observed.key,changed_identity,7,30,true,1,0x3a1595,output)==identity_failed);}
    // Snapshot reuse of the startup address with a changed FName signature is
    // refused rather than accepting a replacement solely by its pointer.
    mem=stable_memory;put(pc+120,pawn);put(camera+128,pawn);put(pawn+16,1u);
    CHECK(!guard(cfg,read,anchor,stamp));
    std::printf("anchor scope/one-use/nested restore/cross-thread/ambiguity/view64/submission128/fresh-scope recovery/projection observation/registry growth/bracket CPU tests passed\n");return 0;
}
