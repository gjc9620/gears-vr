#include "weapon_part_visibility.h"
#include <array>
#include <cstdio>
#include <cstdlib>
namespace {
namespace p=gears_weapon_part_visibility;
unsigned checks{};void check(bool ok,char const* label){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",label);std::exit(1);}}
struct Memory {
    std::array<p::Atom,5> local{},cache{};std::array<p::SpaceMatrix,5> space{};bool fail_read{},ambiguous_write{};unsigned writes{};
    Memory(){for(auto a:{&local,&cache})for(auto& atom:*a){atom.rotation[3]=1;atom.scale=1;}evaluate();}
    void evaluate(){for(unsigned i=0;i<space.size();++i){auto& m=space[i].value;for(auto& v:m)v=0;
        m[0]=m[5]=m[10]=local[i].scale;m[15]=1;}}
    template<class T>bool read(std::uintptr_t at,T& out){
        if(fail_read)return false;
        for(auto a:{&local,&cache}){const auto first=reinterpret_cast<std::uintptr_t>(a->data());
            if(at>=first&&at+sizeof(T)<=first+sizeof(*a)){std::memcpy(&out,reinterpret_cast<void const*>(at),sizeof(T));return true;}}
        const auto first=reinterpret_cast<std::uintptr_t>(space.data());
        if(at>=first&&at+sizeof(T)<=first+sizeof(space)){std::memcpy(&out,reinterpret_cast<void const*>(at),sizeof(T));return true;}
        return false;
    }
    bool write(std::uintptr_t at,float v){float old{};if(!read(at,old))return false;
        std::memcpy(reinterpret_cast<void*>(at),&v,4);++writes;return !ambiguous_write;
    }
    p::Source source(){return {17,25,33,4,
        {reinterpret_cast<std::uintptr_t>(local.data()),5,5},{reinterpret_cast<std::uintptr_t>(cache.data()),5,5},
        {reinterpret_cast<std::uintptr_t>(space.data()),5,5},8,true};}
};
}
int main(){
    Memory m;auto read=[&](auto at,auto& out){return m.read(at,out);};auto write=[&](auto at,float v){return m.write(at,v);};
    auto source=m.source();p::ScaleLease lease;
    check(lease.begin(source,read),"current original5-bone source");
    check(!lease.begin(source,read),"no duplicate transaction");
    check(!lease.decoder_return(source,source.local.data,0,read,write),"root never hidden");
    auto wrong=source;wrong.identity_generation=5;
    check(!lease.decoder_return(wrong,source.local.data+128,4,read,write),"old generation cannot write");
    check(lease.decoder_return(source,source.local.data+128,4,read,write),"only original magazine scalar hidden");
    check(m.local[4].scale==0&&m.local[0].scale==1&&m.local[3].scale==1&&lease.writes()==1,"other gun parts unchanged");
    check(!lease.hidden(source,read),"CPU setter is not original evaluated-space visibility");
    check(m.space[2].value[7]==0&&!lease.hidden(source,read),"matrix2 homogeneous zero cannot fake bone4 hidden receipt");
    m.cache=m.local;m.evaluate();
    check(lease.observe_cache(source,read)&&lease.hidden(source,read),"original sequence copy and original space observed");
    lease.request_restore();
    check(!lease.decoder_return(source,source.local.data+128,4,read,write),"no hide after withdrawal");
    check(!lease.restore(source,read,write)&&lease.bound()&&lease.cpu_restored(),"CPU restore keeps debt until original space regenerated");
    check(m.local[4].scale==1&&m.cache[4].scale==1&&m.space[4].value[0]==0,"restore only own local/cache scalars");
    m.evaluate();++source.epoch;
    check(lease.restore(source,read,write)&&!lease.bound(),"actual original space restore retires");
    Memory b;p::ScaleLease ambiguous;auto rb=[&](auto at,auto& out){return b.read(at,out);};auto wb=[&](auto at,float v){return b.write(at,v);};
    auto s=b.source();check(ambiguous.begin(s,rb),"ambiguous write setup");b.ambiguous_write=true;
    check(ambiguous.decoder_return(s,s.local.data+128,4,rb,wb)&&ambiguous.writes()==1,"readback reconciles successful write with uncertain return");
    b.cache=b.local;b.evaluate();check(ambiguous.observe_cache(s,rb),"own cache copy");
    ambiguous.request_restore();b.fail_read=true;const auto before=b.writes;
    check(!ambiguous.restore(s,rb,wb)&&b.writes==before&&ambiguous.bound(),"missing metadata/read is not cleanup");
    b.fail_read=false;check(!ambiguous.restore(s,rb,wb)&&ambiguous.cpu_restored(),"ambiguous restoration readback reconciled");
    b.evaluate();++s.epoch;check(ambiguous.restore(s,rb,wb),"reconciled restore reaches original output");
    Memory c;p::ScaleLease conflict;auto rc=[&](auto at,auto& out){return c.read(at,out);};auto wc=[&](auto at,float v){return c.write(at,v);};
    s=c.source();check(conflict.begin(s,rc)&&conflict.decoder_return(s,s.local.data+128,4,rc,wc),"conflict setup");
    c.cache=c.local;conflict.observe_cache(s,rc);conflict.request_restore();c.local[4].scale=.5f;
    const auto writes=c.writes;
    check(!conflict.restore(s,rc,wc)&&c.writes==writes&&c.local[4].scale==.5f,"external scalar change is preserved and debt retained");
    auto changed=s;changed.local.data+=32;
    check(!conflict.restore(changed,rc,wc)&&c.writes==writes,"never follow a replaced array");
    conflict.original_destroyed();check(!conflict.bound(),"positive original destruction retires without memory writes");
    Memory d;p::ScaleLease clear;auto rd=[&](auto at,auto& out){return d.read(at,out);};auto wd=[&](auto at,float v){return d.write(at,v);};
    s=d.source();check(clear.begin(s,rd)&&clear.decoder_return(s,s.local.data+128,4,rd,wd),"cache-clear setup");
    d.cache=d.local;d.evaluate();check(clear.observe_cache(s,rd),"cache owns observed copy");clear.request_restore();
    s.cache={};check(!clear.restore(s,rd,wd)&&clear.cpu_restored(),"original cache clear retires backing pointer without touching it");
    d.evaluate();++s.epoch;check(clear.restore(s,rd,wd),"cache-free original output restored");
    Memory e;p::ScaleLease stowed;auto re=[&](auto at,auto& out){return e.read(at,out);};auto we=[&](auto at,float v){return e.write(at,v);};
    s=e.source();check(stowed.begin(s,re)&&stowed.decoder_return(s,s.local.data+128,4,re,we),"stow setup");
    e.cache=e.local;e.evaluate();stowed.observe_cache(s,re);stowed.request_restore();
    check(stowed.restore(s,re,we,true)&&e.local[4].scale==1&&e.cache[4].scale==1&&e.space[4].value[0]==0,
        "positive stow permits CPU retirement without pretending stale space/GPU is visible");
    std::printf("Snub magazine visibility: %u checks passed\n",checks);
}
