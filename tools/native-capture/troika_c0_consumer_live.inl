// Inside troika_c0_commit. The existing original C0 callback alone publishes.
gears_troika_c0_readback::Store consumer;
bool consumer_reading{};std::atomic<std::uint64_t> consumer_reads{},consumer_rejected{};
std::uint64_t consumer_query_publication{},consumer_cached_publication{};std::int64_t consumer_query_ns{};unsigned consumer_query_status{};
gears_rig_tracking::Reply read_consumer(gears_troika_c0::Request const* request,gears_troika_c0::Frame* output) noexcept {
    namespace api=gears_troika_c0;using Reply=gears_rig_tracking::Reply;
    weapon_hand_runtime::detail::Call reservation;
    api::Request query{};
    if(!request||!output||!safe_copy(&query,request,sizeof query)||!api::request_valid(query,rig_tracking_live::clock_ns()))return Reply::invalid;
    if(stopping.load())return Reply::stopping;
    if(GetCurrentThreadId()!=owner_thread.load())return Reply::unavailable;
    consumer_query_publication=query.source.frame.publication_sequence;consumer_cached_publication=consumer.frame().source.frame.publication_sequence;
    consumer_query_ns=rig_tracking_live::clock_ns();consumer_query_status=1;
    if(consumer_reading||!consumer.active()){++consumer_rejected;return Reply::unavailable;}
    consumer_reading=true;struct Exit {~Exit(){consumer_reading=false;}} exit;
    try{
        ::Scope world;
        const auto current=[&](){
            if(!enabled||!hands_enabled||stopping.load()||GetCurrentThreadId()!=owner_thread.load()||
               state.captured||!state.context.mounted.active||!state.troika_consumed_valid||
               state.kernel.failed()||state.kernel.protocol_debt()||state.kernel.original_exception()||
               state.kernel.serial()!=consumer.frame().kernel_serial||!world.world()||!state.context.mounted.storage.current())return false;
            gears_troika_control::Decision control{};gears_weapon::TroikaMountedMapping mapping{};
            if(!troika_control_live::read_for(query.source,control)||std::memcmp(&control,&query.control,sizeof control)||
               !troika_scene_tracking::mapped_for(query.source.frame,mapping)||mapping.units_per_metre!=consumer.frame().units_per_metre)return false;
            for(unsigned side=0;side<2;++side)if(!api::pose_matches(mapping.hands.world[side],consumer.frame().grip_world[side],0,1e-6))return false;
            return true;
        };
        const auto metadata=[&](gears_cover_pose_phase::Metadata& result){
            gears_cover_pose_phase::Request q{};q.owner=state.context.mounted.storage.owner;
            auto copy=[](void* to,void const* from,std::size_t bytes){return safe_copy(to,from,bytes);};
            return gears_cover_pose_phase::metadata(copy,q,result);
        };
        api::Frame result{};
        const bool accepted=consumer.read(query,rig_tracking_live::clock_ns,state.troika_completed,state.source,current,metadata,
            [](void* to,std::uintptr_t from,std::size_t bytes){return safe_copy(to,reinterpret_cast<void const*>(from),bytes);},result);
        if(!accepted||!current()||!api::valid(result,query,rig_tracking_live::clock_ns())){consumer_query_status=2;++consumer_rejected;return Reply::unavailable;}
        if(!safe_copy(output,&result,sizeof result))return Reply::invalid;consumer_query_status=0;++consumer_reads;return Reply::accepted;
    }catch(...){++consumer_rejected;return Reply::unavailable;}
}
