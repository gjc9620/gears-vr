#define GEARS_WEAPON_METADATA_PARSE_TEST_STATS 1
#include <cstdio>
// Production client diagnostics are disabled in this owned-memory fixture.
FILE* log_file{};
#include "weapon_hand_offline_fixture.h"
#include <vector>

int main(){
    using gears_read_validation::Result;
    unsigned cases{};
    {
        // Real UObject-style index at+8 and 24-byte header at+0x40 leave
        // exactly52bytes between witnesses. Use ten real aligned pages so
        // batching must respect both4KiB boundaries and1024byte limits.
        constexpr unsigned size=40*1024,stride=128;
        auto* data=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
        assert(data);for(unsigned i=0;i<size;++i)data[i]=static_cast<unsigned char>(i*37+11);
        auto old=std::make_unique<gears_read_validation::BatchedProof<>>();
        auto current=std::make_unique<gears_read_validation::SourceProof>();
        old->begin();current->begin();const auto address=reinterpret_cast<uptr>(data);
        for(unsigned offset=0;offset<size;offset+=stride){
            const auto capture=[&](auto& proof){
                proof.record(address+offset+8,data+offset+8,4);
                proof.record(address+offset+64,data+offset+64,24);
            };
            capture(*old);capture(*current);
        }
        assert(old->seal()&&current->seal());
        assert(old->used==current->used&&current->used==(size/stride)*28);
        assert(current->validation_reads_per_pass()<old->validation_reads_per_pass());
        assert(current->padding_bytes<=1048576&&current->batched);
        for(unsigned i=0;i<current->batch_count;++i){auto const& b=current->batches[i];
            assert(b.bytes<=1024&&b.address/4096==(b.address+b.bytes-1)/4096);
        }
        unsigned calls{};
        const auto read=[&](void* to,uptr from,unsigned bytes){++calls;std::memcpy(to,reinterpret_cast<void const*>(from),bytes);return true;};
        assert(current->validate(read)==Result::same&&calls==2*current->validation_reads_per_pass());
        // Every original witness byte remains significant under the wider
        // plan, including all bytes adjacent to the added padding.
        unsigned mutated{};
        for(unsigned i=0;i<current->merged;++i){auto const r=current->ranges[i];
            for(unsigned j=0;j<r.bytes;++j){auto* b=reinterpret_cast<unsigned char*>(r.address+j);*b^=1;
                assert(current->validate(read)==Result::changed);*b^=1;++mutated;}
        }
        assert(mutated==current->used);++cases;
        // Unread gaps may change but never become witnesses. A read failure
        // touching that very padding remains terminal, with no scalar retry.
        data[12]^=1;assert(current->validate(read)==Result::same);data[12]^=1;
        calls=0;const auto padding_fault=[&](void* to,uptr from,unsigned bytes){++calls;
            if(from<=address+12&&address+12-from<bytes)return false;
            std::memcpy(to,reinterpret_cast<void const*>(from),bytes);return true;};
        assert(current->validate(padding_fault)==Result::unreadable&&calls==1);++cases;
        DWORD previous{};assert(VirtualProtect(data,4096,PAGE_READWRITE|PAGE_GUARD,&previous));
        calls=0;
        assert(current->validate([&](void* to,uptr from,unsigned bytes){++calls;return safe_copy(to,from,bytes);})==Result::unreadable&&calls==1);
        // A retry could now succeed because PAGE_GUARD was consumed; the
        // validator must already have returned unreadable without that retry.
        assert(current->validate(read)==Result::same);++cases;
        {
            // Too few planning slots must retain the exact validator rather
            // than skip bytes, reject a readable source or grant a witness.
            gears_read_validation::BatchedProof<2,8192,4,0,64> limited;
            limited.begin();limited.record(address,data,4096);
            assert(limited.seal()&&!limited.batched&&limited.merged==1);
            calls=0;assert(limited.validate(read)==Result::same&&calls==32);
            data[4095]^=1;assert(limited.validate(read)==Result::changed);data[4095]^=1;++cases;
        }
        std::printf("UObject40KiB plan: default32gap_reads=%u source64gap_reads=%u witness_bytes=%u padding=%u mutated_bytes=%u; CPU synthetic only\n",
            old->validation_reads_per_pass(),current->validation_reads_per_pass(),current->used,current->padding_bytes,mutated);
        assert(VirtualFree(data,0,MEM_RELEASE));
    }
    {gears_read_validation::Proof<16,1024,32> p;std::array<unsigned char,800> data{};
        for(unsigned i=0;i<data.size();++i)data[i]=static_cast<unsigned char>(i);
        const auto address=reinterpret_cast<uptr>(data.data());p.begin();
        p.record(address+10,data.data()+10,20);p.record(address+15,data.data()+15,5);
        p.record(address+30,data.data()+30,10);p.record(address+500,data.data()+500,280);
        p.record(address+10,data.data()+10,20);assert(p.count==4&&p.seal()&&p.merged==2&&p.used==310);
        unsigned calls{},bytes{};
        auto read=[&](void* to,uptr from,unsigned size){++calls;bytes+=size;
            // No byte of the intervening 460-byte unread gap can be read.
            assert((from>=address+10&&from+size<=address+40)||(from>=address+500&&from+size<=address+780));
            std::memcpy(to,reinterpret_cast<void const*>(from),size);return true;};
        assert(p.validate(read)==Result::same&&calls==6&&bytes==620);++cases;
        for(auto offset:{10u,39u,500u,755u,779u}){data[offset]^=1;assert(p.validate(read)==Result::changed);data[offset]^=1;++cases;}
        data[400]^=1;assert(p.validate(read)==Result::same);++cases;
        calls=0;assert(p.validate([&](void*,uptr,unsigned){++calls;return false;})==Result::unreadable&&calls==1);++cases;
        p.begin();p.record(address,data.data(),10);data[0]^=1;p.record(address,data.data(),10);assert(!p.seal());data[0]^=1;++cases;
        p.begin();p.record(address,data.data(),10);data[5]^=1;p.record(address+5,data.data()+5,10);assert(!p.seal());data[5]^=1;++cases;
        p.begin();p.record(address,data.data(),799);p.record(address+1,data.data()+1,799);assert(!p.seal());++cases;
        p.begin();p.record(UINTPTR_MAX-3,data.data(),10);assert(!p.seal());++cases;
    }
    auto live=[](Memory& m){abi::GripGameState out{};return weaponhand::observe_liveness(nullptr,m.frame,weaponhand::clock_ns(nullptr),out)&&out.known;};
    {auto m=std::make_unique<Memory>();weapon_metadata_parse::TickScope parsing;
        weaponhand::source_validation::Scope scope(m->frame);unsigned original_reads{},fast_reads{};
        read_observer=[&](uptr,std::size_t){++original_reads;};assert(live(*m)&&scope.sealed);
        read_observer=[&](uptr,std::size_t){++fast_reads;};assert(live(*m)&&scope.validations==1);
        assert(fast_reads<original_reads);read_observer={};
        // Every byte the successful production liveness observer actually read
        // must invalidate the proof when changed. No copied selector oracle.
        auto& proof=*scope.proof;unsigned mutations{};
        for(unsigned i=0;i<proof.merged;++i){auto const r=proof.ranges[i];
            for(unsigned j=0;j<r.bytes;++j){auto* b=reinterpret_cast<unsigned char*>(r.address+j);*b^=1;
                assert(proof.validate([](void* to,uptr from,unsigned n){return safe_copy(to,from,n);})==Result::changed);
                *b^=1;++mutations;
            }
        }
        assert(mutations==proof.used);++cases;
        std::printf("production observer reads=%u flat validation reads=%u ranges=%u bytes=%u mutated_bytes=%u\n",original_reads,fast_reads,proof.merged,proof.used,mutations);
        // Busy is a legal new state: readable mismatch reruns the original
        // observer, rather than retaining a stale true/false or false refusal.
        Memory::put(m->pawn,0x128,1u);assert(live(*m)&&scope.fallbacks==1&&scope.sealed);++cases;
        Memory::put(m->pawn,0x108,4u);assert(!live(*m)&&!scope.sealed);++cases;
    }
    for(unsigned negative=0;negative<7;++negative){auto m=std::make_unique<Memory>();
        weaponhand::source_validation::Scope scope(m->frame);assert(live(*m)&&scope.sealed);
        switch(negative){
        case 0: Memory::put(m->pawn,0x120,uptr(0));break;
        case 1: Memory::put(m->pc,0x110,uptr(0));break;
        case 2: Memory::put(m->weapon,0x110,uptr(0));break;
        case 3: Memory::put(m->pawn_class,0x88,1u);break;
        case 4: Memory::put(m->mesh_property,0x94,0xfffffff0u);break;
        case 5: Memory::put(m->weapon,0x50,uptr(0));break;
        case 6: fail_read=m->pawn+0x128;break;
        }
        const bool accepted=live(*m);if(accepted)std::fprintf(stderr,"unexpected negative acceptance: %u\n",negative);
        assert(!accepted);if(negative==6)assert(scope.disabled&&scope.fallbacks==0);++cases;
    }
    {auto m=std::make_unique<Memory>();weaponhand::source_validation::Scope scope(m->frame);assert(live(*m));
        unsigned reads{};read_observer=[&](uptr,std::size_t){++reads;};
        {auto different=m->frame;different.key.frame++;weaponhand::source_validation::Scope nested(different);
            assert(live(*m)&&nested.disabled&&reads>scope.proof->merged*2);}
        reads=0;assert(live(*m)&&reads==scope.proof->validation_reads_per_pass()*2);read_observer={};++cases;
        enabled.store(false);assert(!live(*m));enabled.store(true);++cases;
        const auto old=game_thread.exchange(0);assert(!live(*m));game_thread.store(old);++cases;
        fail_read=m->pawn+0x128;unsigned failed_reads{};
        read_observer=[&](uptr from,std::size_t bytes){if(fail_read>=from&&fail_read-from<bytes)++failed_reads;};
        assert(!live(*m)&&failed_reads==1&&scope.disabled);read_observer={};++cases;
    }
    assert(!gears_read_validation::recording&&!weaponhand::source_validation::active);
    {auto m=std::make_unique<Memory>();weaponhand::source_validation::Scope scope(m->frame);
        assert(live(*m)&&scope.sealed);
        // A warm liveness witness is intentionally NOT a Game payload.
        // Hold verification must explicitly use the full source observer.
        abi::GripGameState boolean_only{},held_game{};
        assert(weaponhand::observe_liveness(nullptr,m->frame,0,boolean_only)&&boolean_only.known&&!boolean_only.main_hand_held);
        assert(weaponhand::observe_source(nullptr,m->frame,0,held_game)&&held_game.known&&held_game.main_hand_held);++cases;
        weaponhand::held_binding.known=false;
        assert(live(*m));abi::GripGameState full{};
        assert(weaponhand::observe_source(nullptr,m->frame,0,full)&&full.known&&!full.main_hand_held);
        Memory::put(m->pawn,0x128,1u);assert(live(*m)&&scope.fallbacks==1);
        assert(weaponhand::observe_source(nullptr,m->frame,0,full)&&full.busy);++cases;
    }
    {auto m=std::make_unique<Memory>();weapon_metadata_parse::TickScope parsing;
        const auto began=weaponhand::clock_ns(nullptr);for(unsigned i=0;i<1000;++i)assert(live(*m));
        const auto original_ns=weaponhand::clock_ns(nullptr)-began;
        weaponhand::source_validation::Scope scope(m->frame);assert(live(*m));
        const auto flat_began=weaponhand::clock_ns(nullptr);for(unsigned i=0;i<1000;++i)assert(live(*m));
        const auto flat_ns=weaponhand::clock_ns(nullptr)-flat_began;
        std::printf("owned artificial memory benchmark 1000 guards: original_ns=%lld fresh_flat_ns=%lld; not a game timing result\n",original_ns,flat_ns);
    }
    std::printf("WeaponSourceValidationTest passed: %u cases; exact fresh reads, no live engine or game writes\n",cases);
}
