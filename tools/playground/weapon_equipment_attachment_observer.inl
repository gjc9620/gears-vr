// Read-only, ordinary PostPlayerTick only. No retry of a dispatched getter.
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
constexpr const wchar_t* bone_names[]={L"b_MF_Hand_R",L"b_MF_Weapon_R",L"b_MF_Weapon_R_End"};
std::array<Name,3> cached_names{};bool names_known{};
bool resolve_names(abi::Frame const& f,std::array<Name,3>& out) {
    if(!nativeequipment::current(nullptr,f))return false;
    if(names_known){bool valid=true;for(unsigned i=0;i<3;++i)valid=valid&&bone_name(cached_names[i],bone_names[i]);
        if(valid){out=cached_names;return nativeequipment::current(nullptr,f);}names_known=false;cached_names={};}
    const auto table=get<Array>(base+0x1c34800);
    if(!table.data||table.count<1000||table.count>table.capacity||table.capacity>4000000)return false;
    unsigned found{};std::array<Name,3> candidate{};
    for(int i=0;i<table.count&&found!=7;++i){
        if((i&255)==0&&!nativeequipment::current(nullptr,f))return false;
        const auto entry=get<uptr>(table.data+size_t(i)*8);wchar_t text[32]{};
        if(!entry||get<unsigned>(entry)!=unsigned(i)||!copy(text,entry+20,sizeof(wchar_t))||text[0]!=L'b')continue;
        bool complete=false;for(unsigned n=1;n<32;++n){if(!copy(text+n,entry+20+n*2,2))break;if(!text[n]){complete=true;break;}}
        if(!complete)continue;for(unsigned n=0;n<3;++n)if(!wcscmp(text,bone_names[n])){candidate[n]={unsigned(i),0};found|=1u<<n;}
    }
    const auto after=get<Array>(base+0x1c34800);
    if(found!=7||std::memcmp(&table,&after,sizeof table)||!nativeequipment::current(nullptr,f))return false;
    for(unsigned i=0;i<3;++i)if(!bone_name(candidate[i],bone_names[i]))return false;
    cached_names=candidate;names_known=true;out=candidate;return nativeequipment::current(nullptr,f);
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
    if(!log_file||!nativeequipment::current(nullptr,f)||!vrequip::model.held_for(f.world.address,f.pawn.address,f.weapon.address))return;
    const auto now=GetTickCount64();if(now<next_sample||(have_key&&abi::same(last_key,f.key)))return;
    next_sample=now+100;last_key=f.key;have_key=true;
    const wchar_t* reason=L"qualification";unsigned attempt{};bool accepted=false;
    vrequip::MeshPoseProbe gun{},again{};std::array<Matrix,3> bones{};Matrix pawn{},pawn_after{};
    try {
        Identity id{},after{};nativeequipment::Snapshot game{};Attachment attached{},attached_after{};
        if(!vrequip::mesh_pose_probe(f.weapon.address,gun)||wcscmp(gun.asset_name,L"COGAssaultRifle")||
            !identity(f,gun.mesh,gun.asset,id))return;
        unsigned slot=0;for(;slot<identities;++slot){bool match=true;
            for(unsigned i=0;i<5;++i)match=match&&abi::same(budgets[slot].id.objects[i],id.objects[i]);if(match)break;}
        if(slot==identities){if(identities==budgets.size())return;budgets[slot].id=id;++identities;}
        if(budgets[slot].used==16)return;attempt=++budgets[slot].used;
        const auto sample=[&]() {
            if(!nativeequipment::inventory(nullptr,f,game)||!attachment(f,game,attached)||gun.hidden)return false;
            reason=L"nonfinite_gun_transform";
            for(float v:gun.local_to_world)if(!std::isfinite(v))return false;
            if(!std::isfinite(gun.translation.x)||!std::isfinite(gun.translation.y)||!std::isfinite(gun.translation.z))return false;
            reason=L"component_matrix";if(!component(f.mesh.address,pawn))return false;
            const int indices[]={35,51,52};std::array<Name,3> names{};reason=L"bone_names_missing_or_source_expired";
            if(!resolve_names(f,names))return false;
            for(unsigned i=0;i<3;++i){int actual=-1;reason=L"match_ref_bone_getter_or_identity";
                if(!bone_name(names[i],bone_names[i])||!getter(f,id,gun.mesh,gun.asset,indices[i],false,&actual,names[i]))return false;
                reason=L"match_ref_bone_index_mismatch";if(actual!=indices[i])return false;
                reason=L"bone_matrix_getter_or_identity";if(!getter(f,id,gun.mesh,gun.asset,indices[i],true,bones[i].data()))return false;
                for(float v:bones[i])if(!std::isfinite(v)){reason=L"nonfinite_bone_matrix";return false;}}
            reason=L"changed_before_record";
            if(!component(f.mesh.address,pawn_after)||pawn!=pawn_after||!vrequip::mesh_pose_probe(f.weapon.address,again)||
                gun.mesh!=again.mesh||gun.asset!=again.asset||gun.hidden!=again.hidden||
                gun.absolute_translation!=again.absolute_translation||gun.absolute_rotation!=again.absolute_rotation||
                gun.absolute_scale!=again.absolute_scale||std::memcmp(&gun.translation,&again.translation,sizeof gun.translation)||
                std::memcmp(&gun.rotation,&again.rotation,sizeof gun.rotation)||
                std::memcmp(gun.local_to_world,again.local_to_world,sizeof gun.local_to_world)||
                !attachment(f,game,attached_after)||std::memcmp(&attached.entries,&attached_after.entries,sizeof(Array))||
                attached.bytes!=attached_after.bytes||!identity(f,gun.mesh,gun.asset,after)||!same(id,after))return false;
            reason=L"qualified";return true;
        };
        accepted=sample();
    }catch(...){reason=L"cpp_exception";}
    if(!attempt)return;
    const auto sampled_after_ns=nativeequipment::clock_ns(nullptr);
    fwprintf(log_file,L"native_equipment_attachment accepted=%u reason=%ls attempt=%u game_start=%llu native_started=%llu cookie=%llu lifecycle=%llu frame=%llu world_generation=%llu pid=%u thread=%u source_sequence=%llu source_now_ns=%lld source_received_ns=%lld source_sync_ns=%lld observed_ns=%lld asset=%ls absolute=%u,%u,%u hidden=%u translation=%.9g,%.9g,%.9g rotation=%d,%d,%d",
        unsigned(accepted),reason,attempt,f.key.game_start,f.key.native_started_qpc,f.key.registration_cookie,f.key.lifecycle,f.key.frame,
        f.key.world_generation,f.key.game_pid,f.key.thread_id,f.controllers.sequence,f.now_ns,f.controllers.received_ns,f.controllers.sync_ns,sampled_after_ns,gun.asset_name,
        unsigned(gun.absolute_translation),unsigned(gun.absolute_rotation),unsigned(gun.absolute_scale),unsigned(gun.hidden),
        gun.translation.x,gun.translation.y,gun.translation.z,gun.rotation.pitch,gun.rotation.yaw,gun.rotation.roll);
    if(accepted){print_matrix(L"bone35",bones[0].data());print_matrix(L"bone51",bones[1].data());print_matrix(L"bone52",bones[2].data());print_matrix(L"gun_LocalToWorld",gun.local_to_world);}
    fwprintf(log_file,L"\n");fflush(log_file);
}
void reset_after_drain() noexcept {budgets={};identities=0;next_sample=0;last_key={};have_key=false;cached_names={};names_known=false;}
} // namespace nativeequipment_attachment
