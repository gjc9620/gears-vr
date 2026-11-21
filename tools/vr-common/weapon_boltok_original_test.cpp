#ifdef NDEBUG
#undef NDEBUG
#endif
#include "weapon_boltok_original.h"
#include <cassert>
#include <cstdio>
namespace api=gears_boltok_original;
struct Fixture {
    std::array<api::Atom,12> local{};std::array<std::array<float,16>,12> space{};
    api::Source source{};
    explicit Fixture(bool conjugated=false){
        for(auto& a:local){a.rotation[3]=-1;a.scale=1;}
        local[0].position[1]=8.7f;
        local[0].rotation[2]=.2f;local[0].rotation[3]=-float(std::sqrt(.96));
        local[1].position[0]=2.0895741f;local[1].position[1]=12.955615f;local[1].position[2]=5.322966f;
        local[2].position[0]=-1.749605f;local[2].position[1]=3.56018f;local[2].position[2]=3.53647f;
        local[3].position[0]=-2.0895741f;local[3].position[1]=-.1826706f;local[3].position[2]=3.645301f;
        local[8].rotation[0]=-.43253204f;local[8].rotation[3]=-.90161854f;
        for(unsigned i=0;i<12;++i){auto m=api::math::matrix(local[i],conjugated);
            if(i){api::Matrix parent{};std::copy(space[unsigned(api::parents[i])].begin(),space[unsigned(api::parents[i])].end(),parent.begin());m=api::math::multiply(m,parent);}
            for(unsigned j=0;j<16;++j)space[i][j]=float(m[j]);}
        source={1,2,3,4,{reinterpret_cast<std::uintptr_t>(local.data()),12,12},{},
            {reinterpret_cast<std::uintptr_t>(space.data()),12,12},1,true};
    }
    template<class T>bool read(std::uintptr_t at,T& v){
        const auto in=[&](auto const& a){auto start=reinterpret_cast<std::uintptr_t>(a.data());return at>=start&&at<=start+sizeof(a)-sizeof(T);};
        if(!in(local)&&!in(space))return false;std::memcpy(&v,reinterpret_cast<void const*>(at),sizeof v);return true;
    }
    bool write(std::uintptr_t at,api::Atom const& a){std::memcpy(reinterpret_cast<void*>(at),&a,sizeof a);return true;}
};
int main(){
    for(bool conjugated:{false,true}){
        Fixture f(conjugated);const auto original=f.local;api::ComposeLease lease;
        const auto read=[&](auto at,auto& v){return f.read(at,v);};const auto write=[&](auto at,auto const& v){return f.write(at,v);};
        assert(lease.bind(f.source,read)&&lease.plan().conjugated==conjugated);
        assert(lease.apply(f.source,api::maximum_angle,false,read,write));
        std::array<api::Matrix,12> composed{};
        for(unsigned i=0;i<12;++i){composed[i]=api::math::matrix(f.local[i],conjugated);
            if(i)composed[i]=api::math::multiply(composed[i],composed[unsigned(api::parents[i])]);}
        api::Matrix rear{};std::copy(f.space[11].begin(),f.space[11].end(),rear.begin());
        assert(api::math::close(composed[11],rear));
        for(unsigned i:{2u,3u}){
            double moved{},before_radius{},after_radius{};
            for(unsigned axis=0;axis<3;++axis){const auto j=12+axis;
                moved+=std::pow(composed[i][j]-f.space[i][j],2);
                before_radius+=std::pow(f.space[i][j]-f.space[1][j],2);
                after_radius+=std::pow(composed[i][j]-composed[1][j],2);}
            assert(moved>1&&std::abs(before_radius-after_radius)<.001);
        }
        assert(lease.restore(f.source,read,write));
        assert(lease.apply(f.source,api::maximum_angle,true,read,write)&&lease.active());
        assert(std::abs(f.local[1].rotation[0])+std::abs(f.local[1].rotation[2])<1e-5);
        assert(std::abs(std::abs(f.local[1].rotation[1])-std::sin(api::maximum_angle/2))<1e-5);
        assert(f.local[3].scale==0);
        for(unsigned i=0;i<12;++i)if(i!=1&&i!=3)assert(api::math::same(f.local[i],original[i]));
        assert(!lease.apply(f.source,0,false,read,write)); // Nested writer rejected.
        auto bad=f.source;++bad.identity_generation;assert(!lease.restore(bad,read,write)&&lease.active());
        assert(lease.restore(f.source,read,write)&&!lease.active()&&!std::memcmp(f.local.data(),original.data(),sizeof original));
        assert(lease.apply(f.source,0,false,read,write)&&!lease.active());
        assert(!lease.apply(f.source,api::maximum_angle+.01,false,read,write));
    }
    for(unsigned mode=0;mode<4;++mode){
        Fixture f;const auto original=f.local;api::ComposeLease lease;unsigned calls=0;
        auto read=[&](auto at,auto& v){return f.read(at,v);};
        assert(lease.bind(f.source,read));
        const auto failed=[&](auto at,api::Atom const& v){++calls;
            if((mode<2&&calls==1)||(mode>=2&&calls==2)){
                if(mode&1u)std::memcpy(reinterpret_cast<void*>(at),&v,8);
                return false;
            }return f.write(at,v);};
        assert(!lease.apply(f.source,1,true,read,failed)&&!lease.active()&&!std::memcmp(f.local.data(),original.data(),sizeof original));
    }
    {Fixture f;api::ComposeLease lease;auto read=[&](auto at,auto& v){return f.read(at,v);};
        auto write=[&](auto at,auto const& v){return f.write(at,v);};assert(lease.bind(f.source,read));
        assert(lease.apply(f.source,1,false,read,write));auto ours=f.local[1];f.local[1].position[0]=123;
        assert(!lease.restore(f.source,read,write)&&lease.active()&&f.local[1].position[0]==123);
        f.local[1]=ours;assert(lease.restore(f.source,read,write)&&!lease.active());}
    std::puts("Boltok original Compose lease: both conventions, fixed bones, nested/recycled source, partial/failed writes and foreign-writer restoration passed (offline only).");
}
