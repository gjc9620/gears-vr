#pragma once
#include "ui_call_boundary.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

// Synchronous observation only. The caller must freshly validate the Function
// schema and current controller before each capture. Saved addresses, code
// windows and local bytes never authorize writes or later pointer dereferences.
// No result buffer, native out-parameter flags or setter enums are interpreted.
namespace gears_cover_call_trace {
static_assert(sizeof(std::uintptr_t)==8,"Cover FFrame observations require the certified 64-bit layout");
inline constexpr unsigned locals_limit=4096,raw_limit=128,window_limit=32,script_limit=131072;
struct RegistryHeader {std::uintptr_t data{};std::int32_t count{},capacity{};};
static_assert(sizeof(RegistryHeader)==16,"Certified registry header layout changed");
// Appending unrelated objects/names keeps the initial selected cohort valid.
// Every selected slot/identity/source is still compared separately. A moved
// allocation, capacity change, shrink or invalid header never matches.
inline bool same_registry_cohort(RegistryHeader initial,RegistryHeader observed,std::int32_t last_count){
    auto valid=[](RegistryHeader h){
        return h.data&&!(h.data&7)&&h.count>1000&&h.count<=h.capacity&&h.capacity<4000000&&
            h.data<=(std::numeric_limits<std::uintptr_t>::max)()-static_cast<std::uintptr_t>(h.capacity)*8;
    };
    return valid(initial)&&valid(observed)&&last_count>=0&&
        initial.data==observed.data&&initial.capacity==observed.capacity&&
        observed.count>=initial.count&&observed.count>=last_count;
}
struct Local {unsigned offset{},width{};};
struct Function {
    std::uintptr_t address{},script{};
    unsigned size{},locals_size{};
    Local pawn_ct{},pawn_ca{},pawn_cd{};
};
struct Values {unsigned char pawn_ct{},pawn_ca{},pawn_cd{};};
enum class Status : unsigned {
    accepted,invalid_function,invalid_caller,frame_read_failed,caller_mismatch,code_outside_script,
    locals_read_failed,invalid_locals,storage_read_failed,script_read_failed,
    frame_changed,locals_changed,storage_changed,script_changed
};
inline const char* status_name(Status status){
    static constexpr const char* names[]{"accepted","invalid_function","invalid_caller","frame_read_failed",
        "caller_mismatch","code_outside_script","locals_read_failed","invalid_locals","storage_read_failed",
        "script_read_failed","frame_changed","locals_changed","storage_changed","script_changed"};
    const auto index=static_cast<unsigned>(status);
    return index<sizeof(names)/sizeof(names[0])?names[index]:"unknown";
}
struct Snapshot {
    gears_ui_call_boundary::Frame frame{},after_frame{};
    std::uintptr_t locals{},after_locals{}; // Diagnostic values, never a lease.
    Values values{},after_values{};
    unsigned code_offset{},script_window_offset{},script_window_size{},raw_locals_size{};
    std::array<unsigned char,window_limit> script_window{};
    std::array<unsigned char,raw_limit> raw_locals{};
    // storage_equal covers the bounded prefix plus the three selected bytes,
    // including selected fields beyond the prefix; it does not cover all 4 KiB.
    bool frame_equal{},locals_pointer_equal{},storage_equal{},script_equal{};
    bool mutation_authorized{};
};
struct Result {
    Status status{Status::invalid_function};
    Snapshot snapshot{};
    explicit operator bool()const{return status==Status::accepted;}
};
inline bool equal(gears_ui_call_boundary::Frame const& a,gears_ui_call_boundary::Frame const& b){
    return a.node==b.node&&a.object==b.object&&a.code==b.code&&a.previous==b.previous;
}
inline bool equal(Values const& a,Values const& b){
    return a.pawn_ct==b.pawn_ct&&a.pawn_ca==b.pawn_ca&&a.pawn_cd==b.pawn_cd;
}
inline bool valid(Function const& function){
    constexpr auto maximum=(std::numeric_limits<std::uintptr_t>::max)();
    if(!function.address||(function.address&7)||!function.script||!function.size||function.size>script_limit||
       function.script>maximum-function.size||!function.locals_size||function.locals_size>locals_limit)return false;
    const std::array<Local,3> fields{function.pawn_ct,function.pawn_ca,function.pawn_cd};
    for(auto const& field:fields)if(field.width!=1||field.offset>=function.locals_size)return false;
    return fields[0].offset!=fields[1].offset&&fields[0].offset!=fields[2].offset&&fields[1].offset!=fields[2].offset;
}

// Read has safe_copy's (destination, source, count)->bool signature. All spans
// are bounded before invoking it. Both endpoint cohorts must agree; this is a
// finite consistency bracket, not an atomic engine snapshot or an outparam ABI.
template<class Read> Result capture(Read read,std::uintptr_t caller,std::uintptr_t expected_self,Function const& function){
    Result result{};auto& out=result.snapshot;
    constexpr auto maximum=(std::numeric_limits<std::uintptr_t>::max)();
    if(!valid(function))return result;
    if(!caller||(caller&7)||caller>maximum-0x40||!expected_self||(expected_self&7)){
        result.status=Status::invalid_caller;return result;}
    auto bytes=[&](void* destination,std::uintptr_t source,std::size_t count){
        return source&&count&&source<=maximum-count&&read(destination,reinterpret_cast<void const*>(source),count);};
    auto reader=[&](void* destination,void const* source,std::size_t count){
        return bytes(destination,reinterpret_cast<std::uintptr_t>(source),count);};
    if(!gears_ui_call_boundary::frame(reader,caller,out.frame)){
        result.status=Status::frame_read_failed;return result;}
    if(out.frame.node!=function.address||out.frame.object!=expected_self){
        result.status=Status::caller_mismatch;return result;}
    if(out.frame.code<function.script||out.frame.code-function.script>=function.size){
        result.status=Status::code_outside_script;return result;}
    out.code_offset=static_cast<unsigned>(out.frame.code-function.script);
    if(!bytes(&out.locals,caller+0x30,sizeof(out.locals))){
        result.status=Status::locals_read_failed;return result;}
    if(!out.locals||(out.locals&7)||out.locals>maximum-function.locals_size){
        result.status=Status::invalid_locals;return result;}
    out.raw_locals_size=(std::min)(raw_limit,function.locals_size);
    const std::array<Local,3> fields{function.pawn_ct,function.pawn_ca,function.pawn_cd};
    auto storage=[&](auto& raw,Values& values){
        if(!bytes(raw.data(),out.locals,out.raw_locals_size))return false;
        std::array<unsigned char,3> selected{};
        for(unsigned i=0;i<fields.size();++i){
            if(fields[i].offset<out.raw_locals_size)selected[i]=raw[fields[i].offset];
            else if(!bytes(&selected[i],out.locals+fields[i].offset,1))return false;
        }
        values={selected[0],selected[1],selected[2]};return true;
    };
    if(!storage(out.raw_locals,out.values)){
        result.status=Status::storage_read_failed;return result;}
    out.script_window_size=(std::min)(window_limit,function.size);
    out.script_window_offset=out.code_offset>window_limit/2?out.code_offset-window_limit/2:0;
    out.script_window_offset=(std::min)(out.script_window_offset,function.size-out.script_window_size);
    if(!bytes(out.script_window.data(),function.script+out.script_window_offset,out.script_window_size)){
        result.status=Status::script_read_failed;return result;}
    std::array<unsigned char,raw_limit> after_storage{};
    std::array<unsigned char,window_limit> after_script{};
    if(!storage(after_storage,out.after_values)){
        result.status=Status::storage_read_failed;return result;}
    if(!bytes(after_script.data(),function.script+out.script_window_offset,out.script_window_size)){
        result.status=Status::script_read_failed;return result;}
    if(!bytes(&out.after_locals,caller+0x30,sizeof(out.after_locals))){
        result.status=Status::locals_read_failed;return result;}
    if(!gears_ui_call_boundary::frame(reader,caller,out.after_frame)){
        result.status=Status::frame_read_failed;return result;}
    out.frame_equal=equal(out.frame,out.after_frame);
    out.locals_pointer_equal=out.locals==out.after_locals;
    out.storage_equal=out.raw_locals==after_storage&&equal(out.values,out.after_values);
    out.script_equal=out.script_window==after_script;
    if(!out.frame_equal)result.status=Status::frame_changed;
    else if(!out.locals_pointer_equal)result.status=Status::locals_changed;
    else if(!out.storage_equal)result.status=Status::storage_changed;
    else if(!out.script_equal)result.status=Status::script_changed;
    else result.status=Status::accepted;
    return result;
}
}
