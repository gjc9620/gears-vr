// Included within full_body_c0_native_live after the existing State/Kernel
// helpers. This dispatch uses that SAME state.backend/kernel/lease only.
namespace troika_c0_commit {
std::atomic<std::uint64_t> attempts{},captured{},planned{},committed{},consumed{},rejected{};
#include "troika_c0_consumer_live.inl"
bool ordinary_endpoint_allowed()noexcept{return !state.context.mounted.active;}
bool begin(Entry const& e) noexcept {
    if(e.kind==0&&consumer.active()&&e.rcx==consumer.frame().source.frame.driver_mesh.address)consumer.revoke();
    if(!troika_driver_scope_live::owns_compose(e))return false;
    ++attempts;
    if(!enabled||!hands_enabled||stopping.load()||e.kind!=0){++rejected;return true;}
    DWORD expected{};owner_thread.compare_exchange_strong(expected,GetCurrentThreadId());
    if(owner_thread.load()!=GetCurrentThreadId()||state.captured){++rejected;return true;}
    if(state.kernel.pending()){note_pending();++rejected;return true;}
    try{
        if(state.kernel.settled_source_age()&&!state.recovery.active){auto& old=state.recovery;
            old.mounted_kind=state.context.mounted.active;if(old.mounted_kind)old.mounted_storage=state.context.mounted.storage;
            old.actor=state.context.actor;old.asset=state.context.asset;old.source=state.context.source;old.metadata=state.backend.metadata();
            old.stamp=state.stamp;old.desired=state.desired;old.reference=state.source.reference;old.units=state.source.units_per_metre;old.model_scale=state.source.model_scale;old.active=true;
        }
        state.context.mounted.active=false;
        if(!state.context.mounted.capture(e)){++rejected;return true;}
        auto& context=state.context;auto const& mounted=context.mounted;
        context.entry=&e;context.copy=e;context.owner=mounted.storage.owner;context.asset=mounted.storage.asset;
        context.tick_entry=mounted.outer;context.tick_copy=mounted.outer_copy;context.world_lease=mounted.lease;
        context.tick=mounted.outer_copy.invocation;context.expired=false;
        // Invalidate ordinary authentication data rather than relabelling it.
        // Any old settled recovery/retirement already owns its exact descriptor.
        context.actor={};context.source={};context.body_input={};context.body_source={};
        context.mount_parent={};context.source_parent={};context.queried_parent={};context.basis_ns=0;context.conflict=0;
        state.backend.~Backend();::new(static_cast<void*>(&state.backend)) native::Backend;
        state.lease.~Lease();::new(static_cast<void*>(&state.lease)) tx::Lease;
        state.prepared=state.notified=state.atom_installed=state.joint_committed=state.consumer_verified=false;
        state.cover_plan=state.cover_completed_valid=state.troika_consumed_valid=false;state.hand_record=0;
        state.troika_solved=state.troika_projected=0;
        state.hand_work.solved_mask=0;state.body_query_active=false;state.right_mount=hands::adapter::MountKind::free_hand;
        state.alpha_grant_mask=state.alpha_grant_count=0;state.alpha_grant_nodes={};state.alpha_grant_original={};
        endpoint_valid=endpoint_observable=false;notify_endpoint(gears_committed_hand_consumer::Kind::revoke);
        tx::Stamp seed{};seed.binding.owner=context.owner.mesh.address;seed.binding.model=context.asset.address;
        seed.binding.metadata_generation=seed.binding.world_generation=context.owner.world_generation;seed.binding.session=context.owner.session;
        // The existing Kernel uses a private real-C0 order stamp. Keep the
        // SAME QPC domain across ordinary/Driver operations. Original source,
        // controller and Tick/Compose identities remain separate and unchanged.
        seed.frame=seed.producer=seed.scope=uint64_t(phase::qpc());
        if(!state.backend.capture(context,context.owner,seed,state.source,state.desired)||
           state.source.units_per_metre!=mounted.source.mapping.units_per_metre||
           !body::prepare_model(state.source.reference,digest,state.model)){
            context.expired=true;state.backend.invalidate();++rejected;return true;
        }
        state.stamp=state.backend.stamp();state.captured=true;++captured;
        if(state.recovery.active){
            const auto status=recovery_current();
            if(status==RecoveryResult::defer){state.context.expired=true;state.backend.invalidate();state.captured=false;++rejected;return true;}
            if(status!=RecoveryResult::okay){state.kernel.request_stop();++rejected;return true;}
            const auto serial=state.kernel.serial();state.kernel.~Kernel();::new(static_cast<void*>(&state.kernel)) tx::Kernel(serial);
            state.recovery={};sync();
        }
        std::array<std::uint64_t,2> available{};
        for(unsigned i=0;i<state.desired.count&&i<body::bones;++i){const auto bone=state.desired.indices[i];
            if(bone<body::bones)available[bone/64]|=std::uint64_t(1)<<(bone%64);}
        const auto result=gears_troika_c0_hands::compose(state.model,state.source,mounted.source,
            rig_tracking_live::clock_ns(),available,state.troika_hands,state.output);
        if(result.status!=gears_troika_c0_hands::Status::okay||!result.solved_mask||
           rig_tracking_live::clock_ns()-mounted.began_ns>40000000||!context.current(tx::Stage::prepare)){
            ++rejected;return true;
        }
        ++planned;state.troika_solved=result.solved_mask;state.troika_projected=result.projected_mask;
        auto prepared=state.kernel.prepare(state.backend,state.stamp,state.source,state.output,state.desired,state.lease);sync();
        if(prepared.code!=tx::Code::prepared){++rejected;return true;}
        state.prepared=true;
        const auto applied=state.kernel.commit(state.backend,state.lease,state.stamp);sync();install_atom_receipt();
        if(applied.code!=tx::Code::committed){++rejected;return true;}
        state.joint_committed=true;++committed;++commits;return true;
    }catch(...){++rejected;return true;}
}
void completed(Entry const& e,tx::Result const& result,bool returned,bool original_called) noexcept {
    state.troika_consumed_valid=false;
    if(!state.context.mounted.active||result.code!=tx::Code::scope_closed||!returned||!original_called||
       !same_entry(e)||!state.joint_committed||!state.consumer_verified||stopping.load()||
       state.kernel.failed()||state.kernel.protocol_debt()||state.kernel.original_exception())return;
    state.troika_consumed_valid=state.backend.read_consumed_pose(state.kernel,state.stamp,state.kernel.serial(),state.troika_completed)==native::ConsumedPoseCode::verified&&
        gears_troika_c0_hands::local_pose_matches(state.output,state.troika_completed.local);
    if(state.troika_consumed_valid){++consumed;
        consumer.publish(state.backend,state.kernel,state.stamp,state.context.mounted.source,
            rig_tracking_live::object(state.context.asset),state.model,state.source,state.output,state.troika_hands.target,
            state.troika_solved,state.troika_projected,state.context.tick,e.invocation,rig_tracking_live::clock_ns,state.troika_completed);
    }
}
void summary(FILE* f){if(f)std::fprintf(f,"troika_c0_attempts=%llu troika_c0_captured=%llu troika_c0_planned=%llu troika_c0_committed=%llu troika_c0_consumed=%llu troika_c0_rejected=%llu troika_c0_same_kernel=1 ",
    attempts.load(),captured.load(),planned.load(),committed.load(),consumed.load(),rejected.load());
    if(f)std::fprintf(f,"troika_c0_old_avatar_retirement_transfers=%llu troika_c0_consumer_reads=%llu troika_c0_consumer_rejected=%llu troika_c0_query_publication=%llu troika_c0_cached_at_query=%llu troika_c0_query_ns=%lld troika_c0_query_status=%u troika_c0_last_completed_ns=%lld ",
        retirement_context.mounted_transfers.load(),consumer_reads.load(),consumer_rejected.load(),consumer_query_publication,consumer_cached_publication,
        consumer_query_ns,consumer_query_status,consumer.frame().completed_ns);}
} // namespace troika_c0_commit
