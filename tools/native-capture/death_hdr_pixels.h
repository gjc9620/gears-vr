#pragma once
#include "death_hdr_timeline.h"
namespace gears_death_hdr_pixels {
struct Texture {uintptr_t canonical{};uint64_t width{};unsigned height{},format{},dimension{},array{},mips{},samples{},quality{};};
inline bool supported(Texture const& t){return t.canonical&&t.width>=2&&t.width<=8192&&!(t.width%2)&&t.height&&t.height<=8192&&
    t.format==26&&t.dimension==3&&t.array==1&&t.mips==1&&t.samples==1&&!t.quality;}
struct Trigger {
    uint64_t ticket{},world{},last_pair_seq{},last_copy_seq{};Texture main{},destination{};unsigned eyes{},copies{};bool invalid{},fired{};
    void sample(uint64_t id,uint64_t generation,unsigned ordinal,uint64_t seq,gears_death_hdr::Event const& e,Texture const& t){
        if(ordinal!=49||fired||invalid||e.quad_caller!=0x2ab6d9)return;
        if(!supported(t)||!e.view.eye_valid||e.view.eye>1||e.target_count!=1||e.target.mip||e.target.format!=26||
           e.target.width!=t.width||e.target.height!=t.height||e.descriptor_capacity_lost||
           !e.rectangles.tracked||e.rectangles.viewport_count!=1||e.flags)return;
        const auto eye=e.view.eye;const float half=float(t.width/2),h=float(t.height);
        // This native quad uses a local destination; the API viewport selects eye.
        if(e.args!=std::array<float,8>{0,0,half,h,eye?half:0,0,half,h}||
           e.dimensions!=std::array<unsigned,4>{unsigned(t.width/2),t.height,unsigned(t.width),t.height}||
           !(e.rectangles.viewport==gears_viewport::Rect{eye?half:0,0,half,h}))return;
        if(!eyes){ticket=id;world=generation;main=t;}
        if(!id||ticket!=id||world!=generation||main.canonical!=t.canonical||main.width!=t.width||main.height!=t.height||
           (eyes&(1u<<eye))||(eye==1&&eyes!=1)||seq<=last_pair_seq){invalid=true;return;}
        eyes|=1u<<eye;last_pair_seq=seq;
    }
    bool copy(uint64_t id,uint64_t generation,unsigned ordinal,uint64_t seq,uintptr_t caller,Texture const& src,Texture const& dst,
              bool full_copy){
        if(ordinal!=49||fired||invalid||eyes!=3||id!=ticket||generation!=world||seq<=last_pair_seq||seq<=last_copy_seq||caller!=0x1c6093||!full_copy||
           !supported(src)||!supported(dst)||src.canonical!=main.canonical||src.width!=main.width||src.height!=main.height||
           dst.canonical==src.canonical||dst.width!=src.width||dst.height!=src.height)return false;
        if(!copies)destination=dst;
        if(dst.canonical!=destination.canonical){invalid=true;return false;}
        last_copy_seq=seq;++copies;if(copies==2){fired=true;return true;}return false;
    }
};
enum Failure:unsigned {none,unknown_membership,duplicate_execution,cross_queue,signal_failed,device_removed,reset_failed,
    reset_before_execute,stop_incomplete,prepare_failed,map_failed,file_failed,reset_identity_mismatch};
struct Lifetime {
    gears_death_hdr::List list{};uintptr_t queue{},held_identity{};bool appended{},executing{},executed{},signalled{},retired{},gpu_complete{},saved{};
    struct ResetReceipt {bool observed{},success{};uintptr_t raw{},identity{};uint64_t tracker_generation{};uintptr_t tracker_identity{};} reset_receipt;
    unsigned failure{};
    void fail(unsigned reason){if(!failure)failure=reason;}
    bool before(bool readable,unsigned occurrences,uintptr_t queue_identity,bool exact_generation){
        if(!appended||failure||saved)return false;
        if(!readable){fail(unknown_membership);return false;}
        if(!occurrences)return false;
        if(!exact_generation){if(!retired)fail(unknown_membership);return false;}
        if(queue&&queue!=queue_identity){fail(cross_queue);return false;}
        if(occurrences!=1||executed||executing||retired){fail(duplicate_execution);return false;}
        if(!queue_identity){fail(unknown_membership);return false;}
        queue=queue_identity;executing=true;return true;
    }
    void after(bool signal_ok){if(!executing||failure)return;executing=false;executed=true;
        if(signal_ok)signalled=true;else fail(signal_failed);}
    void reset(bool success,uintptr_t actual_identity,uint64_t tracker_generation,uintptr_t raw,uintptr_t tracker_identity=0){
        if(!appended||failure||saved||retired)return;
        reset_receipt={true,success,raw,actual_identity,tracker_generation,tracker_identity};
        // The held COM object cannot be reused. A successful original Reset
        // discards its physical recording even when Present erased TrackerRef.
        if(!success){fail(reset_failed);return;}
        if(!actual_identity||!held_identity){fail(unknown_membership);return;}
        if(actual_identity!=held_identity){fail(reset_identity_mismatch);return;}
        if(!executed||!signalled){fail(reset_before_execute);return;}retired=true;
    }
    void completion(uint64_t value){if(!appended||failure||saved)return;
        if(value==UINT64_MAX){fail(device_removed);return;}if(signalled&&value>=1)gpu_complete=true;}
    bool may_map()const{return appended&&executed&&signalled&&retired&&gpu_complete&&!failure&&!saved;}
    void stop(){if(appended&&!saved)fail(stop_incomplete);}
};
inline bool footprint_valid(Texture const& t,uint64_t total,uint64_t offset,unsigned pitch,unsigned rows,uint64_t row_bytes,
    unsigned width,unsigned height,unsigned depth,unsigned format){
    constexpr uint64_t limit=256ull*1024*1024;
    if(!supported(t)||!total||total>limit||offset%512||offset>=total||pitch%256||rows!=t.height||row_bytes!=t.width*4||
       pitch<row_bytes||width!=t.width||height!=t.height||depth!=1||format!=t.format)return false;
    return row_bytes<=total-offset&&uint64_t(rows-1)*pitch<=total-offset-row_bytes;
}
}
