#pragma once
#include "../vr-common/stereo_image_ticket.h"
#include <array>

// CPU association only. Typed resources, actual draw completion and actual
// queue submission must be supplied independently by the live integration.
namespace gears_ngx_final_xr {
struct Draw {
    uint64_t world{},ticket{},list{},generation{},source{},target{};
    unsigned width{},height{},eye{};
};
struct Packet {Draw draw{};gears_vr::StereoImageTicket image{};};
inline bool same_image(gears_vr::StereoImageTicket const& a,gears_vr::StereoImageTicket const& b){
    if(a.id!=b.id||!gears_vr::stereo_same(a.sample,b.sample))return false;
    for(unsigned i=0;i<2;++i){auto x=a.rect[i],y=b.rect[i];
        if(x.x!=y.x||x.y!=y.y||x.width!=y.width||x.height!=y.height)return false;}
    return true;
}
inline bool valid(Packet const& p,bool simulator){
    auto const& d=p.draw;
    if(!d.world||!d.ticket||!d.list||!d.generation||!d.source||!d.target||d.source==d.target||d.eye>1||
       d.width<64||d.height<64||d.width%2||p.image.id!=d.ticket||
       !gears_vr::stereo_image_ticket_valid(p.image,d.width,d.height,simulator))return false;
    for(unsigned eye=0;eye<2;++eye){auto const& r=p.image.rect[eye];
        if(r.x!=int(eye*d.width/2)||r.y||r.width!=int(d.width/2)||r.height!=int(d.height))return false;}
    return true;
}
inline bool pair(Packet const& left,Packet const& right,bool simulator){
    auto const& a=left.draw;auto const& b=right.draw;
    return valid(left,simulator)&&valid(right,simulator)&&a.eye==0&&b.eye==1&&
        a.world==b.world&&a.ticket==b.ticket&&a.list==b.list&&a.generation==b.generation&&
        a.source==b.source&&a.target==b.target&&a.width==b.width&&a.height==b.height&&
        same_image(left.image,right.image);
}
struct Member {uint64_t list{},generation{};};
enum class Match {none,one,ambiguous,invalid};
struct Selection {Match match{Match::none};unsigned index{8};Packet packet{};};
class Pending {
    std::array<Packet,8> entries_{};
public:
    static constexpr unsigned capacity=8;
    unsigned push(Packet const& packet,bool simulator){
        if(!valid(packet,simulator)||packet.draw.eye!=1)return capacity;
        unsigned free=capacity;
        for(unsigned i=0;i<capacity;++i){auto const& p=entries_[i];
            if(p.draw.ticket&&p.draw.world==packet.draw.world&&p.draw.ticket==packet.draw.ticket)return capacity;
            if(!p.draw.ticket&&free==capacity)free=i;}
        if(free<capacity)entries_[free]=packet;return free;
    }
    // Consume each actually submitted packet once. Multiple recorded frames in
    // one Execute cannot identify the final contents of the single staging
    // texture, so none of their poses may be published.
    Selection take(Member const* members,unsigned count,bool readable){
        Selection out;std::array<unsigned,capacity> hits{};bool bad=!readable||(count&&!members);
        if(!bad)for(unsigned i=0;i<count;++i){
            if(!members[i].list)bad=true;
            for(unsigned j=0;j<i;++j)if(members[i].list==members[j].list)bad=true;
            for(unsigned j=0;j<capacity;++j){auto const& p=entries_[j];
                if(p.draw.ticket&&p.draw.list==members[i].list){++hits[j];
                    if(p.draw.generation!=members[i].generation)bad=true;}}
        }
        if(bad){entries_={};out.match=Match::invalid;return out;}
        unsigned found{};
        for(unsigned i=0;i<capacity;++i)if(hits[i]){
            ++found;out.index=i;out.packet=entries_[i];entries_[i]={};}
        out.match=found==0?Match::none:found==1?Match::one:Match::ambiguous;
        return out;
    }
    void reset(uint64_t list){for(auto& p:entries_)if(p.draw.list==list)p={};}
    bool occupied(unsigned i)const{return i<capacity&&entries_[i].draw.ticket;}
};
}
