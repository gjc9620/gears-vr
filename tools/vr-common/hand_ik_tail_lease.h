#pragma once
#include "hand_ik_stream_lease.h"
#include "hand_tail_translation_lease.h"

namespace gears_hand_ik_tail {
namespace primary=gears_hand_ik_stream;
namespace tail=gears_hand_tail_translation;

// Engine adapter contract, in addition to the original four-channel Backend:
// tail_wanted(channel_mask, frozen_targets, wanted[2]), tail_forward_allowed(),
// tail_read(hand, Value&, Strength&), tail_write_field(hand, Field, old, wanted).
// Hand0 is the original right pair0/1; hand1 is the left pair2/3. The adapter
// resolves all real types/identities/flag masks; this class knows no engine ABI.
// Only restoring()/primary_faulted() permit the original rollback admission
// bypass. Aggregate fault must reject FORWARD writes. Neither bypass permits
// skipping live identity/schema/ownership/expected-byte checks. The adapter's
// tail mutations additionally require fresh exact+0 and masked bit protection.
// Caller must keep its original Plan/epoch/action postchecks and operation
// lifetime. No copy/move, new clock, activation API, or automatic fault reset.
class Lease {
    primary::Lease primary_;
    tail::Lease tail_;
    bool faulted_{},busy_{},restoring_{};
    struct Busy {
        bool& value;
        explicit Busy(bool& flag):value(flag){value=true;}
        ~Busy(){value=false;}
        Busy(Busy const&)=delete;Busy& operator=(Busy const&)=delete;
    };
    struct Restore {
        bool& value;
        explicit Restore(bool& flag):value(flag){value=true;}
        ~Restore(){value=false;}
        Restore(Restore const&)=delete;Restore& operator=(Restore const&)=delete;
    };
    template<class Backend> struct TailBackend {
        Backend& backend;
        bool read(unsigned hand,tail::Value& value,tail::Strength& strength){
            return backend.tail_read(hand,value,strength);
        }
        bool write_field(unsigned hand,tail::Field field,tail::FieldValue const& expected,
                         tail::FieldValue const& wanted){
            return backend.tail_write_field(hand,field,expected,wanted);
        }
    };
    static unsigned hands(unsigned channels) noexcept {
        return channels==3?1:channels==12?2:channels==15?3:0;
    }
public:
    Lease()=default;
    Lease(Lease const&)=delete;Lease& operator=(Lease const&)=delete;
    bool dirty()const noexcept {return primary_.dirty()||tail_.dirty();}
    bool faulted()const noexcept {return faulted_||primary_.faulted()||tail_.faulted();}
    bool primary_faulted()const noexcept {return primary_.faulted();}
    bool restoring()const noexcept {return restoring_;}
    bool busy()const noexcept {return busy_;}
    tail::Lease const& translation()const noexcept {return tail_;}
    bool reset() noexcept {
        if(busy_||restoring_||dirty())return false;
        if(!primary_.reset()||!tail_.reset())return false;
        faulted_=false;return true;
    }
    template<class Backend> bool restore(Backend& backend){
        if(busy_){faulted_=true;return false;}
        Busy guard(busy_);Restore restoring(restoring_);
        const bool primary_clean=primary_.restore(backend);
        TailBackend<Backend> tail_backend{backend};
        // Always attempt extras. A false primary helper may have reached zero;
        // only the extra backend's fresh strength read decides its admission.
        const bool tail_clean=tail_.restore_zero(tail_backend);
        return primary_clean&&tail_clean&&!dirty(); // Clean does not clear fault.
    }
    template<class Backend>
    bool acquire(unsigned channel_mask,std::array<primary::Target,4> const& targets,
                 float strength,Backend& backend){
        if(busy_){faulted_=true;return false;} // Outer exit performs ordered cleanup.
        if(faulted()||dirty()||!hands(channel_mask)||!std::isfinite(strength)||strength<=0.f||strength>1.f){
            faulted_=true;(void)restore(backend);return false;
        }
        const auto frozen_targets=targets;
        bool acquired{};
        {
            Busy guard(busy_);TailBackend<Backend> tail_backend{backend};
            std::array<tail::Value,2> wanted{};
            if(tail_.reset()&&backend.tail_forward_allowed()&&
               backend.tail_wanted(channel_mask,frozen_targets,wanted)&&!faulted()&&
               targets==frozen_targets&&backend.tail_forward_allowed()&&
               tail_.prepare(hands(channel_mask),wanted,tail_backend)&&!faulted()&&
               targets==frozen_targets&&backend.tail_forward_allowed()){
                const bool primary_acquired=primary_.acquire(channel_mask,frozen_targets,strength,backend);
                // A nested action in the LAST strength setter can return true
                // and leave every value correct. Recheck the aggregate/context
                // after it returns; readback alone cannot commit that operation.
                acquired=primary_acquired&&!faulted()&&targets==frozen_targets&&backend.tail_forward_allowed();
            }
        }
        if(!acquired){faulted_=true;(void)restore(backend);return false;}
        return true;
    }
    template<class Backend>
    bool acquire(std::array<primary::Target,4> const& targets,float strength,Backend& backend){
        return acquire(15,targets,strength,backend);
    }
};
} // namespace gears_hand_ik_tail
