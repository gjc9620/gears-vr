#pragma once
#include <cstdint>

// Observe only a projection layer actually accepted by xrEndFrame. Copying or
// acquiring a transport slot alone never counts as a new XR submission.
namespace gears_xr_delivery {
struct Key {std::uint64_t session{},generation{},world{},sequence{},ticket{};};
inline bool valid(Key const& k){return k.session&&k.generation&&k.world&&k.sequence&&k.ticket;}
inline bool same(Key const& a,Key const& b){return a.session==b.session&&a.generation==b.generation&&
    a.world==b.world&&a.sequence==b.sequence&&a.ticket==b.ticket;}
struct Counter {
    Key last{};
    std::uint64_t distinct{},repeated{},rejected{};
    void observe(bool successful_projection,Key key){
        if(!successful_projection)return;
        if(!valid(key)){++rejected;return;}
        if(distinct&&same(key,last)){++repeated;return;}
        // A transport generation owns one monotonically increasing sequence.
        // A mismatched ticket or old sequence cannot masquerade as a new image.
        if(distinct&&(key.generation<last.generation||
           (key.generation==last.generation&&(key.session!=last.session||key.world<last.world||key.sequence<=last.sequence)))){++rejected;return;}
        last=key;++distinct;
    }
};
}
