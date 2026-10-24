// Standalone Windows x64 ABI tests; link the two adjacent MASM sources.
// Tests use synthetic functions only. They never install a hook or open a game.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

struct alignas(16) Registers {
    std::uint64_t rax, rcx, rdx, r8, r9, r10, r11;
    std::uint64_t flags, rsp, stack5, stack6, return_address;
    unsigned char xmm[6][16];
};
static_assert(offsetof(Registers, flags) == 0x38);
static_assert(offsetof(Registers, xmm) == 0x60);
static_assert(sizeof(Registers) == 0xc0);

extern "C" {
void GearsCoverBlendTickHook();
void GearsCoverChildPropagateHook();
void GearsCoverBlendBefore(std::uintptr_t self, float opaque1, float opaque2);
void GearsCoverChildBefore(std::uintptr_t self, int childIndex, float totalWeight);
void GearsCoverTestBlendTarget();
void GearsCoverTestChildTarget();
void GearsCoverTestInvoke();
void GearsCoverTestAfterCall();
void* GearsCoverBlendOriginal = reinterpret_cast<void*>(&GearsCoverTestBlendTarget);
void* GearsCoverChildOriginal = reinterpret_cast<void*>(&GearsCoverTestChildTarget);
Registers gCoverTestInput{}, gCoverTestBefore{}, gCoverTestTarget{};
void* gCoverTestHook = nullptr;
std::uint64_t gCoverTestDriverRsp = 0;
std::uint64_t gCoverTestRequestedFlags = 0;
std::uint64_t gCoverTestReturnFlags = 0;
std::uint64_t gCoverTestReturnRax = 0;
alignas(16) unsigned char gCoverTestReturnXmm0[16]{};
std::uint64_t gCoverTestGotRax = 0;
alignas(16) unsigned char gCoverTestGotXmm0[16]{};
std::uint64_t gCoverTestGotFlags = 0;
unsigned gCoverTestOriginalCalls = 0, gCoverTestOriginalKind = 99;
std::uint64_t gCoverTestShadow[4]{};
}

namespace {
constexpr std::uint64_t arithmetic_flags = 0x8d5; // CF/PF/AF/ZF/SF/OF only.
constexpr DWORD test_exception = 0xe042c011;
unsigned checks = 0, failures = 0, before_calls = 0, expected_kind = 0;
unsigned unwind_visits = 0;
bool throw_before = false;

void expect(bool okay, const char* what) {
    ++checks;
    if (!okay) { ++failures; std::fprintf(stderr, "FAIL: %s\n", what); }
}

void check_unwind_metadata(void* hook) {
    DWORD64 base = 0;
    auto* fn = RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(hook), &base, nullptr);
    expect(fn != nullptr, "thunk has RUNTIME_FUNCTION");
    if (!fn) return;
    expect(base + fn->BeginAddress == reinterpret_cast<DWORD64>(hook), "exact thunk begin");
    // UNWIND_INFO is not declared by every public SDK; these are its four
    // documented header bytes (version/flags, prolog size, count, frame).
    const auto* info = reinterpret_cast<const unsigned char*>(base + fn->UnwindData);
    expect((info[0] & 7) == 1 && (info[0] >> 3) == 0, "plain version-1 unwind metadata");
    expect((info[3] & 15) == 5 && (info[3] >> 4) == 12, "RBP describes fixed C0 frame");
}

// Walk an actual callback stack through the MASM callback and production thunk.
// This checks the OS unwinder, not just the bytes of a hand-authored descriptor.
void check_live_unwind() {
    CONTEXT context{};
    RtlCaptureContext(&context);
    DWORD64 hook_base = 0;
    auto* hook = RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(gCoverTestHook), &hook_base, nullptr);
    bool found = false;
    for (unsigned n = 0; n != 24 && context.Rip; ++n) {
        DWORD64 base = 0, establisher = 0;
        PVOID handler_data = nullptr;
        auto* fn = RtlLookupFunctionEntry(context.Rip, &base, nullptr);
        if (!fn) break;
        const bool in_hook = hook && context.Rip >= hook_base + hook->BeginAddress &&
            context.Rip < hook_base + hook->EndAddress;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, context.Rip, fn, &context,
            &handler_data, &establisher, nullptr);
        if (in_hook) {
            found = true;
            expect(context.Rip == reinterpret_cast<DWORD64>(&GearsCoverTestAfterCall),
                "unwind thunk returns directly to original caller");
            expect(context.Rsp == gCoverTestDriverRsp, "unwind restores original caller RSP");
            expect(context.Rbp == gCoverTestDriverRsp + 0x30, "unwind restores caller RBP");
            ++unwind_visits;
            break;
        }
    }
    expect(found, "callback stack unwinds through production thunk");
}

std::uint32_t low_float_bits(const unsigned char* bytes) {
    std::uint32_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

void seed(unsigned kind, std::uint64_t flags) {
    expected_kind = kind;
    gCoverTestHook = kind ? reinterpret_cast<void*>(&GearsCoverChildPropagateHook)
                         : reinterpret_cast<void*>(&GearsCoverBlendTickHook);
    gCoverTestBefore = {};
    gCoverTestTarget = {};
    gCoverTestOriginalCalls = 0;
    gCoverTestOriginalKind = 99;
    before_calls = 0;
    gCoverTestInput.rax = 0x0123456789abcdef;
    gCoverTestInput.rcx = 0x1020304050607080;
    gCoverTestInput.rdx = 0xd2d2d2d2fffffff3; // childIndex == -13, upper half opaque.
    gCoverTestInput.r8 = 0x8877665544332211;
    gCoverTestInput.r9 = 0x9a9b9c9d9e9fa0a1;
    gCoverTestInput.r10 = 0xb1b2b3b4b5b6b7b8;
    gCoverTestInput.r11 = 0xc1c2c3c4c5c6c7c8;
    for (unsigned r = 0; r != 6; ++r)
        for (unsigned b = 0; b != 16; ++b)
            gCoverTestInput.xmm[r][b] = static_cast<unsigned char>(r * 29 + b + 3);
    const float opaque1 = -2.25f, opaque2 = 0.375f;
    std::memcpy(gCoverTestInput.xmm[1], &opaque1, sizeof(opaque1));
    std::memcpy(gCoverTestInput.xmm[2], &opaque2, sizeof(opaque2));
    gCoverTestRequestedFlags = 0x202 | flags;
    gCoverTestReturnFlags = 0x202 | (flags ^ arithmetic_flags);
    gCoverTestReturnRax = 0xe23456789abcdef0;
    for (unsigned b = 0; b != 16; ++b)
        gCoverTestReturnXmm0[b] = static_cast<unsigned char>(0xf0 - b);
}

// Keep SEH separate from functions with C++ objects requiring destruction.
bool invoke_catching() {
    __try { GearsCoverTestInvoke(); }
    __except (GetExceptionCode() == test_exception ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return true;
    }
    return false;
}

void normal_case(unsigned kind, std::uint64_t flags) {
    seed(kind, flags);
    throw_before = false;
    GearsCoverTestInvoke();
    expect(before_calls == 1, "pre-callback called exactly once");
    expect(gCoverTestOriginalCalls == 1 && gCoverTestOriginalKind == kind, "correct original called exactly once");
    expect(std::memcmp(&gCoverTestTarget, &gCoverTestInput, 7 * sizeof(std::uint64_t)) == 0,
        "original sees all seven unchanged volatile GPRs");
    expect(std::memcmp(gCoverTestTarget.xmm, gCoverTestInput.xmm, sizeof(gCoverTestInput.xmm)) == 0,
        "original sees full XMM0..5 values");
    expect((gCoverTestTarget.flags & arithmetic_flags) == flags, "original sees incoming arithmetic RFLAGS");
    expect(gCoverTestTarget.rsp == gCoverTestDriverRsp - 8 && (gCoverTestTarget.rsp & 15) == 8,
        "tail target has exact original entry stack and alignment");
    expect(gCoverTestTarget.return_address == reinterpret_cast<std::uintptr_t>(&GearsCoverTestAfterCall),
        "tail target sees original return address");
    expect(gCoverTestTarget.stack5 == 0x0123456789abcdef && gCoverTestTarget.stack6 == 0xfedcba9876543210,
        "fifth and sixth stack arguments unchanged");
    const std::uint64_t shadow[]{0xa0a0a0a0a0a0a0a0, 0xb1b1b1b1b1b1b1b1,
        0xc2c2c2c2c2c2c2c2, 0xd3d3d3d3d3d3d3d3};
    expect(std::memcmp(gCoverTestShadow, shadow, sizeof(shadow)) == 0, "original caller shadow untouched");
    expect(gCoverTestGotRax == gCoverTestReturnRax, "original RAX return passes directly through");
    expect(std::memcmp(gCoverTestGotXmm0, gCoverTestReturnXmm0, sizeof(gCoverTestGotXmm0)) == 0,
        "original full XMM0 return passes directly through");
    expect((gCoverTestGotFlags & arithmetic_flags) == (gCoverTestReturnFlags & arithmetic_flags),
        "original return flags pass directly through");
}
}

extern "C" void GearsCoverTestBefore(unsigned kind) {
    ++before_calls;
    expect(kind == expected_kind, "correct pre-callback selected");
    expect((gCoverTestBefore.rsp & 15) == 8, "callback entry stack is ABI aligned");
    expect(gCoverTestBefore.rcx == gCoverTestInput.rcx, "callback self in RCX");
    expect((gCoverTestBefore.flags & arithmetic_flags) == (gCoverTestRequestedFlags & arithmetic_flags),
        "callback sees original arithmetic flags");
    if (kind == 0) {
        expect(low_float_bits(gCoverTestBefore.xmm[1]) == 0xc0100000, "blend opaque1 remains in positional XMM1");
        expect(low_float_bits(gCoverTestBefore.xmm[2]) == 0x3ec00000, "blend opaque2 remains in positional XMM2");
    } else {
        expect(static_cast<std::int32_t>(gCoverTestBefore.rdx) == -13, "child index remains in EDX");
        expect(low_float_bits(gCoverTestBefore.xmm[2]) == 0x3ec00000, "child totalWeight remains in positional XMM2");
    }
    check_live_unwind();
    if (throw_before) RaiseException(test_exception, 0, 0, nullptr);
}

int main() {
    check_unwind_metadata(reinterpret_cast<void*>(&GearsCoverBlendTickHook));
    check_unwind_metadata(reinterpret_cast<void*>(&GearsCoverChildPropagateHook));
    for (unsigned kind = 0; kind != 2; ++kind) {
        normal_case(kind, arithmetic_flags);
        normal_case(kind, 0);
        seed(kind, arithmetic_flags);
        throw_before = true;
        expect(invoke_catching(), "SEH unwinds through callback and thunk");
        expect(before_calls == 1 && gCoverTestOriginalCalls == 0, "exception does not call original");
    }
    expect(unwind_visits == 6, "OS unwind traversed all six test callback invocations");
    std::printf("cover_body_thunks: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
