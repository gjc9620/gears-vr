namespace handrender {
namespace wire=gears_committed_hand_consumer;
thread_local bool entered{};
std::atomic<unsigned long long> notices{},applied{},revoked{};
wire::Result consume(wire::Notice const& notice) noexcept {
    const auto& f=notice.frame;++notices;
    if(notice.kind==wire::Kind::revoke){
        if(nativelancerfollow::active())++revoked;
        nativelancerfollow::pre_original_tick(L"pre_compose");
        return nativelancerfollow::faulted?wire::Result::fault:wire::Result::unchanged;
    }
    nativeequipment::ConfigurationRead configuration;
    if(!configuration.held||!enabled.load(std::memory_order_acquire)||
       weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::active){
        nativelancerfollow::render_rejected(f,7,unsigned(!configuration.held)|
            unsigned(!enabled.load(std::memory_order_acquire))*2|
            unsigned(weaponhand::client.phase()!=gears_weapon_hand::ClientPhase::active)*4);
        return wire::Result::unchanged;
    }
    gears_weapon_hand::Frame magazine_source{};
    const bool magazine_ready=f.purpose==gears_committed_hand::Purpose::held_lancer&&
        lancerreload_runtime::prepare_render_result(f,magazine_source);
    const bool gun_changed=nativelancerfollow::on_render_result(f);
    // The same acquired magazine writer follows this completed hand pose
    // before attachment matrices are consumed, just as the held gun does.
    // This callback performs no reload transition or inventory action.
    const bool magazine_changed=magazine_ready&&lancerreload_runtime::on_render_result(f,magazine_source);
    const bool changed=gun_changed||magazine_changed;
    if(changed)++applied;
    if(log_file&&notices.load()<=12){fwprintf(log_file,L"native_hand_visual serial=%llu applied=%u notices=%llu total_applied=%llu revoked=%llu\n",
        f.serial,unsigned(changed),notices.load(),applied.load(),revoked.load());fflush(log_file);}
    return nativelancerfollow::faulted?wire::Result::fault:changed?wire::Result::applied:wire::Result::unchanged;
}
wire::Result __cdecl notify(void* context,wire::Notice const* input) noexcept {
    wire::Notice notice{};
    if(context!=&weaponhand::client||!input||!copy(&notice,reinterpret_cast<uptr>(input),sizeof notice)||
       notice.size!=sizeof notice||notice.abi_version!=wire::version||notice.reserved||
       unsigned(notice.kind)>unsigned(wire::Kind::committed)||entered||!weaponhand::current_gt(nullptr))return wire::Result::fault;
    const auto& f=notice.frame;
    if(f.parent.registration_cookie!=weaponhand::client.cookie()||f.parent.native_started_qpc!=weaponhand::client.native_started_qpc())return wire::Result::unchanged;
    // Parent owns the module pin and actual callback count through this entire
    // synchronous scope. PG additionally retains its existing shutdown count.
    callbacks.fetch_add(1,std::memory_order_acq_rel);
    const bool previous_inside=inside;entered=true;inside=true;
    __try {
        return consume(notice);
    }__finally {inside=previous_inside;entered=false;callbacks.fetch_sub(1,std::memory_order_release);}
}
bool attach_call(wire::Attach fn,wire::Registration const& registration,gears_weapon_hand::RegistrationResult& result,gears_weapon_hand::Reply& reply) noexcept {
    __try {reply=fn(&registration,&result);return true;}
    __except(GetExceptionCode()==0xe06d7363ul||GetExceptionCode()==EXCEPTION_STACK_OVERFLOW?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}
}
gears_weapon_hand::Reply attach(HMODULE native) noexcept {
    using Reply=gears_weapon_hand::Reply;
    if(weaponhand::current_gt(nullptr)||native!=weaponhand::native_module||!native)return Reply::unavailable;
    const auto entry=GetProcAddress(native,wire::attach_export);
    if(!weaponhand::executable(entry,native))return Reply::unavailable;
    wire::Registration registration{};registration.hand_cookie=weaponhand::client.cookie();
    registration.native_started_qpc=weaponhand::client.native_started_qpc();registration.context=&weaponhand::client;registration.notify=notify;
    gears_weapon_hand::RegistrationResult result{};Reply reply=Reply::unavailable;
    if(!attach_call(reinterpret_cast<wire::Attach>(entry),registration,result,reply))return Reply::unavailable;
    if(reply!=Reply::accepted)return reply;
    return result.size==sizeof result&&result.abi_version==gears_weapon_hand::version&&
        result.cookie==registration.hand_cookie&&result.native_started_qpc==registration.native_started_qpc?Reply::accepted:Reply::invalid;
}
}
