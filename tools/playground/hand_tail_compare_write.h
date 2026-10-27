#pragma once
#include <windows.h>
#include "../vr-common/hand_tail_translation_lease.h"

namespace gears_hand_tail_fields {
using Address=std::uintptr_t;
// Not a write permit. Caller must first prove the live owner, property schema,
// addresses, journal, original action and Plan. No engine call may intervene.
// This last SEH boundary repeats the exact +0 and expected-byte proof and owns
// only the selected bits of a packed word. It never activates a controller.
inline bool compare_write(std::array<Address,4> const& addresses,
                          std::array<Address,3> const& strength_addresses,
                          std::uint32_t apply_mask,std::uint32_t add_mask,
                          gears_hand_tail_translation::Field field,
                          gears_hand_tail_translation::FieldValue const& expected,
                          gears_hand_tail_translation::FieldValue const& wanted) noexcept {
    using Field=gears_hand_tail_translation::Field;
    const auto bytes=gears_hand_tail_translation::field_size(field);
    const unsigned index=unsigned(field);
    if(!bytes||index>=addresses.size()||!addresses[index]||!apply_mask||!add_mask||
       (apply_mask&(apply_mask-1))||(add_mask&(add_mask-1))||(apply_mask&add_mask))return false;
    for(const auto address:strength_addresses)if(!address)return false;
    __try{
        std::uint32_t zero{};
        for(unsigned i=0;i<3;++i){
            std::memcpy(&zero,reinterpret_cast<void const*>(strength_addresses[i]),sizeof(zero));
            if(zero)return false;
        }
        auto const address=reinterpret_cast<void*>(addresses[index]);
        if(field==Field::flags){
            const auto mask=apply_mask|add_mask;
            std::uint32_t live{},old{},next{};
            std::memcpy(&live,address,4);std::memcpy(&old,expected.data(),4);std::memcpy(&next,wanted.data(),4);
            if((old&~mask)||(next&~mask)||(live&mask)!=old)return false;
            live=(live&~mask)|next;std::memcpy(address,&live,4);
        }else{
            if(std::memcmp(address,expected.data(),bytes))return false;
            std::memcpy(address,wanted.data(),bytes);
        }
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
} // namespace gears_hand_tail_fields
