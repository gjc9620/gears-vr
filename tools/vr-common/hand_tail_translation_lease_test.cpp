#include "hand_tail_translation_lease.h"
#include "hand_ik_tail_lease.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <type_traits>
#include <vector>

namespace {
namespace tail=gears_hand_tail_translation;
using tail::Field;
using tail::FieldValue;
using tail::Lease;
using tail::Strength;
using tail::Value;
constexpr unsigned all_fields=15u;
constexpr std::uint32_t flag_mask=0x24u;
unsigned case_count{},check_count{},scenario{};

void require(bool passed,const char* label){
    ++check_count;
    if(!passed){
        std::fprintf(stderr,"FAIL case=%u scenario=%u %s\n",case_count,scenario,label);
        std::exit(1);
    }
}
void begin_case(const char* label){
    ++case_count;scenario=0;
    std::printf("CASE %u %s\n",case_count,label);
}
float float_bits(std::uint32_t bits){float result{};std::memcpy(&result,&bits,sizeof(result));return result;}
bool same_strength(Strength const& a,Strength const& b){
    return !std::memcmp(a.data(),b.data(),sizeof(float)*a.size());
}
Value value_for(unsigned hand_index,bool desired){
    Value result{};
    const unsigned seed=(desired?91u:11u)+hand_index*41u;
    for(unsigned byte_index=0;byte_index<result.translation.size();++byte_index)
        result.translation[byte_index]=static_cast<unsigned char>(seed+byte_index);
    for(unsigned byte_index=0;byte_index<result.space_bone.size();++byte_index)
        result.space_bone[byte_index]=static_cast<unsigned char>(seed+31u+byte_index);
    result.space=static_cast<std::uint8_t>((desired?3u:1u)+hand_index);
    result.flags=(hand_index==0)==desired?0x20u:0x04u;
    return result;
}
std::array<Value,2> desired_values(){return {{value_for(0,true),value_for(1,true)}};}

// This is a bounded memory service, not a native/property backend. Its final
// compare-write repeats identity/schema, exact +0 and expected-field checks.
// flags exposes only flag_mask; every write preserves the LIVE unrelated bits.
// Failure modes deliberately include transport-contract violations/torn writes.
struct MemoryBackend {
    enum class Mode {normal,false_before,false_after,torn};
    std::array<Value,2> values{{value_for(0,false),value_for(1,false)}};
    std::array<Strength,2> strengths{};
    std::array<std::uint32_t,2> words{};
    std::array<bool,2> identity_known{{true,true}},schema_known{{true,true}},unknown_next{};
    unsigned reads{},writes{},mutations{},read_mask{},write_mask{},first_mutation_reads{};
    unsigned fail_read_call{},fail_write_call{},post_read_fail_write{},unknown_hand{2};
    Mode failure{Mode::normal};
    std::vector<Field> field_log;
    std::function<void(unsigned)> on_read,on_write;

    MemoryBackend(){for(unsigned hand_index=0;hand_index<2;++hand_index)words[hand_index]=0xa5a50001u|values[hand_index].flags;}
    Value current(unsigned hand_index)const{
        auto result=values[hand_index];result.flags=words[hand_index]&flag_mask;return result;
    }
    void external(unsigned hand_index,Value const& changed){
        values[hand_index]=changed;
        words[hand_index]=(words[hand_index]&~flag_mask)|(changed.flags&flag_mask);
    }
    bool original(unsigned hand_index)const{return tail::exact(current(hand_index),value_for(hand_index,false));}
    bool read(unsigned hand_index,Value& result,Strength& strength){
        ++reads;
        if(hand_index>=2)return false;
        read_mask|=1u<<hand_index;
        if(on_read)on_read(reads);
        result=current(hand_index);strength=strengths[hand_index];
        if(!identity_known[hand_index]||!schema_known[hand_index]||reads==fail_read_call||
           hand_index==unknown_hand||unknown_next[hand_index]){
            unknown_next[hand_index]=false;return false; // Plausible bytes still mean unknown.
        }
        return true;
    }
    bool write_field(unsigned hand_index,Field field,FieldValue const& expected,FieldValue const& wanted){
        ++writes;
        if(hand_index>=2||tail::field_size(field)==0)return false;
        write_mask|=1u<<hand_index;field_log.push_back(field);
        if(on_write)on_write(writes);
        if(!identity_known[hand_index]||!schema_known[hand_index]||!tail::exact_zero(strengths[hand_index])||
           tail::field_value(current(hand_index),field)!=expected)return false;
        if(field==Field::flags){
            std::uint32_t wanted_flags{};std::memcpy(&wanted_flags,wanted.data(),sizeof(wanted_flags));
            if(wanted_flags&~flag_mask)return false;
        }
        const auto mode=writes==fail_write_call?failure:Mode::normal;
        if(mode==Mode::false_before)return false;
        auto written=wanted;
        if(mode==Mode::torn){
            written=expected;
            if(field==Field::space)written[0]^=0x40u;
            else if(field==Field::flags){const std::uint32_t neither{};std::memcpy(written.data(),&neither,sizeof(neither));}
            else for(std::size_t byte_index=0;byte_index<tail::field_size(field)/2;++byte_index)written[byte_index]=wanted[byte_index];
        }
        if(!mutations)first_mutation_reads=read_mask;
        auto updated=current(hand_index);tail::assign_field(updated,field,written);external(hand_index,updated);++mutations;
        if(writes==post_read_fail_write)unknown_next[hand_index]=true;
        return mode!=Mode::false_after&&mode!=Mode::torn;
    }
};

static_assert(!std::is_copy_constructible<Lease>::value&&!std::is_copy_assignable<Lease>::value);
static_assert(!std::is_move_constructible<Lease>::value&&!std::is_move_assignable<Lease>::value);

void selected_masks(){
    begin_case("mask1_2_3_capture_all_originals_before_write_unselected_untouched");
    for(unsigned mask=1;mask<=3;++mask){
        scenario=mask;MemoryBackend backend;Lease lease;const auto wanted=desired_values();
        for(unsigned hand_index=0;hand_index<2;++hand_index)if(!(mask&(1u<<hand_index))){
            backend.schema_known[hand_index]=false;backend.strengths[hand_index][0]=float_bits(0x7fc00001u);
        }
        const auto initial_words=backend.words;const auto initial_strengths=backend.strengths;
        require(lease.capture(mask,wanted,backend),"capture selected originals");
        require(!backend.writes&&!lease.dirty()&&lease.selected_mask()==mask,"capture never writes/publishes pending");
        for(unsigned hand_index=0;hand_index<2;++hand_index){
            if(mask&(1u<<hand_index)){
                require(lease.original(hand_index)&&tail::exact(*lease.original(hand_index),value_for(hand_index,false)),"exact original tuple");
                require(tail::exact(*lease.owned(hand_index),wanted[hand_index])&&lease.selected_fields(hand_index)==all_fields,"desired snapshot and selected fields");
            }else require(!lease.original(hand_index)&&!lease.owned(hand_index)&&!lease.selected_fields(hand_index),"unselected journal absent");
        }
        backend.on_write=[&](unsigned call){if(call==1)require(backend.read_mask==mask,"every selected hand captured before first write");};
        require(lease.prepare(backend)&&lease.prepared()&&lease.dirty()&&!lease.faulted(),"prepare selected tuples");
        require(backend.first_mutation_reads==mask&&backend.write_mask==mask&&backend.read_mask==mask,"no unselected backend calls");
        for(unsigned hand_index=0;hand_index<2;++hand_index){
            require(tail::exact(backend.current(hand_index),(mask&(1u<<hand_index))?wanted[hand_index]:value_for(hand_index,false)),"selected wanted/unselected original");
            require(same_strength(backend.strengths[hand_index],initial_strengths[hand_index]),"extras never change strength");
        }
        backend.on_write={};const auto log_before=backend.field_log.size();
        require(lease.restore_zero(backend)&&!lease.dirty()&&!lease.prepared()&&!lease.faulted(),"restore selected zero-strength tuples");
        for(unsigned hand_index=0;hand_index<2;++hand_index)require(backend.original(hand_index)&&backend.words[hand_index]==initial_words[hand_index],"exact original plus unrelated word");
        require(backend.field_log[log_before]==Field::flags,"reverse restore starts at flags");
        const auto writes_before=backend.writes;require(lease.restore_zero(backend)&&backend.writes==writes_before,"repeat clean restore is inert");
        require(lease.reset()&&!lease.selected_mask()&&!lease.original(0),"explicit clean reset");
    }
}
void unchanged_fields(){
    begin_case("unchanged_and_partially_selected_fields");
    MemoryBackend backend;Lease lease;std::array<Value,2> wanted{{backend.current(0),backend.current(1)}};
    require(lease.prepare(3,wanted,backend)&&lease.prepared()&&!lease.dirty()&&!backend.writes,"equal tuples need no writes");
    require(!lease.selected_fields(0)&&!lease.selected_fields(1)&&lease.restore_zero(backend),"zero changed-field masks");
    require(lease.reset(),"reset equal tuples");wanted[1].space=9;
    require(lease.prepare(2,wanted,backend)&&lease.selected_fields(1)==2u&&backend.writes==1,"only changed field selected");
    require(lease.restore_zero(backend)&&backend.writes==2&&backend.original(1),"only changed field restored");
}
void invalid_and_missing_capture(){
    begin_case("invalid_masks_and_prepare_without_capture");
    for(unsigned mask:std::array<unsigned,3>{{0u,4u,7u}}){
        scenario=mask;MemoryBackend backend;Lease lease;
        require(!lease.capture(mask,desired_values(),backend)&&lease.faulted()&&!lease.selected_mask(),"invalid mask fails closed");
        require(!backend.reads&&!backend.writes&&!lease.dirty(),"invalid mask no backend calls");
    }
    MemoryBackend backend;Lease lease;require(!lease.prepare(backend)&&lease.faulted()&&!backend.reads,"prepare requires capture");
}
void duplicate_and_pending_reset(){
    begin_case("duplicate_capture_prepare_and_pending_reset");
    MemoryBackend backend;Lease lease;const auto wanted=desired_values();
    require(lease.capture(1,wanted,backend),"capture once");const auto initial_reads=backend.reads;
    require(!lease.capture(2,wanted,backend)&&lease.faulted()&&backend.reads==initial_reads,"duplicate capture cannot replace snapshot");
    require(lease.selected_mask()==1&&tail::exact(*lease.original(0),value_for(0,false)),"old capture retained");
    require(lease.reset()&&lease.prepare(1,wanted,backend),"explicit clean reset then valid transaction");
    const auto writes_before=backend.writes;
    require(!lease.prepare(backend)&&!lease.reset()&&lease.dirty()&&backend.writes==writes_before,"duplicate prepare/reset cannot discard pending");
    require(lease.restore_zero(backend)&&lease.faulted()&&!lease.dirty(),"faulted known transaction still restores");
}
void exact_zero_capture(){
    begin_case("capture_requires_bit_exact_positive_zero_not_negative_zero_NaN_or_nonzero");
    const std::array<std::uint32_t,3> bad_bits{{0x80000000u,0x7fc00001u,0x3f800000u}};
    for(unsigned component_index=0;component_index<3;++component_index)for(unsigned bad_index=0;bad_index<bad_bits.size();++bad_index){
        scenario=component_index*3u+bad_index;MemoryBackend backend;Lease lease;
        backend.strengths[0][component_index]=float_bits(bad_bits[bad_index]);
        require(!lease.capture(1,desired_values(),backend)&&lease.faulted(),"non +0 snapshot rejected");
        require(!lease.selected_mask()&&!lease.dirty()&&!backend.writes,"invalid snapshot unpublished");
    }
}
void second_hand_capture_failure(){
    begin_case("second_selected_hand_capture_failure_publishes_nothing");
    MemoryBackend backend;Lease lease;backend.fail_read_call=2;
    require(!lease.prepare(3,desired_values(),backend)&&lease.faulted(),"second capture unknown");
    require(backend.read_mask==3&&!backend.writes&&!lease.selected_mask()&&!lease.dirty(),"first temporary snapshot never published/written");
    require(!lease.original(0)&&!lease.original(1),"no partial captured authority");
}
void field_transport_failure(MemoryBackend::Mode failure,const char* label){
    begin_case(label);
    for(unsigned field_index=0;field_index<4;++field_index){
        scenario=field_index;MemoryBackend backend;Lease lease;backend.fail_write_call=field_index+1;backend.failure=failure;
        require(!lease.prepare(1,desired_values(),backend)&&lease.faulted()&&!lease.prepared(),"false transport stops prepare");
        require(lease.pending_fields(0)==(1u<<(field_index+1u))-1u&&lease.dirty(),"pending recorded before false helper");
        require(!lease.revoked(0),"known old/new transport result remains eligible for cleanup");
        require(lease.restore_zero(backend)&&backend.original(0)&&!lease.dirty()&&lease.faulted(),"known partial cleanup is exact with sticky fault");
    }
}
void unknown_postreads(){
    begin_case("each_field_unknown_postread_permanently_revokes_pending_hand");
    for(unsigned field_index=0;field_index<4;++field_index){
        scenario=field_index;MemoryBackend backend;Lease lease;backend.post_read_fail_write=field_index+1;
        require(!lease.prepare(1,desired_values(),backend)&&lease.dirty()&&lease.faulted()&&lease.revoked(0),"false read is unknown even with populated bytes");
        const auto writes_before=backend.writes;
        require(!lease.restore_zero(backend)&&backend.writes==writes_before&&!lease.reset(),"unknown journal never force-overwritten/reset");
        backend.external(0,value_for(0,false));
        require(!lease.restore_zero(backend)&&lease.revoked(0)&&lease.dirty()&&backend.writes==writes_before,"later original return cannot resurrect revoked ownership");
    }
}
void unknown_before_write(){
    begin_case("unknown_prepare_observation_without_mutation");
    MemoryBackend backend;Lease lease;require(lease.capture(1,desired_values(),backend),"capture known original");
    backend.fail_read_call=backend.reads+1;
    require(!lease.prepare(backend)&&lease.faulted()&&lease.revoked(0)&&!backend.writes,"first live observation unknown");
    require(!lease.dirty()&&!lease.accepts(0,backend.current(0)),"unknown has no fabricated pending mutation or accepts pass");
    require(lease.restore_zero(backend)&&lease.faulted(),"cleanliness does not erase unknown fault");
}
void compare_write_service_guards(){
    begin_case("memory_backend_rechecks_zero_expected_field_and_identity_at_compare_write");
    for(unsigned variant=0;variant<3;++variant){
        scenario=variant;MemoryBackend backend;Lease lease;
        backend.on_write=[&](unsigned call){if(call==1){
            if(variant==0)backend.strengths[0][0]=1.f;
            if(variant==1){auto foreign=backend.current(0);foreign.translation[0]^=0x80u;backend.external(0,foreign);}
            if(variant==2)backend.identity_known[0]=false;
        }};
        require(!lease.prepare(1,desired_values(),backend)&&lease.faulted()&&!backend.mutations,"late service guard refuses mutation");
        if(variant==0){
            require(!lease.revoked(0)&&lease.dirty(),"known nonzero strength is fault, not unknown ownership");
            backend.on_write={};backend.strengths[0]={};
            require(lease.restore_zero(backend)&&lease.faulted(),"fresh +0 may discharge known original tuple");
        }else require(lease.revoked(0)&&!lease.restore_zero(backend),"foreign/unknown compare gap is permanently revoked");
    }
}
void nonzero_restore_then_fresh_zero(){
    begin_case("nonzero_restore_never_writes_or_clears_pending_then_fresh_zero_cleans_faulted");
    for(unsigned variant=0;variant<2;++variant){
        scenario=variant;MemoryBackend backend;Lease lease;
        if(variant){backend.fail_write_call=1;backend.failure=MemoryBackend::Mode::false_before;}
        const bool prepared=lease.prepare(1,desired_values(),backend);
        require(prepared==!variant&&lease.dirty(),"seed known pending tuple");
        backend.strengths[0][1]=.5f;const auto pending_before=lease.pending_fields(0),writes_before=backend.writes;
        require(!lease.restore_zero(backend)&&lease.pending_fields(0)==pending_before&&backend.writes==writes_before,"even original-looking fields cannot clear with nonzero strength");
        require(lease.faulted()&&!lease.revoked(0),"known nonzero remains sticky but restorable");const auto reads_after_refusal=backend.reads;backend.strengths[0]={};
        require(lease.restore_zero(backend)&&backend.reads>reads_after_refusal&&backend.original(0)&&!lease.dirty()&&lease.faulted(),"explicit later read proves fresh zero and cleans, never cached clearance");
    }
}
void false_after_restore(){
    begin_case("false_after_success_restoration_is_clean_but_faulted_each_field");
    for(unsigned field_index=0;field_index<4;++field_index){
        scenario=field_index;MemoryBackend backend;Lease lease;require(lease.prepare(1,desired_values(),backend),"seed prepared tuple");
        backend.fail_write_call=backend.writes+(4u-field_index);backend.failure=MemoryBackend::Mode::false_after;
        require(lease.restore_zero(backend)&&backend.original(0)&&!lease.dirty()&&lease.faulted(),"readback clears exact restoration despite sticky helper false");
        require(!lease.revoked(0),"successful known restore is not foreign");
    }
}
void false_before_restore(){
    begin_case("known_false_before_restore_retains_pending_and_later_explicit_cleanup");
    MemoryBackend backend;Lease lease;require(lease.prepare(1,desired_values(),backend),"seed prepared tuple");
    backend.fail_write_call=backend.writes+1;backend.failure=MemoryBackend::Mode::false_before;
    require(!lease.restore_zero(backend)&&lease.faulted()&&!lease.revoked(0)&&lease.pending_fields(0)==8u,"failed flags restore retains known staged flag obligation");
    require(lease.restore_zero(backend)&&backend.original(0)&&!lease.dirty()&&lease.faulted(),"explicit next zero observation completes only known pending field");
}
void foreign_tuple_return(){
    begin_case("whole_tuple_foreign_and_mixed_old_new_revoke_even_after_return");
    for(unsigned variant=0;variant<2;++variant){
        scenario=variant;MemoryBackend backend;Lease lease;const auto wanted=desired_values();require(lease.prepare(1,wanted,backend),"seed full owned tuple");
        auto foreign=backend.current(0);if(variant)foreign.translation=value_for(0,false).translation;else foreign.space=0xee;
        backend.external(0,foreign);const auto writes_before=backend.writes;
        require(!lease.accepts(0,foreign),"pure full-tuple predicate rejects mixed/foreign");
        require(!lease.restore_zero(backend)&&lease.revoked(0)&&lease.faulted()&&backend.writes==writes_before,"observed foreign permanently withdraws this journal");
        backend.external(0,wanted[0]);require(!lease.accepts(0,wanted[0])&&!lease.restore_zero(backend),"return to owned cannot recover revoked journal");
        backend.external(0,value_for(0,false));require(!lease.restore_zero(backend)&&lease.dirty()&&!lease.reset()&&backend.writes==writes_before,"return to original cannot force-clear revoked dirty");
    }
}
void torn_fields(){
    begin_case("partial_or_torn_each_field_is_unknown_dirty_without_force_overwrite");
    for(unsigned field_index=0;field_index<4;++field_index){
        scenario=field_index;MemoryBackend backend;Lease lease;backend.fail_write_call=field_index+1;backend.failure=MemoryBackend::Mode::torn;
        require(!lease.prepare(1,desired_values(),backend)&&lease.revoked(0)&&lease.dirty()&&lease.faulted(),"torn value is neither complete staged nor alternate tuple");
        const auto writes_before=backend.writes;require(!lease.restore_zero(backend)&&backend.writes==writes_before,"no whole-field overwrite of unknown partial bytes");
    }
}
void other_hand_cleanup(){
    begin_case("other_hand_cleanup_still_attempted_after_unknown_foreign_or_nonzero_first_hand");
    for(unsigned variant=0;variant<3;++variant){
        scenario=variant;MemoryBackend backend;Lease lease;require(lease.prepare(3,desired_values(),backend),"seed two hand journals");
        if(variant==0)backend.unknown_hand=0;
        if(variant==1){auto foreign=backend.current(0);foreign.space=0xee;backend.external(0,foreign);}
        if(variant==2)backend.strengths[0][0]=1.f;
        const auto writes_before=backend.writes;
        require(!lease.restore_zero(backend)&&backend.original(1)&&!lease.pending_fields(1)&&lease.pending_fields(0),"first failure never skips second-hand exact cleanup");
        require(backend.writes==writes_before+4u&&lease.faulted(),"only healthy second hand receives its four restore fields");
        if(variant==2){backend.strengths[0]={};require(lease.restore_zero(backend)&&backend.original(0)&&lease.faulted(),"known nonzero first hand may later explicitly clean");}
        else require(lease.revoked(0)&&!lease.restore_zero(backend),"unknown/foreign first hand never recovers");
    }
}
void frozen_wanted_callbacks(){
    begin_case("wanted_for_both_hands_frozen_before_read_and_write_callbacks");
    MemoryBackend backend;Lease lease;auto wanted=desired_values();const auto frozen=wanted;
    backend.on_read=[&](unsigned call){if(call==1){wanted[0]={};wanted[1]={};}};
    require(lease.capture(3,wanted,backend)&&!backend.writes,"capture snapshots caller inputs before first callback");
    require(tail::exact(*lease.owned(0),frozen[0])&&tail::exact(*lease.owned(1),frozen[1]),"both desired tuples use entry copy");
    backend.on_read={};backend.on_write=[&](unsigned){wanted[0]=value_for(1,false);wanted[1]=value_for(0,false);};
    require(lease.prepare(backend)&&tail::exact(backend.current(0),frozen[0])&&tail::exact(backend.current(1),frozen[1]),"later caller mutation cannot change prepared desired bytes");
    backend.on_write={};require(lease.restore_zero(backend)&&backend.original(0)&&backend.original(1),"frozen-input transaction restores originals");
}
void reentrant_journals(){
    begin_case("reentrant_capture_prepare_restore_and_reset_cannot_discard_journals");
    {
        MemoryBackend backend;Lease lease;const auto wanted=desired_values();
        backend.on_read=[&](unsigned call){if(call==1){require(!lease.reset(),"busy capture reset refused");require(!lease.capture(3,wanted,backend),"recursive capture refused");}};
        require(!lease.capture(3,wanted,backend)&&lease.faulted()&&!lease.selected_mask()&&!backend.writes,"nested capture fault prevents partial publication");
    }
    {
        MemoryBackend backend;Lease lease;const auto wanted=desired_values();
        backend.on_write=[&](unsigned call){if(call==1){require(lease.pending_fields(0)==1u&&!lease.reset(),"pending exists before callback and busy reset refused");require(!lease.prepare(backend),"recursive prepare refused");}};
        require(!lease.prepare(1,wanted,backend)&&lease.dirty()&&lease.faulted()&&lease.pending_fields(0)==1u,"nested prepare preserves original pending journal");
        require(tail::exact(*lease.original(0),value_for(0,false)),"callback cannot replace captured original");backend.on_write={};
        require(lease.restore_zero(backend)&&backend.original(0)&&lease.faulted(),"outer pending remains safely restorable");
    }
    {
        MemoryBackend backend;Lease lease;require(lease.prepare(1,desired_values(),backend),"seed restore callback journal");const auto next_write=backend.writes+1;
        backend.on_write=[&](unsigned call){if(call==next_write){require(!lease.reset()&&lease.pending_fields(0)==all_fields,"busy restore reset cannot drop pending");require(!lease.restore_zero(backend),"recursive restore refused");}};
        require(lease.restore_zero(backend)&&backend.original(0)&&!lease.dirty()&&lease.faulted(),"outer restore completes exact bytes while reentry fault stays");
    }
}
void masked_flags_service(){
    begin_case("memory_masked_flags_preserve_live_unrelated_bits_and_reject_invalid_bits");
    MemoryBackend backend;Lease lease;const auto wanted=desired_values();
    require(lease.capture(1,wanted,backend),"capture selected flag bits");backend.words[0]^=0x40000200u;
    const auto unrelated=backend.words[0]&~flag_mask;
    require(lease.prepare(backend)&&(backend.words[0]&~flag_mask)==unrelated,"masked write preserves newly changed unrelated word bits");
    require(lease.restore_zero(backend)&&(backend.words[0]&~flag_mask)==unrelated&&backend.original(0),"restore owns only selected bool bits");
    MemoryBackend invalid_backend;Lease invalid_lease;auto invalid_wanted=desired_values();invalid_wanted[0].flags|=0x80u;
    const auto initial_unrelated=invalid_backend.words[0]&~flag_mask;
    require(!invalid_lease.prepare(1,invalid_wanted,invalid_backend)&&invalid_lease.faulted(),"backend rejects bits outside declared validated mask");
    require((invalid_backend.words[0]&~flag_mask)==initial_unrelated&&invalid_lease.restore_zero(invalid_backend)&&invalid_backend.original(0),"invalid selected flags never clobber unrelated bits and known prior fields clean");
}

namespace primary=gears_hand_ik_stream;
using Composite=gears_hand_ik_tail::Lease;
primary::Target primary_target(unsigned seed){
    primary::Target result{};
    for(unsigned byte_index=0;byte_index<result.size();++byte_index)result[byte_index]=static_cast<unsigned char>(seed+byte_index);
    return result;
}
std::array<primary::Target,4> primary_targets(){
    return {{primary_target(71),primary_target(91),primary_target(111),primary_target(131)}};
}
unsigned mapped_hands(unsigned channels){return channels==3?1u:channels==12?2u:channels==15?3u:0u;}
struct CombinedBackend {
    enum class EventKind {primary_read,primary_target,primary_strength,tail_read,tail_write};
    struct Event {EventKind kind;unsigned index;bool restoring;};
    Composite& owner;
    MemoryBackend extras;
    std::array<primary::Value,4> values{};
    std::array<bool,4> known{{true,true,true,true}};
    std::array<primary::Target,4> frozen_seen{};
    std::vector<Event> events;
    unsigned primary_reads{},target_writes{},strength_writes{},primary_mutations{},primary_read_mask{},primary_write_mask{};
    unsigned wanted_calls{},forward_checks{},deny_forward_call{},fail_target_call{},fail_strength_call{};
    bool closed{},wanted_known{true},fail_target_after{},fail_strength_after{};
    bool check_original_before_tail{},checked_tail_order{};
    std::function<void(unsigned,unsigned)> before_target;
    std::function<void(unsigned,unsigned,float)> after_strength;
    std::function<void()> wanted_callback;

    explicit CombinedBackend(Composite& value):owner(value){
        for(unsigned channel=0;channel<4;++channel)values[channel].target=primary_target(11u+channel*13u);
    }
    bool primary_original(unsigned channel)const{
        return primary::exact(values[channel].target,primary_target(11u+channel*13u))&&primary::exact(values[channel].strength,primary::Strength{});
    }
    bool all_original()const{
        for(unsigned channel=0;channel<4;++channel)if(!primary_original(channel))return false;
        return extras.original(0)&&extras.original(1);
    }
    // Explicit adapter policy. Aggregate fault is never a forward bypass.
    // Both rollback bypasses still keep live known/expected-byte checks below.
    bool primary_write_allowed()const{
        return owner.restoring()||owner.primary_faulted()||(!owner.faulted()&&!closed);
    }
    bool read(unsigned channel,primary::Value& result){
        ++primary_reads;if(channel>=4)return false;primary_read_mask|=1u<<channel;
        events.push_back({EventKind::primary_read,channel,owner.restoring()});
        if(!known[channel])return false;result=values[channel];return true;
    }
    bool write_target(unsigned channel,primary::Target const& expected,primary::Target const& wanted){
        ++target_writes;if(channel>=4)return false;
        events.push_back({EventKind::primary_target,channel,owner.restoring()});
        if(before_target)before_target(target_writes,channel);
        if(!primary_write_allowed()||!known[channel]||!primary::exact(values[channel].target,expected)||
           !primary::exact(values[channel].strength,primary::Strength{}))return false;
        if(target_writes==fail_target_call&&!fail_target_after)return false;
        values[channel].target=wanted;++primary_mutations;primary_write_mask|=1u<<channel;
        return target_writes!=fail_target_call;
    }
    bool set_strength(unsigned channel,float strength){
        ++strength_writes;if(channel>=4)return false;
        events.push_back({EventKind::primary_strength,channel,owner.restoring()});
        if(!primary_write_allowed()||!known[channel])return false;
        if(strength_writes==fail_strength_call&&!fail_strength_after)return false;
        values[channel].strength={{strength,strength,0.f}};++primary_mutations;primary_write_mask|=1u<<channel;
        if(after_strength)after_strength(strength_writes,channel,strength);
        // Models a synchronous callback after the actual setter. The composite
        // must independently reject a newly closed/faulted final operation.
        return strength_writes!=fail_strength_call;
    }
    bool tail_forward_allowed(){
        ++forward_checks;
        return forward_checks!=deny_forward_call&&!owner.faulted()&&!owner.restoring()&&!closed;
    }
    bool tail_wanted(unsigned channels,std::array<primary::Target,4> const& frozen,std::array<Value,2>& wanted){
        ++wanted_calls;frozen_seen=frozen;wanted=desired_values();
        require(mapped_hands(channels)!=0,"tail wanted receives a complete valid channel pair mask");
        if(wanted_callback)wanted_callback();return wanted_known;
    }
    bool tail_read(unsigned hand_index,Value& result,Strength& strength){
        if(hand_index>=2)return false;
        events.push_back({EventKind::tail_read,hand_index,owner.restoring()});
        extras.strengths[hand_index]=values[hand_index*2u+1u].strength;
        if(owner.restoring()&&check_original_before_tail&&!checked_tail_order){
            for(unsigned channel=0;channel<4;++channel)require(primary_original(channel),"original four-channel cleanup precedes first extra restore read");
            checked_tail_order=true;
        }
        return extras.read(hand_index,result,strength);
    }
    bool tail_write_field(unsigned hand_index,Field field,FieldValue const& expected,FieldValue const& wanted){
        if(hand_index>=2)return false;
        events.push_back({EventKind::tail_write,hand_index,owner.restoring()});
        if(!primary_write_allowed())return false;
        extras.strengths[hand_index]=values[hand_index*2u+1u].strength;
        return extras.write_field(hand_index,field,expected,wanted);
    }
};
static_assert(!std::is_copy_constructible<Composite>::value&&!std::is_move_constructible<Composite>::value);

void composite_mapping_and_order(){
    begin_case("composite_masks3_12_15_all_extras_prepared_before_primary_activation");
    for(unsigned channels:std::array<unsigned,3>{{3u,12u,15u}}){
        scenario=channels;Composite lease;CombinedBackend backend(lease);const auto targets=primary_targets();
        backend.before_target=[&](unsigned call,unsigned){if(call==1){
            require(lease.translation().prepared()&&backend.extras.read_mask==mapped_hands(channels),"all selected extra snapshots and fields finish before first primary write");
            require(backend.primary_read_mask==channels,"all selected primary originals captured before first primary write");
            const auto wanted=desired_values();
            for(unsigned hand_index=0;hand_index<2;++hand_index)if(mapped_hands(channels)&(1u<<hand_index))require(tail::exact(backend.extras.current(hand_index),wanted[hand_index]),"selected extra complete before activation");
        }};
        backend.after_strength=[&](unsigned,unsigned channel,float strength){if(strength>0){
            Value sampled{};Strength sampled_strength{};
            require(backend.tail_read(channel/2u,sampled,sampled_strength),"memory extra strength sample known");
            require(same_strength(sampled_strength,backend.values[(channel/2u)*2u+1u].strength),"tail strength maps original rotation channel1 or3, not position channel");
            if(!(channel&1u))require(tail::exact_zero(sampled_strength),"position activation alone does not fake a rotation strength");
        }};
        require(lease.acquire(channels,targets,.5f,backend)&&lease.dirty()&&!lease.faulted(),"composite actual acquire succeeds");
        require(lease.translation().selected_mask()==mapped_hands(channels)&&backend.primary_write_mask==channels,"pair mapping only");
        for(unsigned channel=0;channel<4;++channel){
            if(channels&(1u<<channel))require(primary::exact(backend.values[channel].target,targets[channel])&&primary::exact(backend.values[channel].strength,primary::Strength{{.5f,.5f,0.f}}),"selected primary exact target and strength");
            else require(backend.primary_original(channel),"unselected primary untouched");
        }
        backend.before_target={};backend.after_strength={};backend.check_original_before_tail=true;
        require(lease.restore(backend)&&backend.checked_tail_order&&backend.all_original()&&!lease.dirty()&&!lease.faulted(),"ordered complete restoration");
        require(lease.reset(),"composite explicit clean reset");
    }
}
void composite_both_cleanup_paths(){
    begin_case("composite_primary_first_extras_still_attempted_if_primary_cleanup_fails");
    for(unsigned variant=0;variant<2;++variant){
        scenario=variant;Composite lease;CombinedBackend backend(lease);require(lease.acquire(15,primary_targets(),.5f,backend),"seed composite both hands");
        if(variant==0)backend.values[0].target[0]^=0x80u;else backend.values[1].strength={{.25f,.25f,0.f}};
        const auto tail_reads_before=backend.extras.reads,tail_writes_before=backend.extras.writes;
        require(!lease.restore(backend)&&lease.dirty()&&lease.faulted(),"primary foreign retains failure");
        require(backend.extras.reads>tail_reads_before,"extras attempted despite primary false");
        if(variant==0)require(backend.extras.original(0)&&backend.extras.original(1)&&!lease.translation().dirty(),"zero rotation channels allow extra cleanup despite foreign position");
        else require(lease.translation().pending_fields(0)&&!lease.translation().pending_fields(1)&&backend.extras.original(1)&&backend.extras.writes==tail_writes_before+4u,"nonzero first rotation blocks its extras but second hand still cleans");
        require(!lease.reset(),"dirty composite cannot reset");
    }
}
void composite_primary_failure(){
    begin_case("composite_primary_write_failures_cleanup_prepared_extras");
    for(unsigned variant=0;variant<2;++variant){
        scenario=variant;Composite lease;CombinedBackend backend(lease);
        if(variant==0)backend.fail_target_call=1;else{backend.fail_strength_call=1;backend.fail_strength_after=true;}
        require(!lease.acquire(15,primary_targets(),.5f,backend)&&lease.faulted()&&lease.primary_faulted(),"actual old Lease failure propagates");
        require(backend.extras.writes==16u&&!lease.dirty()&&backend.all_original(),"prepared extras cleaned after original primary rollback reaches zero");
    }
}
void composite_extra_failure(){
    begin_case("composite_extra_failure_prevents_any_original_activation");
    Composite lease;CombinedBackend backend(lease);backend.extras.fail_write_call=2;backend.extras.failure=MemoryBackend::Mode::false_after;
    require(!lease.acquire(15,primary_targets(),.5f,backend)&&lease.faulted()&&!lease.primary_faulted(),"extra transport fault rejects composite");
    require(!backend.target_writes&&!backend.strength_writes&&!lease.dirty()&&backend.all_original(),"no activation and known partial extras restored");
}
void composite_context_denials(){
    begin_case("composite_tail_wanted_and_context_denial_before_writes_or_after_extras");
    for(unsigned variant=0;variant<4;++variant){
        scenario=variant;Composite lease;CombinedBackend backend(lease);
        if(variant==0)backend.wanted_known=false;else backend.deny_forward_call=variant;
        require(!lease.acquire(3,primary_targets(),.5f,backend)&&lease.faulted()&&!lease.dirty()&&backend.all_original(),"wanted/context refusal is not success");
        require(!backend.primary_mutations,"denial never activates original hand");
        if(variant<3)require(!backend.extras.mutations,"denial before extras writes is inert");
        else require(backend.extras.mutations==8u,"post-extra context denial performs exact extra cleanup");
    }
}
void composite_last_setter_reentry(){
    begin_case("composite_LAST_setter_nested_restore_rejected_aggregate_fault_not_rollback_bypass");
    Composite lease;CombinedBackend backend(lease);bool nested_seen{};
    backend.after_strength=[&](unsigned call,unsigned channel,float strength){if(call==4&&channel==3&&strength>0){
        nested_seen=true;require(!lease.restore(backend),"nested restore fails while composite acquire busy");
        require(lease.faulted()&&!lease.primary_faulted()&&!lease.restoring()&&!backend.primary_write_allowed(),"aggregate fault alone must not borrow primary-fault rollback bypass");
        const auto saved=backend.values[0];require(!backend.set_strength(0,0.f)&&primary::exact(backend.values[0].strength,saved.strength),"new forward mutation rejected after aggregate-only fault");
    }};
    require(!lease.acquire(15,primary_targets(),.5f,backend)&&nested_seen&&!lease.primary_faulted(),"correct old readback cannot commit nested aggregate failure");
    require(lease.faulted()&&!lease.dirty()&&backend.all_original(),"outer exit performs ordered cleanup with sticky fault");
}
void composite_last_setter_closed(){
    begin_case("composite_LAST_setter_context_closed_without_nested_call_rejects_commit");
    Composite lease;CombinedBackend backend(lease);
    backend.after_strength=[&](unsigned call,unsigned channel,float strength){if(call==4&&channel==3&&strength>0)backend.closed=true;};
    require(!lease.acquire(15,primary_targets(),.5f,backend)&&backend.closed&&lease.faulted()&&!lease.primary_faulted(),"final correct setter readback cannot bypass newly closed context");
    require(!lease.dirty()&&backend.all_original(),"restoring bypass allows exact cleanup under closed context");
}
void composite_invalid_and_frozen_inputs(){
    begin_case("composite_invalid_masks_strength_and_mutated_caller_targets_never_commit");
    for(unsigned channels:std::array<unsigned,8>{{0u,1u,2u,4u,7u,8u,14u,16u}}){
        scenario=channels;Composite lease;CombinedBackend backend(lease);
        require(!lease.acquire(channels,primary_targets(),.5f,backend)&&lease.faulted()&&!lease.dirty(),"incomplete/invalid pair mask rejected");
        require(!backend.wanted_calls&&!backend.primary_mutations&&!backend.extras.mutations,"invalid mask no preparation or writes");
    }
    for(float strength:std::array<float,5>{{0.f,-1.f,1.5f,float_bits(0x7fc00001u),float_bits(0x7f800000u)}}){
        Composite lease;CombinedBackend backend(lease);
        require(!lease.acquire(3,primary_targets(),strength,backend)&&lease.faulted()&&!backend.primary_mutations&&!backend.extras.mutations,"invalid activation strength refused");
    }
    for(unsigned variant=0;variant<3;++variant){
        scenario=variant;Composite lease;CombinedBackend backend(lease);auto targets=primary_targets();const auto frozen=targets;
        if(variant==0)backend.wanted_callback=[&](){targets[0][0]^=0x80u;};
        if(variant==1)backend.extras.on_write=[&](unsigned call){if(call==8)targets[0][0]^=0x80u;};
        if(variant==2)backend.after_strength=[&](unsigned call,unsigned,float strength){if(call==4&&strength>0)targets[0][0]^=0x80u;};
        require(!lease.acquire(15,targets,.5f,backend)&&lease.faulted()&&!lease.dirty()&&backend.all_original(),"callback caller mutation refuses commit and cleans exact journals");
        require(backend.frozen_seen==frozen&&targets!=frozen,"adapter received entry frozen target bytes");
        if(variant<2)require(!backend.primary_mutations,"pre-activation target change never activates primary");
        else require(!lease.primary_faulted(),"final target input change rejected even with successful primary readback");
    }
}
} // namespace

int main(){
    selected_masks();unchanged_fields();invalid_and_missing_capture();duplicate_and_pending_reset();
    exact_zero_capture();second_hand_capture_failure();
    field_transport_failure(MemoryBackend::Mode::false_before,"each_field_false_before_mutation");
    field_transport_failure(MemoryBackend::Mode::false_after,"each_field_false_after_mutation");
    unknown_postreads();unknown_before_write();compare_write_service_guards();nonzero_restore_then_fresh_zero();
    false_after_restore();false_before_restore();foreign_tuple_return();torn_fields();other_hand_cleanup();
    frozen_wanted_callbacks();reentrant_journals();masked_flags_service();
    composite_mapping_and_order();composite_both_cleanup_paths();composite_primary_failure();composite_extra_failure();
    composite_context_denials();composite_last_setter_reentry();composite_last_setter_closed();composite_invalid_and_frozen_inputs();
    std::printf("RESULT cases=%u checks=%u passed=1 memory_backend_only=1\n",case_count,check_count);
    return 0;
}
