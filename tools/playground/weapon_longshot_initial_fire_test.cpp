// Actual stock bytes plus the production proof adapter in owned artificial
// metadata. No live process, engine call, input dispatch or VM execution.
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cstdlib>
#include <type_traits>
#include "../vr-common/weapon_longshot_initial_fire.h"
#include "../vr-common/weapon_lancer_refire_loop.h"
#include "longshot-initial-stock-fixture.inl"
using uptr=std::uintptr_t;
constexpr uptr base=0x10000000,outer_function=0x2000,previous_field=stock_previous;
namespace fixture {
struct Model {
    bool path=true,structure=true,property=true,script=true,parent=true,retained=true,unchanged=true;
    unsigned count{},flags{},native{},result=0xffff,element=8,dimension=1,offset{},property_flags{};
    uptr exec=base+0x1d6ca0,next{},child=previous_field;
    unsigned bytes=41,parent_calls{};std::array<unsigned char,41> body=stock_code;
} model;
}
namespace meta {
struct Name {unsigned index{},number{};};
struct Identity {uptr address{},klass{},outer{};Name name{};wchar_t const* label{};};
struct Snapshot {std::array<Identity,11> identities{{
    {outer_function,0x101,0x102,{},L"BeginState"},{0x101,0,0,{},L"Function"},
    {0x102,0x103,0x104,{},L"WeaponFiring"},{0x103,0,0,{},L"State"},
    {0x104,0x105,0x106,{},L"Weap_SniperRifle"},{0x105,0,0,{},L"Class"},
    {0x106,0,0,{},L"WarfareGame"},{previous_field,0,outer_function,{},L"PreviousStateName"},
    {0x108,0,0,{},L"NameProperty"},{0x109,0x101,0x104,{stock_feedback,0},L"DelayedWeaponStopPS5TriggerFX"},
    {0x110,0,0,{},L"Unused"}}};};
struct Structure {uptr children{};unsigned extent{};};
struct Property {unsigned identity{},type{},dimension{},element_bytes{},offset{},flags{};};
bool property_fits(Property const& p,unsigned n){return p.offset<=n&&p.element_bytes<=n-p.offset;}
struct Reader {
    Snapshot& result;
    bool path(uptr a,unsigned& slot){if(!fixture::model.path)return false;
        for(unsigned i=0;i<result.identities.size();++i)if(result.identities[i].address==a){slot=i;return true;}return false;}
    bool named(unsigned i,wchar_t const* s){return i<result.identities.size()&&!std::wcscmp(result.identities[i].label,s);}
    bool structure(uptr a,Structure& s){s={fixture::model.child,8};return a==outer_function&&fixture::model.structure;}
    bool property(uptr a,Property& p){auto const& m=fixture::model;p={7,8,m.dimension,m.element,m.offset,m.property_flags};return a==previous_field&&m.property;}
    template<class T>bool at(uptr a,unsigned offset,T& value){if(a!=outer_function)return false;auto const& m=fixture::model;
        std::uint64_t v{};switch(offset){case 0x106:v=m.count;break;case 0x108:v=m.result;break;
        case 0xf8:v=m.native;break;case 0xf4:v=m.flags;break;case 0x114:v=m.exec;break;default:return false;}
        value=static_cast<T>(v);return true;}
};
}
namespace weaponhand {bool core_type(meta::Reader& r,unsigned i,wchar_t const* s){return r.named(i,s);}}
namespace playground {uptr field(uptr a,wchar_t const* name,wchar_t const* =nullptr){
    if(a==outer_function&&!std::wcscmp(name,L"PreviousStateName"))return previous_field;
    if(a==0x104&&!std::wcscmp(name,L"DelayedWeaponStopPS5TriggerFX"))return 0x109;
    return 0;
}}
template<class T>T get(uptr a){return a==previous_field+0x68?static_cast<T>(fixture::model.next):T{};}
struct Script {struct {int count{};} array;std::array<unsigned char,256> bytes{};};
bool script(meta::Reader&,uptr a,Script& s){auto const& m=fixture::model;s.array.count=int(m.bytes);
    std::memcpy(s.bytes.data(),m.body.data(),m.body.size());return a==outer_function&&m.script;}
bool unchanged(meta::Reader&,uptr a,Script const& s){return a==outer_function&&fixture::model.unchanged&&
    s.array.count==int(fixture::model.bytes)&&!std::memcmp(s.bytes.data(),fixture::model.body.data(),41);}
namespace unified_weapon_fire {bool retained_metadata(meta::Reader&){return fixture::model.retained;}}
// The adapter must call the ORIGINAL base proof; this fixture supplies its
// result, not a substitute timer or dispatcher. Its original byte matcher and
// once-only ledger are checked below and separately in their frozen test.
bool initial_proof(meta::Reader&,uptr weapon,uptr parent,Script&,unsigned& count,uptr& timer,unsigned& reason){
    ++fixture::model.parent_calls;count=0;timer=0x5555;reason=0;
    return weapon==0x9999&&parent==stock_parent&&fixture::model.parent;
}
#include "weapon_longshot_initial_fire_proof.inl"
int main(){
    _set_error_mode(_OUT_TO_STDERR);_set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    unsigned checks{};
    auto matches=[](auto const& bytes){return gears_longshot_initial_fire::stock_wrapper(bytes.data(),bytes.size(),stock_parent,stock_previous,{stock_feedback,0});};
    assert(matches(stock_code));++checks;
    for(unsigned i=0;i<stock_code.size();++i)for(unsigned bit=0;bit<8;++bit){auto bad=stock_code;bad[i]^=static_cast<unsigned char>(1u<<bit);assert(!matches(bad));++checks;}
    for(std::size_t n=0;n<stock_code.size();++n){assert(!gears_longshot_initial_fire::stock_wrapper(stock_code.data(),n,stock_parent,stock_previous,{stock_feedback,0}));++checks;}
    assert(!gears_longshot_initial_fire::stock_wrapper(nullptr,41,stock_parent,stock_previous,{stock_feedback,0}));++checks;
    assert(!gears_longshot_initial_fire::stock_wrapper(stock_code.data(),41,0,stock_previous,{stock_feedback,0}));++checks;
    assert(!gears_longshot_initial_fire::stock_wrapper(stock_code.data(),41,stock_parent,0,{stock_feedback,0}));++checks;
    assert(!gears_longshot_initial_fire::stock_wrapper(stock_code.data(),41,stock_parent,stock_previous,{stock_feedback,1}));++checks;
    assert(gears_longshot_initial_fire::stock_release(stock_release_code.data(),96,stock_release_bindings));++checks;
    for(unsigned i=0;i<96;++i)for(unsigned bit=0;bit<8;++bit){auto bad=stock_release_code;bad[i]^=static_cast<unsigned char>(1u<<bit);
        assert(!gears_longshot_initial_fire::stock_release(bad.data(),96,stock_release_bindings));++checks;}
    for(unsigned n=0;n<96;++n){assert(!gears_longshot_initial_fire::stock_release(stock_release_code.data(),n,stock_release_bindings));++checks;}
    assert(!gears_longshot_initial_fire::stock_release(nullptr,96,stock_release_bindings));++checks;
    for(unsigned invalid=0;invalid<8;++invalid){auto b=stock_release_bindings;
        switch(invalid){case 0:b.instigator=0;break;case 1:b.human=0;break;case 2:b.mode=0;break;case 3:b.controller=0;break;
        case 4:b.fire=0;break;case 5:b.result=0;break;case 6:b.end_fire.index=0;break;case 7:b.end_fire.number=1;break;}
        assert(!gears_longshot_initial_fire::stock_release(stock_release_code.data(),96,b));++checks;}
    auto proof=[](meta::Snapshot& s,unsigned& bytes,unsigned& reason){meta::Reader r{s};Script b{};uptr timer{};
        const bool okay=longshot_initial_proof(r,0x9999,outer_function,b,bytes,timer,reason);
        if(okay)assert(timer==0x5555&&reason==0&&fixture::model.parent_calls==1);return okay;};
    for(unsigned count:{0u,8u}){fixture::model={};fixture::model.count=count;fixture::model.property_flags=count?0x80:0;
        meta::Snapshot s;unsigned bytes{},reason{};assert(proof(s,bytes,reason)&&bytes==count);++checks;}
    for(unsigned negative=0;negative<30;++negative){fixture::model={};meta::Snapshot s;auto& m=fixture::model;
        switch(negative){case 0:m.path=false;break;case 1:s.identities[0].label=L"Other";break;
        case 2:s.identities[1].label=L"Object";break;case 3:s.identities[2].label=L"Active";break;
        case 4:s.identities[3].label=L"Class";break;case 5:s.identities[4].label=L"Weap_COGPistol";break;
        case 6:s.identities[5].label=L"Object";break;case 7:s.identities[6].label=L"OtherPackage";break;
        case 8:s.identities[6].outer=0x8888;break;case 9:m.structure=false;break;case 10:m.count=4;break;
        case 11:m.result=0;break;case 12:m.native=1;break;case 13:m.flags=0x400;break;case 14:++m.exec;break;
        case 15:m.property=false;break;case 16:s.identities[7].outer=0x1234;break;
        case 17:s.identities[8].label=L"IntProperty";break;case 18:m.dimension=2;break;case 19:m.element=4;break;
        case 20:m.offset=1;break;case 21:m.property_flags=0x80;break;case 22:m.child=0;break;case 23:m.next=0x1111;break;
        case 24:s.identities[9].outer=0x1111;break;case 25:m.script=false;break;case 26:m.bytes=40;break;
        case 27:m.parent=false;break;case 28:m.retained=false;break;case 29:m.unchanged=false;break;}
        unsigned bytes{},reason{};assert(!proof(s,bytes,reason));++checks;
    }
    for(unsigned i=0;i<41;++i){fixture::model={};fixture::model.body[i]^=1;meta::Snapshot s;unsigned bytes{},reason{};
        assert(!proof(s,bytes,reason));++checks;}
    std::printf("LongshotInitialFireTest: %u checks; exact stock wrapper, strict adapter and original base-proof delegation; no game\n",checks);
}
