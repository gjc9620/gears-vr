#pragma once
#include "owned_ui_ledger.h"

// CPU/UI stage only. Callers serialize admission and supply independent native
// renderer/Canvas and GPU retirement proof. This helper invokes no engine/GPU,
// allocates no resources, clears no TLS and never reopens producer admission.
namespace gears_owned_ui_world {
struct Guards {
 uint64_t old_world{},ticket_cutoff{};
 unsigned semantic_pending{};
 bool production_closed{},material_pending{},material_broken{},route_fault{},resources_retired{};
};
inline bool safe(Guards const& g){return g.old_world&&g.production_closed&&!g.semantic_pending&&
 !g.material_pending&&!g.material_broken&&!g.route_fault;}
struct Receipt {uint64_t old_world{};gears_owned_ui::CpuRetirement cpu{};};
inline bool cpu_ready(gears_owned_ui::Ledger const& ledger,Guards const& g,int64_t qpc,unsigned kind_mask){
 return safe(g)&&ledger.cpu_drained()&&ledger.retirement_ready(g.ticket_cutoff,qpc,kind_mask);
}
inline bool prepare_cpu(gears_owned_ui::Ledger& ledger,Guards const& g,int64_t qpc,unsigned kind_mask,Receipt& out){
 if(!g.resources_retired||!cpu_ready(ledger,g,qpc,kind_mask))return false;
 Receipt candidate{};candidate.old_world=g.old_world;
 if(!ledger.discard_completed_through(g.ticket_cutoff,qpc,kind_mask,candidate.cpu))return false;
 out=candidate;return true;
}
// Final check belongs under the closed world reader gate and existing outer
// frame/session locks, immediately before payload/binding/sidecar retirement.
// GPU readiness is still an independent required condition at the root caller.
inline bool current(gears_owned_ui::Ledger const& ledger,Guards const& g,Receipt const& receipt){
 return safe(g)&&g.resources_retired&&receipt.old_world==g.old_world&&receipt.cpu.ticket_cutoff==g.ticket_cutoff&&
  ledger.retirement_current(receipt.cpu);
}
}
