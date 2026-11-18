// Inside Broker public section. One synchronous reservation spans both PG
// callbacks AND the Native sole-owner decision/finalization between them.
Reply attach_troika_control(troika_control::Registration const* input,RegistrationResult* output,bool enabled) {
    if(reentrant())return Reply::reentrant;
    troika_control::Registration r{};RegistrationResult result{};
    if(!enabled||!weapon_broker_win32::copy(&r,input,sizeof r)||!weapon_broker_win32::copy(&result,output,sizeof result)||
       !troika_control::shape(r)||result.size!=sizeof result||result.abi_version!=version||
       !backend_.troika_control_module||!backend_.troika_control_commit||!backend_.troika_control_decide||!backend_.troika_control_complete||
       !backend_.troika_geometry_module||!backend_.troika_geometry_prepare||!backend_.troika_geometry_current)return Reply::invalid;
    std::lock_guard<std::mutex> lock(mutex_);auto const& g=r.geometry;
    if(!published_||g.hand_cookie!=cookie_.load()||g.native_started_qpc!=session_.native_started_qpc)return Reply::wrong_cookie;
    if(stopping_||state_.load()!=State::active||aux_debt_.load()||action_debt())return Reply::draining;
    if(g.context!=registration_.context||!backend_.troika_geometry_module(backend_.context,g,pin_)||
       !backend_.troika_control_module(backend_.context,r,pin_))return Reply::invalid;
    if((troika_geometry_attached_&&!troika_geometry::same(g,troika_geometry_registration_))||
       (troika_control_attached_&&!troika_control::same(r,troika_control_registration_)))return Reply::busy;
    result.cookie=cookie_.load();result.native_started_qpc=session_.native_started_qpc;
    if(!weapon_broker_win32::copy(output,&result,sizeof result))return Reply::invalid;
    troika_control_registration_=r;troika_control_attached_=true;troika_geometry_registration_=g;troika_geometry_attached_=true;
    return Reply::accepted;
}
MainMountQueryResult transact_troika_control(troika_geometry::Source const& source,troika_control::Decision& output,
    troika_geometry::Response& geometry_output) noexcept {
    using Code=MainMountQueryResult::Code;using Branch=MainMountQueryResult::Branch;
    output={};geometry_output={};MainMountQueryResult result{};InvalidateCall invalidation{};bool counted{},finished_native{};
    if(current_){for(auto* p=current_;p;p=p->previous)p->violated=true;result.code=Code::fault;result.branch=Branch::reentrant;return result;}
    troika_control::Registration registration{};std::uint64_t revision{},epoch{},parent_cookie{},generation{};
    const auto contexts=[&](){return main_mount_context_clear()&&!transactions_.load()&&!aux_scopes_.load()&&!aux_debt_.load()&&
        !action_pending()&&!action_scopes_.load()&&!external_scopes_.load()&&!action_barriers_.load()&&
        !invalidate_running_&&!invalidate_failed_&&!final_invalidate_&&invalidate_done_;};
    const auto current=[&](){return published_&&!stopping_&&state_.load()==State::active&&cookie_.load()==parent_cookie&&
        revision_.load()==revision&&operation_epoch_==epoch&&revocation_.generation()==generation&&
        callbacks_.load()==1&&normal_calls_==1&&contexts();};
    const auto fresh=[&](){auto const& s=troika_control_work_.shadow.source;
        return troika_geometry::source_valid(s,backend_.clock_ns(backend_.context),s.frame.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated);};
    try{
        {std::lock_guard<std::mutex> lock(mutex_);
            if(!troika_control_attached_||!published_||stopping_||state_.load()!=State::active)return result;
            if(!contexts()||callbacks_.load()||normal_calls_){result.code=Code::context_rejected;result.branch=Branch::context;return result;}
            auto const& n=source.frame.native;
            if(n.game_pid!=session_.game_pid||n.game_start!=session_.game_start||n.native_started_qpc!=session_.native_started_qpc||
               n.registration_cookie!=cookie_.load()||n.game_thread!=game_thread_||
               !troika_geometry::source_valid(source,backend_.clock_ns(backend_.context),source.frame.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated)){
                result.code=Code::stale;result.branch=Branch::source_before;return result;}
            if(source.frame.publication_sequence<=troika_geometry_attempted_)return result;
            troika_geometry_attempted_=source.frame.publication_sequence;
            if(next_troika_geometry_query_==UINT64_MAX){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=Branch::query_limit;}
            else{auto& w=troika_control_work_;w.request={};w.request.query_id=next_troika_geometry_query_++;snapshot(w.request.source,source);
                snapshot(w.shadow,w.request);w.response={};w.response_shadow={};w.decision={};w.decision_shadow={};registration=troika_control_registration_;
                revision=revision_.load();epoch=operation_epoch_;parent_cookie=cookie_.load();generation=revocation_.generation();
                ++callbacks_;++normal_calls_;counted=true;result.query_id=w.request.query_id;}
        }
        if(!counted){run_invalidate(invalidation);return result;}
        auto& w=troika_control_work_;
        bool before=backend_.troika_geometry_current(backend_.context,w.shadow.source),okay=true,violated=false,malformed=false,accepted=false;
        std::uint32_t raw=unsigned(troika_geometry::Status::source_mismatch),commit_raw=UINT32_MAX;
        if(before){CallbackMark mark(this);okay=!mark.violated&&backend_.troika_geometry_prepare(backend_.context,registration.geometry,&w.request,&w.response,raw);violated=mark.violated;}
        result.callback_status=before?raw:UINT32_MAX;
        malformed=!okay||violated||!main_mount_context_clear()||!equal_bytes(w.request,w.shadow)||
            (before&&(raw>unsigned(troika_geometry::Status::geometry_unavailable)||
                (raw==unsigned(troika_geometry::Status::ready)?!troika_geometry::valid(w.response,w.shadow):!troika_geometry::empty(w.response))));
        bool eligible=before&&!malformed&&raw==unsigned(troika_geometry::Status::ready)&&
            backend_.troika_geometry_current(backend_.context,w.shadow.source);
        {std::lock_guard<std::mutex> lock(mutex_);eligible=eligible&&current()&&fresh();}
        if(eligible){
            snapshot(w.response_shadow,w.response);
            // These are fixed Native functions, not PG-provided callbacks.
            // They invoke the existing participant/left owner, outside mutex.
            eligible=backend_.troika_control_decide(backend_.context,w.shadow,w.response_shadow,w.decision);
            malformed=malformed||!equal_bytes(w.request,w.shadow)||!equal_bytes(w.response,w.response_shadow)||
                (eligible&&!troika_control::valid(w.decision,w.shadow,backend_.clock_ns(backend_.context)));
            snapshot(w.decision_shadow,w.decision);
            {std::lock_guard<std::mutex> lock(mutex_);eligible=eligible&&!malformed&&current()&&fresh();}
            if(eligible)eligible=backend_.troika_geometry_current(backend_.context,w.shadow.source);
            if(eligible){CallbackMark mark(this);okay=!mark.violated&&backend_.troika_control_commit(backend_.context,registration,
                    &w.request,&w.response,&w.decision,commit_raw);violated=mark.violated;
                malformed=!okay||violated;}
            malformed=malformed||!main_mount_context_clear()||!equal_bytes(w.request,w.shadow)||
                !equal_bytes(w.response,w.response_shadow)||!equal_bytes(w.decision,w.decision_shadow)||
                (eligible&&commit_raw>unsigned(troika_control::CommitReply::rejected));
            accepted=eligible&&!malformed&&commit_raw==unsigned(troika_control::CommitReply::accepted)&&
                backend_.troika_geometry_current(backend_.context,w.shadow.source);
            {std::lock_guard<std::mutex> lock(mutex_);accepted=accepted&&current()&&fresh();}
        }
        const bool completed=backend_.troika_control_complete(backend_.context,w.shadow,w.decision_shadow,accepted);finished_native=true;
        bool closing=accepted&&completed&&backend_.troika_geometry_current(backend_.context,w.shadow.source);
        {std::lock_guard<std::mutex> lock(mutex_);closing=closing&&current()&&fresh();}
        // A stop/identity change racing completion withdraws the same actual
        // owner result BEFORE the reservation can be released or pin unbound.
        if(accepted&&!closing)backend_.troika_control_complete(backend_.context,w.shadow,w.decision_shadow,false);
        {std::unique_lock<std::mutex> lock(mutex_);
            if(closing&&(!current()||!fresh())){
                closing=false;lock.unlock();backend_.troika_control_complete(backend_.context,w.shadow,w.decision_shadow,false);lock.lock();
            }
            if(malformed){invalidate_locked(InvalidateReason::callback_fault,true,invalidation);result.code=Code::fault;result.branch=Branch::callback_fault;}
            else if(closing){result.code=Code::ready;result.branch=Branch::reply;snapshot(output,w.decision_shadow);snapshot(geometry_output,w.response_shadow);}
            else{result.code=before?Code::not_ready:Code::context_rejected;result.branch=before?Branch::changed:Branch::source_before;}
            --callbacks_;--normal_calls_;counted=false;
            if(phase_invalid_&&final_invalidate_&&!invalidate_running_&&!invalidate_failed_)invalidate_locked(phase_reason_,state_.load()==State::draining,invalidation);
            refresh_locked();
        }
        run_invalidate(invalidation);return result;
    }catch(...){
        if(counted&&!finished_native)backend_.troika_control_complete(backend_.context,troika_control_work_.shadow,troika_control_work_.decision_shadow,false);
        try{std::lock_guard<std::mutex> lock(mutex_);if(counted){--callbacks_;--normal_calls_;}invalidate_locked(InvalidateReason::callback_fault,true,invalidation);refresh_locked();}catch(...){}
        try{run_invalidate(invalidation);}catch(...){}output={};geometry_output={};result.code=Code::fault;result.branch=Branch::exception;return result;
    }
}
