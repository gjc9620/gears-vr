#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace gears_hand_tail_translation {
using Strength=std::array<float,3>;
struct Value {
    std::array<unsigned char,12> translation{};
    std::uint8_t space{};
    std::array<unsigned char,8> space_bone{};
    std::uint32_t flags{}; // Only validated bApplyTranslation/bAddTranslation bits.
};
enum class Field:unsigned {translation,space,space_bone,flags,count};
using FieldValue=std::array<unsigned char,12>; // Zero padding; only field_size bytes are meaningful.
constexpr std::size_t field_size(Field field) noexcept {
    return field==Field::translation?12:field==Field::space?1:
        field==Field::space_bone?8:field==Field::flags?4:0;
}
inline FieldValue field_value(Value const& value,Field field) noexcept {
    FieldValue out{};
    switch(field){
    case Field::translation:out=value.translation;break;
    case Field::space:out[0]=value.space;break;
    case Field::space_bone:std::memcpy(out.data(),value.space_bone.data(),8);break;
    case Field::flags:std::memcpy(out.data(),&value.flags,4);break;
    default:break;
    }
    return out;
}
inline void assign_field(Value& value,Field field,FieldValue const& bytes) noexcept {
    switch(field){
    case Field::translation:value.translation=bytes;break;
    case Field::space:value.space=bytes[0];break;
    case Field::space_bone:std::memcpy(value.space_bone.data(),bytes.data(),8);break;
    case Field::flags:std::memcpy(&value.flags,bytes.data(),4);break;
    default:break;
    }
}
inline bool exact(Value const& a,Value const& b) noexcept {
    return a.translation==b.translation&&a.space==b.space&&a.space_bone==b.space_bone&&a.flags==b.flags;
}
inline bool exact_zero(Strength const& strength) noexcept {
    static_assert(sizeof(float)==4);const Strength zero{{0.f,0.f,0.f}};
    return !std::memcmp(strength.data(),zero.data(),sizeof(float)*3); // -0 is not +0.
}

// No engine enums, FName/flag masks, activation, clocks or authorization here.
// Backend: bool read(hand,Value&,Strength&); bool write_field(hand,Field,
// FieldValue const& expected,FieldValue const& wanted). Every operation must
// validate the SAME live identity/schema. At the actual compare-write, backend
// must again require exact +0 strength and expected field bytes, validate the
// selected bool masks, and preserve every unrelated storage-word bit. VM gaps
// in a backend need its own final non-invoking checks; accepts is not authority.
// Caller supplies all desired bytes and verifies its Plan/action/epoch gates.
// Prepare these extras BEFORE activating the original four-channel hand Lease.
// Cleanup MUST attempt that original Lease first, then restore_zero; a failed
// zero-strength proof never permits forced extra-field writes or unload.
// Unknown/foreign observation revokes this hand's pending journal permanently;
// a later unobserved write-and-return cannot be detected by value comparison.
// Serialized owner only. No copying/moving journals. No automatic rollback.
class Lease {
    struct Journal {
        Value original{},owned{},staged{},alternate{};
        unsigned fields{},pending{};
        bool alternate_known{},revoked{};
    };
    std::array<Journal,2> hands_{};
    unsigned selected_{};
    bool faulted_{},busy_{},prepared_{};
    struct Busy {
        bool& flag;explicit Busy(bool& value):flag(value){flag=true;}
        ~Busy(){flag=false;}
        Busy(Busy const&)=delete;Busy& operator=(Busy const&)=delete;
    };
    bool fail() noexcept {faulted_=true;return false;}
    bool revoke(unsigned hand) noexcept {hands_[hand].revoked=true;return fail();}
    template<class Backend> bool observe(unsigned hand,Backend& backend,Value& value) {
        Strength strength{};
        if(!backend.read(hand,value,strength))return revoke(hand);
        if(!accepts(hand,value))return revoke(hand);
        if(!exact_zero(strength))return fail();
        auto& journal=hands_[hand];journal.staged=value;journal.alternate_known=false;
        return true;
    }
    template<class Backend>
    bool write_one(unsigned hand,Field field,FieldValue const& wanted,Backend& backend,bool restoring,bool& zero_observed) {
        zero_observed=false;auto& journal=hands_[hand];Value current{};
        if(!observe(hand,backend,current)||(!restoring&&faulted_))return false;
        const auto expected=field_value(current,field);
        if(expected==wanted){zero_observed=true;return true;}
        journal.alternate=current;assign_field(journal.alternate,field,wanted);
        journal.alternate_known=true;
        journal.pending|=1u<<unsigned(field); // BEFORE a helper that may mutate and return false.
        const bool transported=backend.write_field(hand,field,expected,wanted);
        if(!transported)faulted_=true;
        Value after{};
        if(!observe(hand,backend,after))return false;
        zero_observed=true;const bool completed=field_value(after,field)==wanted;
        if(!completed)faulted_=true;
        return transported&&completed&&(restoring||!faulted_);
    }
public:
    Lease()=default;
    Lease(Lease const&)=delete;Lease& operator=(Lease const&)=delete;
    unsigned selected_mask()const noexcept {return selected_;}
    unsigned selected_fields(unsigned hand)const noexcept {return original(hand)?hands_[hand].fields:0;}
    unsigned pending_fields(unsigned hand)const noexcept {return original(hand)?hands_[hand].pending:0;}
    Value const* original(unsigned hand)const noexcept {
        return hand<2&&(selected_&(1u<<hand))?&hands_[hand].original:nullptr;
    }
    Value const* owned(unsigned hand)const noexcept {return original(hand)?&hands_[hand].owned:nullptr;}
    Value const* staged(unsigned hand)const noexcept {return original(hand)?&hands_[hand].staged:nullptr;}
    bool revoked(unsigned hand)const noexcept {return original(hand)&&hands_[hand].revoked;}
    bool dirty()const noexcept {return hands_[0].pending||hands_[1].pending;}
    bool faulted()const noexcept {return faulted_;}
    bool prepared()const noexcept {return prepared_;}
    // Pure predicate; a caller observing rejection must retain its own terminal
    // decision. Internal read paths below record sticky per-hand revocation.
    bool accepts(unsigned hand,Value const& current)const noexcept {
        if(!original(hand)||hands_[hand].revoked)return false;
        auto const& journal=hands_[hand];
        return exact(current,journal.staged)||(journal.alternate_known&&exact(current,journal.alternate));
    }
    bool reset() noexcept {
        if(busy_||dirty())return false;
        hands_={};selected_=0;faulted_=prepared_=false;return true;
    }
    template<class Backend>
    bool capture(unsigned mask,std::array<Value,2> const& wanted,Backend& backend) {
        if(busy_||faulted_||selected_||!mask||(mask&~3u))return fail();
        Busy busy(busy_);std::array<Journal,2> captured{};const auto frozen=wanted;
        // Snapshot EVERY selected hand before any mutation or published journal.
        for(unsigned hand=0;hand<2;++hand)if(mask&(1u<<hand)){
            auto& journal=captured[hand];Strength strength{};
            if(!backend.read(hand,journal.original,strength)||!exact_zero(strength)||faulted_)return fail();
            journal.staged=journal.original;journal.owned=frozen[hand];
            for(unsigned f=0;f<unsigned(Field::count);++f)
                if(field_value(journal.original,Field(f))!=field_value(journal.owned,Field(f)))journal.fields|=1u<<f;
        }
        hands_=captured;selected_=mask;return true;
    }
    template<class Backend> bool prepare(Backend& backend) {
        if(busy_||faulted_||!selected_||prepared_)return fail();
        Busy busy(busy_);
        for(unsigned hand=0;hand<2;++hand)if(selected_&(1u<<hand)){
            auto const& journal=hands_[hand];
            for(unsigned f=0;f<unsigned(Field::count);++f)if(journal.fields&(1u<<f)){
                bool zero_observed{};
                if(!write_one(hand,Field(f),field_value(journal.owned,Field(f)),backend,false,zero_observed))return false;
            }
        }
        for(unsigned hand=0;hand<2;++hand)if(selected_&(1u<<hand)){
            Value current{};
            if(!observe(hand,backend,current)||!exact(current,hands_[hand].owned)||faulted_)return fail();
        }
        prepared_=true;return true;
    }
    template<class Backend>
    bool prepare(unsigned mask,std::array<Value,2> const& wanted,Backend& backend) {
        return capture(mask,wanted,backend)&&prepare(backend);
    }
    // Returns journal cleanliness, NOT absence of sticky helper/observation faults.
    // Reverse field order: selected flags first, translation last, only at +0.
    template<class Backend> bool restore_zero(Backend& backend) {
        if(busy_)return fail();
        Busy busy(busy_);prepared_=false;
        for(unsigned hand=0;hand<2;++hand){
            auto& journal=hands_[hand];
            if(!journal.pending||journal.revoked)continue;
            Value current{};
            if(!observe(hand,backend,current))continue;
            for(unsigned f=unsigned(Field::count);f>0;--f){
                const unsigned bit=1u<<(f-1);if(!(journal.pending&bit))continue;
                const auto field=Field(f-1);const auto wanted=field_value(journal.original,field);
                bool zero_observed{};(void)write_one(hand,field,wanted,backend,true,zero_observed);
                if(!zero_observed||journal.revoked||journal.alternate_known)break;
                if(field_value(journal.staged,field)==wanted)journal.pending&=~bit;
            }
        }
        return !dirty();
    }
};
} // namespace gears_hand_tail_translation
