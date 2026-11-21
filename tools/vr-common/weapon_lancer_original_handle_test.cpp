#include "weapon_lancer_handle_compose.h"
#include <cstdio>
#include <limits>
namespace h=gears_lancer_original_handle;
unsigned checks{},failures{};
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line%d\n",__LINE__);}}while(false)
struct Fixture {
    std::array<h::Atom,9> local{},cache{};std::array<std::array<float,16>,9> space{};
    h::Source source{};h::Atom closed{};h::Matrix root=h::identity(),handle=h::identity();
    Fixture(){
        closed.rotation[2]=float(std::sqrt(.5));closed.rotation[3]=float(std::sqrt(.5));closed.scale=1;
        closed.position[0]=32.844944f;closed.position[1]=-4.6859608f;closed.position[2]=10.6705f;
        local[4]=closed;root[0]=root[5]=0;root[1]=1;root[4]=-1;
        handle[12]=4.6859608;handle[13]=32.844944;handle[14]=10.6705;
        for(unsigned i=0;i<16;++i){space[0][i]=float(root[i]);space[4][i]=float(handle[i]);}
        source={1,2,3,4,{reinterpret_cast<std::uintptr_t>(local.data()),9,9},
          {reinterpret_cast<std::uintptr_t>(cache.data()),0,9},{reinterpret_cast<std::uintptr_t>(space.data()),9,9},1,true};
    }
    template<class T>bool read(std::uintptr_t at,T& out){
        const auto copy=[&](auto const& a){const auto p=reinterpret_cast<std::uintptr_t>(a.data());
            if(at<p||at-p>sizeof a||sizeof(T)>sizeof(a)-(at-p))return false;std::memcpy(&out,reinterpret_cast<void const*>(at),sizeof(T));return true;};
        return copy(local)||copy(cache)||copy(space);
    }
    bool write(std::uintptr_t at,h::Atom const& a){
        if(at!=source.local.data+4*32&&at!=source.cache.data+4*32)return false;
        std::memcpy(reinterpret_cast<void*>(at),&a,sizeof a);return true;
    }
    auto reader(){return [this](auto at,auto& out){return read(at,out);};}
    auto writer(){return [this](auto at,h::Atom const& a){return write(at,a);};}
    template<class Write>bool decode(h::Lease& lease,Write write){
        local[4]=closed;local[4].rotation[3]=-local[4].rotation[3];
        const bool ok=lease.decoder_return(source,source.local.data+4*32,4,reader(),write);
        // Independent reproduction of the certified caller's later store.
        local[4].rotation[3]=-local[4].rotation[3];return ok;
    }
    bool decode(h::Lease& lease){return decode(lease,writer());}
    void update_space(){const auto m=h::multiply(h::matrix(local[4],true),root);for(unsigned i=0;i<16;++i)space[4][i]=float(m[i]);}
};
int main(){
    // Exhaustively compare the policy, with fresh observers and all boolean
    // combinations. Include NaN/Inf/out-of-range values and spring returns.
    const double angles[]={0,.2,h::maximum_angle,-.1,h::maximum_angle+.1,
        std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()};
    const double pulls[]={0,1,h::maximum_pull_raw,-.1,h::maximum_pull_raw+.1,
        std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()};
    for(double a:angles)for(double ba:angles)for(double p:pulls)for(double bp:pulls)
        for(unsigned flags=0;flags<16;++flags){
            const bool render=(flags&1)!=0,part=(flags&2)!=0,socket=(flags&4)!=0,home=(flags&8)!=0;
            unsigned pc{},sc{};
            const bool actual=h::update_admitted_lazy(a,p,ba,bp,render,
                [&](){++pc;return part;},[&](){++sc;return socket;},home);
            CHECK(actual==h::update_admitted(a,p,ba,bp,render,part,socket,home));
            CHECK(pc<=1&&sc<=1);
            if(!render)CHECK(!pc&&!sc);
        }
    const auto counted=[](double a,double p,double ba,double bp,bool render,bool part,bool socket,
        bool home,unsigned expected_part,unsigned expected_socket,bool expected){
        unsigned rc{},pc{},sc{};
        const auto render_read=[&](){++rc;return render;};
        CHECK(h::update_admitted_lazy(a,p,ba,bp,render_read(),[&](){++pc;return part;},
            [&](){++sc;return socket;},home)==expected);
        CHECK(rc==1&&pc==expected_part&&sc==expected_socket);
    };
    counted(0,0,0,0,true,false,false,false,0,0,true); // closed idle: three admissions -> one
    counted(.2,0,.2,0,true,false,false,false,0,0,true); // held unchanged
    counted(.2,0,0,0,true,true,false,false,1,0,true); // deploy with part
    counted(.2,0,0,0,true,false,true,false,1,1,true); // deploy with socket
    counted(.2,0,0,0,true,false,false,false,1,1,false);
    counted(0,0,.2,0,true,false,false,false,0,0,true); // empty fold
    counted(0,1,.2,1,true,false,true,false,1,0,false); // loaded fold needs part
    counted(.2,1,.2,0,true,false,true,false,1,0,false); // socket never slides
    counted(.2,0,.2,1,true,false,false,true,0,0,true); // authenticated return policy
    counted(.2,1,.2,0,false,true,true,false,0,0,false);
    counted(-.1,0,0,0,true,true,true,false,0,0,false);
    CHECK(h::defer_closed_reload(false,false,false,false,false,{},0));
    CHECK(h::defer_closed_reload(true,false,false,true,true,{},0));
    CHECK(!h::defer_closed_reload(true,false,false,false,true,{},0));
    CHECK(!h::defer_closed_reload(true,false,false,true,false,{},0));
    CHECK(!h::defer_closed_reload(true,true,false,true,true,{},0));
    CHECK(!h::defer_closed_reload(true,false,true,true,true,{},0));
    CHECK(!h::defer_closed_reload(true,false,false,true,true,{.1,0},.1));
    CHECK(!h::defer_closed_reload(true,false,false,true,true,{0,.1},0));
    CHECK(!h::defer_closed_reload(true,false,false,true,true,{},.1));
    // Untouched reference-pose node: no decoder, no sequence and empty cache.
    // The original composer is represented by update_space(), never by a
    // production SpaceBases store. Only local bone4 may be borrowed.
    {
        Fixture f;h::ComposeLease p;auto read=f.reader();auto write=f.writer();
        CHECK(p.begin(f.source,read));CHECK(p.ready(f.source,read));
        const auto all_before=f.local;const auto cache_before=f.cache;
        CHECK(p.wanted(h::maximum_angle,4));CHECK(!p.ready(f.source,read));
        CHECK(p.compose_begin(f.source,read,write));CHECK(p.active());
        CHECK(!p.compose_begin(f.source,read,write)); // no nested journal
        for(unsigned i=0;i<9;++i)if(i!=4)CHECK(h::same(f.local[i],all_before[i]));
        f.update_space();++f.source.epoch;
        CHECK(p.compose_end(f.source,true,read,write));CHECK(!p.active());
        CHECK(h::same(f.local[4],f.closed));CHECK(p.ready(f.source,read));
        CHECK(!std::memcmp(f.cache.data(),cache_before.data(),sizeof f.cache));
        p.request_restore();CHECK(!p.restore(f.source,read,write)); // open evaluated geometry
        f.update_space();++f.source.epoch;CHECK(p.restore(f.source,read,write));CHECK(!p.bound());
    }
    {
        Fixture f;h::ComposeLease p;auto read=f.reader();CHECK(p.begin(f.source,read));
        CHECK(p.wanted(.5,1));unsigned stores{};
        auto partial=[&](auto at,h::Atom const& a){++stores;std::memcpy(reinterpret_cast<void*>(at),&a,8);return false;};
        CHECK(!p.compose_begin(f.source,read,partial));
        CHECK(p.restore_local(f.source,read,f.writer()));CHECK(!p.active());
        CHECK(h::same(f.local[4],f.closed));CHECK(stores>0);
        CHECK(p.compose_begin(f.source,read,f.writer()));
        f.local[4].scale=2; // third-party value is not ours to restore
        CHECK(!p.restore_local(f.source,read,f.writer()));CHECK(p.active());
        auto other=f.source;other.local.data+=32;
        CHECK(!p.restore_local(other,read,f.writer()));CHECK(p.active());
        CHECK(f.local[4].scale==2);
    }
    {
        Fixture f;h::ComposeLease p;CHECK(p.begin(f.source,f.reader()));CHECK(p.wanted(.4));
        CHECK(p.compose_begin(f.source,f.reader(),f.writer()));
        CHECK(!p.compose_end(f.source,false,f.reader(),f.writer()));
        CHECK(p.restoring());CHECK(!p.active());CHECK(h::same(f.local[4],f.closed));
        CHECK(!p.ready(f.source,f.reader()));CHECK(p.restore(f.source,f.reader(),f.writer()));
    }

    // Same canonical rest orientation with the opposite quaternion sign is
    // not a foreign animation. Journal/restore its exact CURRENT bytes.
    {
        Fixture f;h::ComposeLease p;CHECK(p.begin(f.source,f.reader()));CHECK(p.wanted(h::maximum_angle,8));
        for(auto& q:f.local[4].rotation)q=-q;
        f.local[4].position[0]=std::nextafter(f.local[4].position[0],100.f);
        const auto current_rest=f.local[4];CHECK(h::original(current_rest));
        const bool composed=p.compose_begin(f.source,f.reader(),f.writer());CHECK(composed);
        if(composed){f.update_space();++f.source.epoch;CHECK(p.compose_end(f.source,true,f.reader(),f.writer()));
            CHECK(h::same(f.local[4],current_rest));CHECK(p.ready(f.source,f.reader()));}
    }
    // A genuinely animated/noncanonical C atom must still be left untouched.
    {
        Fixture f;h::ComposeLease p;CHECK(p.begin(f.source,f.reader()));CHECK(p.wanted(h::maximum_angle,8));
        f.local[4].position[0]+=.05f;const auto foreign=f.local[4];
        CHECK(!p.compose_begin(f.source,f.reader(),f.writer()));CHECK(h::same(f.local[4],foreign));CHECK(!p.active());
    }

    //62548: partial release readback2.00046 differs from the last accepted
    //2.000674 request. Reissuing it needs a now-released part lease and leaves
    //C unready, so a same-position regrasp wrongly acquires the magazine.
    h::DisplayTarget locked{};
    CHECK(!h::update_admitted(h::maximum_angle,2.00046,h::maximum_angle,2.000674,true,false,true));
    CHECK(h::display_target(true,{h::maximum_angle,2.00046},{h::maximum_angle,2.000674},true,locked));
    CHECK(locked.pull==2.000674&&locked.angle==h::maximum_angle);
    CHECK(h::update_admitted(locked.angle,locked.pull,h::maximum_angle,2.000674,true,false,true));
    CHECK(h::display_target(false,{h::maximum_angle,7.5},locked,true,locked));
    CHECK(locked.pull==7.5);
    CHECK(!h::update_admitted(locked.angle,locked.pull,h::maximum_angle,2.000674,true,false,true));
    CHECK(h::update_admitted(locked.angle,locked.pull,h::maximum_angle,2.000674,true,true,false));
    CHECK(!h::display_target(true,{0,0},{0,0},false,locked));
    CHECK(!h::display_target(false,{0,9},{0,0},true,locked));
    // Runtime12780: the old angle-increase predicate rejected neutral socket
    // deployment because it had no part/action lease. Only the angle may move.
    CHECK(!((.2<=0&&0<=0)||false)); // old admission, same observed inputs
    CHECK(h::update_admitted(.2,0,0,0,true,false,true));
    CHECK(h::update_admitted(h::maximum_angle,0,.2,0,true,false,true));
    CHECK(!h::update_admitted(.3,0,.2,0,true,false,false));
    CHECK(!h::update_admitted(.3,0,.2,0,false,true,true));
    for(bool socket:{false,true}){
        CHECK(!h::update_admitted(h::maximum_angle,1,h::maximum_angle,0,true,false,socket));
        CHECK(!h::update_admitted(h::maximum_angle,0,h::maximum_angle,1,true,false,socket));
        CHECK(!h::update_admitted(0,1,h::maximum_angle,1,true,false,socket));
        CHECK(h::update_admitted(h::maximum_angle,1,h::maximum_angle,1,true,false,socket));
    }
    CHECK(h::update_admitted(0,0,h::maximum_angle,0,true,false,false));
    CHECK(h::update_admitted(h::maximum_angle,7,h::maximum_angle,2,true,true,false));
    CHECK(h::update_admitted(h::maximum_angle,0,h::maximum_angle,7,true,true,false));
    CHECK(!h::update_admitted(h::maximum_angle,9,h::maximum_angle,2,true,true,true));
    CHECK(!h::update_admitted(std::numeric_limits<double>::quiet_NaN(),0,0,0,true,true,true));
    // Released cosmetic spring can only go home; proximity never pushes it.
    CHECK(h::update_admitted(h::maximum_angle,7,h::maximum_angle,8,true,false,false,true));
    CHECK(h::update_admitted(h::maximum_angle,0,h::maximum_angle,1,true,false,false,true));
    CHECK(!h::update_admitted(h::maximum_angle,8,h::maximum_angle,7,true,false,true,true));
    CHECK(!h::update_admitted(0,0,h::maximum_angle,1,true,false,false,true));
    CHECK(!h::update_admitted(h::maximum_angle,0,h::maximum_angle,1,false,false,false,true));
    Fixture f;h::Plan p{};CHECK(p.bind(f.closed,f.root,f.handle));CHECK(p.conjugated);
    // Independent original-surface trajectory: radius is preserved around
    // the actual handle pivot; no disconnected translating cord or cap.
    const auto g=h::closed_grasp;const double dx=g[0]-f.handle[12],dy=g[1]-f.handle[13];
    for(unsigned i=0;i<=32;++i){
        const double angle=h::maximum_angle*i/32.;h::Atom a{};h::Matrix expected{};
        CHECK(p.target(angle,a,expected));const auto actual=h::multiply(h::matrix(a,p.conjugated),f.root);
        const auto bind_inverse=h::inverse(f.handle);const h::Matrix skin=h::multiply(bind_inverse,actual);
        double moved[3]{};for(unsigned j=0;j<3;++j)moved[j]=g[0]*skin[j]+g[1]*skin[4+j]+g[2]*skin[8+j]+skin[12+j];
        const double x=f.handle[12]+dx*std::cos(angle)-dy*std::sin(angle),y=f.handle[13]+dx*std::sin(angle)+dy*std::cos(angle);
        CHECK(std::hypot(moved[0]-x,moved[1]-y)<.0001);CHECK(std::abs(moved[2]-g[2])<.0001);CHECK(a.scale==1);
    }
    h::Atom unused{};h::Matrix m{};CHECK(!p.target(-.1,unused,m));CHECK(!p.target(2.,unused,m));
    // The authored VR gesture slides the unfolded C bar rearward. Translation
    // must not turn the hand or move another bone, and must restore exactly.
    h::Matrix unfolded{};CHECK(p.target(h::maximum_angle,0,unused,unfolded));
    for(unsigned i=0;i<=16;++i){
        const double pull=h::maximum_pull_raw*i/16.;h::Atom a{};h::Matrix slid{};
        CHECK(p.target(h::maximum_angle,pull,a,slid));
        CHECK(std::abs(slid[13]-(unfolded[13]+pull))<.0001);
        for(unsigned j=0;j<12;++j)CHECK(std::abs(slid[j]-unfolded[j])<.0001);
        CHECK(h::close(h::multiply(h::matrix(a,p.conjugated),f.root),slid));
    }
    CHECK(!p.target(h::maximum_angle,-1,unused,m));CHECK(!p.target(h::maximum_angle,h::maximum_pull_raw+.1,unused,m));
    {
        Fixture moving;h::Lease slide;CHECK(slide.begin(moving.source,moving.reader()));
        CHECK(slide.wanted(h::maximum_angle,h::maximum_pull_raw));CHECK(moving.decode(slide));
        moving.update_space();CHECK(slide.ready(moving.source,moving.reader()));
        moving.source.cache.count=9;moving.cache=moving.local;CHECK(slide.observe_cache(moving.source,moving.reader()));
        slide.request_restore();CHECK(!slide.restore(moving.source,moving.reader(),moving.writer()));
        CHECK(h::same(moving.local[4],moving.closed)&&h::same(moving.cache[4],moving.closed));
        moving.update_space();CHECK(slide.restore(moving.source,moving.reader(),moving.writer()));CHECK(!slide.bound());
    }
    CHECK(!p.target(std::numeric_limits<double>::quiet_NaN(),unused,m));
    auto bad=f.root;bad[0]=1;h::Plan invalid{};CHECK(!invalid.bind(f.closed,bad,f.handle));
    bad=f.handle;bad[12]+=1;CHECK(!invalid.bind(f.closed,f.root,bad));
    auto wrong=f.closed;wrong.position[0]+=1;CHECK(!invalid.bind(wrong,f.root,f.handle));
    h::Plan direct{};CHECK(direct.bind(f.closed,f.root,h::multiply(h::matrix(f.closed,false),f.root)));CHECK(!direct.conjugated);
    h::Lease lease;CHECK(lease.begin(f.source,f.reader()));CHECK(!lease.begin(f.source,f.reader()));
    CHECK(lease.ready(f.source,f.reader()));CHECK(lease.writes()==0);
    f.space[4][12]+=1;CHECK(!lease.ready(f.source,f.reader()));f.space[4][12]-=1;
    CHECK(lease.wanted(.4));CHECK(!lease.ready(f.source,f.reader()));
    const auto before=f.local;CHECK(!lease.decoder_return(f.source,f.source.local.data+3*32,3,f.reader(),f.writer()));
    CHECK(f.decode(lease));
    for(unsigned i=0;i<9;++i)if(i!=4)CHECK(h::same(f.local[i],before[i]));
    CHECK(!lease.ready(f.source,f.reader()));f.update_space();CHECK(lease.ready(f.source,f.reader()));
    // Observe an ACTUAL evaluator cache copy, then change the desired angle.
    // The old cached atom must retain its own journal, not the newer value.
    f.source.cache.count=9;f.cache=f.local;CHECK(lease.observe_cache(f.source,f.reader()));CHECK(lease.wanted(.9));
    f.local[4]=f.closed;++f.source.epoch;
    CHECK(f.decode(lease));f.update_space();
    CHECK(lease.ready(f.source,f.reader()));lease.request_restore();CHECK(!lease.wanted(.2));
    CHECK(!lease.restore(f.source,f.reader(),f.writer()));CHECK(h::same(f.local[4],f.closed));CHECK(h::same(f.cache[4],f.closed));
    CHECK(lease.cpu_restored());f.update_space();CHECK(lease.restore(f.source,f.reader(),f.writer()));CHECK(!lease.bound());
    Fixture a;h::Lease interrupted;CHECK(interrupted.begin(a.source,a.reader()));CHECK(interrupted.wanted(.7));
    CHECK(a.decode(interrupted));
    interrupted.request_restore();a.local[4].position[0]+=3;
    CHECK(!interrupted.restore(a.source,a.reader(),a.writer(),true));CHECK(interrupted.bound());
    Fixture z;h::Lease stale;CHECK(stale.begin(z.source,z.reader()));auto other=z.source;other.identity_generation++;
    CHECK(!stale.decoder_return(other,z.source.local.data+4*32,4,z.reader(),z.writer()));
    other=z.source;other.space.data=other.local.data;CHECK(!h::valid(other));
    Fixture e;h::Lease ambiguous;CHECK(ambiguous.begin(e.source,e.reader()));CHECK(ambiguous.wanted(.4));
    const auto partial=[&](auto at,h::Atom const& value){auto a=value;a.position[0]+=2;return e.write(at,a);};
    CHECK(!e.decode(ambiguous,partial));
    ambiguous.request_restore();CHECK(!ambiguous.restore(e.source,e.reader(),e.writer(),true));CHECK(ambiguous.bound());
    Fixture conflict;h::Lease foreign;CHECK(foreign.begin(conflict.source,conflict.reader()));CHECK(foreign.wanted(.8));
    CHECK(conflict.decode(foreign));
    foreign.request_restore();conflict.local[4]=conflict.closed;conflict.local[4].rotation[2]*=-1;
    CHECK(!foreign.restore(conflict.source,conflict.reader(),conflict.writer(),true));CHECK(foreign.bound());
    std::printf("OriginalLancerHandle: %u checks, %u failures; CPU only\n",checks,failures);return failures?1:0;
}
