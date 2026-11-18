// Inside Broker's public section. The mounted provider is supplemental to the
// existing registration/pin; it cannot publish a normal Frame or authorize it.
Reply attach_troika_geometry(troika_geometry::Registration const* input,RegistrationResult* output,bool enabled) {
    if(reentrant())return Reply::reentrant;
    troika_geometry::Registration r{};RegistrationResult result{};
    if(!enabled||!weapon_broker_win32::copy(&r,input,sizeof r)||!weapon_broker_win32::copy(&result,output,sizeof result)||
       !troika_geometry::shape(r)||result.size!=sizeof result||result.abi_version!=version||
       !backend_.troika_geometry_module||!backend_.troika_geometry_prepare||!backend_.troika_geometry_current)return Reply::invalid;
    std::lock_guard<std::mutex> lock(mutex_);
    if(!published_||r.hand_cookie!=cookie_.load()||r.native_started_qpc!=session_.native_started_qpc)return Reply::wrong_cookie;
    if(stopping_||state_.load()!=State::active||aux_debt_.load()||action_debt())return Reply::draining;
    if(r.context!=registration_.context||!backend_.troika_geometry_module(backend_.context,r,pin_))return Reply::invalid;
    if(troika_geometry_attached_&&!troika_geometry::same(r,troika_geometry_registration_))return Reply::busy;
    result.cookie=cookie_.load();result.native_started_qpc=session_.native_started_qpc;
    if(!weapon_broker_win32::copy(output,&result,sizeof result))return Reply::invalid;
    troika_geometry_registration_=r;troika_geometry_attached_=true;return Reply::accepted;
}
MainMountQueryResult query_troika_geometry(troika_geometry::Source const& source,troika_geometry::Response& output) noexcept {
    using Code=MainMountQueryResult::Code;using Branch=MainMountQueryResult::Branch;
    output={};MainMountQueryResult result{};InvalidateCall invalidation{};bool counted{};
    if(current_){for(auto* p=current_;p;p=p->previous)p->violated=true;result.code=Code::fault;result.branch=Branch::reentrant;return result;}
    troika_geometry::Registration registration{};std::uint64_t revision{},epoch{},parent_cookie{};
    // A completed ordinary context-loss invalidation is expected while PC
    // possesses the turret. All lifetime/debt/GT/callback barriers still apply.
    const auto idle=[&](){return main_mount_context_clear()&&!callbacks_.load()&&!normal_calls_&&!transactions_.load()&&
        !aux_scopes_.load()&&!aux_debt_.load()&&!action_pending()&&!action_scopes_.load()&&!external_scopes_.load()&&
        !action_barriers_.load()&&!invalidate_running_&&!invalidate_failed_&&!final_invalidate_&&invalidate_done_;};
    try{
        {std::lock_guard<std::mutex> lock(mutex_);
            if(!troika_geometry_attached_||!published_||stopping_||state_.load()!=State::active)return result;
            if(!idle()){result.code=Code::context_rejected;result.branch=Branch::context;return result;}
            auto const& n=source.frame.native;
            if(n.game_pid!=session_.game_pid||n.game_start!=session_.game_start||n.native_started_qpc!=session_.native_started_qpc||
               n.registration_cookie!=cookie_.load()||n.game_thread!=game_thread_||
               !troika_geometry::source_valid(source,backend_.clock_ns(backend_.context),source.frame.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)){
                result.code=Code::stale;result.branch=Branch::source_before;return result;}
            // Burn this actual publication BEFORE invoking user code. Failure
            // and render retries never repeat callbacks or renew input age.
            if(source.frame.publication_sequence<=troika_geometry_attempted_)return result;
            troika_geometry_attempted_=source.frame.publication_sequence;
            if(next_troika_geometry_query_==UINT64_MAX){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=Branch::query_limit;}
            else{auto& w=troika_geometry_work_;w.request={};w.request.query_id=next_troika_geometry_query_++;
                snapshot(w.request.source,source);snapshot(w.shadow,w.request);w.response={};registration=troika_geometry_registration_;
                revision=revision_.load();epoch=operation_epoch_;parent_cookie=cookie_.load();++callbacks_;++normal_calls_;counted=true;result.query_id=w.request.query_id;}
        }
        if(!counted){run_invalidate(invalidation);return result;}
        const bool before=backend_.troika_geometry_current(backend_.context,troika_geometry_work_.shadow.source);
        bool okay=true,violated=false;std::uint32_t raw=unsigned(troika_geometry::Status::source_mismatch);
        if(before){CallbackMark mark(this);okay=!mark.violated&&backend_.troika_geometry_prepare(backend_.context,registration,&troika_geometry_work_.request,&troika_geometry_work_.response,raw);violated=mark.violated;}
        const bool after=before&&backend_.troika_geometry_current(backend_.context,troika_geometry_work_.shadow.source);
        result.callback_status=before?raw:UINT32_MAX;
        {std::lock_guard<std::mutex> lock(mutex_);--callbacks_;--normal_calls_;counted=false;
            const bool malformed=!okay||violated||!main_mount_context_clear()||!equal_bytes(troika_geometry_work_.request,troika_geometry_work_.shadow)||
                (before&&(raw>unsigned(troika_geometry::Status::geometry_unavailable)||
                    (raw==unsigned(troika_geometry::Status::ready)?!troika_geometry::valid(troika_geometry_work_.response,troika_geometry_work_.shadow):
                        !troika_geometry::empty(troika_geometry_work_.response))));
            if(malformed){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=Branch::callback_fault;}
            else if(stopping_||state_.load()!=State::active||cookie_.load()!=parent_cookie||revision_.load()!=revision||operation_epoch_!=epoch||!idle()){
                result.code=Code::unavailable;result.branch=Branch::changed;}
            else if(!before||!after){result.code=Code::context_rejected;result.branch=before?Branch::source_after:Branch::source_before;}
            else if(!troika_geometry::source_valid(troika_geometry_work_.shadow.source,backend_.clock_ns(backend_.context),
                troika_geometry_work_.shadow.source.frame.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)){
                result.code=Code::stale;result.branch=Branch::source_after;}
            else{result.code=raw==unsigned(troika_geometry::Status::ready)?Code::ready:Code::not_ready;result.branch=Branch::reply;
                if(result.code==Code::ready)snapshot(output,troika_geometry_work_.response);}
            if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
            refresh_locked();
        }
        run_invalidate(invalidation);return result;
    }catch(...){
        try{std::lock_guard<std::mutex> lock(mutex_);if(counted){--callbacks_;--normal_calls_;}invalidate_locked(InvalidateReason::callback_fault,true,invalidation);refresh_locked();}catch(...){}
        try{run_invalidate(invalidation);}catch(...){}output={};result.code=Code::fault;result.branch=Branch::exception;return result;
    }
}
