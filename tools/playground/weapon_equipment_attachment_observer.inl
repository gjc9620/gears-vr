// Read-only, ordinary PostPlayerTick only. No retry of a dispatched getter.
#include "weapon_equipment_names.h"
namespace nativeequipment_attachment {
namespace abi=::gears_weapon_hand;
using Matrix=std::array<float,16>;
struct Identity {std::array<abi::ObjectIdentity,7> objects{};};
struct Budget {Identity id{};unsigned used{};};
std::array<Budget,8> budgets{};unsigned identities{};
ULONGLONG next_sample{};abi::FrameKey last_key{};bool have_key{};
bool same(Identity const& a,Identity const& b) noexcept {
    for(unsigned i=0;i<a.objects.size();++i)if(!abi::same(a.objects[i],b.objects[i]))return false;return true;
}
bool identity(abi::Frame const& f,uptr gun,uptr asset,Identity& out) noexcept {
    const abi::ObjectIdentity expected[]={f.world,f.controller,f.pawn,f.mesh,f.weapon};
    const uptr addresses[]={f.world.address,f.controller.address,f.pawn.address,f.mesh.address,f.weapon.address,gun,asset};
    if(!nativeequipment::current(nullptr,f))return false;
    for(unsigned i=0;i<7;++i)if(!nativeequipment::identify(nullptr,addresses[i],out.objects[i])||
        (i<5&&!abi::same(expected[i],out.objects[i])))return false;
    return playground::pointer(f.pawn.address,L"Mesh")==f.mesh.address&&
        playground::pointer(f.weapon.address,L"Mesh")==gun&&playground::pointer(gun,L"SkeletalMesh")==asset&&
        nativeequipment::current(nullptr,f);
}
bool parameter(playground::Args& c,uptr p,const wchar_t* kind,unsigned bytes,unsigned flags) {
    if(!registered(p)||get<uptr>(p+0x40)!=c.r.function||!playground::named(get<uptr>(p+0x50),kind)||
        get<unsigned>(p+0x70)!=1||get<unsigned>(p+0x74)!=bytes||(get<unsigned long long>(p+0x78)&0x580)!=flags)return false;
    const auto offset=get<unsigned>(p+0x94);return offset<=c.r.size&&bytes<=c.r.size-offset;
}
bool getter(abi::Frame const& f,Identity const& expected,uptr gun,uptr asset,int bone,bool matrix,void* output,Name named={}) {
    playground::Args c(f.mesh.address,matrix?L"GetBoneMatrix":L"MatchRefBone");
    const auto input_name=matrix?L"BoneIndex":L"BoneName";
    const auto in=c.prop(input_name),out=c.prop(L"ReturnValue");
    // Capture scalar schema evidence before dispatch. A failed contract must
    // be diagnosable without another game run or another original call.
    const auto owner=c.r.function?get<uptr>(c.r.function+0x40):0;
    const auto in_flags=in?get<unsigned long long>(in+0x78):0;
    const auto out_flags=out?get<unsigned long long>(out+0x78):0;
    const auto in_size=in?get<unsigned>(in+0x74):0,out_size=out?get<unsigned>(out+0x74):0;
    const unsigned return_offset=c.r.function?get<unsigned short>(c.r.function+0x108):0xffffu;
    const auto rejected=[&](const wchar_t* stage){
        fwprintf(log_file,L"native_equipment_attachment_getter stage=%ls frame=%llu bone=%d matrix=%u function=%llx owner=%llx bytes=%u input_size=%u input_flags=%llx output_size=%u output_flags=%llx return_offset=%u\n",
            stage,f.key.frame,bone,unsigned(matrix),c.r.function,owner,c.r.size,in_size,in_flags,out_size,out_flags,unsigned(return_offset));
        return false;
    };
    if(!c.valid||!registered(c.r.function)||!playground::named(owner,L"SkeletalMeshComponent"))return rejected(L"function_owner");
    if(!parameter(c,in,matrix?L"IntProperty":L"NameProperty",matrix?4:8,0x80))return rejected(L"input_schema");
    if(!parameter(c,out,matrix?L"StructProperty":L"IntProperty",matrix?64:4,0x580))return rejected(L"return_schema");
    if(matrix&&(!registered(get<uptr>(out+0xd0))||!playground::named(get<uptr>(out+0xd0),L"Matrix")||
        get<unsigned>(get<uptr>(out+0xd0)+0x88)!=64))return rejected(L"return_struct");
    const auto a=get<unsigned>(in+0x94),b=get<unsigned>(out+0x94),n=matrix?64u:4u,input_bytes=matrix?4u:8u;
    if((a<b+n&&b<a+input_bytes)||return_offset!=b)return rejected(L"parameter_range");
    if(matrix)c.set(input_name,bone);else c.set(input_name,named);
    if(!c.valid)return rejected(L"parameter_range");
    Identity before{},after{};
    if(!identity(f,gun,asset,before)||!same(before,expected))return rejected(L"identity_before");
    if(!c.run())return rejected(L"original_getter");
    if(!identity(f,gun,asset,after)||!same(after,expected))return rejected(L"identity_after");
    std::memcpy(output,c.r.params+b,n);return true;
}
bool bone_name(Name n,const wchar_t* wanted) {
    if(n.number)return false;const auto names=get<Array>(base+0x1c34800);
    if(!names.data||names.count<1000||names.count>names.capacity||names.capacity>4000000||n.index>=unsigned(names.count))return false;
    const auto entry=get<uptr>(names.data+size_t(n.index)*8);wchar_t text[32]{};
    const auto bytes=(wcslen(wanted)+1)*sizeof(wchar_t);
    return entry&&bytes<=sizeof text&&get<unsigned>(entry)==n.index&&copy(text,entry+20,bytes)&&!std::memcmp(text,wanted,bytes);
}
constexpr auto& bone_names=gears_equipment_names::Lookup::wanted;
gears_equipment_names::Lookup name_lookup;
abi::FrameKey preparation_key{};bool have_preparation_key{};
gears_equipment_names::State preparation_state=gears_equipment_names::State::pending;
unsigned preparation_logs{};
unsigned preparation_cursor{};
gears_equipment_names::State resolve_names(abi::Frame const& f,std::array<Name,3>& out) {
    std::int64_t scan_started{};
    std::array<unsigned,3> indices{};
    const auto result=name_lookup.step(
        [](gears_equipment_names::Table& out_table){
            const auto table=get<Array>(base+0x1c34800);
            out_table={table.data,unsigned(table.count),unsigned(table.capacity)};return table.count>=0&&table.capacity>=0;
        },
        [](gears_equipment_names::Table table,unsigned index,std::array<wchar_t,32>& text){
            const auto entry=get<uptr>(table.data+size_t(index)*8);unsigned observed_index{};
            return entry&&copy(&observed_index,entry,4)&&observed_index==index&&copy(text.data(),entry+20,sizeof text);
        },
        [&](){return nativeequipment::current(nullptr,f);},
        [&](){const auto now=nativeequipment::clock_ns(nullptr);
            if(!scan_started){scan_started=now;return now>0;}
            return now>=scan_started&&now-scan_started<3000000;},indices);
    if(result==gears_equipment_names::State::ready)
        for(unsigned i=0;i<3;++i)out[i]={indices[i],0};
    return result;
}
struct Attachment {Array entries{};std::array<unsigned char,128*52> bytes{};};
bool attachment(abi::Frame const& f,nativeequipment::Snapshot const& s,Attachment& out) {
    if(!nativeequipment::attached(nullptr,f,s))return false;
    const auto p=playground::property(f.mesh.address,L"Attachments");
    if(!p||!copy(&out.entries,f.mesh.address+get<unsigned>(p+0x94),sizeof(Array))||
        out.entries.count<0||out.entries.count>out.entries.capacity||out.entries.capacity>128)return false;
    return !out.entries.count||copy(out.bytes.data(),out.entries.data,size_t(out.entries.count)*52);
}
bool component(uptr mesh,Matrix& out) {
    const auto p=playground::property(mesh,L"LocalToWorld"),cls=get<uptr>(mesh+0x50);
    if(!p||get<unsigned>(p+0x74)!=64||!playground::named(get<uptr>(p+0x50),L"StructProperty"))return false;
    const auto offset=get<unsigned>(p+0x94),size=get<unsigned>(cls+0x88);
    if(offset>size||64>size-offset||!copy(out.data(),mesh+offset,64))return false;
    for(float v:out)if(!std::isfinite(v))return false;return true;
}
void print_matrix(const wchar_t* label,float const* m) {
    fwprintf(log_file,L" %ls=",label);for(unsigned i=0;i<16;++i)fwprintf(log_file,L"%ls%.9g",i?L",":L"",m[i]);
}
void tick(abi::Frame const& f) noexcept {
    if(!log_file||!nativeequipment::current(nullptr,f))return;
    // Prepare global names in small steps before a held-weapon sample. Only
    // indices persist; every actual getter still uses this tick's Frame.
    std::array<Name,3> names{};
    if(have_preparation_key&&abi::same(preparation_key,f.key))return;
    preparation_key=f.key;have_preparation_key=true;
    const auto preparation_started=nativeequipment::clock_ns(nullptr);
    const auto prepared=resolve_names(f,names);
    if(preparation_logs<8&&(!preparation_logs||prepared!=preparation_state||name_lookup.cursor()-preparation_cursor>=8192)){
        fwprintf(log_file,L"native_equipment_attachment_prepare state=%u cursor=%u found=%u elapsed_ns=%lld cookie=%llu frame=%llu pid=%u game_start=%llu\n",
            unsigned(prepared),name_lookup.cursor(),name_lookup.found(),nativeequipment::clock_ns(nullptr)-preparation_started,
            f.key.registration_cookie,f.key.frame,f.key.game_pid,f.key.game_start);
        fflush(log_file);++preparation_logs;preparation_cursor=name_lookup.cursor();
    }
    preparation_state=prepared;
    if(prepared!=gears_equipment_names::State::ready||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return;
    const auto now=GetTickCount64();if(now<next_sample||(have_key&&abi::same(last_key,f.key)))return;
    next_sample=now+100;last_key=f.key;have_key=true;
    const wchar_t* reason=L"qualification";unsigned attempt{},selected{};bool accepted=false;
    vrequip::MeshPoseProbe gun{},again{};std::array<Matrix,3> bones{};Matrix pawn{},pawn_after{};
    try {
        Identity id{},after{};nativeequipment::Snapshot game{};Attachment attached{},attached_after{};
        if(!vrequip::mesh_pose_probe(f.weapon.address,gun)||wcscmp(gun.asset_name,L"COGAssaultRifle")||
            !identity(f,gun.mesh,gun.asset,id))return;
        unsigned slot=0;for(;slot<identities;++slot){bool match=true;
            for(unsigned i=0;i<5;++i)match=match&&abi::same(budgets[slot].id.objects[i],id.objects[i]);if(match)break;}
        if(slot==identities){if(identities==budgets.size())return;budgets[slot].id=id;++identities;}
        if(budgets[slot].used==24)return;attempt=++budgets[slot].used;selected=(attempt+1)%3;
        const auto sample=[&]() {
            if(!nativeequipment::inventory(nullptr,f,game)||!attachment(f,game,attached)||gun.hidden)return false;
            reason=L"nonfinite_gun_transform";
            for(float v:gun.local_to_world)if(!std::isfinite(v))return false;
            if(!std::isfinite(gun.translation.x)||!std::isfinite(gun.translation.y)||!std::isfinite(gun.translation.z))return false;
            reason=L"component_matrix";if(!component(f.mesh.address,pawn))return false;
            const int indices[]={35,51,52};
            // Each record compares ONE original bone with the gun on this
            // exact Frame. Never combine independently sampled bones into a
            // fictitious same-frame chain. Two original calls, not six, leave
            // room for all existing before/after identity and age checks.
            {const auto i=selected;int actual=-1;reason=L"match_ref_bone_getter_or_identity";
                if(!bone_name(names[i],bone_names[i])||!getter(f,id,gun.mesh,gun.asset,indices[i],false,&actual,names[i]))return false;
                reason=L"match_ref_bone_index_mismatch";if(actual!=indices[i])return false;
                reason=L"bone_matrix_getter_or_identity";if(!getter(f,id,gun.mesh,gun.asset,indices[i],true,bones[i].data()))return false;
                for(float v:bones[i])if(!std::isfinite(v)){reason=L"nonfinite_bone_matrix";return false;}}
            reason=L"body_component_changed";
            if(!component(f.mesh.address,pawn_after)||pawn!=pawn_after)return false;
            reason=L"gun_component_changed";
            if(!vrequip::mesh_pose_probe(f.weapon.address,again)||gun.mesh!=again.mesh||gun.asset!=again.asset||gun.hidden!=again.hidden||
                gun.absolute_translation!=again.absolute_translation||gun.absolute_rotation!=again.absolute_rotation||
                gun.absolute_scale!=again.absolute_scale||std::memcmp(&gun.translation,&again.translation,sizeof gun.translation)||
                std::memcmp(&gun.rotation,&again.rotation,sizeof gun.rotation)||
                std::memcmp(gun.local_to_world,again.local_to_world,sizeof gun.local_to_world))return false;
            reason=L"attachment_changed_or_source_expired";
            if(!attachment(f,game,attached_after)||std::memcmp(&attached.entries,&attached_after.entries,sizeof(Array))||
                attached.bytes!=attached_after.bytes)return false;
            reason=L"final_identity_or_source_expired";
            if(!identity(f,gun.mesh,gun.asset,after)||!same(id,after))return false;
            reason=L"qualified";return true;
        };
        accepted=sample();
    }catch(...){reason=L"cpp_exception";}
    if(!attempt)return;
    const auto sampled_after_ns=nativeequipment::clock_ns(nullptr);
    if(accepted&&!abi::frame_active(f,sampled_after_ns)){accepted=false;reason=L"source_expired_at_record";}
    const unsigned sampled_indices[]={35,51,52};
    fwprintf(log_file,L"native_equipment_attachment accepted=%u reason=%ls attempt=%u bone=%u game_start=%llu native_started=%llu cookie=%llu lifecycle=%llu frame=%llu world_generation=%llu pid=%u thread=%u source_sequence=%llu source_now_ns=%lld source_received_ns=%lld source_sync_ns=%lld observed_ns=%lld asset=%ls absolute=%u,%u,%u hidden=%u translation=%.9g,%.9g,%.9g rotation=%d,%d,%d",
        unsigned(accepted),reason,attempt,sampled_indices[selected],f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,f.key.lifecycle,f.key.frame,
        f.key.world_generation,f.key.game_pid,f.key.thread_id,f.controllers.sequence,f.now_ns,f.controllers.received_ns,f.controllers.sync_ns,sampled_after_ns,gun.asset_name,
        unsigned(gun.absolute_translation),unsigned(gun.absolute_rotation),unsigned(gun.absolute_scale),unsigned(gun.hidden),
        gun.translation.x,gun.translation.y,gun.translation.z,gun.rotation.pitch,gun.rotation.yaw,gun.rotation.roll);
    if(accepted){const wchar_t* labels[]={L"bone35",L"bone51",L"bone52"};
        print_matrix(labels[selected],bones[selected].data());print_matrix(L"gun_LocalToWorld",gun.local_to_world);}
    fwprintf(log_file,L"\n");fflush(log_file);
}
void reset_after_drain() noexcept {budgets={};identities=0;next_sample=0;last_key={};have_key=false;name_lookup.reset();preparation_key={};have_preparation_key=false;preparation_state=gears_equipment_names::State::pending;preparation_logs=0;preparation_cursor=0;}
} // namespace nativeequipment_attachment
