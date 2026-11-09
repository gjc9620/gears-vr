#ifndef GEARS_COVER_PHYSICS_QUERY_LIVE_INCLUDED
#define GEARS_COVER_PHYSICS_QUERY_LIVE_INCLUDED
#include "cover_physx_query.h"
#include "cover_physics_roles_live.inl"
// Original-query ownership on the existing Native worker and admitted C0.
// No hooks, body writes or safe-movement/occupancy result are supplied here.
namespace cover_physics_query_live {
namespace q=gears_cover_physx_query;
namespace roles=cover_physics_roles_live;
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
template<class Engine=q::Queries> class Session {
    Engine engine_{};roles::Workspace work_{};roles::Snapshot before_{},after_{};
    q::AllSweepOwners owner_result_{};
    struct NoLocalResult {};
    std::atomic<bool> configured_{},enabled_{},stopped_{},busy_{},closed_{true};
    std::atomic<DWORD> worker_{};
    struct Finish {std::atomic<bool>& busy;~Finish(){busy=false;}};
    template<class Guard> struct SourceGuard {
        Session& session;weapon_hand_scene_binding::Binding const& actor;
        gears_cover_pose_phase::Metadata const& metadata;Guard& owner;
        bool operator()(){return !session.stopped_.load()&&
            roles::sample(actor,metadata,owner,session.work_,session.after_)==roles::Code::current&&
            roles::same(session.before_,session.after_);}
        bool operator()(q::source::Snapshot const& raw){return (*this)()&&roles::matches_query_source(session.before_,raw);}
    };
    template<class Shape,class Read,class Guard,class Output,class Invoke> q::Code execute(
        weapon_hand_scene_binding::Binding const& actor,gears_cover_pose_phase::Metadata const& metadata,
        Shape const& shape,Read read,Guard guard,Output& output,Invoke invoke){
        if(!enabled_.load()||stopped_.load())return q::Code::unavailable;
        if(q::source::aliases(&output,sizeof output,this,sizeof *this)||q::source::aliases(&output,sizeof output,&actor,sizeof actor)||
           q::source::aliases(&output,sizeof output,&metadata,sizeof metadata)||q::source::aliases(&output,sizeof output,&shape,sizeof shape))return q::Code::request;
        bool expected=false;if(!busy_.compare_exchange_strong(expected,true))return q::Code::reentrant;Finish finish{busy_};
        if(stopped_.load())return q::Code::unavailable;
        const auto owner=actor;const auto source=metadata;const Shape geometry=shape;
        if(!q::detail::valid(geometry))return q::Code::request;
        if(roles::sample(owner,source,guard,work_,before_)!=roles::Code::current)return q::Code::source;
        SourceGuard<Guard> current{*this,owner,source,guard};
        // The larger ownership packet lives in the already serialized
        // Session, never as a 139KB temporary on the live game stack.
        std::conditional_t<std::is_same_v<Output,q::AllSweepOwners>,NoLocalResult,Output> local{};
        Output& result=[&]() -> Output& {if constexpr(std::is_same_v<Output,q::AllSweepOwners>)return owner_result_;else return local;}();
        const auto code=invoke(engine_,before_.source,geometry,read,current,result);
        if(code==q::Code::fault_unknown||engine_.uncertain())enabled_=false;
        if(code!=q::Code::queried)return code;
        if(!current()||!weapon_hand_scene_binding::same_key(owner,actor)||owner.scene_invocation!=actor.scene_invocation||
           owner.special_move!=actor.special_move||!gears_cover_pose_phase::metadata_equal(source,metadata)||
           std::memcmp(&geometry,&shape,sizeof shape))return q::Code::changed;
        output=result;return q::Code::queried;
    }
public:
    Session()=default;Session(Session const&)=delete;Session& operator=(Session const&)=delete;
    // One startup attempt; worker only, outside DllMain and game callbacks.
    bool start(bool requested,bool owner_enabled) noexcept {
        bool expected=false;if(!configured_.compare_exchange_strong(expected,true))return false;
        worker_=GetCurrentThreadId();if(stopped_.load()||(requested&&!owner_enabled))return false;
        if(!requested)return true;
        try{closed_=false;if(!engine_.prime()){closed_=engine_.close();return false;}enabled_=true;return true;}
        catch(...){closed_=engine_.close();return false;}
    }
    void request_drain()noexcept{stopped_=true;enabled_=false;}
    // Called explicitly by the startup/cleanup worker without private locks.
    // A fault remains pending forever; never unlock/reset or force-close it.
    bool poll_stop()noexcept{
        if(closed_.load())return true;
        if(!stopped_.load()||busy_.load()||GetCurrentThreadId()!=worker_.load()||engine_.pending())return false;
        closed_=engine_.close();return closed_.load();
    }
    unsigned pending_count()const noexcept{return unsigned(busy_.load()||engine_.pending()||!closed_.load());}
    bool uncertain()const noexcept{return engine_.uncertain();}
    bool enabled()const noexcept{return enabled_.load()&&!stopped_.load();}
    template<class Shape,class Read,class Guard> q::Code overlap(weapon_hand_scene_binding::Binding const& actor,
        gears_cover_pose_phase::Metadata const& metadata,Shape const& shape,Read read,Guard guard,q::Overlap& output)noexcept{
        try{return execute(actor,metadata,shape,read,guard,output,[](auto& engine,auto const& source,auto const& geometry,auto reader,auto current,auto& result){
            return engine.overlap(source,geometry,reader,current,result);});}catch(...){return q::Code::source;}
    }
    template<class Shape,class Read,class Guard> q::Code sweep(weapon_hand_scene_binding::Binding const& actor,
        gears_cover_pose_phase::Metadata const& metadata,Shape const& shape,q::Vector const& delta,Read read,Guard guard,q::Sweep& output)noexcept{
        if(q::source::aliases(&output,sizeof output,&delta,sizeof delta)||!q::detail::motion(delta))return q::Code::request;
        if(q::source::aliases(&output,sizeof output,this,sizeof *this)||q::source::aliases(&output,sizeof output,&actor,sizeof actor)||
           q::source::aliases(&output,sizeof output,&metadata,sizeof metadata)||q::source::aliases(&output,sizeof output,&shape,sizeof shape))return q::Code::request;
        const auto movement=delta;q::Sweep result{};
        try{const auto code=execute(actor,metadata,shape,read,guard,result,[&](auto& engine,auto const& source,auto const& geometry,auto reader,auto current,auto& raw){
            return engine.sweep(source,geometry,movement,reader,current,raw);});
            if(code!=q::Code::queried)return code;
            if(std::memcmp(&movement,&delta,sizeof delta))return q::Code::changed;
            output=result;return q::Code::queried;
        }catch(...){return q::Code::source;}
    }
    template<class Shape,class Read,class Guard,class Output> q::Code sweep_all(weapon_hand_scene_binding::Binding const& actor,
        gears_cover_pose_phase::Metadata const& metadata,Shape const& shape,q::Vector const& delta,q::StreamLimits const& limits,
        Read read,Guard guard,Output& output)noexcept{
        static_assert(std::is_same_v<Output,q::AllSweep>||std::is_same_v<Output,q::AllSweepOwners>);
        if(!q::detail::motion(delta)||!q::detail::stream_limits(limits)||q::source::aliases(&output,sizeof output,&delta,sizeof delta)||
           q::source::aliases(&output,sizeof output,&limits,sizeof limits)||q::source::aliases(&output,sizeof output,this,sizeof *this)||
           q::source::aliases(&output,sizeof output,&actor,sizeof actor)||q::source::aliases(&output,sizeof output,&metadata,sizeof metadata)||
           q::source::aliases(&output,sizeof output,&shape,sizeof shape))return q::Code::request;
        const auto movement=delta;const auto budget=limits;
        try{return execute(actor,metadata,shape,read,guard,output,[&](auto& engine,auto const& source,auto const& geometry,auto reader,auto current,auto& raw){
            const auto code=engine.sweep_all(source,geometry,movement,budget,reader,current,raw);
            if(code!=q::Code::queried)return code;
            return std::memcmp(&movement,&delta,sizeof delta)||std::memcmp(&budget,&limits,sizeof limits)?q::Code::changed:q::Code::queried;});
        }catch(...){return q::Code::source;}
    }
};
#endif
} // namespace cover_physics_query_live
#endif
