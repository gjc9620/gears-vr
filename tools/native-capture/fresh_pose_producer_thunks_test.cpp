// Standalone production control-thunk ABI/SEH tests; reuses existing synthetic ASM support.
// Synthetic targets only: no hook installation, process access, or game code.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "hde64.h"

struct alignas(16) Registers {
    std::uint64_t rax, rcx, rdx, r8, r9, r10, r11;
    std::uint64_t flags, rsp, opaque5, opaque6, return_address;
    unsigned char xmm[6][16];
};
struct alignas(16) CallPacket {
    std::array<std::uint64_t,7> gpr;
    std::uint64_t flags;
    unsigned char xmm[6][16];
};
static_assert(offsetof(Registers,flags)==0x38 && offsetof(Registers,xmm)==0x60 && sizeof(Registers)==0xc0);
static_assert(offsetof(CallPacket,flags)==0x38 && offsetof(CallPacket,xmm)==0x40 && sizeof(CallPacket)==0xa0);

extern "C" {
void GearsFreshRootHook();
void GearsFreshOptionalHook();
void GearsFreshPerBoneHook();
void GearsCoverControlSingleHook();
void GearsCoverControlWeightHook();
void GearsCoverBlendBefore(std::uintptr_t,float,float);
void GearsCoverChildBefore(std::uintptr_t,int,float);
void GearsCoverTestAfterCallback(unsigned);
void GearsFreshRawInvoke5(CallPacket*,void*,std::uintptr_t) noexcept(false);
void GearsFreshRawInvoke4(CallPacket*,void*) noexcept(false);
void GearsFreshInvokeProtected(CallPacket*,unsigned,std::uintptr_t,std::uintptr_t,std::uintptr_t) noexcept(false);
void GearsCoverTestBlendTarget();
void GearsCoverTestChildTarget();
void GearsCoverTestInvoke() noexcept(false);
void GearsCoverTestAfterCall();
Registers gCoverTestInput{},gCoverTestBefore{},gCoverTestTarget{},gCoverTestReturned{},gCoverTestOutput{};
void* gCoverTestHook=nullptr;
std::uint64_t gCoverTestDriverRsp=0,gCoverTestRequestedFlags=0,gCoverTestReturnFlags=0;
std::uint64_t gCoverTestReturnRax=0,gCoverTestGotRax=0,gCoverTestGotFlags=0;
alignas(16) unsigned char gCoverTestReturnXmm0[16]{},gCoverTestGotXmm0[16]{};
unsigned gCoverTestOriginalCalls=0,gCoverTestOriginalKind=99;
std::uint64_t gCoverTestShadow[4]{};
}
namespace {
constexpr std::uint64_t arithmetic_flags=0x8d5; // CF/PF/AF/ZF/SF/OF only.
constexpr DWORD test_exception=0xe042c011;
struct CppException {unsigned marker;};
unsigned checks{},failures{},before_calls{},post_calls{},finally_calls{},protected_live{},expected_kind{},unwind_visits{};
unsigned throw_stage{};DWORD caught_code{};
CallPacket post_packet{};
void expect(bool okay,const char* what){++checks;if(!okay){++failures;std::fprintf(stderr,"FAIL: %s\n",what);}}
void check_unwind_metadata(void* address,unsigned frame_offset,bool handler){
    DWORD64 base{};auto* fn=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(address),&base,nullptr);
    expect(fn!=nullptr,"wrapper has RUNTIME_FUNCTION");if(!fn)return;
    expect(base+fn->BeginAddress==reinterpret_cast<DWORD64>(address),"exact function begin");
    const auto* info=reinterpret_cast<const unsigned char*>(base+fn->UnwindData);
    expect((info[0]&7)==1,"version-1 unwind metadata");
    if(handler)expect(((info[0]>>3)&2)!=0,"protected bridge has compiler UHandler metadata");
    else expect((info[0]>>3)==0 && (info[3]&15)==5 && unsigned(info[3]>>4)==frame_offset,"ASM RBP/allocation metadata matches frame");
}
// Walk actual pre/original/post stacks through both assembly frames and the
// compiler-owned protected frame. Virtual unwinding must not run the finally.
void check_live_unwind(){
    CONTEXT context{};RtlCaptureContext(&context);DWORD64 hook_base{};
    auto* hook=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(gCoverTestHook),&hook_base,nullptr);bool found{};
    const unsigned live=protected_live,finals=finally_calls;
    for(unsigned n=0;n!=32&&context.Rip;++n){
        DWORD64 base{},establisher{};PVOID data{};auto* fn=RtlLookupFunctionEntry(context.Rip,&base,nullptr);if(!fn)break;
        const bool in_hook=hook&&context.Rip>=hook_base+hook->BeginAddress&&context.Rip<hook_base+hook->EndAddress;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER,base,context.Rip,fn,&context,&data,&establisher,nullptr);
        if(in_hook){found=true;expect(context.Rip==reinterpret_cast<DWORD64>(&GearsCoverTestAfterCall),"OS unwind reaches original caller");
            expect(context.Rsp==gCoverTestDriverRsp,"OS unwind restores caller RSP");
            expect(context.Rbp==gCoverTestDriverRsp+0x30,"OS unwind restores caller RBP");++unwind_visits;break;}
    }
    expect(found,"callback/original stack unwinds through production thunk");
    expect(protected_live==live&&finally_calls==finals,"virtual unwind does not execute cleanup");
}
std::uint32_t bits(const unsigned char* p){std::uint32_t v{};std::memcpy(&v,p,4);return v;}
void seed(unsigned kind,std::uint64_t flags){
    expected_kind=kind;gCoverTestHook=kind==1?reinterpret_cast<void*>(&GearsFreshOptionalHook):
        kind==2?reinterpret_cast<void*>(&GearsFreshPerBoneHook):reinterpret_cast<void*>(&GearsFreshRootHook);
    gCoverTestBefore={};gCoverTestTarget={};gCoverTestReturned={};post_packet={};
    before_calls=post_calls=finally_calls=protected_live=gCoverTestOriginalCalls=0;gCoverTestOriginalKind=99;caught_code=0;throw_stage=0;
    gCoverTestInput.rax=0x0123456789abcdef;gCoverTestInput.rcx=0x1020304050607080;
    gCoverTestInput.rdx=0xd2d2d2d2fffffff3;gCoverTestInput.r8=0x8877665544332211;
    gCoverTestInput.r9=0x9a9b9c9d9e9fa0a1;gCoverTestInput.r10=0xb1b2b3b4b5b6b7b8;gCoverTestInput.r11=0xc1c2c3c4c5c6c7c8;
    for(unsigned r=0;r!=6;++r)for(unsigned b=0;b!=16;++b){gCoverTestInput.xmm[r][b]=static_cast<unsigned char>(r*29+b+3);gCoverTestOutput.xmm[r][b]=static_cast<unsigned char>(0xf0-r*17-b);}
    const float f1=-2.25f,f2=.375f;std::memcpy(gCoverTestInput.xmm[1],&f1,4);std::memcpy(gCoverTestInput.xmm[2],&f2,4);
    gCoverTestRequestedFlags=0x202|flags;gCoverTestReturnFlags=0x202|(flags^arithmetic_flags);
    gCoverTestOutput.rax=gCoverTestReturnRax=0xe23456789abcdef0;gCoverTestOutput.rcx=0xa1a1a1a1a1a1a1a1;
    gCoverTestOutput.rdx=0xa2a2a2a2a2a2a2a2;gCoverTestOutput.r8=0xa8a8a8a8a8a8a8a8;gCoverTestOutput.r9=0xa9a9a9a9a9a9a9a9;
    gCoverTestOutput.r10=0xaaaaaaaaaaaaaaaa;gCoverTestOutput.r11=0xabababababababab;
    std::memcpy(gCoverTestReturnXmm0,gCoverTestOutput.xmm[0],16);
}
bool invoke_catching(){
    __try{GearsCoverTestInvoke();}
    __except(GetExceptionCode()==test_exception?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught_code=GetExceptionCode();return true;}
    return false;
}
void normal_case(unsigned kind,std::uint64_t flags){
    seed(kind,flags);GearsCoverTestInvoke();
    expect(before_calls==1&&post_calls==1&&finally_calls==1&&!protected_live,"pre/post/finally exactly once and balanced");
    expect(gCoverTestOriginalCalls==1&&gCoverTestOriginalKind==(kind%2),"correct original called exactly once");
    expect(!std::memcmp(&gCoverTestTarget,&gCoverTestInput,7*sizeof(std::uint64_t)),"original sees all seven input volatile GPRs");
    expect(!std::memcmp(gCoverTestTarget.xmm,gCoverTestInput.xmm,sizeof gCoverTestInput.xmm),"original sees full input XMM0..5");
    expect((gCoverTestTarget.flags&arithmetic_flags)==flags,"original sees incoming arithmetic RFLAGS");
    if(kind!=1)expect(gCoverTestTarget.opaque5==0x0123456789abcdef,"Root/per-bone original receives exact fifth argument");
    expect((gCoverTestTarget.rsp&15)==8,"original entry has ABI stack alignment");
    expect(gCoverTestTarget.return_address!=reinterpret_cast<std::uintptr_t>(&GearsCoverTestAfterCall),"original has wrapper return address: fixed ABI, no tailcall guarantee");
    const std::uint64_t shadow[]{0xa0a0a0a0a0a0a0a0,0xb1b1b1b1b1b1b1b1,0xc2c2c2c2c2c2c2c2,0xd3d3d3d3d3d3d3d3};
    expect(!std::memcmp(gCoverTestShadow,shadow,sizeof shadow),"original caller home space remains untouched");
    expect(!std::memcmp(&gCoverTestReturned,&gCoverTestOutput,7*sizeof(std::uint64_t)),"all seven original return GPRs survive clobbering post");
    expect(!std::memcmp(gCoverTestReturned.xmm,gCoverTestOutput.xmm,sizeof gCoverTestOutput.xmm),"full original XMM0..5 survive clobbering post");
    expect((gCoverTestGotFlags&arithmetic_flags)==(gCoverTestReturnFlags&arithmetic_flags),"original return RFLAGS survive post");
    expect(post_packet.gpr[0]==gCoverTestReturnRax&&!std::memcmp(post_packet.xmm[0],gCoverTestReturnXmm0,16),"post receives immediate saved RAX/XMM0 result");
}
}
extern "C" void GearsCoverTestBefore(unsigned kind){
    ++before_calls;expect(kind==(expected_kind%2),"correct pre selected");expect(protected_live==1,"residency covers pre");
    expect((gCoverTestBefore.rsp&15)==8,"pre entry stack aligned");expect(gCoverTestBefore.rcx==gCoverTestInput.rcx,"self remains in RCX");
    if(!kind){expect(bits(gCoverTestBefore.xmm[1])==0xc0100000,"blend positional XMM1");expect(bits(gCoverTestBefore.xmm[2])==0x3ec00000,"blend positional XMM2");}
    else{expect(static_cast<std::int32_t>(gCoverTestBefore.rdx)==-13,"child index in EDX");expect(bits(gCoverTestBefore.xmm[2])==0x3ec00000,"child positional XMM2");}
    check_live_unwind();if(throw_stage==1)RaiseException(test_exception,0,0,nullptr);
}
extern "C" void GearsCoverTestInOriginal(unsigned kind) noexcept(false){
    expect(kind==(expected_kind%2)&&protected_live==1,"residency spans correct original");check_live_unwind();
    if(throw_stage==2)RaiseException(test_exception,0,0,nullptr);
    if(throw_stage==4)throw CppException{0x12345678};
}
extern "C" void GearsCoverTestAfter(unsigned kind){
    ++post_calls;expect(kind==expected_kind&&protected_live==1,"residency spans correct post");check_live_unwind();
    if(throw_stage==3)RaiseException(test_exception,0,0,nullptr);
}
// Same protected routing used by the production module. No catch spans the
// original; compiler UHandler executes the final release for SEH and C++ EH.
extern "C" __declspec(noinline) void GearsFreshInvokeProtected(CallPacket* packet,unsigned kind,std::uintptr_t caller,std::uintptr_t entry_rsp,std::uintptr_t fifth) noexcept(false){
    expect(caller==reinterpret_cast<std::uintptr_t>(&GearsCoverTestAfterCall),"exact original caller RA forwarded");
    expect(entry_rsp==gCoverTestDriverRsp-8,"true original entry RSP forwarded as fourth argument");
    expect(fifth==(kind==1?0ull:0x0123456789abcdefull),"Root/per-bone fifth is forwarded; Optional fifth is unread/zero");
    expect((entry_rsp&15)==8,"original entry RSP alignment");
    bool returned{};float f1{},f2{};std::memcpy(&f1,packet->xmm[1],4);std::memcpy(&f2,packet->xmm[2],4);++protected_live;
    __try{
        if(kind!=1)GearsCoverBlendBefore(packet->gpr[1],f1,f2);else GearsCoverChildBefore(packet->gpr[1],static_cast<std::int32_t>(packet->gpr[2]),f2);
        if(kind==1)GearsFreshRawInvoke4(packet,reinterpret_cast<void*>(&GearsCoverTestChildTarget));
        else GearsFreshRawInvoke5(packet,reinterpret_cast<void*>(&GearsCoverTestBlendTarget),fifth);returned=true;
    }__finally{
        __try{if(returned&&!AbnormalTermination()){post_packet=*packet;GearsCoverTestAfterCallback(kind);}}
        __finally{++finally_calls;--protected_live;}
    }
}
// Decode the ACTUAL linked assembly, not source text. Its explicit memory
// operations are a small fixed set. Reject any unrecognised memory opcode.
// Hook frame: RSP=RSP0-D8, RBP=RSP0-8. Raw frame: RSP=RSP0-48,
// RBP=RSP0-18. No explicit memory operand occurs while PUSHFQ is unmatched.
void read_window(void* target,bool root_hook,bool raw){
    DWORD64 image{};auto* fn=RtlLookupFunctionEntry(reinterpret_cast<DWORD64>(target),&image,nullptr);
    expect(fn!=nullptr,"read window has exact unwind-bounded code");if(!fn)return;
    auto* begin=reinterpret_cast<unsigned char const*>(image+fn->BeginAddress);
    const auto size=fn->EndAddress-fn->BeginAddress;unsigned caller_reads{},fifth_reads{};
    for(unsigned offset=0;offset<size;){hde64s h{};const auto n=hde64_disasm(begin+offset,&h);
        expect(n&&!(h.flags&F_ERROR)&&n<=size-offset,"actual ASM instruction decodes exactly");if(!n||(h.flags&F_ERROR))return;offset+=n;
        if(!(h.flags&F_MODRM)||h.modrm_mod==3)continue;
        const bool address_only=h.opcode==0x8d;
        const bool write=h.opcode==0x89||h.opcode==0xc7||(h.opcode==0x0f&&h.opcode2==0x7f);
        const bool read=h.opcode==0x8b||(h.opcode==0x0f&&h.opcode2==0x6f)||(h.opcode==0xff&&(h.modrm_reg==2||h.modrm_reg==6));
        expect(address_only||write||read,"all explicit memory operations have a classified read window");if(!read)continue;
        unsigned reg=h.modrm_rm;if(reg==4){expect(h.sib_index==4,"no indexed stack read");reg=h.sib_base;}
        if(h.rex_b)reg+=8;
        if(reg==3&&raw)continue; // RBX is the separate owned CallPacket.
        expect(reg==4||reg==5,"all remaining reads use frame RSP/RBP");if(reg!=4&&reg!=5)continue;
        const int disp=(h.flags&F_DISP8)?static_cast<signed char>(h.disp.disp8):(h.flags&F_DISP32)?static_cast<int>(h.disp.disp32):0;
        const int relative=(reg==4?(raw?-0x48:-0xd8):(raw?-0x18:-8))+disp;
        const int width=h.opcode==0x0f?16:8;
        if(relative>=0)++caller_reads;
        if(root_hook&&relative==0x28&&width==8){++fifth_reads;continue;}
        expect(relative+width<=(raw?0:8),"Optional/read-only bridge cannot read caller shadow end or fifth slot");
    }
    expect(fifth_reads==(root_hook?1u:0u),"only Root has exactly one caller fifth read");
    if(raw)expect(!caller_reads,"RawInvoke4 never reads incoming stack arguments");
}
int main(){
    read_window(reinterpret_cast<void*>(&GearsFreshRootHook),true,false);
    read_window(reinterpret_cast<void*>(&GearsFreshOptionalHook),false,false);
    read_window(reinterpret_cast<void*>(&GearsFreshPerBoneHook),true,false);
    read_window(reinterpret_cast<void*>(&GearsFreshRawInvoke4),false,true);
    check_unwind_metadata(reinterpret_cast<void*>(&GearsFreshRootHook),13,false);
    check_unwind_metadata(reinterpret_cast<void*>(&GearsFreshPerBoneHook),13,false);

    check_unwind_metadata(reinterpret_cast<void*>(&GearsFreshRawInvoke5),3,false);
    check_unwind_metadata(reinterpret_cast<void*>(&GearsFreshRawInvoke4),3,false);
    check_unwind_metadata(reinterpret_cast<void*>(&GearsFreshInvokeProtected),0,true);

    for(unsigned kind=0;kind!=3;++kind){
        normal_case(kind,arithmetic_flags);normal_case(kind,0);
        for(unsigned stage=1;stage!=4;++stage){seed(kind,arithmetic_flags);throw_stage=stage;expect(invoke_catching()&&caught_code==test_exception,"same SEH propagates through protected thunk");
            expect(finally_calls==1&&!protected_live,"SEH releases exactly once");expect(gCoverTestOriginalCalls==(stage==1?0u:1u),"SEH never duplicates original");expect(post_calls==(stage==3?1u:0u),"exception does not run a normal post");}
        seed(kind,0);throw_stage=4;bool caught{};try{GearsCoverTestInvoke();}catch(CppException const& e){caught=e.marker==0x12345678;}
        expect(caught&&finally_calls==1&&!protected_live&&gCoverTestOriginalCalls==1&&!post_calls,"same C++ exception propagates with balanced finally");
    }
    expect(unwind_visits>=13,"OS unwind exercised pre/original/post and exception paths");
    std::printf("fresh_pose_thunks: %u checks, %u failures\n",checks,failures);return failures?1:0;
}

extern "C" void GearsCoverInvokeProtected(CallPacket*,unsigned) noexcept(false){expect(false,"unused body bridge invoked");}
