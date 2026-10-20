#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
namespace gears_vr_dof {
using Float4=std::array<float,4>;using Descriptor=std::array<uint16_t,3>;
inline bool finite(Float4 const& value){for(auto v:value)if(!std::isfinite(v))return false;return true;}
inline bool equal(Float4 const& a,Float4 const& b){return std::memcmp(a.data(),b.data(),16)==0;}
inline bool supported(Descriptor const& descriptor,unsigned index,Float4 const& value){
    return !index&&descriptor==Descriptor{0,192,16}&&finite(value)&&(value[0]==0.0f||value[0]==1.0f);
}
inline Float4 forwarded(Float4 const& source){auto out=source;if(out[0]==1.0f)out[0]=0.0f;return out;}
template<class Fn> uint64_t call_setter(Fn original,uintptr_t rcx,const uint16_t* descriptor,const void* source,unsigned index,
                                      Float4 const& copy,bool change){
    return original(rcx,descriptor,change?static_cast<const void*>(copy.data()):source,index);
}
inline uintptr_t descriptor_owner(uintptr_t address){return address>=0xfe&&address%2==0?address-0xfe:0;}
inline bool quad_caller(uintptr_t caller){return caller==0xd808d8||caller==0xd80a7b;}
inline bool native_eye(uintptr_t current,uintptr_t views,int count,unsigned& eye){
    if(count!=2||!views||views%8||views>UINTPTR_MAX-0xd30||current<views)return false;
    const auto offset=current-views;if(offset!=0&&offset!=0xd30)return false;eye=offset?1u:0u;return true;
}
// Owned transport needs an independently frozen runtime size and its own
// publication/retirement boundary. Other experimental owned paths stay denied.
inline bool transport_supported(bool owned_publisher,bool owned_bindings,bool runtime_size){
    return owned_publisher?(owned_bindings&&runtime_size):!owned_bindings;
}
struct Key {uint64_t world{},id{};explicit operator bool()const{return world&&id;}
    bool operator==(Key const& b)const{return world==b.world&&id==b.id;}};
enum class Failure:unsigned {none,duplicate_begin,duplicate_finish,after_finish,unknown_eye,duplicate_setter,unsupported,identity_changed,read_failed,post_setter,duplicate_quad,prequad};
enum class Decision:unsigned {no_site,zero_proved,missing,incomplete,rejected,overflow,count};
struct Empty {};
template<class Payload=Empty> struct Store {
    static constexpr unsigned capacity=128;
    struct Entry {
        Key key{};bool complete{},site{};Failure failure{};unsigned setter_mask{},quad_mask{};
        std::array<unsigned,2> setters{},quads{};Payload payload{};
        void fail(Failure reason){if(failure==Failure::none)failure=reason;}
        bool reserve(unsigned eye,bool setter){
            site=true;if(complete){fail(Failure::after_finish);return false;}
            if(eye>1){fail(Failure::unknown_eye);return false;}
            auto& count=setter?setters[eye]:quads[eye];if(++count!=1){fail(setter?Failure::duplicate_setter:Failure::duplicate_quad);return false;}
            return failure==Failure::none;
        }
        void proved(unsigned eye,bool setter,bool valid){
            site=true;if(complete){fail(Failure::after_finish);return;}
            if(eye>1){fail(Failure::unknown_eye);return;}
            if((setter?setters[eye]:quads[eye])!=1){fail(setter?Failure::post_setter:Failure::prequad);return;}
            if(!valid){fail(setter?Failure::post_setter:Failure::prequad);return;}
            (setter?setter_mask:quad_mask)|=1u<<eye;
        }
    };
    std::array<Entry,capacity> entries{};uint64_t retired{},observed{},late{},overflow{},invalid{},begins{},finishes{};bool capacity_lost{};
    Entry* find(Key key){for(auto& e:entries)if(e.key==key&&key)return &e;return nullptr;}
    Entry* begin(Key key){
        if(!key){++invalid;return nullptr;}if(key.id<=retired){++late;return nullptr;}
        if(key.id>observed)observed=key.id;++begins;
        if(auto* e=find(key)){e->fail(Failure::duplicate_begin);return e;}
        for(auto& e:entries)if(e.key.id==key.id){e.fail(Failure::identity_changed);++invalid;return nullptr;}
        if(capacity_lost)return nullptr;
        for(auto& e:entries)if(!e.key){e={};e.key=key;return &e;}
        ++overflow;capacity_lost=true;return nullptr;
    }
    Entry* participate(Key key){auto* e=find(key);if(!e){++late;return nullptr;}e->site=true;if(e->complete)e->fail(Failure::after_finish);return e;}
    void finish(Key key){if(!key)return;auto* e=find(key);if(!e){++late;return;}++finishes;
        if(e->complete)e->fail(Failure::duplicate_finish);e->complete=true;}
    Decision decision(Key key){
        if(capacity_lost)return Decision::overflow;auto* e=find(key);if(!e)return Decision::missing;
        if(e->failure!=Failure::none)return Decision::rejected;if(!e->complete)return Decision::incomplete;
        if(!e->site)return Decision::no_site;
        return e->setter_mask==3&&e->quad_mask==3?Decision::zero_proved:Decision::rejected;
    }
    bool retire_through(uint64_t ticket){
        if(!ticket||ticket>observed||ticket<retired)return false;
        retired=ticket;
        for(auto& e:entries)if(e.key&&e.key.id<=retired)e={};
        return true;
    }
    void retire_window(){
        if(observed>retired)retire_through(observed);
    }
    // Caller has stopped old-world production and drained world readers and
    // resource consumers. Preflight every entry before changing the global ID
    // watermark: a foreign entry below it cannot be silently left behind.
    bool world_retirement_ready(uint64_t old_world,uint64_t cutoff)const{
        if(!old_world||!cutoff||cutoff<retired||capacity_lost)return false;
        for(auto const& e:entries)if(e.key){
            if(!e.complete)return false;
            if(e.key.world==old_world){if(e.key.id>cutoff)return false;}
            else if(e.key.id<=cutoff)return false;
        }
        return true;
    }
    bool retire_world(uint64_t old_world,uint64_t cutoff){
        if(!world_retirement_ready(old_world,cutoff))return false;
        for(auto& e:entries)if(e.key.world==old_world&&e.key.id<=cutoff)e={};
        retired=cutoff; // Global frame IDs remain monotonic, including gaps.
        return true;
    }
};
inline bool allowed(Decision decision){return decision==Decision::no_site||decision==Decision::zero_proved;}
}
