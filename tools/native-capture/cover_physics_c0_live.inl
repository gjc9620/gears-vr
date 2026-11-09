#include "cover_physics_c0_live.h"
#include "cover_physx_query.h"
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
#include "cover_physics_query_live.inl"
#endif
namespace cover_physics_c0_live {
namespace q=gears_cover_physx_query;
using Entry=gears_continuous_hand_entry::Entry;using Core=gears_native_rig_tracking::CoreSnapshot;
#if defined(GEARS_COVER_PHYSX_QUERIES) && GEARS_COVER_PHYSX_QUERIES
namespace n=full_body_c0_native_live;
cover_physics_query_live::Session<> session;
bool configure(wchar_t const* ini,FILE* log){
    wchar_t value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"0",value,8,ini);
    if(std::wcscmp(value,L"0")&&std::wcscmp(value,L"1"))return false;
    const bool requested=!std::wcscmp(value,L"1");
    const bool okay=session.start(requested,n::enabled&&continuous_session);
    if(log)fprintf(log,"cover_queries_compiled=1 requested=%u ready=%u raw_observations_only=1 movement_permission=0\n",unsigned(requested),unsigned(session.enabled()));
    return okay;
}
void request_drain()noexcept{session.request_drain();}
bool poll_stop()noexcept{return session.poll_stop();}
unsigned pending_count()noexcept{return session.pending_count();}
void summary(FILE* f){if(f)fprintf(f,"cover_queries_ready=%u pending=%u uncertain=%u raw_observations_only=1\n",unsigned(session.enabled()),session.pending_count(),unsigned(session.uncertain()));}
template<class Shape,class Output,class Invoke> q::Code query(Entry const& entry,Core const& source,Shape const& shape,Output& output,Invoke invoke)noexcept{
    n::Call call;auto& s=n::state;
    if(!session.enabled()||GetCurrentThreadId()!=n::owner_thread.load()||!n::enabled||n::stopping.load()||entry.kind||!entry.selected||
       !s.captured||s.prepared||s.kernel.pending()||!n::same_entry(entry)||!source.cover_type||
       !(source.flags&gears_rig_tracking::cover_key_known)||std::memcmp(&source,&s.context.source,sizeof source))return q::Code::unavailable;
    const auto metadata=s.backend.metadata();
    const Core expected_source=source;
    cover_context_live::CurrentKey cover{};
    if(!cover_context_live::sample_current(s.context.actor,cover)||cover.type!=source.cover_type||cover.slot!=source.cover_slot||
       !gears_weapon_hand::same(rig_tracking_live::object(cover.link),source.cover_link))return q::Code::source;
    auto guard=[&](){
        gears_cover_pose_phase::Request request{};request.owner=s.context.owner;
        auto reader=[](void* out,void const* address,size_t size){return n::native::read_memory(out,reinterpret_cast<uintptr_t>(address),size);};
        gears_cover_pose_phase::Metadata current{};
        cover_context_live::CurrentKey current_cover{};
        return session.enabled()&&s.captured&&!s.prepared&&!s.kernel.pending()&&n::same_entry(entry)&&
            !std::memcmp(&expected_source,&s.context.source,sizeof source)&&!std::memcmp(&expected_source,&source,sizeof source)&&
            s.context.current(n::tx::Stage::prepare)&&cover_context_live::sample_current(s.context.actor,current_cover)&&
            cover_context_live::same(cover,current_cover)&&
            gears_cover_pose_phase::metadata(reader,request,current)&&gears_cover_pose_phase::metadata_equal(metadata,current)&&
            gears_cover_pose_phase::metadata_equal(metadata,s.backend.metadata());};
    if(!guard())return q::Code::source;
    // No binding/stereo/session lock is held across this original operation.
    auto read=[](void* out,void const* address,size_t size){return n::native::read_memory(out,reinterpret_cast<uintptr_t>(address),size);};
    if(q::source::aliases(&output,sizeof output,&s,sizeof s)||q::source::aliases(&output,sizeof output,&entry,sizeof entry)||
       q::source::aliases(&output,sizeof output,&source,sizeof source))return q::Code::request;
    const auto code=invoke(s.context.actor,metadata,shape,read,guard,output);
    if(code==q::Code::fault_unknown)request_session_stop(SessionStop::proof_failure);
    return code;
}
template<class Shape> q::Code do_overlap(Entry const& e,Core const& c,Shape const& shape,q::Overlap& out)noexcept{
    return query(e,c,shape,out,[](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){return session.overlap(actor,metadata,geometry,read,guard,output);});
}
template<class Shape> q::Code do_sweep(Entry const& e,Core const& c,Shape const& shape,q::Vector const& delta,q::Sweep& out)noexcept{
    return query(e,c,shape,out,[&](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){return session.sweep(actor,metadata,geometry,delta,read,guard,output);});
}
template<class Shape,class Output> q::Code do_sweep_all(Entry const& e,Core const& c,Shape const& shape,q::Vector const& delta,q::StreamLimits const& limits,Output& out)noexcept{
    return query(e,c,shape,out,[&](auto const& actor,auto const& metadata,auto const& geometry,auto read,auto guard,auto& output){return session.sweep_all(actor,metadata,geometry,delta,limits,read,guard,output);});
}
#else
bool configure(wchar_t const* ini,FILE*){wchar_t value[8]{};GetPrivateProfileStringW(L"FullBodyC0Native",L"cover_queries",L"0",value,8,ini);return !std::wcscmp(value,L"0");}
void request_drain()noexcept{}
bool poll_stop()noexcept{return true;}
unsigned pending_count()noexcept{return 0;}
void summary(FILE*){}
template<class Shape> q::Code do_overlap(Entry const&,Core const&,Shape const&,q::Overlap&)noexcept{return q::Code::unavailable;}
template<class Shape> q::Code do_sweep(Entry const&,Core const&,Shape const&,q::Vector const&,q::Sweep&)noexcept{return q::Code::unavailable;}
template<class Shape,class Output> q::Code do_sweep_all(Entry const&,Core const&,Shape const&,q::Vector const&,q::StreamLimits const&,Output&)noexcept{return q::Code::unavailable;}
#endif
q::Code overlap(Entry const& e,Core const& c,q::Box const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
q::Code overlap(Entry const& e,Core const& c,q::Capsule const& s,q::Overlap& out)noexcept{return do_overlap(e,c,s,out);}
q::Code sweep(Entry const& e,Core const& c,q::Box const& s,q::Vector const& d,q::Sweep& out)noexcept{return do_sweep(e,c,s,d,out);}
q::Code sweep(Entry const& e,Core const& c,q::Capsule const& s,q::Vector const& d,q::Sweep& out)noexcept{return do_sweep(e,c,s,d,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Box const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweep& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Capsule const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweep& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Box const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweepOwners& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
q::Code sweep_all(Entry const& e,Core const& c,q::Capsule const& s,q::Vector const& d,q::StreamLimits const& l,q::AllSweepOwners& out)noexcept{return do_sweep_all(e,c,s,d,l,out);}
}
