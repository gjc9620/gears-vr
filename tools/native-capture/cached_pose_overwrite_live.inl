// Included inside fresh_pose_producer_live, after both original code guards.
namespace cached=gears_cached_pose_overwrite;
struct CachedWitness {cached::Source before{};CopyImport copy{};gears_anchor::Identity identity{};};
std::atomic<uint64_t> cached_candidates{},cached_started{},cached_finished{};
std::array<std::atomic<uint64_t>,8> cached_rejected{};
std::atomic<uintptr_t> cached_import_target{};
std::atomic<uint64_t> cover_forward_total{},cover_forward_mirrored{},cover_forward_per_bone{};
void observe_cover_forward(gears_root_blend_overwrite::Source const& source){
    auto const& current=hand::slot.data.record.source;
    if(!(current.flags&gears_rig_tracking::cover_key_known)||!current.cover_type||current.mesh.address!=source.mesh||
       !source.mesh_epoch||source.root_epoch>=source.mesh_epoch)return;
    float full{};memcpy(&full,&source.full,4);
    for(unsigned i=0;i<unsigned(source.child_count);++i){float weight{};memcpy(&weight,&source.child[i].weight,4);
        if(std::isfinite(weight)&&weight>=full){
            ++cover_forward_total;if(source.child[i].mirror&1)++cover_forward_mirrored;
            uintptr_t table{},method{};if(read(table,source.child[i].node)&&field(method,table,0x270)&&method==base+0xfacb0)++cover_forward_per_bone;
            break;
        }
    }
}
bool cached_import_current(CopyImport& value){
    copy_import(value);
    const auto module=GetModuleHandleW(L"VCRUNTIME140.dll");
    const auto function=module?GetProcAddress(module,"memcpy"):nullptr;
    wchar_t system[MAX_PATH]{},expected[MAX_PATH]{};
    const auto length=GetSystemDirectoryW(system,MAX_PATH);
    const bool path=length&&length<MAX_PATH&&swprintf_s(expected,L"%s\\VCRUNTIME140.dll",system)>0&&
        !_wcsicmp(expected,value.module_path.data());
    const bool current=value.read_mask==63&&value.slot==base+0x131ca18&&module&&function&&path&&
        value.module_base==reinterpret_cast<uintptr_t>(module)&&value.target==reinterpret_cast<uintptr_t>(function)&&
        gears_cached_memcpy_certificate::current(safe_copy,value.module_base,value.target);
    if(current)cached_import_target=value.target;return current;
}
bool cached_code_current(unsigned kind){return kind==2?per_bone_code_current():kind==0&&root_code_current();}
bool cached_begin(Call& c,pose::CallPacket const& packet){
    const auto mesh=native_atom_overwrite_live::registered_mesh();cached::Header selected{};
    if(!enabled||!sequence_witness_enabled||(c.kind!=0&&c.kind!=2)||!mesh||local.depth>32||
       !cached::header(safe_copy,base,packet.gpr[1],mesh,packet.gpr[2],packet.gpr[3],c.kind,selected))return false;
    ++cached_candidates;c.cached=new(std::nothrow) CachedWitness;
    if(!c.cached){++cached_rejected[0];return true;}
    c.blend_scope=::new(static_cast<void*>(c.blend_scope_bytes))::Scope;
    if(!c.blend_scope->world()){++cached_rejected[1];return true;}
    auto& witness=*c.cached;cached::Source repeat{};
    if(!cached::snapshot(safe_copy,base,selected,witness.before)||!cached::snapshot(safe_copy,base,selected,repeat)||
       !cached::same(witness.before,repeat)){++cached_rejected[2];return true;}
    if(!cached_code_current(c.kind)||!cached_import_current(witness.copy)){++cached_rejected[3];return true;}
    {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        if(!lock.owns_lock()||!binding::metadata_current()||!binding::identity(selected.node,witness.identity,true)){++cached_rejected[4];return true;}}
    if(!native_atom_overwrite_live::blend_begin(c.blend_ticket,selected.mesh_epoch)){++cached_rejected[5];return true;}
    c.blend_active=true;++cached_started;return true;
}
bool cached_return(Call& c,bool returned){
    if(!c.cached)return false;if(!c.blend_active)return true;
    auto const& witness=*c.cached;cached::Source after{},repeat{};CopyImport current{};
    bool valid=returned&&blend_world_current()&&cached_code_current(c.kind)&&cached_import_current(current)&&
        current.target==witness.copy.target&&current.module_base==witness.copy.module_base&&current.prefix==witness.copy.prefix&&
        cached::snapshot(safe_copy,base,witness.before.header,after)&&cached::snapshot(safe_copy,base,witness.before.header,repeat)&&
        cached::same(after,repeat)&&cached::returned(witness.before,after);
    if(valid){std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);gears_anchor::Identity id{};
        valid=lock.owns_lock()&&binding::metadata_current()&&binding::identity(witness.before.header.node,id,true)&&binding::same(id,witness.identity);}
    // Only the attested original cached branch + current CRT memcpy export +
    // normal return and stable canonical arrays prove the full 115*32 store.
    // Data equality is an additional check, never the source of authority.
    if(native_atom_overwrite_live::blend_return(c.blend_ticket,valid))++cached_finished;else ++cached_rejected[6];
    c.blend_active=false;return true;
}
void cached_summary(FILE* file){if(!file)return;
    fprintf(file,"cached_pose_candidates=%llu cached_pose_started=%llu cached_pose_finished=%llu cached_pose_memcpy=%llu ",
        cached_candidates.load(),cached_started.load(),cached_finished.load(),static_cast<unsigned long long>(cached_import_target.load()));
    fprintf(file,"cover_forward_total=%llu cover_forward_mirrored=%llu cover_forward_per_bone=%llu ",
        cover_forward_total.load(),cover_forward_mirrored.load(),cover_forward_per_bone.load());
    for(unsigned i=0;i<cached_rejected.size();++i)fprintf(file,"cached_pose_rejected_%u=%llu ",i,cached_rejected[i].load());
}
