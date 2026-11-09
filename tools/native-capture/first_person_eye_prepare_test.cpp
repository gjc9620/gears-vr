// Actual production prepare, with explicit CPU seams for current owner/eye.
// No game, XR tracking, physical pose, pixel or runtime admission is claimed.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include "first_person_view.h"
#include "first_person_weapon.h"
#include "anchor_observer.h"
bool simulator_operator=true,continuous_session=true;std::atomic<bool> recording{};std::atomic<unsigned> active{};
enum class SessionPhase:unsigned {running,stop_requested,native_closed,draining,stopped};
std::atomic<SessionPhase> session_phase{SessionPhase::running};
wchar_t directory[MAX_PATH]{};
gears_anchor::Config anchor_config{};double stereo_units_per_metre=100;
unsigned foreground(){return 0;}long long now(){static long long n=100;return ++n;}
// Existing initialize() loop shadows its event-name local. The test keeps
// that unrelated production initializer intact; other warnings remain /WX.
#pragma warning(push)
#pragma warning(disable:4456)
#include "first_person_live.inl"
#pragma warning(pop)
namespace fp=first_person_live;namespace trace=gears_first_person_eye_trace;
unsigned ordinary_calls{},observed_calls{};bool eye_ok=true,evidence_ok=true;
namespace first_person_live {
bool authored_eye(gears_anchor::Receipt const*,gears_vr::Vector& out){++ordinary_calls;if(eye_ok)out={1,2,3};return eye_ok;}
bool authored_eye_observed(gears_anchor::Receipt const* receipt,gears_vr::Vector& out,trace::Geometry* diagnostic){
    ++observed_calls;*diagnostic={};diagnostic->world_generation=receipt->invocation+1;
    diagnostic->status=evidence_ok?trace::Status::ready:trace::Status::read_failed;
    diagnostic->owner_closed=unsigned(eye_ok);if(eye_ok)out={1,2,3};return eye_ok;
}
}
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %d %s\n",__LINE__,#x);}}while(0)
int main(){
    fp::enabled=fp::body_basis=true;fp::toggle.initialize(true);anchor_config.cinematic_enabled=true;
    gears_anchor::Receipt receipt{};receipt.invocation=292;receipt.thread=GetCurrentThreadId();
    receipt.scene_qpc=1;receipt.family_qpc=2;receipt.renderer_qpc=3;
    receipt.anchor.pawn=receipt.anchor.target_identity.address=0x12300;
    receipt.anchor.pawn_identity.address=0x12300;receipt.key.family=10;receipt.key.source=20;receipt.key.local=30;
    const gears_vr::StereoMatrix incoming{0,0,1,0,1,0,0,0,0,1,0,0,8,9,10,1};
    auto view=incoming;C(fp::prepare(&receipt,view));C(ordinary_calls==1&&observed_calls==0&&fp::eye_journal.count==0);
    const auto expected=view;fp::eye_trace_enabled=true;
    for(unsigned test=0;test<4;++test){
        fp::eye_journal.count=0;fp::eye_journal.sampled=false;
        eye_ok=test!=2;evidence_ok=test!=1;view=incoming;
        if(test==3)view[0]=NAN;
        const bool result=fp::prepare(&receipt,view);
        C(result==(test<2));C(fp::eye_journal.count==1);
        auto const& row=fp::eye_journal.rows[0];
        C(row.scene==292&&row.thread==receipt.thread&&row.scene_qpc==1&&row.family_qpc==2&&row.renderer_qpc==3);
        C(row.family==10&&row.source==20&&row.local==30&&row.begin_qpc<row.end_qpc);
        C(row.geometry.world_generation==293&&row.eye_accepted==unsigned(eye_ok)&&row.view_prepared==unsigned(test<2));
        if(test<2){C(view==expected&&row.incoming_view==incoming&&row.prepared_view==view);C(row.eye.x==1&&row.eye.y==2&&row.eye.z==3);}
        if(test==1)C(row.geometry.status==trace::Status::read_failed);
        if(test==2)C(view==incoming&&!row.geometry.owner_closed);
    }
    C(ordinary_calls==1&&observed_calls==4);
    // The production exporter is inert while admission/recording is active;
    // its drained path writes a file with the checked version/row count.
    wchar_t temporary[MAX_PATH]{},folder[MAX_PATH]{},artifact[MAX_PATH]{};
    C(GetTempPathW(MAX_PATH,temporary)>0&&GetTempFileNameW(temporary,L"eye",0,folder));
    C(DeleteFileW(folder)&&CreateDirectoryW(folder,nullptr));wcscpy_s(directory,folder);
    swprintf_s(artifact,L"%s\\authored-eye-geometry.bin",folder);
    FILE* log{};C(!tmpfile_s(&log)&&log);
    if(log){
        fp::print_eye_trace(log);C(!fp::eye_trace_exported&&GetFileAttributesW(artifact)==INVALID_FILE_ATTRIBUTES);
        session_phase=SessionPhase::draining;active=1;fp::print_eye_trace(log);C(!fp::eye_trace_exported);
        active=0;recording=true;fp::print_eye_trace(log);C(!fp::eye_trace_exported);
        recording=false;fp::print_eye_trace(log);C(fp::eye_trace_exported&&fp::eye_trace_export_failures==0);
        FILE* exported{};C(!_wfopen_s(&exported,artifact,L"rb")&&exported);
        if(exported){trace::Header header{};C(fread(&header,sizeof header,1,exported)==1&&header.count==1&&header.row_bytes==sizeof(trace::Row));C(!fclose(exported));}
        C(!fclose(log));
    }
    C(DeleteFileW(artifact)&&RemoveDirectoryW(folder));
    printf("first_person_eye_prepare: %u checks, %u failures; production prepare with CPU eye seam\n",checks,failures);
    return failures?1:0;
}
