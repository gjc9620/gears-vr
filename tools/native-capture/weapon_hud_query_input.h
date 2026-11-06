#pragma once
#include "ui_call_boundary.h"
#include <cmath>
#include <cstring>

namespace gears_weapon_hud_query {
// Caller must authenticate UpdateHUDActions, its current script/callsite and
// the three named Vector locals from reflection before supplying this layout.
// This helper locates invocation-local bytes only; it grants no write lease.
struct Layout {
    std::uintptr_t function{},controller{},code{};
    unsigned locals_size{};
    std::array<unsigned,3> offsets{}; // StartTrace, EndTrace, TraceDir
};
struct Input {
    std::uintptr_t frame{},locals{},previous{};
    std::array<std::array<float,3>,3> values{};
};
inline bool aimed_values(Input const& input,std::array<float,3> const& origin,
    std::array<double,3> const& direction,std::array<std::array<float,3>,3>& out){
    double range2{},norm2{};
    for(unsigned i=0;i<3;++i){
        if(!std::isfinite(origin[i])||std::abs(origin[i])>1e8f||!std::isfinite(direction[i]))return false;
        const double delta=double(input.values[1][i])-input.values[0][i];
        range2+=delta*delta;norm2+=direction[i]*direction[i];
    }
    if(!std::isfinite(range2)||range2<1||range2>1e12||!std::isfinite(norm2)||std::abs(norm2-1)>1e-6)return false;
    const double range=std::sqrt(range2);auto result=input.values;
    for(unsigned i=0;i<3;++i){
        result[0][i]=origin[i];result[1][i]=float(double(origin[i])+range*direction[i]);
        result[2][i]=float(direction[i]);
        if(!std::isfinite(result[1][i])||std::abs(result[1][i])>1e8f)return false;
    }
    out=result;return true;
}
// At CallFunction entry the caller PC follows VirtualFunction + FName.
// Accept only two direct local Vector operands, optionally followed by the
// omitted ImpactList argument. Do not scan for matching bytes elsewhere.
template<class Read> bool direct_arguments(Read read,Layout const& layout,
    std::uintptr_t script,unsigned size,std::uint32_t callee_name,
    std::uintptr_t start_property,std::uintptr_t end_property){
    if(!script||size<28||size>131072||script>UINTPTR_MAX-size||
       layout.code<script||layout.code-script<9||layout.code-script>size-19||
       !start_property||!end_property||start_property==end_property)return false;
    std::array<unsigned char,28> expected{},actual{};
    expected[0]=0x1b;
    std::memcpy(expected.data()+1,&callee_name,4); // FName number remains zero.
    expected[9]=0;std::memcpy(expected.data()+10,&start_property,8);
    expected[18]=0;std::memcpy(expected.data()+19,&end_property,8);
    if(!read(actual.data(),reinterpret_cast<void const*>(layout.code-9),actual.size()))return false;
    const auto tail=actual[27];actual[27]=0;
    if(actual!=expected)return false;
    if(tail==0x16)return true;
    // Retail UpdateHUDActions at PC 0x1ee uses 0x50, then EndFunctionParms.
    // The earlier 0x4a guess rejected every observed HUD query.
    if(tail!=0x50||layout.code-script>size-20)return false;
    unsigned char end{};
    return read(&end,reinterpret_cast<void const*>(layout.code+19),1)&&end==0x16;
}
template<class Read> bool capture(Read read,std::uintptr_t frame,Layout const& layout,Input& out){
    if(!frame||frame>UINTPTR_MAX-0x38||!layout.function||!layout.controller||!layout.code||
       layout.locals_size<36||layout.locals_size>65536)return false;
    for(unsigned i=0;i<3;++i){
        if((layout.offsets[i]&3)||layout.offsets[i]>layout.locals_size-12)return false;
        for(unsigned j=0;j<i;++j)
            if(layout.offsets[i]<layout.offsets[j]+12&&layout.offsets[j]<layout.offsets[i]+12)return false;
    }
    gears_ui_call_boundary::Frame before{},after{};Input result{};std::uintptr_t again{};
    if(!gears_ui_call_boundary::frame(read,frame,before)||before.node!=layout.function||
       before.object!=layout.controller||before.code!=layout.code||!before.previous||
       !read(&result.locals,reinterpret_cast<void const*>(frame+0x30),8)||
       !result.locals||(result.locals&3)||result.locals>UINTPTR_MAX-layout.locals_size)return false;
    for(unsigned i=0;i<3;++i){
        if(!read(result.values[i].data(),reinterpret_cast<void const*>(result.locals+layout.offsets[i]),12))return false;
        for(float v:result.values[i])if(!std::isfinite(v)||std::abs(v)>1e8f)return false;
    }
    if(!gears_ui_call_boundary::frame(read,frame,after)||after.node!=before.node||
       after.object!=before.object||after.code!=before.code||after.previous!=before.previous||
       !read(&again,reinterpret_cast<void const*>(frame+0x30),8)||again!=result.locals)return false;
    result.frame=frame;result.previous=before.previous;out=result;return true;
}
enum class ReplaceResult {rejected,applied,restored,uncertain};
// writable() authenticates the held source; lifetime() authenticates the live
// invocation independently so source expiry still permits rollback. exchange()
// changes one aligned word only when its bytes still equal the expected value.
// Successful values belong to the caller's locals until that HUD invocation
// ends; never restore them using a frame pointer after the caller has returned.
template<class Read,class Exchange,class Writable,class Lifetime> ReplaceResult replace(Read read,Exchange exchange,
    Writable writable,Lifetime lifetime,Layout const& layout,Input const& before,
    std::array<std::array<float,3>,3> const& desired){
    Input verified_input{};
    if(!writable()||!lifetime()||!capture(read,before.frame,layout,verified_input)||verified_input.locals!=before.locals||
       verified_input.previous!=before.previous||verified_input.values!=before.values)return ReplaceResult::rejected;
    for(auto const& vector:desired)for(float v:vector)
        if(!std::isfinite(v)||std::abs(v)>1e8f)return ReplaceResult::rejected;
    std::array<std::uint32_t,9> old_words{},new_words{};
    static_assert(sizeof(old_words)==sizeof(before.values));
    std::memcpy(old_words.data(),before.values.data(),sizeof(old_words));
    std::memcpy(new_words.data(),desired.data(),sizeof(new_words));
    auto alive=[&]{Input now{};return lifetime()&&capture(read,before.frame,layout,now)&&
        now.locals==before.locals&&now.previous==before.previous;};
    unsigned changed{};
    for(;changed<9;++changed){bool swapped{};
        const auto address=before.locals+layout.offsets[changed/3]+4*(changed%3);
        if(!writable()||!alive()||!exchange(address,old_words[changed],new_words[changed],swapped)||!swapped)break;
    }
    if(changed==9&&writable()&&alive())return ReplaceResult::applied;
    if(!changed)return ReplaceResult::rejected;
    while(changed){--changed;bool swapped{};
        const auto address=before.locals+layout.offsets[changed/3]+4*(changed%3);
        if(!alive()||!exchange(address,new_words[changed],old_words[changed],swapped)||!swapped)
            return ReplaceResult::uncertain;
    }
    return ReplaceResult::restored;
}
}
