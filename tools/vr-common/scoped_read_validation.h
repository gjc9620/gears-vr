#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>
#if defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
#endif

// Bounded record of exact successful reads, for a caller-owned synchronous
// scope. Validation reads ALL recorded bytes again; no readable padding,
// page cache, cached permission or currentness result is used.
namespace gears_read_validation {
enum class Result {same,changed,unreadable};
enum class Failure:unsigned {none,bad_range,duplicate_changed,read_capacity,byte_capacity,hash_capacity,read_fault,empty,overlap_changed};
template<unsigned MaxReads=32768,unsigned MaxBytes=1048576,unsigned HashSlots=65536,bool CacheOrder=false>
struct Proof {
    static_assert((HashSlots&(HashSlots-1))==0&&HashSlots>MaxReads);
    struct Range {std::uintptr_t address;unsigned bytes,offset;};
    struct OrderCache {std::array<Range,MaxReads> raw{},sorted{};unsigned count{},hits{},misses{};bool valid{};};
    struct NoOrderCache {};
    std::conditional_t<CacheOrder,OrderCache,NoOrderCache> order_cache{};
    std::array<Range,MaxReads> ranges{};
    std::array<unsigned,HashSlots> slots{};
    std::array<unsigned char,MaxBytes> captured{},compact{};
    unsigned count{},used{},merged{};bool valid{};
    Failure failure{};std::uintptr_t failure_address{};std::size_t failure_bytes{};
    void begin() noexcept {count=used=merged=0;valid=true;failure=Failure::none;failure_address=0;failure_bytes=0;slots.fill(0);}
    void fail(Failure why,std::uintptr_t address=0,std::size_t bytes=0) noexcept {
        if constexpr(CacheOrder)order_cache.valid=false;
        if(failure==Failure::none){failure=why;failure_address=address;failure_bytes=bytes;}valid=false;
    }
    void record(std::uintptr_t address,void const* data,std::size_t bytes) noexcept {
        if(!valid)return;
        if(!address||!data||!bytes||bytes>MaxBytes||bytes>UINTPTR_MAX-address){fail(Failure::bad_range,address,bytes);return;}
        // Mix high and low address bits before truncating. UE allocation pages
        // can repeat identical low offsets across many distant object blocks.
        auto mixed=std::uint64_t(address)^(std::uint64_t(bytes)*0x9e3779b97f4a7c15ull);
        mixed^=mixed>>30;mixed*=0xbf58476d1ce4e5b9ull;mixed^=mixed>>27;mixed*=0x94d049bb133111ebull;mixed^=mixed>>31;
        auto hash=unsigned(mixed&(HashSlots-1));
        for(unsigned probe=0;probe<128;++probe,hash=(hash+1)&(HashSlots-1)){
            const auto slot=slots[hash];
            if(slot){auto const& r=ranges[slot-1];
                if(r.address==address&&r.bytes==bytes){
                    if(std::memcmp(captured.data()+r.offset,data,bytes))fail(Failure::duplicate_changed,address,bytes);return;
                }
            }else{
                if(count==MaxReads){fail(Failure::read_capacity,address,bytes);return;}
                if(bytes>MaxBytes-used){fail(Failure::byte_capacity,address,bytes);return;}
                ranges[count]={address,unsigned(bytes),used};slots[hash]=++count;
                std::memcpy(captured.data()+used,data,bytes);used+=unsigned(bytes);return;
            }
        }
        fail(Failure::hash_capacity,address,bytes);
    }
    bool seal() noexcept {
        if(!valid)return false;
        if(!count){fail(Failure::empty);return false;}
        bool replayed=false;
        if constexpr(CacheOrder){
            static_assert(sizeof(Range)==sizeof(std::uintptr_t)+2*sizeof(unsigned));
            auto& order=order_cache;
            if(order.valid&&order.count==count&&!std::memcmp(order.raw.data(),ranges.data(),size_t(count)*sizeof(Range))){
                std::memcpy(ranges.data(),order.sorted.data(),size_t(count)*sizeof(Range));++order.hits;replayed=true;
            }else{
                order.valid=false;order.count=count;++order.misses;
                std::memcpy(order.raw.data(),ranges.data(),size_t(count)*sizeof(Range));
            }
        }
        if(!replayed){std::sort(ranges.begin(),ranges.begin()+count,[](Range const& a,Range const& b){
            return a.address<b.address||(a.address==b.address&&a.bytes<b.bytes);
        });
            if constexpr(CacheOrder)std::memcpy(order_cache.sorted.data(),ranges.data(),size_t(count)*sizeof(Range));
        }
        unsigned size=0;
        for(unsigned i=0;i<count;++i){const auto r=ranges[i];
            if(merged){auto& previous=ranges[merged-1];const auto end=previous.address+previous.bytes;
                if(r.address<=end){
                    const auto overlap=unsigned(std::min<std::uintptr_t>(end-r.address,r.bytes));
                    if(std::memcmp(compact.data()+previous.offset+(r.address-previous.address),captured.data()+r.offset,overlap)){fail(Failure::overlap_changed,r.address,overlap);return false;}
                    const auto extra=r.bytes-overlap;
                    if(extra){std::memcpy(compact.data()+size,captured.data()+r.offset+overlap,extra);size+=extra;previous.bytes+=extra;}
                    continue;
                }
            }
            std::memcpy(compact.data()+size,captured.data()+r.offset,r.bytes);
            ranges[merged++]={r.address,r.bytes,size};size+=r.bytes;
        }
        used=size;if constexpr(CacheOrder)order_cache.valid=true;return true;
    }
    template<class Read>Result validate(Read&& read) const noexcept {
        if(!valid||!merged)return Result::changed;
        std::array<unsigned char,256> now{};
        // Two complete closing rereads. No retry after a failed read.
        for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<merged;++i){auto const& r=ranges[i];
            for(unsigned offset=0;offset<r.bytes;){const auto n=std::min<unsigned>(unsigned(now.size()),r.bytes-offset);
                if(!read(now.data(),r.address+offset,n))return Result::unreadable;
                if(std::memcmp(now.data(),compact.data()+r.offset+offset,n))return Result::changed;
                offset+=n;
            }
        }
        return Result::same;
    }
};
// Optional read batching on top of the unchanged exact witness set. Every
// original byte has an all-one comparison mask. Only newly read padding has
// a zero mask; it is never a witness. Groups stay inside one4KiB page, span
// at most1024bytes. Generic defaults bridge gaps<=32bytes and add<=256KiB
// per complete pass; SourceProof explicitly uses64bytes/1MiB for UObject gaps.
// Planning exhaustion uses the original exact-range validator, not a permit.
template<unsigned MaxReads=32768,unsigned MaxBytes=1048576,unsigned HashSlots=65536,unsigned MaxPadding=262144,unsigned MaxGap=32,bool CacheOrder=false>
struct BatchedProof:Proof<MaxReads,MaxBytes,HashSlots,CacheOrder> {
    using Base=Proof<MaxReads,MaxBytes,HashSlots,CacheOrder>;
    struct Batch {std::uintptr_t address;unsigned bytes,offset;};
    std::array<Batch,MaxReads> batches{};
    std::array<unsigned char,MaxBytes+MaxPadding> expected{},mask{};
    unsigned batch_count{},planned_bytes{},padding_bytes{};bool batched{};
    void begin() noexcept {Base::begin();batch_count=planned_bytes=padding_bytes=0;batched=false;}
    bool seal() noexcept {
        if(!Base::seal())return false;
        batch_count=planned_bytes=padding_bytes=0;batched=false;
        for(unsigned i=0;i<this->merged;++i){auto const& r=this->ranges[i];
            for(unsigned at=0;at<r.bytes;){const auto address=r.address+at;
                const auto n=std::min<unsigned>(r.bytes-at,std::min<unsigned>(1024,4096-unsigned(address%4096)));
                unsigned gap{};bool append{};
                if(batch_count){auto const& previous=batches[batch_count-1];const auto end=previous.address+previous.bytes;
                    if(address>=end&&address-end<=MaxGap&&previous.address/4096==address/4096&&
                       previous.bytes+(address-end)+n<=1024&&address-end<=MaxPadding-padding_bytes){gap=unsigned(address-end);append=true;}
                }
                if(!append){
                    if(batch_count==MaxReads){batch_count=planned_bytes=padding_bytes=0;return true;}
                    batches[batch_count++]={address,0,planned_bytes};
                }
                if(n+gap>expected.size()-planned_bytes){batch_count=planned_bytes=padding_bytes=0;return true;}
                auto& batch=batches[batch_count-1];
                if(gap){std::memset(expected.data()+planned_bytes,0,gap);std::memset(mask.data()+planned_bytes,0,gap);planned_bytes+=gap;padding_bytes+=gap;batch.bytes+=gap;}
                std::memcpy(expected.data()+planned_bytes,this->compact.data()+r.offset+at,n);
                std::memset(mask.data()+planned_bytes,0xff,n);planned_bytes+=n;batch.bytes+=n;at+=n;
            }
        }
        batched=true;return true;
    }
    unsigned validation_reads_per_pass() const noexcept {
        if(batched)return batch_count;unsigned reads{};
        for(unsigned i=0;i<this->merged;++i)reads+=(this->ranges[i].bytes+255)/256;return reads;
    }
    template<class Read>Result validate(Read&& read) const noexcept {
        if(!batched)return Base::validate(read);
        if(!this->valid||!batch_count)return Result::changed;
        std::array<unsigned char,1024> now{};
        for(unsigned pass=0;pass<2;++pass)for(unsigned i=0;i<batch_count;++i){auto const& b=batches[i];
            if(!read(now.data(),b.address,b.bytes))return Result::unreadable; // Never retry/split after a fault.
            unsigned at{};
#if defined(_M_X64) || defined(__SSE2__)
            for(;at+16<=b.bytes;at+=16){
                const auto fresh=_mm_loadu_si128(reinterpret_cast<__m128i const*>(now.data()+at));
                const auto wanted=_mm_loadu_si128(reinterpret_cast<__m128i const*>(expected.data()+b.offset+at));
                const auto bits=_mm_loadu_si128(reinterpret_cast<__m128i const*>(mask.data()+b.offset+at));
                const auto different=_mm_and_si128(_mm_xor_si128(fresh,wanted),bits);
                if(_mm_movemask_epi8(_mm_cmpeq_epi8(different,_mm_setzero_si128()))!=0xffff)return Result::changed;
            }
#endif
            for(;at+8<=b.bytes;at+=8){std::uint64_t fresh{},wanted{},bits{};
                std::memcpy(&fresh,now.data()+at,8);std::memcpy(&wanted,expected.data()+b.offset+at,8);std::memcpy(&bits,mask.data()+b.offset+at,8);
                if((fresh^wanted)&bits)return Result::changed;
            }
            if(at+4<=b.bytes){std::uint32_t fresh{},wanted{},bits{};
                std::memcpy(&fresh,now.data()+at,4);std::memcpy(&wanted,expected.data()+b.offset+at,4);std::memcpy(&bits,mask.data()+b.offset+at,4);
                if((fresh^wanted)&bits)return Result::changed;at+=4;
            }
            if(at+2<=b.bytes){std::uint16_t fresh{},wanted{},bits{};
                std::memcpy(&fresh,now.data()+at,2);std::memcpy(&wanted,expected.data()+b.offset+at,2);std::memcpy(&bits,mask.data()+b.offset+at,2);
                if((fresh^wanted)&bits)return Result::changed;at+=2;
            }
            if(at<b.bytes&&((now[at]^expected[b.offset+at])&mask[b.offset+at]))return Result::changed;
        }
        return Result::same;
    }
};
using SourceProof=BatchedProof<32768,1048576,65536,1048576,64,true>;
inline thread_local SourceProof* recording{};
inline void observed(std::uintptr_t address,void const* bytes,std::size_t count) noexcept {
    if(recording)recording->record(address,bytes,count);
}
inline void unreadable() noexcept {if(recording)recording->fail(Failure::read_fault);}
struct Capture {
    SourceProof* previous{};
    explicit Capture(SourceProof& proof) noexcept:previous(recording){proof.begin();recording=&proof;}
    ~Capture() noexcept {recording=previous;}
    Capture(Capture const&)=delete;Capture& operator=(Capture const&)=delete;
};
}
