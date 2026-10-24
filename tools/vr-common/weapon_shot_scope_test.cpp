#include "weapon_shot_scope.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#ifdef NDEBUG
#error WeaponShotScopeTest requires active assertions
#endif
namespace shot=gears_weapon_shot;
namespace hand=gears_weapon_hand;
namespace {
struct alignas(8) Frame {std::array<unsigned char,64> bytes{};};
struct Host {
    std::array<Frame,8> frames{};
    hand::ObjectIdentity weapon=object(0x5000);
    static hand::ObjectIdentity object(std::uint64_t p,std::uint64_t outer=0x8000,std::uint64_t cls=0x9000){
        return {p,outer,cls,unsigned(p/8),7,0,1};
    }
    static shot::Binding binding(shot::Role role){
        const auto n=std::uint64_t(role);
        shot::Binding b{};b.role=role;b.script=0x10000+n*0x100;b.bytes=b.capacity=128;b.script_hash=1+n;b.exec=0x20000;
        b.identities={object(0x1000+n*8,0x2000,0x9000),object(0x2000,0x3000,0xa000),
            object(0x3000,0,0xb000),object(0x9000,0xc000,0xa000),object(0xa000,0xc000,0xa000)};
        assert(b.valid());return b;
    }
    std::uintptr_t address(unsigned i){return reinterpret_cast<std::uintptr_t>(frames[i].bytes.data());}
    void frame(unsigned i,std::uint64_t node,std::uint64_t object_address,std::uint64_t pc,std::uint64_t previous){
        auto p=frames[i].bytes.data();std::memcpy(p+0x18,&node,8);std::memcpy(p+0x20,&object_address,8);
        std::memcpy(p+0x28,&pc,8);std::memcpy(p+0x38,&previous,8);
    }
    void body(unsigned i,shot::Binding const& b,std::uint64_t previous){frame(i,b.identities[0].address,weapon.address,b.script+16,previous);}
    bool read(void* out,void const* p,std::size_t size){
        const auto address_value=reinterpret_cast<std::uintptr_t>(p);
        for(auto const& f:frames){const auto start=reinterpret_cast<std::uintptr_t>(f.bytes.data());
            if(address_value>=start&&address_value-start<=f.bytes.size()&&size<=f.bytes.size()-(address_value-start)){
                std::memcpy(out,p,size);return true;
            }
        }
        return false;
    }
    auto reader(){return [this](void* out,void const* p,std::size_t n){return read(out,p,n);};}
    hand::GripSnapshot grip(hand::Grip value=hand::Grip::single){
        hand::GripSnapshot s{};s.grip=value;s.key={11,22,33,1,1,1,44,55};
        s.world=object(0x6000);s.controller=object(0x6010);s.pawn=object(0x6020);s.mesh=object(0x6030);s.weapon=weapon;
        s.committed_ns=1000000000;s.input_received_ns=s.stereo_received_ns=990000000;return s;
    }
    Host(){frame(0,0x7770,weapon.address,0x8880,0);}
};
}
int main(){
    using Role=shot::Role;using Grip=hand::Grip;
    const auto war=Host::binding(Role::war_instant),engine=Host::binding(Role::engine_instant);
    unsigned cases{};
    {Host h;shot::Stack s;auto token=s.enter(war,h.weapon,h.address(0),h.grip(),1000000001,h.reader());
        h.body(1,war,h.address(0));h.frame(2,0x7000,h.weapon.address,0x8000,h.address(1));
        shot::Packet first{},second{};assert(s.packet(h.weapon,h.address(2),h.reader(),first));
        assert(s.packet(h.weapon,h.address(2),h.reader(),second)&&first.shot==second.shot&&s.current(first));
        auto forged=first;forged.grip.grip=Grip::two;assert(!s.current(forged));
        s.leave(token);assert(s.drained()&&!s.current(first));++cases;}
    {Host h;shot::Stack s;auto root=s.enter(war,h.weapon,h.address(0),h.grip(),1000000001,h.reader());
        h.body(1,war,h.address(0));assert(s.aliases(engine,h.weapon,h.address(1),h.reader()));
        auto alias=s.enter(engine,h.weapon,h.address(1),h.grip(Grip::two),1300000000,h.reader());
        h.body(3,engine,h.address(1));h.frame(4,0x7000,h.weapon.address,0x8000,h.address(3));
        shot::Packet parent{};assert(s.packet(h.weapon,h.address(4),h.reader(),parent)&&parent.grip.grip==Grip::single);
        auto nested=s.enter(war,h.weapon,h.address(3),h.grip(Grip::two),1000000001,h.reader());
        h.body(5,war,h.address(3));h.frame(6,0x7000,h.weapon.address,0x8000,h.address(5));
        shot::Packet child{};assert(s.packet(h.weapon,h.address(6),h.reader(),child)&&child.shot!=parent.shot&&child.grip.grip==Grip::two);
        s.leave(nested);assert(s.current(parent));s.leave(alias);
        shot::Packet outer{};assert(s.packet(h.weapon,h.address(1),h.reader(),outer)&&outer.shot==parent.shot);
        s.leave(root);assert(s.drained());++cases;}
    {Host h;shot::Stack s;auto root=s.enter(war,h.weapon,h.address(0),h.grip(),1000000001,h.reader());
        h.body(1,war,h.address(0));shot::Packet p{};
        auto masked=s.mask();assert(!s.packet(h.weapon,h.address(1),h.reader(),p));s.leave(masked);
        assert(s.packet(h.weapon,h.address(1),h.reader(),p));
        auto invalid=war;invalid.role=Role::opaque;auto opaque=s.enter(invalid,h.weapon,h.address(1),h.grip(Grip::two),1000000001,h.reader());
        assert(!s.packet(h.weapon,h.address(1),h.reader(),p));s.leave(opaque);assert(s.current(p)==false);
        assert(s.packet(h.weapon,h.address(1),h.reader(),p));s.leave(root);++cases;}
    {Host h;shot::Stack s;auto root=s.enter(war,h.weapon,h.address(0),h.grip(),1000000001,h.reader());
        h.body(1,war,h.address(0));std::array<shot::Token,16> nested{};
        for(auto& t:nested)t=s.enter(war,h.weapon,h.address(0),h.grip(Grip::two),1000000001,h.reader());
        shot::Packet p{};assert(!s.packet(h.weapon,h.address(1),h.reader(),p));
        for(auto i=nested.size();i;i--)s.leave(nested[i-1]);
        assert(!s.poisoned()&&s.packet(h.weapon,h.address(1),h.reader(),p)&&p.grip.grip==Grip::single);
        s.leave(root);assert(s.drained());++cases;}
    {Host h;shot::Stack s;auto custom=Host::binding(Role::shotgun_custom),pellets=Host::binding(Role::shotgun_pellets);
        auto root=s.enter(custom,h.weapon,h.address(0),h.grip(),1000000001,h.reader());h.body(1,custom,h.address(0));
        auto batch=s.enter(pellets,h.weapon,h.address(1),{},1000000001,h.reader());h.body(3,pellets,h.address(1));
        shot::Packet p{};assert(s.packet(h.weapon,h.address(3),h.reader(),p)&&p.root==Role::shotgun_custom&&p.grip.grip==Grip::single);
        s.leave(batch);s.leave(root);auto orphan=s.enter(pellets,h.weapon,h.address(0),h.grip(),1000000001,h.reader());
        h.body(3,pellets,h.address(0));assert(!s.packet(h.weapon,h.address(3),h.reader(),p));s.leave(orphan);++cases;}
    {Host h;shot::Stack s;auto projectile=Host::binding(Role::war_projectile);
        auto root=s.enter(projectile,h.weapon,h.address(0),h.grip(),1000000001,h.reader());h.body(1,projectile,h.address(0));
        shot::Packet p{};assert(!s.packet(h.weapon,h.address(1),h.reader(),p));s.leave(root);++cases;}
    {Host h;shot::Stack s;auto root=s.enter(war,h.weapon,h.address(0),h.grip(),1200000000,h.reader());
        h.body(1,war,h.address(0));shot::Packet p{};assert(s.packet(h.weapon,h.address(1),h.reader(),p)&&p.grip.grip==Grip::unknown);
        s.leave(root);++cases;}
    {Host h;shot::Stack s;auto root=s.enter(war,h.weapon,h.address(0),h.grip(),1000000001,h.reader());
        h.body(1,war,h.address(0));shot::Packet p{};assert(!s.packet(h.weapon,h.address(0),h.reader(),p));
        h.frame(2,0x7000,h.weapon.address,0x8000,h.address(2));assert(!s.packet(h.weapon,h.address(2),h.reader(),p));
        h.frame(1,war.identities[0].address,h.weapon.address,war.script+war.bytes,h.address(0));
        assert(!s.packet(h.weapon,h.address(1),h.reader(),p)&&!s.aliases(engine,h.weapon,h.address(1),h.reader()));
        s.leave(root);++cases;}
    {Host h;shot::Stack s;auto root=s.enter(war,h.weapon,h.address(0),h.grip(),1000000001,h.reader());
        auto child=s.mask();s.leave(root);assert(s.poisoned());s.leave(child);shot::Packet p{};
        assert(!s.packet(h.weapon,h.address(1),h.reader(),p));++cases;}
    std::printf("WeaponShotScopeTest passed: %u cases; local frames and immutable source only\n",cases);
}
