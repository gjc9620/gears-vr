#pragma once
#include "weapon_lancer_original_handle.h"

namespace gears_lancer_original_handle {
// One original Compose invocation, whether the node has an animation or uses
// its reference pose. Only LocalAtoms[4] is borrowed. The original composer
// produces SpaceBases; neither those matrices nor CachedBoneAtoms are written.
class ComposeLease {
    Source source_{};Plan plan_{};Atom before_{},written_{};
    bool bound_{},restoring_{},owned_{},published_{};
    double angle_{},pull_{};std::uint64_t writes_{};unsigned epoch_{};
    static std::uintptr_t at(Source const& s) noexcept{return s.local.data+4*32;}
    bool match(Source const& s) const noexcept {
        return bound_&&valid(s)&&part::identity(source_,s)&&
            part::same(source_.local,s.local)&&part::same(source_.space,s.space);
    }
    template<class Read>bool geometry(Source const& s,bool closed,Read read) const noexcept {
        std::array<float,16> h{},r{};Matrix actual{},root{},expected{};Atom a{};
        if(!match(s)||!read(s.space.data+4*64,h)||!read(s.space.data,r))return false;
        std::copy(h.begin(),h.end(),actual.begin());std::copy(r.begin(),r.end(),root.begin());
        if(!close(root,plan_.root))return false;
        if(closed)return close(actual,plan_.space);
        return plan_.target(angle_,pull_,a,expected)&&close(actual,expected);
    }
public:
    bool bound() const noexcept{return bound_;}
    bool active() const noexcept{return owned_;}
    bool restoring() const noexcept{return restoring_;}
    bool cpu_restored() const noexcept{return restoring_&&!owned_;}
    std::uint64_t writes() const noexcept{return writes_;}
    double wanted() const noexcept{return angle_;}
    double wanted_pull() const noexcept{return pull_;}
    Plan const& plan() const noexcept{return plan_;}
    void request_restore() noexcept{if(bound_)restoring_=true;}
    template<class Read>bool begin(Source const& s,Read read) noexcept {
        if(bound_||!valid(s))return false;
        Atom a{};std::array<float,16> r{},h{};Matrix root{},space{};
        if(!read(at(s),a)||!read(s.space.data,r)||!read(s.space.data+4*64,h))return false;
        std::copy(r.begin(),r.end(),root.begin());std::copy(h.begin(),h.end(),space.begin());
        Plan p{};if(!p.bind(a,root,space))return false;
        source_=s;plan_=p;bound_=true;return true;
    }
    bool wanted(double angle,double pull=0) noexcept {
        if(!bound_||restoring_||!std::isfinite(angle)||angle<0||angle>maximum_angle||
            !std::isfinite(pull)||pull<0||pull>maximum_pull_raw)return false;
        if(angle!=angle_||pull!=pull_)published_=false;
        angle_=angle;pull_=pull;return true;
    }
    template<class Read>bool observe_cache(Source const& s,Read) const noexcept{return match(s);}
    // A failed/partial store is still our debt. Compare every byte against the
    // original and requested atom before restoring; never overwrite a third value.
    template<class Read,class Write>bool restore_local(Source const& s,Read read,Write write) noexcept {
        if(!match(s))return false;if(!owned_)return true;
        Atom current{};if(!read(at(s),current))return false;
        const auto* c=reinterpret_cast<unsigned char const*>(&current);
        const auto* b=reinterpret_cast<unsigned char const*>(&before_);
        const auto* w=reinterpret_cast<unsigned char const*>(&written_);
        for(unsigned i=0;i<sizeof(Atom);++i)if(c[i]!=b[i]&&c[i]!=w[i])return false;
        if(!same(current,before_))(void)write(at(s),before_);
        if(!read(at(s),current)||!same(current,before_))return false;
        owned_=false;return true;
    }
    template<class Read,class Write>bool compose_begin(Source const& s,Read read,Write write) noexcept {
        if(!match(s)||restoring_||owned_)return false;
        published_=false;Matrix expected{};
        if(!plan_.target(angle_,pull_,written_,expected)||!read(at(s),before_)||
            !original(before_)||!close(matrix(before_,plan_.conjugated),matrix(plan_.closed,plan_.conjugated)))return false;
        // q and -q encode the same canonical rest. Accept only the original
        // tightly bounded rest geometry, then journal THESE exact bytes.
        // A real animation/foreign value still fails without any store.
        if(same(before_,written_))return true;
        owned_=true;(void)write(at(s),written_);
        Atom actual{};if(!read(at(s),actual)||!same(actual,written_)){
            (void)restore_local(s,read,write);return false;
        }
        ++writes_;return true;
    }
    template<class Read,class Write>bool compose_end(Source const& s,bool normal,Read read,Write write) noexcept {
        if(!restore_local(s,read,write))return false;
        if(!normal){request_restore();return false;}
        if(restoring_)return true;
        published_=geometry(s,false,read);if(published_)epoch_=s.epoch;
        return published_;
    }
    template<class Read>bool ready(Source const& s,Read read) const noexcept {
        return !restoring_&&!owned_&&match(s)&&s.epoch>=epoch_&&
            (published_||(angle_==0&&pull_==0))&&geometry(s,false,read);
    }
    template<class Read,class Write>bool restore(Source const& s,Read read,Write write,bool inactive=false) noexcept {
        if(!restoring_||!restore_local(s,read,write)||(!inactive&&!geometry(s,true,read)))return false;
        bound_=false;return true;
    }
};
}
