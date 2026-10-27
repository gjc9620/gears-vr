#include "weapon_lancer_muzzle_pose.h"
#include <cassert>
#include <cstdio>
#include <functional>
namespace m=gears_weapon_lancer_muzzle;
namespace {
struct Memory {
    std::array<unsigned char,1024> mesh{};
    std::array<unsigned char,256> asset{};
    std::array<m::Matrix,9> space{};
    std::array<unsigned char,9*32> local{};
    std::array<unsigned char,9*68> reference{};
    m::Matrix ctw{0,1,0,0,-1,0,0,0,0,0,1,0,100,200,300,1};
    std::function<void(m::Address)> mutation;
    template<class T>static void put(m::Address a,std::size_t offset,T const& value){std::memcpy(reinterpret_cast<void*>(a+offset),&value,sizeof value);}
    m::Address body()const{return reinterpret_cast<m::Address>(mesh.data());}
    m::Address model()const{return reinterpret_cast<m::Address>(asset.data());}
    Memory(){
        for(unsigned i=0;i<9;++i){space[i]={1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1};
            put(reinterpret_cast<m::Address>(reference.data()),i*68,m::Name{100+i,0});}
        put(body(),0xa0,ctw);put(body(),0x218,model());put(body(),0x338,17u);
        put(body(),0x26c,m::Array{reinterpret_cast<m::Address>(space.data()),9,9});
        put(body(),0x27c,m::Array{reinterpret_cast<m::Address>(local.data()),9,9});
        put(model(),0xa4,m::Array{reinterpret_cast<m::Address>(reference.data()),9,9});
    }
    bool read(m::Address address,void* target,std::size_t bytes){
        if(mutation)mutation(address);
        const auto allowed=[&](auto const& range){const auto base=reinterpret_cast<m::Address>(range.data());return address>=base&&address-base<=sizeof range&&bytes<=sizeof range-(address-base);};
        if(!allowed(mesh)&&!allowed(asset)&&!allowed(space)&&!allowed(local)&&!allowed(reference))return false;
        std::memcpy(target,reinterpret_cast<void const*>(address),bytes);return true;
    }
    bool capture(m::Pose& out){return m::capture([&](auto a,auto b,auto c){return read(a,b,c);},body(),model(),ctw,out);}
};
}
int main(){
    unsigned cases{};
    {Memory f;m::Pose out{};assert(f.capture(out));m::Socket socket{};socket.object=7;socket.configured_name=socket.socket_name={8,0};socket.bone_name={103,0};socket.location={1,2,3};
        unsigned bone=99;std::array<double,3> world{};assert(m::location(out,socket,bone,world));
        assert(bone==3&&world[0]==78&&world[1]==211&&world[2]==333);++cases;
        auto changed=out;assert(m::same(out,changed));changed.local_values[0]=1;assert(!m::same(out,changed));
        changed=out;changed.epoch++;assert(!m::same(out,changed));++cases;
        for(unsigned kind=0;kind<4;++kind){auto s=socket;auto p=out;bone=99;world={9,9,9};
            if(kind==0)s.bone_name.index=999;if(kind==1)std::memcpy(p.reference_values.data(),&s.bone_name,8);
            if(kind==2)s.socket_name.number=1;if(kind==3)s.location[0]=std::numeric_limits<float>::quiet_NaN();
            assert(!m::location(p,s,bone,world)&&bone==99&&world[0]==9);++cases;}
    }
    for(unsigned kind=0;kind<6;++kind){Memory f;m::Pose out{};out.epoch=123;
        if(kind==0)Memory::put(f.body(),0x26c,m::Array{reinterpret_cast<m::Address>(f.space.data()),8,9});
        if(kind==1)Memory::put(f.body(),0x29c,m::Address{1});
        if(kind==2)f.space[2][0]=std::numeric_limits<float>::infinity();
        if(kind==3)f.space[2][0]=0;
        if(kind==4)f.mutation=[&](auto a){if(a==f.model()+0xa4)Memory::put(f.body(),0x338,18u);};
        if(kind==5)Memory::put(f.model(),0xa4,m::Array{std::numeric_limits<m::Address>::max()-8,9,9});
        assert(!f.capture(out)&&out.epoch==123);++cases;
    }
    std::printf("Lancer muzzle pose: %u owned-memory cases; no Game/getter/aim authority\n",cases);
}
