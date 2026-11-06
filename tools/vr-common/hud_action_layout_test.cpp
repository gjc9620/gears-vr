#include "hud_action_layout.h"
#include <cassert>
#include <cstdio>
#include <limits>
using gears_hud_action_layout::Scope;
struct Memory {
    std::array<float,2> values{{960,24}};unsigned writes{},fail_at{};bool fail_after_store{};
    bool read(std::uintptr_t at,float& out){if(at!=0x100&&at!=0x104)return false;out=values[(at-0x100)/4];return true;}
    bool write(std::uintptr_t at,float value){
        ++writes;if(at!=0x100&&at!=0x104)return false;
        if(writes==fail_at&&!fail_after_store)return false;
        values[(at-0x100)/4]=value;return writes!=fail_at;
    }
};
int main(){
    for(auto size:{std::array<float,2>{{1920,1080}},{{3660,3200}},{{7320,3200}}}){
        std::array<float,2> p{};
        assert(gears_hud_action_layout::plan(size[0],size[1],{{size[0]/2,24}},p));
        assert(p[0]>0&&p[0]<size[0]/2&&p[1]>size[1]/2&&p[1]<size[1]);
    }
    std::array<float,2> unchanged{{1,2}},p=unchanged;
    for(float bad:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),20000.f}){
        assert(!gears_hud_action_layout::plan(bad,1080,{{960,24}},p)&&p==unchanged);
    }
    auto scope=[](){return Scope{{{0x100,0x104}},{{960,24}},{{461,778}},0};};
    for(unsigned fail=1;fail<=2;++fail)for(bool after:{false,true}){
        Memory m{};m.fail_at=fail;m.fail_after_store=after;auto s=scope();
        auto read=[&](auto at,float& v){return m.read(at,v);};auto write=[&](auto at,float v){return m.write(at,v);};
        assert(!s.apply(read,write)&&!s.owned&&m.values==s.before);
        auto calls=m.writes;assert(s.restore(read,write)&&calls==m.writes);
    }
    Memory m{};auto s=scope();auto read=[&](auto at,float& v){return m.read(at,v);};auto write=[&](auto at,float v){return m.write(at,v);};
    assert(s.apply(read,write)&&m.values==s.wanted);
    // Nested original calls inherit the same presentation. Closing the inner
    // scope must not prematurely restore the outer invocation's placement.
    auto inner=scope();inner.before=m.values;assert(inner.apply(read,write));
    assert(inner.restore(read,write)&&m.values==s.wanted);
    assert(s.restore(read,write)&&m.values==s.before);
    auto calls=m.writes;assert(s.restore(read,write)&&calls==m.writes);
    assert(s.apply(read,write));m.values[0]=731;calls=m.writes;
    assert(!s.restore(read,write)&&s.owned==1&&m.values[0]==731&&m.values[1]==24&&m.writes==calls+1);
    // A changed owner/property must decline before invoking this raw restore.
    // The scalar journal itself never overwrites the unexpected value.
    assert(!s.restore(read,write)&&m.values[0]==731);
    std::puts("HUD action layout: original-call nesting, partial/false stores, unwind restoration and foreign-writer preservation; CPU only");
}
