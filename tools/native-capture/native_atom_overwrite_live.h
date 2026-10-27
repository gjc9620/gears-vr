#pragma once
#include "native_atom_overwrite_receipt.h"
namespace native_atom_overwrite_live {
bool configure(wchar_t const*,FILE*);bool reserve();bool reserved_target(void*);bool world_current()noexcept;
bool ready_for_commit()noexcept;
bool install(gears_atom_overwrite::Context&,gears_atom_overwrite::tx::Stamp const&,std::uint32_t)noexcept;
gears_atom_overwrite::ReceiptSource* receipts()noexcept;std::uint64_t journal_version()noexcept;
void retired(std::uint64_t,std::uint64_t)noexcept;void request_drain();unsigned pending_count();void summary(FILE*);
}
namespace full_body_c0_native_live {void atom_overwrite_completed(std::uint64_t)noexcept;}
