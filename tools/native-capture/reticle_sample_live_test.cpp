#include <windows.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <mutex>
#include <string>
#include "frame_link.h"
#include "ui_reticle_direction_plan.h"
// The real worker/observer implementation is included below. Only its engine
// inputs are synthetic; no process/window/game APIs are opened by this test.
static std::uintptr_t base=0x140000000;
static LONGLONG session_started_qpc{};
static std::atomic<std::uint64_t> committed_world_generation{1};
static std::atomic<bool> recording{true};
static std::recursive_mutex frame_hook_mutex;
static std::mutex ui_payload_mutex;
static LONGLONG now(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return value.QuadPart;}
struct UiPayloadContext {
    std::uintptr_t command{},payload{};std::uint64_t generation{},root_first{},root_last{};
    unsigned vertex_count{},receipts{},kind_mask{};std::array<unsigned,3> kind_vertices{};std::uint64_t unknown_vertices{};
};
struct UiIndexContext {
    std::uint64_t execute_sequence{};gears_ui_indices::Snapshot snapshot{};unsigned ordinal{};bool callback_valid{};
    std::array<gears_ui_reticle::Extent,gears_ui_indices::segment_capacity> reticle_extents{};
};
static thread_local UiIndexContext* ui_index_context{};
struct UiPayloadGpuRow {
    UiPayloadContext context{};std::uint64_t ticket{};gears_frame_link::ListRef list{};gears_frame_link::Target target{};
    long long qpc{};unsigned thread{},indices{},instances{},start{};int vertex{};std::uint64_t execute_sequence{};
    gears_ui_indices::Segment segment{};unsigned segment_ordinal{};bool index_callback_matched{},command_completed{};
    gears_vr::StereoTicketGeometry geometry{};bool geometry_matched{},hud_plane_applied{};
    gears_viewport::OverlayPlan hud_plan{};bool reticle_direction_applied{};gears_viewport::OverlayPlan reticle_plan{};
    gears_ui_reticle::Extent reticle_extent{};
};
static bool geometry_available=true;
[[maybe_unused]] static bool ui_ticket_geometry(std::uint64_t,gears_vr::StereoTicketGeometry& out){out={};return geometry_available;}
#include "reticle_sample_live.inl"
struct OwnFile {
    wchar_t path[MAX_PATH]{};
    OwnFile(){wchar_t temp[MAX_PATH]{};assert(GetTempPathW(MAX_PATH,temp));assert(GetTempFileNameW(temp,L"grt",0,path));}
    ~OwnFile(){DeleteFileW(path);}
    void write(std::string const& value){HANDLE f=CreateFileW(path,GENERIC_WRITE,0,nullptr,TRUNCATE_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);assert(f!=INVALID_HANDLE_VALUE);
        DWORD n{};assert(WriteFile(f,value.data(),DWORD(value.size()),&n,nullptr)&&n==value.size());CloseHandle(f);}
    std::string read(){HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);assert(f!=INVALID_HANDLE_VALUE);
        char bytes[4096]{};DWORD n{};assert(ReadFile(f,bytes,sizeof(bytes),&n,nullptr));CloseHandle(f);return std::string(bytes,n);}
};
static std::string log_text(FILE* file){assert(fflush(file)==0);assert(fseek(file,0,SEEK_SET)==0);std::string result;char block[2048];
    while(auto n=fread(block,1,sizeof(block),file))result.append(block,n);assert(fseek(file,0,SEEK_END)==0);return result;}
#if defined(GEARS_UI_RETICLE_LATE_SAMPLE) && GEARS_UI_RETICLE_LATE_SAMPLE
static std::string request(){const auto id=ui_reticle_late_sample::identity();LARGE_INTEGER hz{};QueryPerformanceFrequency(&hz);const auto t=now();
    return "[ReticleSample]\nschema=1\npid="+std::to_string(id.pid)+"\ncreation_time="+std::to_string(id.creation_time)+"\nbase="+std::to_string(id.base)+
        "\nstarted_qpc="+std::to_string(id.started_qpc)+"\nworld_generation="+std::to_string(id.world_generation)+"\nnonce=77\nrequest_qpc="+std::to_string(t)+"\nexpires_qpc="+std::to_string(t+hz.QuadPart)+"\n";}
static void set_geometry(UiIndexContext& index,UiPayloadContext& context,unsigned quads){
    index={};context={};index.callback_valid=true;index.execute_sequence=90+quads;auto& s=index.snapshot;
    s.payload=100+quads;s.generation=7;s.vertex_count=quads*4;s.segment_count=1;s.total_indices=quads*6;
    auto& segment=s.segments[0];segment.minimum=0;segment.maximum=int(s.vertex_count-1);segment.index_count=s.total_indices;
    segment.kind_mask=1;segment.entry_mask=(1u<<quads)-1;segment.known[0]=s.total_indices;segment.expected={s.total_indices,1,0,0,0};
    const unsigned q[6]{0,1,2,1,3,2};for(unsigned i=0;i<quads;++i)for(unsigned j=0;j<6;++j)s.indices[i*6+j]=s.global_vertices[i*6+j]=i*4+q[j];
    context.payload=s.payload;context.generation=s.generation;context.root_first=context.root_last=60;
    context.vertex_count=quads*4;context.receipts=quads;context.kind_mask=1;context.kind_vertices[0]=quads*4;
    index.reticle_extents[0]={true,1900,1060,1940,1100,60};
}
#endif
int main(){session_started_qpc=now();FILE* status{};assert(tmpfile_s(&status)==0&&status);OwnFile arm;
    assert(ui_reticle_late_sample::configure(false)&&!ui_reticle_late_sample::enabled);
    arm.write("unknown unrelated contents");const auto original=arm.read();
    ui_reticle_late_sample::poll(arm.path,status);assert(log_text(status).empty()); // runtime default OFF
#if defined(GEARS_UI_RETICLE_LATE_SAMPLE) && GEARS_UI_RETICLE_LATE_SAMPLE
    assert(ui_reticle_late_sample::configure(true));ui_reticle_late_sample::poll(arm.path,status);
    assert(!ui_reticle_late_sample::bank.gate.accepted&&arm.read()==original);
    const auto wire=request();arm.write(wire);ui_reticle_late_sample::poll(arm.path,status);
    auto& bank=ui_reticle_late_sample::bank;assert(bank.gate.acknowledged&&bank.count==0&&arm.read()==wire);
    const auto log=log_text(status);assert(log.find("ui_reticle_late_ack schema=1 nonce=77")!=std::string::npos);
    ui_reticle_late_sample::poll(arm.path,status);assert(log_text(status)==log); // same file never rearmed
    UiIndexContext index{};UiPayloadContext context{};ui_index_context=&index;
    gears_viewport::OverlayPlan plan{};plan.valid=true;plan.viewports={{{10,20,30,40},{50,60,30,40}}};plan.scissors=plan.viewports;
    const auto observe=[&](unsigned quads,unsigned ticket,bool applied,std::uint64_t t){set_geometry(index,context,quads);
        ui_reticle_late_sample::observe(context,ticket,{123,4},{456,3840,2160},t,quads*6,1,0,0,applied,applied?&plan:nullptr);};
    observe(4,100,true,bank.gate.arm_qpc+1);observe(1,100,true,bank.gate.arm_qpc+2);
    assert(bank.count==2&&bank.ticket_count==1&&bank.rows[0].context.vertex_count==16&&bank.rows[1].context.vertex_count==4);
    assert(bank.rows[0].reticle_plan.viewports==bank.rows[1].reticle_plan.viewports);
    geometry_available=false;observe(1,101,true,bank.gate.arm_qpc+3);assert(bank.count==2);geometry_available=true;
    observe(1,101,false,bank.gate.arm_qpc+4);assert(bank.count==3&&!bank.rows[2].reticle_direction_applied);
    set_geometry(index,context,1);index.snapshot.segments[0].known[0]=5;index.snapshot.segments[0].unknown=1;
    ui_reticle_late_sample::observe(context,102,{123,4},{456,3840,2160},bank.gate.arm_qpc+5,6,1,0,0,true,&plan);assert(bank.count==3);
    observe(1,103,true,bank.gate.end_qpc);assert(bank.count==3&&bank.gate.frozen);
    ui_reticle_late_sample::complete(94,true);ui_reticle_late_sample::complete(91,false);
    assert(bank.rows[0].command_completed&&!bank.rows[1].command_completed);
    ui_reticle_late_sample::print(status);const auto printed=log_text(status);
    assert(printed.find("ui_reticle_late_index_row bank=late nonce=77 kind_mask=1 row=0")!=std::string::npos);
    assert(printed.find("command_completed=1 geometry_matched=1")!=std::string::npos);
#else
    assert(!ui_reticle_late_sample::configure(true)&&!ui_reticle_late_sample::enabled);ui_reticle_late_sample::poll(arm.path,status);
    assert(log_text(status).empty()&&arm.read()==original);ui_reticle_late_sample::print(status);
    assert(log_text(status).find("compiled=0 enabled=0")!=std::string::npos);
#endif
    fclose(status);std::puts("ReticleSampleLiveTest passed: actual poll, armfile preservation, defaults/ACK and actual observer/completion path");
}
