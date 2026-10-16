#pragma once
#include <Windows.h>
#include <cstdint>

namespace gears_quiet {
constexpr DWORD abi = 1;
enum Call : unsigned { show, foreground, active, focus, position, bring, create_w, create_a, count };
struct Status {
    DWORD version{abi};
    DWORD installed{};
    std::uint64_t suppressed[count]{};
};
using Install = DWORD (WINAPI*)(void*);
using Read = DWORD (WINAPI*)(Status*);
constexpr DWORD trace_abi=1,trace_capacity=128,trace_stack_capacity=24;
// Trace-only IDs; Call::count and the eight Status counters remain unchanged.
enum TraceCall : unsigned { def_window_w=count,def_window_a,native_foreground,show_async,trace_call_count };
struct TraceRecord {
    DWORD sequence{},call{},thread{},suppressed{},stack_count{},reserved{};
    std::uint64_t target{},related{},result_window{};
    std::uint64_t argument{},effective_argument{},extra_argument{};
    std::uint64_t enter_tick{},leave_tick{},foreground_before{},foreground_after{};
    std::uint64_t stack[trace_stack_capacity]{};
};
// Independent ABI; Status is unchanged. Export takes an output buffer of this
// exact size. Records are compacted in reservation order, sequence is 1-based.
// completed counts only release/acquire-published records; busy_skipped counts
// reservations not yet complete at the snapshot. overflow is a sticky boolean:
// calls after the first 128 reservations are omitted, never overwrite records.
// HWNDs/stack addresses are raw uint64 values; create target is NULL on entry,
// related is parent, result_window is the created HWND, extra_argument exstyle.
// argument/effective_argument are ShowWindow command, SetWindowPos flags or
// CreateWindowEx style; suppressed means the existing policy intervened.
// For trace-only DefWindowProcW/A: argument=message, related=wParam,
// extra_argument=lParam (raw pointer-width bits), suppressed is always zero.
struct TraceSnapshot {
    DWORD version{trace_abi},size{sizeof(TraceSnapshot)},enabled{},capacity{trace_capacity};
    DWORD reserved_count{},completed{},overflow{},busy_skipped{};
    TraceRecord records[trace_capacity]{};
};
static_assert(sizeof(TraceRecord)==296 && sizeof(TraceSnapshot)==37920,"Trace ABI layout");
using ReadTrace = DWORD (WINAPI*)(void*);
// V2 has a distinct export: old callers of GearsQuietTrace retain the V1 size.
struct GuiThreadSnapshot {
    DWORD valid{},error{},flags{},reserved{};
    std::uint64_t active{},focus{},capture{};
};
struct StartupSnapshot {
    DWORD flags{},show_window{},x{},y{},width{},height{},columns{},rows{},fill{},reserved{};
    std::uint64_t input{},output{},error{};
};
struct TraceRecord2 : TraceRecord {
    std::uint64_t enter_qpc{},leave_qpc{},returned_value{},native_value{};
    DWORD return_valid{},native_valid{},native_call{},native_last_error{};
    GuiThreadSnapshot gui_before{},gui_after{};
};
struct TraceSnapshot2 {
    DWORD version{2},size{sizeof(TraceSnapshot2)},enabled{},capacity{trace_capacity};
    DWORD reserved_count{},completed{},overflow{},busy_skipped{};
    std::uint64_t qpc_frequency{};
    StartupSnapshot startup{};
    TraceRecord2 records[trace_capacity]{};
};
static_assert(sizeof(TraceRecord2)==424 && sizeof(TraceSnapshot2)==54376,"Trace V2 layout");
// Independent CBT observer ABI. call is HCBT_ACTIVATE or HCBT_SYSCOMMAND;
// argument is fMouse or raw command, related is prior active or raw lParam.
struct CbtSnapshot {
    DWORD version{1},size{sizeof(CbtSnapshot)},enabled{},capacity{trace_capacity};
    DWORD reserved_count{},completed{},overflow{},busy_skipped{};
    DWORD thread{},install_error{},installed{},reserved{};
    std::uint64_t qpc_frequency{};
    TraceRecord2 records[trace_capacity]{};
};
static_assert(sizeof(CbtSnapshot)==54328,"CBT trace layout");
}
