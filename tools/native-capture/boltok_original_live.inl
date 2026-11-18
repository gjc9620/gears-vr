// Use the existing authenticated Compose hook, not the decoder: Boltok's
// ObjectXX skeleton has no matching Snub animation tracks to intercept.
namespace boltok_original_live {
namespace api=gears_boltok_original;namespace hand=gears_weapon_hand;
namespace existing=weapon_part_visibility_live;namespace binding=weapon_hand_scene_binding;
namespace observation=gears_boltok_observation;
std::atomic<unsigned> calls{},retained{};std::atomic<bool> stopping{};
bool pending() noexcept {return calls.load()||retained.load();}
// Drain may originate off-GT. Mechanical cache owns no resources and must
// never be mutated here; it dies with the module after ordinary cleanup.
void request_drain() noexcept {stopping.store(true);}
#if defined(GEARS_FULL_BODY_C0_NATIVE_BACKEND) && GEARS_FULL_BODY_C0_NATIVE_BACKEND
struct Bound {
    lancer_original_handle_live::Bound metadata{};api::ComposeLease lease;
    hand::Frame source{};double angle{};bool hidden{},restoring{},published{};
    std::uint64_t writes{};unsigned epoch{};
    gears_boltok_park::Ticket resumed{};gears_boltok_park::State confirmed{};bool confirmed_known{};bool resume_armed{};
};
Bound bound;std::uint64_t next_handle{1},retired_cookie{};api::Receipt retired{};
api::Observation observed{};api::Source observed_metadata{};
bool observation_known{};
void clear_observation() noexcept {observed={};observed_metadata={};observation_known=false;}
struct Call {Call(){++calls;}~Call(){--calls;}};
gears_boltok_park::Store parked;
struct ParkPending {gears_boltok_park::Key key{};gears_boltok_park::State state{};std::uint64_t handle{},cookie{};unsigned thread{};};
ParkPending park_pending;api::Receipt park_ack{};std::uint64_t park_ack_cookie{};unsigned park_ack_thread{};
gears_boltok_park::Key park_key(hand::Frame const& f,hand::ObjectIdentity component,hand::ObjectIdentity asset) noexcept {
    return {f.world,f.pawn,f.weapon,component,asset,f.key.game_start,f.key.native_started_qpc,
        f.key.registration_cookie,f.key.world_generation,f.stereo.session,f.stereo.space_epoch,
        f.controllers.input_epoch,f.controllers.profile_generation,f.key.lifecycle};
}

template<class T>bool read(std::uintptr_t at,T& out) noexcept {return existing::read(at,out);}
bool write(std::uintptr_t at,api::Atom const& value) noexcept {return lancer_original_handle_live::write(at,value);}
bool code_current() noexcept {return cover_pose_phase_live::certified&&cover_pose_phase_live::own_entries();}
bool snapshot_unlocked(lancer_original_handle_live::Bound const& b,api::Source& out) {
    out={};if(GetCurrentThreadId()!=b.thread||!binding::metadata_current()||!binding::current(b.proof))return false;
    for(auto const& id:{b.world,b.pawn,b.weapon,b.component,b.asset,b.node})if(!existing::object(id))return false;
    std::uintptr_t mesh{},asset{},node_mesh{},held{},pawn{};api::part::Array reference{},required{},repeat{},local{},space{};
    if(!read(b.weapon.address+b.mesh.offset,mesh)||mesh!=b.component.address||
       !read(b.weapon.address+b.instigator.offset,pawn)||pawn!=b.pawn.address||
       !read(b.pawn.address+b.pawn_weapon.offset,held)||held!=b.weapon.address||
       !read(b.node.address+0x60,node_mesh)||node_mesh!=b.component.address||
       !read(mesh+0x218,asset)||asset!=b.asset.address||!read(asset+0xa4,reference)||
       reference.count!=12||reference.capacity<12||reference.capacity>512||!reference.data||
       reference.data>UINTPTR_MAX-std::size_t(reference.capacity)*68)return false;
    for(unsigned i=0;i<12;++i){int parent{};
        if(!existing::named_bone(reference.data+i*68,api::labels[i])||
           !read(reference.data+i*68+0x38,parent)||parent!=api::parents[i])return false;}
    if(!read(mesh+0x27c,out.local)||!read(mesh+0x26c,out.space)||!read(mesh+0x338,out.epoch)||
       !read(mesh+0x28c,required)||required.count<1||required.count>12||required.capacity<required.count||
       required.capacity>512||!required.data||required.data>UINTPTR_MAX-std::size_t(required.capacity))return false;
    bool seen[12]{};for(int i=0;i<required.count;++i){unsigned char bone{};
        if(!read(required.data+std::size_t(i),bone)||bone>=12||seen[bone])return false;seen[bone]=true;}
    if(!seen[1]||!seen[2]||!seen[3]||!seen[11])return false;
    out.component=mesh;out.asset=asset;out.node=b.node.address;out.identity_generation=b.handle;out.verified=true;
    return api::valid(out)&&read(asset+0xa4,repeat)&&api::part::same(reference,repeat)&&
        read(mesh+0x27c,local)&&api::part::same(local,out.local)&&read(mesh+0x26c,space)&&
        api::part::same(space,out.space)&&binding::current(b.proof)&&binding::metadata_current();
}
bool snapshot(api::Source& out) noexcept {
    try{std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
        return lock.owns_lock()&&boltok_original_live::snapshot_unlocked(bound.metadata,out);}catch(...){return false;}
}
bool inactive() noexcept {std::uintptr_t a{},b{};const auto& m=bound.metadata;
    const auto world=committed_world_generation.load();if(world&&world!=m.key.world_generation)return true;
    return read(m.pawn.address+m.pawn_weapon.offset,a)&&read(m.pawn.address+m.pawn_weapon.offset,b)&&a==b&&a!=m.weapon.address;
}
void retire(bool off=false) noexcept {
    clear_observation();
    retired={};retired.handle=bound.metadata.handle;retired.writes=bound.writes;
    retired.cpu_restored=true;retired.result=off?api::Result::inactive_restored:api::Result::restored;
    retired_cookie=bound.metadata.key.registration_cookie;bound={};retained.store(0);
}
api::Receipt receipt() noexcept {
    if(!retained.load())return retired;
    api::Receipt r{};r.handle=bound.metadata.handle;r.writes=bound.writes;r.angle=bound.angle;
    r.hidden=bound.hidden;r.epoch=bound.epoch;r.cpu_restored=!bound.lease.active();
    r.result=bound.restoring?api::Result::restoring:bound.published?api::Result::ready:api::Result::pending;return r;
}
void compose_failure_trace(const char* phase,unsigned gate,unsigned& rows,
    std::int64_t entry_ns=0,std::int64_t snapshot_ns=0) noexcept {
    static unsigned total{};
    if(!diagnostic_status||rows>=16||total>=32||(!bound.hidden&&bound.angle==0))return;
    ++rows;++total;
    const auto now=gears_vr::monotonic_ns();
    const std::int64_t sync=bound.source.controllers.sync_ns;
    long long age=-1;
    if(sync>0&&now>=sync)age=static_cast<long long>(now-sync);
    long long entry_age=-1;
    if(sync>0&&entry_ns>=sync)
        entry_age=static_cast<long long>(entry_ns-sync);
    fprintf(diagnostic_status,
        "boltok_compose_failure phase=%s gate=%u source_frame=%llu "
        "source_sequence=%llu source_age_ns=%lld entry_age_ns=%lld "
        "snapshot_ns=%lld handle=%llu angle=%.9g published=%u "
        "restoring=%u observation_known=%u hidden=%u\n",
        phase,gate,
        static_cast<unsigned long long>(bound.source.key.frame),
        static_cast<unsigned long long>(bound.source.controllers.sequence),
        age,entry_age,static_cast<long long>(snapshot_ns),
        static_cast<unsigned long long>(bound.metadata.handle),
        bound.angle,unsigned(bound.published),unsigned(bound.restoring),
        unsigned(observation_known),unsigned(bound.hidden));
    fflush(diagnostic_status);
}

void compose_begin(std::uintptr_t component,std::uintptr_t local,
                   std::uintptr_t required,unsigned kind,
                   std::uintptr_t caller,Ticket& ticket) noexcept {
    static unsigned trace_rows{};
    ticket={};if(kind||!retained.load()||component!=bound.metadata.component.address||
       GetCurrentThreadId()!=bound.metadata.thread||local!=component+0x27c||
       required!=component+0x28c||
       caller!=base+gears_cover_pose_phase::compose_caller)return;
    const auto entry_ns=gears_vr::monotonic_ns();
    try {
        clear_observation();
        if(!code_current()){compose_failure_trace("begin",1,trace_rows);return;}
        if(stopping.load())bound.restoring=true;
        // Negative preflight only: no owned CPU lease or restoration may be
        // skipped. A passing check never replaces the original final admission.
        if(!bound.restoring&&!bound.lease.active()){
            auto* early_broker=weapon_hand_runtime::current();
            if(!early_broker||
               !early_broker->admit_weapon_render(bound.source)){
                bound.published=false;
                compose_failure_trace("begin",9,trace_rows,entry_ns);
                return;
            }
        }
        api::Source source{};
        const auto snapshot_begin=gears_vr::monotonic_ns();
        const bool snapshot_ok=snapshot(source);
        const auto snapshot_ns=
            gears_vr::monotonic_ns()-snapshot_begin;
        if(!snapshot_ok){
            compose_failure_trace("begin",2,trace_rows,entry_ns,snapshot_ns);
            return;
        }
        if(bound.lease.active()){
            bound.restoring=true;
            compose_failure_trace("begin",3,trace_rows);
            return;
        }
        if(bound.resumed&&!bound.resume_armed&&!bound.restoring){
            bound.published=false;
            compose_failure_trace("begin",4,trace_rows);
            return;
        }
        ticket.handle=bound.metadata.handle;ticket.admitted=true;
        bound.published=false;
        if(bound.restoring)return;
        auto* broker=weapon_hand_runtime::current();
        if(!broker){
            bound.published=false;
            compose_failure_trace("begin",5,trace_rows);
            return;
        }
        if(!broker->admit_weapon_render(bound.source)){
            bound.published=false;
            compose_failure_trace("begin",6,trace_rows,entry_ns,snapshot_ns);
            return;
        }
        if(!bound.lease.apply(source,bound.angle,bound.hidden,
            [](auto at,auto& v){return read(at,v);},write)){
            bound.restoring=true;
            ticket.changed=bound.lease.active();
            compose_failure_trace("begin",7,trace_rows);
            return;
        }
        ticket.changed=bound.lease.active();
    }catch(...){bound.restoring=true;}
}

void compose_end(Ticket const& ticket,bool normal) noexcept {
    static unsigned trace_rows{};
    if(!ticket.admitted||!retained.load()||
       ticket.handle!=bound.metadata.handle||
       GetCurrentThreadId()!=bound.metadata.thread)return;
    clear_observation();
    try {
        api::Source s{};
        if(!snapshot(s)){
            bound.restoring=true;
            compose_failure_trace("end",1,trace_rows);
            return;
        }
        if(bound.lease.active()&&!bound.lease.restore(s,
            [](auto at,auto& v){return read(at,v);},write)){
            bound.restoring=true;
            compose_failure_trace("end",2,trace_rows);
            return;
        }
        if(!normal){
            bound.restoring=true;
            compose_failure_trace("end",3,trace_rows);
            return;
        }
        if(bound.restoring){
            std::array<float,16> a{},b{},r{};api::Matrix support{},cylinder{},root{};
            if(!read(s.space.data+64,a)||!read(s.space.data+3*64,b)||!read(s.space.data,r))return;
            std::copy(a.begin(),a.end(),support.begin());std::copy(b.begin(),b.end(),cylinder.begin());
            std::copy(r.begin(),r.end(),root.begin());if(!api::math::proper(root))return;
            const auto& p=bound.lease.plan();
            const auto closed=api::math::multiply(api::math::matrix(p.support,p.conjugated),root);
            if(api::math::close(support,closed)&&api::math::close(cylinder,
                api::math::multiply(api::math::matrix(p.cylinder,p.conjugated),closed)))retire();
            return;
        }
        std::array<float,16> raw{},root_raw{};
        api::Matrix actual{},root{};api::Atom target{};
        if(!read(s.space.data+64,raw)||!read(s.space.data,root_raw)||
           !bound.lease.plan().target(bound.angle,target)){
            compose_failure_trace("end",8,trace_rows);
            return;
        }
        std::copy(raw.begin(),raw.end(),actual.begin());
        std::copy(root_raw.begin(),root_raw.end(),root.begin());
        if(!api::math::proper(root)){
            compose_failure_trace("end",9,trace_rows);
            return;
        }
        const auto expected=api::math::multiply(
            api::math::matrix(target,bound.lease.plan().conjugated),root);
        std::array<float,16> cylinder_raw{};api::Matrix cylinder_actual{};
        if(!read(s.space.data+3*64,cylinder_raw)){
            compose_failure_trace("end",10,trace_rows);
            return;
        }
        std::copy(cylinder_raw.begin(),cylinder_raw.end(),cylinder_actual.begin());
        auto cylinder_expected=api::math::multiply(
            api::math::matrix(bound.lease.plan().cylinder,
                bound.lease.plan().conjugated),expected);
        if(bound.hidden)for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)
            cylinder_expected[i*4+j]=0;
        if(!api::math::close(actual,expected)){
            compose_failure_trace("end",11,trace_rows);
            return;
        }
        if(!api::math::close(cylinder_actual,cylinder_expected)){
            compose_failure_trace("end",12,trace_rows);
            return;
        }
        bound.published=true;bound.epoch=s.epoch;++bound.writes;
        bound.confirmed={bound.angle,bound.hidden,s.epoch};
        bound.confirmed_known=true;
        if(bound.resumed){parked.consume(bound.resumed);bound.resumed={};}
        api::Observation candidate{};
        candidate.handle=bound.metadata.handle;
        candidate.publish_serial=bound.writes;
        candidate.epoch=bound.epoch;candidate.source_epoch=s.epoch;
        candidate.observed_ns=gears_vr::monotonic_ns();
        candidate.source=bound.source;
        candidate.component=bound.metadata.component;
        candidate.asset=bound.metadata.asset;
        candidate.node=bound.metadata.node;
        candidate.root=root;candidate.support=actual;
        candidate.cylinder=cylinder_actual;candidate.hidden=bound.hidden;
        candidate.published=true;candidate.verified=true;
        if(candidate.observed_ns>0&&candidate.epoch&&
           observation::matrices_valid(candidate)){
            observed=candidate;observed_metadata=s;observation_known=true;
        }else compose_failure_trace("end",13,trace_rows);
    }catch(...){bound.restoring=true;}
}

api::Result begin(api::Request const* input,api::Receipt* output) noexcept {
    Call call;api::Request q{};
    try {auto* broker=weapon_hand_runtime::current();
        if(!input||!output||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=1||
           !broker||stopping.load()||retained.load()||!next_handle||next_handle==UINT64_MAX||
           existing::pending()||lancer_original_handle_live::pending()||!code_current()||
           !broker->admit_weapon_render(q.source))return api::Result::rejected;
        Bound candidate{};auto& b=candidate.metadata;b.key=q.source.key;b.world=q.source.world;b.pawn=q.source.pawn;
        b.weapon=q.source.weapon;b.component=q.component;b.asset=q.asset;b.node=q.node;
        b.thread=GetCurrentThreadId();b.handle=next_handle++;candidate.source=q.source;
        {std::unique_lock<std::mutex> lock(binding::mutex,std::try_to_lock);
            if(!lock.owns_lock()||!binding::metadata_current())return api::Result::rejected;
            for(auto const& id:{b.world,b.pawn,b.weapon,b.component,b.asset,b.node})if(!existing::object(id))return api::Result::rejected;
            gears_anchor::Identity asset{},package{};
            if(!existing::derives(b.weapon,L"Weap_LocustPistol",L"WarfareGame")||
               !existing::derives(b.component,L"SkeletalMeshComponent",L"Engine")||
               !existing::derives(b.asset,L"SkeletalMesh",L"Engine")||!existing::derives(b.node,L"AnimNodeSequence",L"Engine")||
               !binding::identity(b.asset.address,asset,true)||!binding::named(asset,L"Boltock_Pistol")||
               !binding::identity(asset.outer,package,true)||!binding::named(package,L"Geist_Pistol")||package.outer||
               !lancer_original_handle_live::properties(b))return api::Result::rejected;
            api::Source s{};if(!boltok_original_live::snapshot_unlocked(b,s)||!candidate.lease.bind(s,[](auto at,auto& v){return read(at,v);}))return api::Result::rejected;
        }
        parked.prune([&](gears_boltok_park::Key const& key) noexcept {
            return key.game!=q.source.key.game_start||key.native!=q.source.key.native_started_qpc||
                key.registration!=q.source.key.registration_cookie||key.world_generation!=q.source.key.world_generation;
            // A failed object read is NOT proof of retirement. Keep same-world
            // data on unknown reads; bounded capacity fails closed, no eviction.
        });
        gears_boltok_park::State saved{};
        if(parked.lookup(park_key(q.source,q.component,q.asset),candidate.resumed,saved)){
            // Restoring our own prior confirmed mechanical value is not a new
            // hand action. The NEW binding/source still passes all original
            // object/geometry/broker checks; no saved Frame or lease is used.
            candidate.angle=saved.angle;candidate.hidden=saved.hidden;
        }
        // Keep final admission AFTER all cache/retirement metadata work.
        // No old currentness result may authorize the new binding.
        if(!broker->admit_weapon_render(q.source))return api::Result::rejected;
        clear_observation();bound=candidate;retained.store(1);auto r=receipt();
        if(!safe_copy(output,&r,sizeof r)){bound.restoring=true;return api::Result::restoring;}return r.result;
    }catch(...){if(retained.load())bound.restoring=true;return api::Result::rejected;}
}
api::Result update(api::Control const* input,api::Receipt* output,bool restore) noexcept {
    Call call;api::Control q{};
    try {
        if(!input||!output||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=1||!q.handle||!q.cookie)return api::Result::rejected;
        if(!retained.load()){
            if(q.handle!=retired.handle||q.cookie!=retired_cookie)return api::Result::rejected;
            return safe_copy(output,&retired,sizeof retired)?retired.result:api::Result::rejected;
        }
        if(q.handle!=bound.metadata.handle||q.cookie!=bound.metadata.key.registration_cookie||GetCurrentThreadId()!=bound.metadata.thread)return api::Result::rejected;
        if(restore||stopping.load()){clear_observation();bound.restoring=true;}
        else {
            auto* b=weapon_hand_runtime::current();
            if(bound.restoring||!b||!hand::same(q.source.weapon,bound.metadata.weapon)||
               !hand::same(q.source.world,bound.metadata.world)||!hand::same(q.source.pawn,bound.metadata.pawn)||
               !b->admit_weapon_render(q.source)||!std::isfinite(q.angle)||q.angle<0||q.angle>api::maximum_angle||
               ((q.angle!=bound.angle||q.hide_cylinder!=bound.hidden)&&!b->admit_part_visibility(q.source)))return api::Result::rejected;
            if(bound.resumed&&!bound.published&&(q.angle!=bound.angle||q.hide_cylinder!=bound.hidden))return api::Result::rejected;
            if(q.angle!=bound.angle||q.hide_cylinder!=bound.hidden){bound.published=false;clear_observation();}
            if(!observation::same_domain(q.source,bound.source))clear_observation();
            bound.source=q.source;bound.angle=q.angle;bound.hidden=q.hide_cylinder;
            if(bound.resumed)bound.resume_armed=true; // All original identity/GT/current-source/value guards passed.
        }
        if(bound.restoring&&!bound.lease.active()&&inactive())retire(true);
        auto r=receipt();return safe_copy(output,&r,sizeof r)?r.result:api::Result::rejected;
    }catch(...){if(retained.load())bound.restoring=true;return api::Result::rejected;}
}
bool observed_root_current(api::Source const& source,api::Matrix const& expected) noexcept {
    std::array<float,16> raw{};api::Matrix value{};
    if(!read(source.space.data,raw))return false;
    std::copy(raw.begin(),raw.end(),value.begin());
    return api::math::proper(value)&&api::math::close(value,expected);
}
bool read_observation(api::ObservationRequest const* input,api::Observation* output) noexcept {
    Call call;api::ObservationRequest q{};
    static unsigned diagnostic_rows{};
    bool request_ready=false,correct_thread=false;
    bool first_snapshot=false,final_snapshot=false;
    api::Source first{},final{};
    const auto failure=[&](unsigned gate) noexcept {
        if(diagnostic_status&&request_ready&&correct_thread&&diagnostic_rows<16){
            ++diagnostic_rows;
            const auto now=gears_vr::monotonic_ns();
            const auto age=[now](std::int64_t stamp) noexcept -> long long {
                if(stamp<=0||now<stamp)return -1;
                return static_cast<long long>(now-stamp);
            };
            fprintf(diagnostic_status,
                "boltok_observation_rejected gate=%u source_frame=%llu "
                "source_sequence=%llu published=%u restoring=%u lease=%u "
                "known=%u saved_epoch=%u current_epoch=%u "
                "observed_age_ns=%lld source_age_ns=%lld "
                "first_snapshot=%u final_snapshot=%u\n",
                gate,
                static_cast<unsigned long long>(q.source.key.frame),
                static_cast<unsigned long long>(q.source.controllers.sequence),
                unsigned(bound.published),unsigned(bound.restoring),
                unsigned(bound.lease.active()),unsigned(observation_known),
                unsigned(observed.epoch),unsigned(bound.epoch),
                age(observed.observed_ns),
                age(static_cast<std::int64_t>(q.source.controllers.sync_ns)),
                unsigned(first_snapshot),unsigned(final_snapshot));
            fflush(diagnostic_status);
        }
        return false;
    };
    try {
        if(!input||!output)return failure(1);
        if(!safe_copy(&q,input,sizeof q)||!observation::request_shape(q))return failure(1);
        request_ready=true;
        correct_thread=GetCurrentThreadId()==bound.metadata.thread;
        if(stopping.load()||!retained.load()||!observation_known||
           q.handle!=bound.metadata.handle||
           q.cookie!=bound.metadata.key.registration_cookie)return failure(2);
        if(GetCurrentThreadId()!=bound.metadata.thread)return failure(3);
        correct_thread=true;
        if(!code_current())return failure(4);
        if(!bound.published||bound.restoring||bound.lease.active())
            return failure(5);

        auto* broker=weapon_hand_runtime::current();
        if(!broker)return failure(6);
        if(!broker->admit_weapon_render(q.source))return failure(7);

        first_snapshot=snapshot(first);
        if(!first_snapshot)return failure(8);

        const auto candidate=observed;
        if(!hand::same(candidate.node,bound.metadata.node))return failure(9);
        if(!observed_root_current(first,candidate.root))return failure(10);
        if(!observation::accepted(q,bound.source,bound.metadata.component,
              bound.metadata.asset,observed_metadata,first,candidate,
              gears_vr::monotonic_ns()))return failure(11);
        if(bound.epoch!=candidate.epoch)return failure(12);

        final_snapshot=snapshot(final);
        if(!final_snapshot)return failure(13);
        if(!observation::metadata_same(first,final))return failure(14);
        if(!observed_root_current(final,candidate.root))return failure(15);
        if(!broker->admit_weapon_render(q.source))return failure(16);

        if(stopping.load()||!retained.load())return failure(17);
        if(!bound.published||bound.restoring||bound.lease.active())
            return failure(18);
        if(bound.epoch!=candidate.epoch)return failure(19);
        if(!observation::accepted(q,bound.source,bound.metadata.component,
              bound.metadata.asset,observed_metadata,final,candidate,
              gears_vr::monotonic_ns()))return failure(20);

        if(!safe_copy(output,&candidate,sizeof candidate))return failure(21);
        return true;
    }catch(...){return failure(22);}
}

api::Result park(api::Control const* input,api::Receipt* output) noexcept {
    Call call;api::Control q{};
    try {
        if(!input||!output||stopping.load()||!safe_copy(&q,input,sizeof q)||q.size!=sizeof q||q.version!=1||!q.handle||!q.cookie)return api::Result::rejected;
        if(q.handle==park_ack.handle&&q.cookie==park_ack_cookie)
            return GetCurrentThreadId()==park_ack_thread&&safe_copy(output,&park_ack,sizeof park_ack)?park_ack.result:api::Result::rejected;
        if(!park_pending.handle){
            if(!retained.load()||q.handle!=bound.metadata.handle||q.cookie!=bound.metadata.key.registration_cookie||
               GetCurrentThreadId()!=bound.metadata.thread||bound.lease.active())return api::Result::rejected;
            gears_boltok_park::State value=bound.confirmed;
            // A freshly bound, never-moved original has only its certified
            // closed reference. Never turn an unconfirmed nonzero command
            // into a persisted endpoint.
            if(!bound.confirmed_known){
                if(bound.resumed){
                    gears_boltok_park::Ticket inherited{};
                    if(!parked.lookup(park_key(bound.source,bound.metadata.component,bound.metadata.asset),inherited,value)||
                       inherited.slot!=bound.resumed.slot||inherited.generation!=bound.resumed.generation||
                       value.angle!=bound.angle||value.hidden!=bound.hidden)return api::Result::rejected;
                }else{if(bound.angle!=0||bound.hidden)return api::Result::rejected;value={};}
            }
            park_pending={park_key(bound.source,bound.metadata.component,bound.metadata.asset),value,q.handle,q.cookie,GetCurrentThreadId()};
        }
        if(q.handle!=park_pending.handle||q.cookie!=park_pending.cookie||GetCurrentThreadId()!=park_pending.thread)return api::Result::rejected;
        api::Receipt r{};const auto result=update(&q,&r,true);
        if(result!=api::Result::restored&&result!=api::Result::inactive_restored)
            return safe_copy(output,&r,sizeof r)?result:api::Result::rejected;
        if(!r.cpu_restored||r.handle!=q.handle||r.result!=result||retained.load()||
           !parked.save(park_pending.key,park_pending.state,api::maximum_angle))return api::Result::rejected;
        r.angle=park_pending.state.angle;r.hidden=park_pending.state.hidden;r.epoch=park_pending.state.verified_epoch;
        park_ack=r;park_ack_cookie=q.cookie;park_ack_thread=park_pending.thread;park_pending={};
        return safe_copy(output,&r,sizeof r)?r.result:api::Result::rejected;
    }catch(...){return api::Result::rejected;}
}
#else
void compose_begin(std::uintptr_t,std::uintptr_t,std::uintptr_t,unsigned,std::uintptr_t,Ticket&) noexcept {}
void compose_end(Ticket const&,bool) noexcept {}
api::Result begin(api::Request const*,api::Receipt*) noexcept {return api::Result::rejected;}
api::Result park(api::Control const*,api::Receipt*) noexcept {return api::Result::rejected;}
api::Result update(api::Control const*,api::Receipt*,bool) noexcept {return api::Result::rejected;}
bool read_observation(api::ObservationRequest const*,api::Observation*) noexcept {return false;}
#endif
}
