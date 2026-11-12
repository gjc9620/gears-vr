#include "cached_pose_overwrite_source.h"
#include "cached_pose_memcpy_certificate.h"
#include <cstdio>
#include <numeric>
namespace c=gears_cached_pose_overwrite;namespace p=c::pose;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
struct Fixture {
    static constexpr uintptr_t base=0x140000000;
    std::array<unsigned char,0x300> node{},table{};std::array<unsigned char,0x400> mesh{};
    std::array<p::Atom,115> local{},cache{};std::array<unsigned char,115> desired{};
    uintptr_t n(){return reinterpret_cast<uintptr_t>(node.data());}uintptr_t m(){return reinterpret_cast<uintptr_t>(mesh.data());}
    template<class T>void put(unsigned char* memory,size_t offset,T value){memcpy(memory+offset,&value,sizeof value);}
    void setup(unsigned kind){
        node={};table={};mesh={};std::iota(desired.begin(),desired.end(),static_cast<unsigned char>(0));
        memset(local.data(),0xa5,sizeof local);memset(cache.data(),0x3c,sizeof cache);
        put(node.data(),0,reinterpret_cast<uintptr_t>(table.data()));put(table.data(),0x270,base+(kind==0?uintptr_t(0xf7940):uintptr_t(0xfacb0)));
        put(node.data(),0x60,m());put(node.data(),0xbc,uint32_t(42));put(mesh.data(),0x338,uint32_t(42));
        put(mesh.data(),0x27c,p::ArrayHeader{reinterpret_cast<uintptr_t>(local.data()),115,115});
        put(mesh.data(),0x28c,p::ArrayHeader{reinterpret_cast<uintptr_t>(desired.data()),115,115});
        put(node.data(),0x80,p::ArrayHeader{reinterpret_cast<uintptr_t>(cache.data()),115,115});
    }
    bool read(void* out,void const* in,size_t count){
        const uintptr_t from=reinterpret_cast<uintptr_t>(in);
        const std::pair<uintptr_t,size_t> areas[]={{n(),node.size()},{m(),mesh.size()},{reinterpret_cast<uintptr_t>(table.data()),table.size()},
            {reinterpret_cast<uintptr_t>(local.data()),sizeof local},{reinterpret_cast<uintptr_t>(cache.data()),sizeof cache},{reinterpret_cast<uintptr_t>(desired.data()),desired.size()}};
        for(auto const& area:areas)if(from>=area.first&&from-area.first<=area.second&&count<=area.second-(from-area.first)){memcpy(out,in,count);return true;}return false;
    }
};
int main(){Fixture f;
    for(unsigned kind:{0u,2u}){
        f.setup(kind);auto read=[&](void*o,void const*i,size_t n){return f.read(o,i,n);};c::Header selected{};
        C(c::header(read,f.base,f.n(),f.m(),f.m()+0x27c,f.m()+0x28c,kind,selected));
        c::Source before{},after{};C(c::snapshot(read,f.base,selected,before));C(!c::returned(before,before));
        memcpy(f.local.data(),f.cache.data(),sizeof f.local);C(c::snapshot(read,f.base,selected,after));C(c::returned(before,after));
        reinterpret_cast<unsigned char*>(f.local.data())[70]^=1;C(c::snapshot(read,f.base,selected,after));C(!c::returned(before,after));
        memcpy(f.local.data(),f.cache.data(),sizeof f.local);reinterpret_cast<unsigned char*>(f.cache.data())[71]^=1;
        C(c::snapshot(read,f.base,selected,after));C(!c::returned(before,after));
        f.setup(kind);C(c::header(read,f.base,f.n(),f.m(),f.m()+0x27c,f.m()+0x28c,kind,selected));C(c::snapshot(read,f.base,selected,before));
        bool changed{};auto racing=[&](void*o,void const*i,size_t n){const bool good=f.read(o,i,n);
            if(i==f.cache.data()&&!changed){changed=true;f.put(f.node.data(),0xbc,uint32_t(41));}return good;};
        after=before;C(!c::snapshot(racing,f.base,selected,after));C(c::same(before,after));
        for(unsigned bad=0;bad<20;++bad){f.setup(kind);selected={};selected.node=123;
            uintptr_t out=f.m()+0x27c,desired=f.m()+0x28c,base=f.base,node=f.n();unsigned argument_kind=kind;
            if(bad==0)argument_kind=1;
            if(bad==1)out+=8;
            if(bad==2)f.put(f.table.data(),0x270,base+0xf8af4);
            if(bad==3)f.put(f.node.data(),0xbc,uint32_t(41));
            if(bad==4){f.put(f.node.data(),0xbc,uint32_t(0));f.put(f.mesh.data(),0x338,uint32_t(0));}
            if(bad==5){f.put(f.node.data(),0xbc,UINT32_MAX);f.put(f.mesh.data(),0x338,UINT32_MAX);}
            if(bad==6)f.put(f.mesh.data(),0x27c,p::ArrayHeader{reinterpret_cast<uintptr_t>(f.local.data()),115,116});
            if(bad==7)f.put(f.mesh.data(),0x27c,p::ArrayHeader{reinterpret_cast<uintptr_t>(f.local.data()),114,115});
            if(bad==8)f.put(f.node.data(),0x80,p::ArrayHeader{reinterpret_cast<uintptr_t>(f.cache.data()),114,115});
            if(bad==9)f.put(f.node.data(),0x80,p::ArrayHeader{reinterpret_cast<uintptr_t>(f.cache.data()),115,4097});
            if(bad==10)f.put(f.mesh.data(),0x28c,p::ArrayHeader{reinterpret_cast<uintptr_t>(f.desired.data()),114,115});
            if(bad==11)f.desired[114]=0;
            if(bad==12)f.put(f.node.data(),0x80,p::ArrayHeader{reinterpret_cast<uintptr_t>(f.local.data()),115,115});
            if(bad==13)f.put(f.mesh.data(),0x27c,p::ArrayHeader{f.m()+8,115,115});
            if(bad==14)f.put(f.node.data(),0x80,p::ArrayHeader{f.n()+0x80,115,115});
            if(bad==15)f.put(f.mesh.data(),0x28c,p::ArrayHeader{reinterpret_cast<uintptr_t>(f.cache.data()),115,115});
            if(bad==16)f.put(f.node.data(),0x60,f.m()+8);
            if(bad==17)base=0;
            if(bad==18)node=0x1000;
            if(bad==19)desired+=8;
            C(!c::header(read,base,node,f.m(),out,desired,argument_kind,selected));C(selected.node==123);
        }
    }
    {namespace crt=gears_cached_memcpy_certificate;std::array<unsigned char,0x29424> image{};const auto module=reinterpret_cast<uintptr_t>(image.data());
        memcpy(image.data()+crt::body_rva,crt::body.data(),crt::body.size());memcpy(image.data()+crt::table_rva,crt::tail.data(),crt::tail.size());
        memcpy(image.data()+crt::export_rva,crt::dispatch.data(),crt::dispatch.size());
        auto read=[&](void*o,void const*i,size_t n){const auto at=reinterpret_cast<uintptr_t>(i);
            if(at<module||at-module>image.size()||n>image.size()-(at-module))return false;memcpy(o,i,n);return true;};
        C(crt::current(read,module,module+crt::export_rva));image[crt::body_rva+20]^=1;C(!crt::current(read,module,module+crt::export_rva));
        image[crt::body_rva+20]^=1;image[crt::table_rva+4]^=1;C(!crt::current(read,module,module+crt::export_rva));
        image[crt::table_rva+4]^=1;image[crt::export_rva+1]^=1;C(!crt::current(read,module,module+crt::export_rva));
        image[crt::export_rva+1]^=1;C(!crt::current(read,module,module+crt::export_rva+1));C(!crt::current(read,0,crt::export_rva));
    }
    printf("CachedPoseSource checks=%u failures=%u; data/code continuity only, no game or write/retirement permission\n",checks,failures);
    return failures?1:0;
}
