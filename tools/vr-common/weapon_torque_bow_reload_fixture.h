#pragma once
#include "weapon_torque_bow_driver_fixture.h"
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif
namespace {
struct ReloadBackend:Backend {
 bool reload{},complete{},permit=true,accept_wait{},receipt_wait{};
 std::int64_t reload_started_tick{};unsigned starts{},completions{},retirements{};
 std::uint64_t serial{10},arrow_id{100},grip{1},component{201};w::TorqueBowRequest request{};
 bool reload_owned()const noexcept{return reload;}
 bool configure(h::Frame const& f,h::GripGameState const& g,d::Snapshot const& s,w::TorqueBowConfig& c)noexcept{
  if(!Backend::configure(f,g,s,c))return false;c.manual_reload_phase_verified=c.reload_plan_verified=true;
  c.reload_step_count=1;c.reload_steps[0]={w::TorqueBowReloadAction::insert_arrow,component};c.game_ticks_per_second=1000;return true;}
 bool reload_begin(h::Frame const&,h::GripGameState const&,h::Decision const& decision,w::TorqueBowInventory const& i)noexcept{
  if(reload||owner||!decision.new_grant||i.loaded||!i.reserve)return false;reload=true;reload_started_tick=actual.game_tick;++starts;++serial;++arrow_id;return true;}
 R reload_accept(h::Frame const&,h::GripGameState const&,d::ReloadAcceptance& out)noexcept{
  if(!reload)return R::rejected;if(accept_wait)return R::unknown;out={actual,serial,arrow_id,grip,reload_started_tick,true,true,true};return R::accepted;}
 bool reload_sample(h::Frame const& f,d::Snapshot& out)noexcept{return reload&&observe(f,{},out);}
 bool reload_regrab(h::Frame const&,h::Decision const& decision,w::TorqueBowArrowReceipt& out)noexcept{
  if(!reload||!decision.new_grant)return false;++arrow_id;++grip;
  out={arrow_id,w::TorqueBowAmmoSource::left_shoulder,true,true,grip,true,true};return true;}
 bool reload_complete(h::Frame const&,h::GripGameState const&,w::TorqueBowRequest const& r)noexcept{
  if(!reload||complete||!permit)return false;assert(!actual.loaded&&actual.reserve>0);++completions;request=r;complete=true;actual.loaded=1;--actual.reserve;return true;}
 bool reload_receipt(h::Frame const*,w::TorqueBowActionReceipt& r,d::Snapshot& out)noexcept{
  if(!reload||!complete||receipt_wait)return false;r={};r.request_token=request.token;r.accepted=true;r.inventory=request.after;
  r.inventory.revision=request.before.revision+1;r.original=actual.original;r.reload_result_applied=request.outcome;out=actual;return true;}
 bool reload_retire(std::uint64_t token,w::TorqueBowInteraction const& model)noexcept{
  if(!complete||token!=request.token||model.last_loaded_token()!=token)return false;reload=complete=false;++retirements;return true;}
};
}
