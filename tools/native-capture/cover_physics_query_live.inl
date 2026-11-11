#ifndef GEARS_COVER_PHYSICS_QUERY_LIVE_INCLUDED
#define GEARS_COVER_PHYSICS_QUERY_LIVE_INCLUDED
#include "cover_physx_query.h"
#include "cover_physx_capsule_path_observation.h"
#include "cover_query_guard_observation.h"
#include "cover_query_timing.h"
#include "cover_physics_roles_live.inl"
#include "cover_collider_roles_live.inl"
#include "cover_component_roles_live.inl"
// Original-query ownership on the existing Native worker and admitted C0.
// No hooks, body writes or safe-movement/occupancy result are supplied here.
namespace cover_physics_query_live {
namespace q=gears_cover_physx_query;
namespace roles=cover_physics_roles_live;
namespace collider_roles=cover_collider_roles_live;
namespace component_roles=cover_component_roles_live;
namespace path=gears_cover_capsule_path;
namespace timing=gears_cover_query_timing;
enum class Stage:unsigned {idle,geometry,physics_roles,collider_roles,component_roles,engine,closing,complete};
struct Observation {Stage stage{};unsigned detail{};q::QueryProgress engine{};roles::LayoutObservation layout{};gears_cover_query_guard::Witness guard{};};
#if defined(GEARS_COVER_BODY_PROTOTYPE) && GEARS_COVER_BODY_PROTOTYPE
template<class Engine=q::Queries> class Session {
    Engine engine_{};roles::Workspace work_{};roles::Snapshot before_{},after_{};
    collider_roles::Workspace collider_work_{};collider_roles::Snapshot collider_before_{},collider_after_{};
    component_roles::Workspace component_work_{};component_roles::Snapshot component_before_{},component_after_{};
    gears_cover_physx_components::Source component_input_{};
    q::AllSweepOwners owner_result_{};
    q::AllOverlapOwners overlap_owner_result_{};
    path::Workspace path_work_{};path::RawPath path_result_{};
    Stage stage_{};unsigned detail_{};
    gears_cover_query_guard::Witness guard_witness_{};
    struct NoLocalResult {};
    std::atomic<bool> configured_{},enabled_{},stopped_{},busy_{},closed_{true};
    std::atomic<DWORD> worker_{};
    struct Finish {std::atomic<bool>& busy;~Finish(){busy=false;}};
    template<class Guard,bool WithCollider> struct SourceGuard {
        Session& session;weapon_hand_scene_binding::Binding const& actor;
        gears_cover_pose_phase::Metadata const& metadata;Guard& owner;
        gears_cover_physx_collider::Source collider_source{};
        gears_cover_physx_components::Source const* component_source{};
        using Gate=gears_cover_query_guard::SessionStage;
        bool reject(Gate gate,unsigned detail=0){return session.guard_witness_.reject(unsigned(gate),detail);}
        bool components_current(){if constexpr(WithCollider){session.guard_witness_.enter();
            if(session.stopped_.load())return reject(Gate::stopped);
            const auto code=timing::measure(timing::Stage::components_repeat,[&](){return component_roles::sample(actor,owner,session.component_work_,session.component_after_);});
            if(code!=roles::Code::current)return reject(Gate::component_read,unsigned(code));
            if(!component_roles::same(session.component_before_,session.component_after_))return reject(Gate::component_match);
        }return true;}
        bool operator()(){
            session.guard_witness_.enter();
            if(session.stopped_.load())return reject(Gate::stopped);
            const auto code=timing::measure(timing::Stage::physics_repeat,[&](){return roles::recheck(actor,metadata,owner,session.work_,session.after_);});
            if(code!=roles::Code::current)return reject(Gate::physics_read,unsigned(code));
            if(!roles::same(session.before_,session.after_))return reject(Gate::physics_match);
            if constexpr(WithCollider){
                const auto collider=timing::measure(timing::Stage::collider_repeat,[&](){return collider_roles::recheck(actor,owner,session.collider_work_,session.collider_after_);});
                if(collider!=roles::Code::current)return reject(Gate::collider_read,unsigned(collider));
                if(!collider_roles::same(session.collider_before_,session.collider_after_))return reject(Gate::collider_match);
            }
            return true;
        }
        bool operator()(q::source::Snapshot const& raw){return (*this)()&&
            (roles::matches_query_source(session.before_,raw)||reject(Gate::query_source_match));}
        bool operator()(gears_cover_physx_asset_actors::Snapshot const& raw){return (*this)()&&
            (roles::matches_asset_actors(session.before_,raw)||reject(Gate::asset_match));}
        bool operator()(gears_cover_physx_collider::Snapshot const& raw){return WithCollider&&(*this)()&&
            (roles::matches_query_source(session.before_,raw.query_source)||reject(Gate::query_source_match))&&
            (collider_roles::matches(session.collider_before_,raw)||reject(Gate::collider_source_match));}
        bool operator()(gears_cover_physx_components::Snapshot const& raw){return WithCollider&&(*this)()&&components_current()&&
            (roles::matches_query_source(session.before_,raw.query_source)||reject(Gate::query_source_match))&&
            (component_roles::matches(session.component_before_,raw)||reject(Gate::component_source_match));}
    };
    template<class Shape,class Read,class Guard,class Output,class Invoke> q::Code execute(
        weapon_hand_scene_binding::Binding const& actor,gears_cover_pose_phase::Metadata const& metadata,
        Shape const& shape,Read read,Guard guard,Output& output,Invoke invoke){
        if(!enabled_.load()||stopped_.load())return q::Code::unavailable;
        if(q::source::aliases(&output,sizeof output,this,sizeof *this)||q::source::aliases(&output,sizeof output,&actor,sizeof actor)||
           q::source::aliases(&output,sizeof output,&metadata,sizeof metadata)||q::source::aliases(&output,sizeof output,&shape,sizeof shape))return q::Code::request;
        bool expected=false;if(!busy_.compare_exchange_strong(expected,true))return q::Code::reentrant;Finish finish{busy_};
        stage_=Stage::geometry;detail_=0;work_.witness={};guard_witness_={};
        if(stopped_.load())return q::Code::unavailable;
        const auto owner=actor;const auto source=metadata;const Shape geometry=shape;
        if(!q::detail::valid(geometry))return q::Code::request;
        stage_=Stage::physics_roles;
        const auto physics=timing::measure(timing::Stage::physics_initial,[&](){return roles::sample(owner,source,guard,work_,before_);});detail_=unsigned(physics);
        if(physics!=roles::Code::current)return q::Code::source;
        constexpr bool with_collider=std::is_same_v<Output,q::AllSweepOwners>||std::is_same_v<Output,q::AllOverlapOwners>||std::is_same_v<Output,path::RawPath>;
        if constexpr(with_collider){stage_=Stage::collider_roles;const auto collider=timing::measure(timing::Stage::collider_initial,[&](){return collider_roles::sample(owner,guard,collider_work_,collider_before_);});detail_=unsigned(collider);
            if(collider!=roles::Code::current)return q::Code::source;}
        if constexpr(with_collider){stage_=Stage::component_roles;const auto components=timing::measure(timing::Stage::components_initial,[&](){return component_roles::sample(owner,guard,component_work_,component_before_);});detail_=unsigned(components);
            if(components!=roles::Code::current||!component_roles::input(component_before_,component_input_))return q::Code::source;}
        SourceGuard<Guard,with_collider> current{*this,owner,source,guard};
        if constexpr(with_collider)current.collider_source=collider_roles::input(collider_before_);
        if constexpr(with_collider)current.component_source=&component_input_;
        // The larger ownership packet lives in the already serialized
        // Session, never as a 139KB temporary on the live game stack.
        std::conditional_t<with_collider,NoLocalResult,Output> local{};
        Output& result=[&]() -> Output& {if constexpr(std::is_same_v<Output,q::AllSweepOwners>)return owner_result_;
            else if constexpr(std::is_same_v<Output,q::AllOverlapOwners>)return overlap_owner_result_;
            else if constexpr(std::is_same_v<Output,path::RawPath>)return path_result_;else return local;}();
        stage_=Stage::engine;detail_=0;
        const auto code=timing::measure(timing::Stage::engine,[&](){return invoke(engine_,before_.source,geometry,read,current,result);});detail_=unsigned(code);
        if(code==q::Code::fault_unknown||engine_.uncertain())enabled_=false;
        if(code!=q::Code::queried)return code;
        stage_=Stage::closing;
        timing::Span closing_timing(timing::Stage::closing);
        if(!current()||!current.components_current()||!weapon_hand_scene_binding::same_key(owner,actor)||owner.scene_invocation!=actor.scene_invocation||
           owner.special_move!=actor.special_move||!gears_cover_pose_phase::metadata_equal(source,metadata)||
           std::memcmp(&geometry,&shape,sizeof shape))return q::Code::changed;
        output=result;stage_=Stage::complete;return q::Code::queried;
    }
public:
    template<class Read,class Guard,class Output> q::Code overlap_all(weapon_hand_scene_binding::Binding const& actor,
        gears_cover_pose_phase::Metadata const& metadata,q::Capsule const& shape,q::StreamLimits const& limits,
        Read read,Guard guard,Output& output)noexcept{
        static_assert(std::is_same_v<Output,q::AllOverlap>||std::is_same_v<Output,q::AllOverlapOwners>);
        if(!q::detail::stream_limits(limits)||q::source::aliases(&output,sizeof output,&limits,sizeof limits))return q::Code::request;
        const auto budget=limits;const auto requested_shape=shape;
        auto stable=[&](){return budget.max_hits==limits.max_hits&&budget.max_reports==limits.max_reports&&
            !std::memcmp(&requested_shape,&shape,sizeof shape);};
        auto current_owner=[&](){if(!stable())return false;const bool okay=guard();return okay&&stable();};
        try{return execute(actor,metadata,shape,read,current_owner,output,[&](auto& engine,auto const& source,auto const& geometry,auto reader,auto current,auto& raw){
            const auto code=[&](){if constexpr(std::is_same_v<Output,q::AllOverlapOwners>)
                return engine.overlap_all(source,geometry,budget,reader,current,raw,&current.collider_source,current.component_source);
                else return engine.overlap_all(source,geometry,budget,reader,current,raw);}();
            if(code!=q::Code::queried)return code;
            return stable()?q::Code::queried:q::Code::changed;});
        }catch(...){return q::Code::source;}
    }
    // One serialized owner operation. Every segment uses the same typed
    // source/collider/component closure and original timestamps. A prefix is
    // private scratch until endpoint and final owner checks both succeed.
    template<class Read,class Guard> q::Code sweep_capsule_path(
        weapon_hand_scene_binding::Binding const& actor,gears_cover_pose_phase::Metadata const& metadata,
        q::Capsule const& shape,q::Vector const& delta,path::Policy const& policy,q::StreamLimits const& limits,
        Read read,Guard guard,path::RawPath& output)noexcept {
        if(q::source::aliases(&output,sizeof output,&delta,sizeof delta)||
           q::source::aliases(&output,sizeof output,&policy,sizeof policy)||
           q::source::aliases(&output,sizeof output,&limits,sizeof limits)||!q::detail::stream_limits(limits))return q::Code::request;
        const auto movement=delta;const auto options=policy;const auto budget=limits;
        auto stable=[&](){return !std::memcmp(&movement,&delta,sizeof delta)&&
            options.segment_limit==policy.segment_limit&&options.maximum_radius_expansion==policy.maximum_radius_expansion&&
            budget.max_hits==limits.max_hits&&budget.max_reports==limits.max_reports;};
        auto current_owner=[&](){if(!stable())return false;const bool okay=guard();return okay&&stable();};
        try{return execute(actor,metadata,shape,read,current_owner,output,[&](auto& engine,auto const& source,auto const& geometry,auto reader,auto current,auto& result){
            const auto prepared=path::prepare(geometry,movement,options,path_work_,result.plan);
            if(prepared!=path::Code::planned)return prepared==path::Code::capacity?q::Code::truncated:q::Code::request;
            result.spans={};
            static_assert(std::is_trivially_copyable_v<q::AllOverlapOwners>);
            std::memset(&result.endpoint,0,sizeof result.endpoint);
            static_assert(std::is_trivially_copyable_v<q::AllSweepOwners>);
            std::memset(&result.collected,0,sizeof result.collected); // no large stack temporary
            auto& accumulated=result.collected;
            for(unsigned i=0;i<result.plan.count;++i){
                if(stopped_.load())return q::Code::unavailable;
                auto const& totals=accumulated.raw;
                if(totals.hit_count>=budget.max_hits||totals.reports>=budget.max_reports)return q::Code::truncated;
                const q::StreamLimits remaining{budget.max_hits-totals.hit_count,budget.max_reports-totals.reports};
                auto const& segment=result.plan.segments[i];
                const auto code=engine.sweep_all(source,segment.shape,segment.movement,remaining,reader,current,owner_result_,
                                                  &current.collider_source,current.component_source);
                if(code!=q::Code::queried)return code;
                if(!stable())return q::Code::changed;
                auto const& packet=owner_result_;auto const& raw=packet.raw;
                if(raw.raw_count!=raw.hit_count||raw.hit_count>remaining.max_hits||raw.reports>remaining.max_reports)return q::Code::callback_rejected;
                if(i==0){accumulated.asset_bodies=packet.asset_bodies;accumulated.collider_source=packet.collider_source;
                    accumulated.component_sources=packet.component_sources;}
                else if(!gears_cover_physx_asset_actors::same(accumulated.asset_bodies,packet.asset_bodies)||
                        !gears_cover_physx_collider::same(accumulated.collider_source,packet.collider_source)||
                        !gears_cover_physx_components::same(accumulated.component_sources,packet.component_sources))return q::Code::changed;
                const auto start=accumulated.raw.hit_count;
                result.spans[i]={start,raw.hit_count,raw.raw_count,raw.reports};
                for(unsigned j=0;j<raw.hit_count;++j){accumulated.raw.hits[start+j]=raw.hits[j];accumulated.owners[start+j]=packet.owners[j];accumulated.self_sources[start+j]=packet.self_sources[j];}
                accumulated.raw.hit_count+=raw.hit_count;accumulated.raw.raw_count+=raw.raw_count;accumulated.raw.reports+=raw.reports;
            }
            if(stopped_.load())return q::Code::unavailable;
            auto const& totals=accumulated.raw;
            if(totals.hit_count>=budget.max_hits||totals.reports>=budget.max_reports)return q::Code::truncated;
            const q::StreamLimits remaining{budget.max_hits-totals.hit_count,budget.max_reports-totals.reports};
            const auto code=engine.overlap_all(source,result.plan.endpoint,remaining,reader,current,result.endpoint,
                                               &current.collider_source,current.component_source);
            if(code!=q::Code::queried)return code;
            if(!stable())return q::Code::changed;
            auto const& endpoint=result.endpoint;
            if(endpoint.raw.raw_count!=endpoint.raw.shape_count||endpoint.raw.shape_count>remaining.max_hits||
               endpoint.raw.reports>remaining.max_reports)return q::Code::callback_rejected;
            if(!gears_cover_physx_asset_actors::same(accumulated.asset_bodies,endpoint.asset_bodies)||
               !gears_cover_physx_collider::same(accumulated.collider_source,endpoint.collider_source)||
               !gears_cover_physx_components::same(accumulated.component_sources,endpoint.component_sources))return q::Code::changed;
            return q::Code::queried;
        });}catch(...){return q::Code::source;}
    }
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
    // Same serialized owner only; this is not a cross-thread publication.
    Observation observation()const noexcept{return {stage_,detail_,engine_.observation(),roles::observation(work_),guard_witness_};}
    bool enabled()const noexcept{return enabled_.load()&&!stopped_.load();}
    template<class Shape,class Read,class Guard> q::Code overlap(weapon_hand_scene_binding::Binding const& actor,
        gears_cover_pose_phase::Metadata const& metadata,Shape const& shape,Read read,Guard guard,q::Overlap& output)noexcept{
        try{return execute(actor,metadata,shape,read,guard,output,[](auto& engine,auto const& source,auto const& geometry,auto reader,auto current,auto& result){
            return engine.overlap(source,geometry,reader,current,result);});}catch(...){return q::Code::source;}
    }
    template<class Shape,class Read,class Guard> q::Code sweep(weapon_hand_scene_binding::Binding const& actor,
        gears_cover_pose_phase::Metadata const& metadata,Shape const& shape,q::Vector const& delta,Read read,Guard guard,q::Sweep& output)noexcept{
        if(q::source::aliases(&output,sizeof output,&delta,sizeof delta)||!q::detail::sweep_motion(shape,delta))return q::Code::request;
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
        if(!q::detail::sweep_motion(shape,delta)||!q::detail::stream_limits(limits)||q::source::aliases(&output,sizeof output,&delta,sizeof delta)||
           q::source::aliases(&output,sizeof output,&limits,sizeof limits)||q::source::aliases(&output,sizeof output,this,sizeof *this)||
           q::source::aliases(&output,sizeof output,&actor,sizeof actor)||q::source::aliases(&output,sizeof output,&metadata,sizeof metadata)||
           q::source::aliases(&output,sizeof output,&shape,sizeof shape))return q::Code::request;
        const auto movement=delta;const auto budget=limits;
        try{return execute(actor,metadata,shape,read,guard,output,[&](auto& engine,auto const& source,auto const& geometry,auto reader,auto current,auto& raw){
            const auto code=[&](){if constexpr(std::is_same_v<Output,q::AllSweepOwners>)
                return engine.sweep_all(source,geometry,movement,budget,reader,current,raw,&current.collider_source,current.component_source);
                else return engine.sweep_all(source,geometry,movement,budget,reader,current,raw);}();
            if(code!=q::Code::queried)return code;
            return std::memcmp(&movement,&delta,sizeof delta)||std::memcmp(&budget,&limits,sizeof limits)?q::Code::changed:q::Code::queried;});
        }catch(...){return q::Code::source;}
    }
};
#endif
} // namespace cover_physics_query_live
#endif
