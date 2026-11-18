// Exact-owner non-firing Bow cancellation. Called by the existing GT input
// owner, never by a driver, render callback or raw controller producer.
namespace torquebow_cancel_live {
namespace hand=gears_weapon_hand;namespace meta=gripdamage_metadata;
using Result=gears_weapon::TorqueBowCallResult;using Phase=gears_weapon::TorqueBowCancelPhase;
using Route=gears_weapon::TorqueBowCancelRoute;using Identity=gears_weapon::TorqueBowIdentity;
enum class Method {clear_pending,charge_ended,active,put_down};
#include "weapon_torque_bow_putdown_schema.inl"
struct Entry {hand::ObjectIdentity function{};Method method{};unsigned bytes{};std::uint64_t fingerprint{};};
const wchar_t* label(Method m) noexcept {
 return m==Method::clear_pending?L"ClearPendingFire":m==Method::charge_ended?L"ChargeEnded":m==Method::active?L"GotoState":L"PutDownWeapon";
}
struct BindingProbe {
 playground::Args& call;Method method;unsigned gate{1};
 ~BindingProbe() noexcept {
  static std::array<unsigned,4> rows{};auto& count=rows[unsigned(method)];
  if(!gate||!log_file||count>=2||!registered(call.r.function))return;++count;
  const auto fn=call.r.function,owner=get<uptr>(fn+0x40);wchar_t owner_name[128]{};
  if(registered(owner))name(owner,owner_name,128);
  const auto script=get<meta::Table>(fn+0x8c);
  fwprintf(log_file,L"torquebow_cancel_bind method=%ls gate=%u function=%llx owner=%ls bytes=%u extent=%u return_offset=%u native=%u flags=%x exec=%llx script_count=%d script_capacity=%d observation_only=1\n",
   label(method),gate,fn,owner_name,call.r.size,get<unsigned>(fn+0x88),unsigned(get<unsigned short>(fn+0x108)),
   unsigned(get<unsigned short>(fn+0xf8)),get<unsigned>(fn+0xf4),get<uptr>(fn+0x114),script.count,script.capacity);
  auto child=get<uptr>(fn+0x80);
  for(unsigned i=0;child&&i<8&&registered(child);++i){
   wchar_t field_name[128]{},kind[128]{};name(child,field_name,128);const auto type=get<uptr>(child+0x50);
   if(registered(type))name(type,kind,128);
   fwprintf(log_file,L"torquebow_cancel_bind_field method=%ls row=%u name=%ls type=%ls bytes=%u dimension=%u offset=%u flags=%llx owner=%llx next=%llx observation_only=1\n",
    label(method),i,field_name,kind,get<unsigned>(child+0x74),get<unsigned>(child+0x70),get<unsigned>(child+0x94),
    get<unsigned long long>(child+0x78),get<uptr>(child+0x40),get<uptr>(child+0x68));
   child=get<uptr>(child+0x68);
  }
  fflush(log_file);
 }
};
bool bind_entry(uptr weapon,Method method,Entry& out) noexcept {
 out={};playground::Args call(weapon,label(method));
 BindingProbe probe{call,method};
 const bool owner=method==Method::clear_pending?torquebow_original::original_function(call,L"Weapon",L"Engine"):
  method==Method::charge_ended?torquebow_original::original_function(call,L"Weap_Bow_base",L"WarfareGame"):
  method==Method::active?torquebow_original::original_function(call,L"Object",L"Core"):
  (torquebow_original::original_function(call,L"Weapon",L"Engine")||torquebow_original::original_function(call,L"WarWeapon",L"WarfareGame"));
 if(!owner)return false;
 probe.gate=2;
 unsigned transition_bytes{};
 if(method==Method::clear_pending&&(call.r.size!=4||!torquebow_original::input_property(call,L"FireMode",L"IntProperty",4,0)))return false;
 if(method==Method::charge_ended&&call.r.size)return false;
 if(method==Method::active&&!torquebow_original::transition_schema(call,transition_bytes))return false;
 if(method==Method::put_down&&!put_down_schema(call))return false;
 probe.gate=3;
 meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::function_fields};
 r.reuse_initial_identities=true; // This bind's discovery only; retained checks below read fresh.
 unsigned fn{};meta::Structure structure{};std::uint16_t native{};unsigned flags{};uptr exec{};meta::Table script{};
 if(!meta::tables(base,schema.objects,schema.names)||!r.path(call.r.function,fn)||!r.structure(call.r.function,structure)||
  !r.at(call.r.function,0xf8,native)||!r.at(call.r.function,0xf4,flags)||!r.at(call.r.function,0x114,exec)||!exec||
  !r.at(call.r.function,0x8c,script)||script.count<0||script.count>script.capacity||script.capacity>131072)return false;
 // GotoState is the original native113 state transition, not an arbitrary
 // same-name helper. The other methods are the original script functions.
 probe.gate=4;if(method==Method::active?native!=113||!(flags&0x400):native||(flags&0x400))return false;
 std::uint64_t script_hash{};
 probe.gate=5;
 if(method!=Method::active&&(!script.count||!weaponshot::hash_script(script.data,unsigned(script.count),script_hash)))return false;
 std::array<uptr,meta::max_fields> seen{};unsigned count{};
 probe.gate=6;
 for(auto child=structure.children;child;){
  if(count==seen.size())return false;for(unsigned i=0;i<count;++i)if(seen[i]==child)return false;
  seen[count++]=child;meta::Property p{};
  if(!r.property(child,p)||schema.identities[p.identity].outer!=call.r.function||!meta::property_fits(p,structure.extent))return false;
  if(p.flags&0x80){
   if(p.dimension!=1||!meta::property_fits(p,call.r.size)||schema.field_count==schema.fields.size())return false;
   schema.fields[schema.field_count++]=p;
  }
  child=p.next;
 }
 r.reuse_initial_identities=false;
 probe.gate=7;if(!unified_weapon_fire::retained_metadata(r))return false;
 unified_weapon_fire::Fingerprint hash;unified_weapon_fire::schema_fingerprint(schema,hash);
 hash.word(unsigned(method));hash.word(native);hash.word(flags);hash.word(exec);hash.word(script.data);hash.word(unsigned(script.count));hash.word(script_hash);
 probe.gate=8;hand::ObjectIdentity function{};if(!lancermain_asset::identify(r,call.r.function,function))return false;
 out={function,method,call.r.size,hash.value};probe.gate=0;return true;
}
bool active_name(uptr weapon,Name& out) noexcept {
 const auto state=playground::field(get<uptr>(weapon+0x50),L"Active",L"State");
 const auto owner=state?get<uptr>(state+0x40):0,package=owner?get<uptr>(owner+0x40):0;
 if(!registered(state)||!registered(owner)||!registered(package)||!playground::named(owner,L"WarWeapon")||
  !playground::named(package,L"WarfareGame")||get<uptr>(package+0x40)){
   static unsigned rows{};if(log_file&&rows++<2){
    wchar_t state_name[128]{},owner_name[128]{},package_name[128]{};
    if(registered(state))name(state,state_name,128);if(registered(owner))name(owner,owner_name,128);
    if(registered(package))name(package,package_name,128);
    fwprintf(log_file,L"torquebow_active_name_rejected state=%llx name=%ls owner=%ls package=%ls observation_only=1\n",state,state_name,owner_name,package_name);fflush(log_file);
   }return false;
  }
 out=get<Name>(state+0x48);return !out.number;
}
class Owner {
 hand::Frame source_{};torquebow_cleanup::Anchor cleanup_{};std::uint64_t serial_{};
 Identity identity_{};gears_weapon::TorqueBowOriginalCancel recipe_;
 std::array<Entry,4> entries_{};
 bool(*owns_)(hand::Frame const&,std::uint64_t) noexcept{};
 bool(*stop_local_)(std::uint64_t) noexcept{};
 bool injection_fenced_{},projectile_accounted_{};unsigned end_state_returns_{};std::uint64_t last_end_state_call_{};
 Result off_field(bool input) noexcept {
  if(recipe_.phase()!=(input?Phase::stop_injection:Phase::clear_end))return Result::rejected;
  const auto object=input?source_.controller.address:source_.weapon.address;
  // Fixed typed selectors only. Preserve neighbouring packed bits, and
  // distinguish a pre-write rejection from an unknown attempted write.
  meta::Snapshot schema{};schema.module_base=base;meta::Reader r{schema,meta::Stage::final_check};hand::ObjectIdentity object_identity{};
  weaponhand::BoolField field{};
  if(!meta::tables(base,schema.objects,schema.names)||!lancermain_asset::identify(r,object,object_identity))return Result::rejected;
  const bool found=input?weaponhand::boolean(r,object,L"bHoldingButton_R2",field):weaponhand::boolean(r,object,L"bPendingEndFire",field);
  if(!found||schema.field_count==schema.fields.size()||
     !(input?r.named(field.owner,L"WarPC"):r.named(field.owner,L"Weap_Bow_base")))return Result::rejected;
  schema.fields[schema.field_count++]=field.property;unsigned checked_bits{};
  if(!current(identity_,serial_)||!unified_weapon_fire::retained_metadata(r)||
     !r.at(object,field.property.offset,checked_bits)||checked_bits!=field.bits)return Result::rejected;
  const auto wanted=field.bits&~field.mask;if(wanted==field.bits)return Result::accepted;
  if((object+field.property.offset)&3)return Result::rejected;
  // Reuse the existing compare/exchange primitive. A neighbouring packed bit
  // changing after the checked read must reject a store, never be overwritten.
  bool exchanged{};
  if(!unified_weapon_fire::compare_word(object+field.property.offset,field.bits,wanted,exchanged))return Result::unknown;
  if(!exchanged)return Result::rejected;
  weaponhand::BoolField after{};
  const bool read=input?weaponhand::boolean(r,object,L"bHoldingButton_R2",after):weaponhand::boolean(r,object,L"bPendingEndFire",after);
  return read&&meta::same_property(field.property,after.property)&&field.mask==after.mask&&after.bits==wanted&&current(identity_,serial_)?Result::accepted:Result::unknown;
 }
 Result original(Method method) noexcept {
  const auto expected=method==Method::clear_pending?Phase::clear_pending:method==Method::charge_ended?Phase::charge_ended:Phase::leave_charge;
  if(recipe_.phase()!=expected||!current(identity_,serial_))return Result::rejected;
  const auto& saved=entries_[unsigned(method)];Entry now{};
  if(!bind_entry(source_.weapon.address,method,now)||!hand::same(saved.function,now.function)||
    saved.fingerprint!=now.fingerprint||saved.bytes!=now.bytes)return Result::rejected;
  playground::Args call(source_.weapon.address,label(method));
  if(!call.valid||call.r.function!=saved.function.address||call.r.size!=saved.bytes)return Result::rejected;
  if(method==Method::clear_pending&&!call.set(L"FireMode",int{0}).valid)return Result::rejected;
  if(method==Method::active){Name active{};if(!active_name(source_.weapon.address,active)||!call.set(L"NewState",active).valid)return Result::rejected;}
  if(!current(identity_,serial_))return Result::rejected;
  // recipe_ has already journalled this step before synchronous invocation.
  if(!call.run())return Result::unknown;
  // PutDownWeapon is void. Its normal return is only a step result; the
  // existing recipe separately checks the stopped state and EndState return.
  return Result::accepted;
 }
public:
 // owns authenticates the journal serial until final retirement; stop_local
 // fences further injection without relinquishing that cleanup journal.
 bool begin(hand::Frame const& source,torquebow_cleanup::Anchor const& cleanup,std::uint64_t serial,
  bool(*owns)(hand::Frame const&,std::uint64_t) noexcept,bool(*stop_local)(std::uint64_t) noexcept,Route route,
  bool projectile_accounted=false) noexcept {
  if(serial_||!serial||!owns||!stop_local||!owns(source,serial)||
   !cleanup.current(source,serial,owns))return false;
  torquebow_state::Observed actual{};
  if(!torquebow_state::read_owned(source,actual,cleanup,serial,owns)||(actual.fire&&!projectile_accounted))return false;
  std::array<Entry,4> entries{};
  for(auto method:{Method::clear_pending,Method::charge_ended,Method::active,Method::put_down})
   if(!bind_entry(source.weapon.address,method,entries[unsigned(method)]))return false;
  source_=source;cleanup_=cleanup;serial_=serial;owns_=owns;stop_local_=stop_local;entries_=entries;
  projectile_accounted_=projectile_accounted;
  identity_={source.world.address,source.pawn.address,source.controller.address,source.weapon.address,source.key.world_generation,
   source.controllers.session,source.controllers.space_epoch,source.controllers.input_epoch};
  gears_weapon::TorqueBowCancelSnapshot snapshot{};if(!read(identity_,snapshot)||!recipe_.begin(serial_,snapshot,route)){
   source_={};cleanup_={};serial_=0;owns_=nullptr;stop_local_=nullptr;projectile_accounted_=false;return false;}
  return true;
 }
 bool current(Identity const& id,std::uint64_t serial) noexcept {
  return serial_&&serial==serial_&&id==identity_&&owns_&&owns_(source_,serial_)&&nativeequipment::restoration_quiet()&&
   cleanup_.current(source_,serial_,owns_)&&torquebow_state::actors(source_);
 }
 bool read(Identity const& id,gears_weapon::TorqueBowCancelSnapshot& out) noexcept {
  if(!(id==identity_)||!current(id,serial_))return false;
  torquebow_state::Observed actual{};if(!torquebow_state::read_owned(source_,actual,cleanup_,serial_,owns_))return false;
  out={id,{1,unsigned(actual.inventory.capacity-actual.inventory.used),unsigned(actual.inventory.reserve)},actual.charge,actual.fire,
   actual.pending_fire,actual.pending_end,injection_fenced_&&!actual.input_down&&!actual.fire_requested,
   actual.stopped()&&end_state_returns_==(recipe_.began_in_charge()?1u:0u),true,projectile_accounted_};return true;
 }
 Result stop_injection(Identity const& id,std::uint64_t serial) noexcept {
  if(recipe_.phase()!=Phase::stop_injection||!current(id,serial)||!stop_local_||!stop_local_(serial))return Result::rejected;
  injection_fenced_=true;
  const auto stopped=torquebow_controller_fire::clear_owned(source_.controller,[&]() noexcept {
   return recipe_.phase()==Phase::stop_injection&&current(id,serial);
  });
  if(stopped!=Result::accepted)return stopped;
  return off_field(true);
 }
 Result clear_owned_pending_fire(Identity const& id,std::uint64_t serial) noexcept {return current(id,serial)?original(Method::clear_pending):Result::rejected;}
 Result clear_owned_pending_end(Identity const& id,std::uint64_t serial) noexcept {
  return current(id,serial)?off_field(false):Result::rejected;
 }
 Result charge_ended(Identity const& id,std::uint64_t serial) noexcept {return current(id,serial)?original(Method::charge_ended):Result::rejected;}
 Result leave_charge(Identity const& id,std::uint64_t serial,Route route) noexcept {return current(id,serial)?original(route==Route::active?Method::active:Method::put_down):Result::rejected;}
 // Sole existing Call subscriber supplies the successful ORIGINAL Charge.EndState
 // normal return. This is observation only; never manually call EndState here.
 void end_state_return(uptr object,uptr function,std::uint64_t call_id) noexcept {
  if(!serial_||!call_id||call_id==last_end_state_call_||object!=source_.weapon.address||
   !current(identity_,serial_)||!playground::named(function,L"EndState"))return;
  const auto state=get<uptr>(function+0x40),owner=state?get<uptr>(state+0x40):0,package=owner?get<uptr>(owner+0x40):0;
  if(registered(function)&&registered(state)&&registered(owner)&&registered(package)&&
   playground::named(state,L"Charge")&&playground::named(get<uptr>(state+0x50),L"State")&&
   playground::named(owner,L"Weap_Bow_base")&&playground::named(package,L"WarfareGame")&&!get<uptr>(package+0x40)){
   last_end_state_call_=call_id;if(end_state_returns_<2)++end_state_returns_;
  }
 }
 Result reconcile_applied(Identity const& id,std::uint64_t serial,Phase step) noexcept {
  if(!current(id,serial))return Result::unknown;
  torquebow_state::Observed s{};if(!torquebow_state::read_owned(source_,s,cleanup_,serial_,owns_))return Result::unknown;
  bool applied=false;
  switch(step){
   case Phase::stop_injection:applied=injection_fenced_&&!s.input_down&&!s.fire_requested;break;
   case Phase::clear_pending:applied=!s.pending_fire;break;
   case Phase::clear_end:applied=!s.pending_end;break;
   case Phase::charge_ended:applied=!s.tension_timer&&!s.aim_audio;break;
   case Phase::leave_charge:applied=!s.charge&&!s.fire&&end_state_returns_==1;break;
   default:break;
  }
  return applied?Result::accepted:Result::unknown;
 }
 bool drain() noexcept {return recipe_.phase()==Phase::uncertain?recipe_.reconcile_on_gt(*this):recipe_.drain_on_gt(*this);}
 bool retire_completed(std::uint64_t serial) noexcept {
  if(serial!=serial_||recipe_.phase()!=Phase::complete)return false;
  *this=Owner{};return true;
 }
 bool pending() const noexcept {return serial_&&recipe_.pending();}
 bool began_in_charge() const noexcept {return recipe_.began_in_charge();}
 Phase phase() const noexcept {return recipe_.phase();}
};
}

