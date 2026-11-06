// Included inside first_person_head_render_live after the existing mask.
// Same Update/Upload owner, no new hook or material/asset/skeleton write.
namespace head_section {
namespace api=gears_first_person_head_section;
bool enabled{};
std::atomic<uint64_t> attempted{},applied{},already{},failed_writes{};
std::array<std::atomic<uint64_t>,10> rejected{};
void reject(unsigned n){if(n<rejected.size())++rejected[n];}
bool certificate() {
    namespace cert=gears_first_person_head_section_certificate;
    std::array<unsigned char,4096> bytes{};bone::rig::Signature digest{};
    return cert::reader_bytes<=bytes.size()&&safe_copy(bytes.data(),reinterpret_cast<void const*>(base+cert::reader_rva),cert::reader_bytes)&&
        first_person_live::authored_digest(bytes.data(),cert::reader_bytes,digest)&&digest==cert::reader_sha256;
}
// Same bounded diagnostic row as the existing Update/palette observer. Read
// the ACTUAL per-chunk shader constants after the original upload, rather
// than treating its full-skeleton input array as proof of consumed fingers.
// No writes or feature grants; unavailable/moved headers leave valid bits off.
void observe_hand_shader(uintptr_t gpu,uintptr_t data,Context const& c,
    raw::ArrayHeader const& chunks,std::array<uintptr_t,4> const& factories) {
    const auto index=c.trace_observation_index;
    if(!rig_tracking_live::metadata_reject_trace||index>=palette_observations.size())return;
    auto& row=palette_observations[index];
    if(row.gpu!=gpu||row.data!=data||row.mesh!=c.owner.key.mesh.address)return;
    for(unsigned chunk=0;chunk<factories.size();++chunk){
        const auto factory=factories[chunk];raw::ArrayHeader map{},palette{};unsigned slot{};
        std::array<uint16_t,bone::bones> bones{};
        if(!binding::field(map,chunks.data+chunk*68,40)||map.count<=0||map.count>int(bone::bones)||
           !raw::header_valid(map,2)||!safe_copy(bones.data(),reinterpret_cast<void const*>(map.data),map.count*2)||
           !binding::field(slot,factory,828)||slot>1||!binding::field(palette,factory,784+16*slot)||
           palette.count!=map.count||!raw::header_valid(palette,sizeof(api::Matrix)))continue;
        for(unsigned j=0;j<trace_bones.size();++j){
            const auto wanted=trace_bones[j];
            if(wanted<35||wanted>50||(row.shader_valid_mask&(1u<<j)))continue;
            const auto end=bones.begin()+map.count;const auto found=std::find(bones.begin(),end,uint16_t(wanted));
            if(found==end)continue;
            api::Matrix observed{};raw::ArrayHeader closing_map{},closing_palette{};unsigned closing_slot{};uintptr_t installed{};
            const auto offset=unsigned(found-bones.begin())*sizeof(api::Matrix);
            if(!safe_copy(&observed,reinterpret_cast<void const*>(palette.data+offset),sizeof observed)||
               !std::all_of(observed.values.begin(),observed.values.end(),[](float v){return std::isfinite(v);})||
               !binding::field(closing_map,chunks.data+chunk*68,40)||!raw::header_equal(map,closing_map)||
               !binding::field(closing_slot,factory,828)||closing_slot!=slot||
               !binding::field(closing_palette,factory,784+16*slot)||!raw::header_equal(palette,closing_palette)||
               !binding::field(installed,gpu,0x50)||installed!=data||!recording.load()||phase::stopping.load())continue;
            row.shader[j]=observed;row.shader_chunk[j]=chunk;row.shader_slot[j]=slot;row.shader_valid_mask|=1u<<j;
        }
    }
}
void after_upload(uintptr_t gpu,uintptr_t data,Context const& c) {
    if(!enabled||!c.valid)return;
    ++attempted;
    // Consume only the just-masked payload from THIS existing upload. The
    // original producer/owner clock remains unchanged, including its100ms cap.
    LARGE_INTEGER frequency{};const auto at=phase::qpc();
    if(!recording.load()||phase::stopping.load()||gpu!=c.gpu||selected_gpu.load()!=gpu||
       c.owner.key.session!=uint64_t(session_started_qpc)||c.owner.key.world_generation!=committed_world_generation.load()||
       !QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0||at<c.update_qpc||at-c.update_qpc>frequency.QuadPart/10){reject(1);return;}
    uintptr_t installed{},asset{},owner_gpu{};int lod{},morphs{},special_factory{};
    raw::ArrayHeader asset_lods{},gpu_lods{};
    if(!binding::field(installed,gpu,0x50)||installed!=data||!binding::field(asset,gpu,0x14)||asset!=c.asset||
       !binding::field(owner_gpu,c.owner.key.mesh.address,0x260)||owner_gpu!=gpu||
       !binding::field(lod,data,40)||lod!=0||!binding::field(morphs,data,60)||morphs!=0||
       !binding::field(special_factory,gpu,36)||special_factory!=0||
       !binding::field(asset_lods,asset,0xd4)||asset_lods.count!=1||!raw::header_valid(asset_lods,8)||
       !binding::field(gpu_lods,gpu,44)||gpu_lods.count!=1||!raw::header_valid(gpu_lods,152)){reject(2);return;}
    uintptr_t model{};raw::ArrayHeader sections{},chunks{},map{};
    if(!binding::read(model,asset_lods.data)||!model||!binding::read(sections,model)||sections.count!=4||!raw::header_valid(sections,12)||
       !binding::field(chunks,model,16)||chunks.count!=4||!raw::header_valid(chunks,68)){reject(3);return;}
    constexpr std::array<unsigned,4> starts{{0,76089,76359,107031}},triangles{{25363,90,10224,960}};
    for(unsigned i=0;i<4;++i){
        const auto p=sections.data+i*12;unsigned first{};unsigned short mat{},which{},count{};
        if(!binding::read(mat,p)||mat!=i||!binding::field(which,p,2)||which!=i||
           !binding::field(first,p,4)||first!=starts[i]||!binding::field(count,p,8)||count!=triangles[i]){reject(3);return;}
    }
    std::array<std::uint16_t,api::bone_map.size()> actual_map{};unsigned influences{};
    const auto head_chunk=chunks.data+api::chunk*68;
    if(!binding::field(map,head_chunk,40)||map.count!=int(actual_map.size())||!raw::header_valid(map,2)||
       !safe_copy(actual_map.data(),reinterpret_cast<void const*>(map.data),sizeof actual_map)||actual_map!=api::bone_map||
       !binding::field(influences,head_chunk,64)||influences!=4){reject(4);return;}
    raw::ArrayHeader factories{};
    std::array<uintptr_t,4> vf{};
    if(!binding::field(factories,gpu_lods.data,72)||factories.count!=4||!raw::header_valid(factories,8)||
       !safe_copy(vf.data(),reinterpret_cast<void const*>(factories.data),sizeof vf)){reject(5);return;}
    for(unsigned i=0;i<vf.size();++i)if(!vf[i]||vf[i]%8||(i!=api::chunk&&vf[i]==vf[api::chunk])){reject(5);return;}
    observe_hand_shader(gpu,data,c,chunks,vf);
    unsigned slot{},shader_influences{};raw::ArrayHeader palette{};
    const auto factory=vf[api::chunk];
    if(!binding::field(slot,factory,828)||slot>1||!binding::field(shader_influences,factory,816)||shader_influences!=influences||
       !binding::field(palette,factory,784+16*slot)||palette.count!=int(api::bone_map.size())||!raw::header_valid(palette,sizeof(api::Matrix))){reject(6);return;}
    raw::ArrayHeader source{};
    if(!binding::field(source,data,8)||source.count!=int(bone::bones)||!raw::header_valid(source,64)){reject(6);return;}
    using bone::d::range;
    const bone::d::Range buffers[]={range(reinterpret_cast<void const*>(palette.data),sizeof(api::Palette)),
        range(reinterpret_cast<void const*>(source.data),bone::bones*64),range(reinterpret_cast<void const*>(c.space.data),bone::bones*64),
        range(reinterpret_cast<void const*>(c.local.data),bone::bones*32),range(reinterpret_cast<void const*>(c.inverse.data),bone::bones*64),
        range(reinterpret_cast<void const*>(c.reference.data),bone::bones*68)};
    if(!bone::d::disjoint(buffers,6)){reject(6);return;}
    api::Palette before{},wanted{},check{};
    if(!safe_copy(before.data(),reinterpret_cast<void const*>(palette.data),sizeof before)||!api::plan(before,wanted)){reject(7);return;}
    // Reobserve every owning header after staging, before any store. No
    // pointer into the shared mesh or ordinary full-body palette is written.
    const auto current=[&]() {
        uintptr_t d{},a{},m{},f{};unsigned s{};raw::ArrayHeader p{},x{},y{},z{};
        return recording.load()&&!phase::stopping.load()&&c.owner.key.world_generation==committed_world_generation.load()&&
            selected_gpu.load()==gpu&&binding::field(d,gpu,0x50)&&d==data&&binding::field(a,gpu,0x14)&&a==asset&&
            binding::field(m,c.owner.key.mesh.address,0x260)&&m==gpu&&binding::field(s,factory,828)&&s==slot&&
            binding::field(p,factory,784+16*slot)&&raw::header_equal(p,palette)&&
            binding::field(x,asset,0xd4)&&raw::header_equal(x,asset_lods)&&binding::field(y,gpu,44)&&raw::header_equal(y,gpu_lods)&&
            binding::field(z,gpu_lods.data,72)&&raw::header_equal(z,factories)&&
            binding::read(f,factories.data+api::chunk*8)&&f==factory;
    };
    if(!current()){reject(8);return;}
    if(before[api::spine_slot].values==wanted[api::spine_slot].values){++already;return;}
    const auto destination=palette.data+api::spine_slot*sizeof(api::Matrix);
    if(!writable_store(destination,&wanted[api::spine_slot],sizeof(api::Matrix))||
       !safe_copy(check.data(),reinterpret_cast<void const*>(palette.data),sizeof check)||
       std::memcmp(check.data(),wanted.data(),sizeof check)||!current()){
        ++failed_writes;
        // Same serial upload, before the shader reader can consume it. A
        // failed partial store must be restored; never leave a masked body
        // matrix or silently claim successful cleanup.
        api::Matrix restored{};
        if(!current()||!writable_store(destination,&before[api::spine_slot],sizeof(api::Matrix))||
           !safe_copy(&restored,reinterpret_cast<void const*>(destination),sizeof restored)||restored.values!=before[api::spine_slot].values)
            request_session_stop(SessionStop::proof_failure);
        return;
    }
    ++applied;
}
}
