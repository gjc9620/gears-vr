#include "../vr-common/pose_editor.h"
namespace pose_editor_client {
namespace api=gears_pose_editor;
HMODULE owner{};api::Read reader{};api::Report reporter{};
api::Config retained[6]{}; // Static authoring only; never retained input/permission.
void connect(HMODULE native_owner) noexcept {
    owner=nullptr;reader=nullptr;reporter=nullptr;for(auto& saved:retained)saved={};if(!native_owner)return;
    auto r=GetProcAddress(native_owner,api::read_export),p=GetProcAddress(native_owner,api::report_export);
    if(!r||!p||!weaponhand::executable(r,native_owner)||!weaponhand::executable(p,native_owner))return;
    reader=reinterpret_cast<api::Read>(r);reporter=reinterpret_cast<api::Report>(p);owner=native_owner;
}
bool read(api::Config& out,unsigned part=0) noexcept {
    if(part>5)return false;
    if(!owner||owner!=weaponhand::native_module||!reader)return false;
    __try {
        api::Config current{};
        if(reader(part,&current)&&api::valid(current))retained[part]=current;
        out=retained[part];return api::valid(out);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void receipt(std::uint64_t revision,std::uint64_t frame,unsigned kind=2) noexcept {
    if(!revision||!owner||owner!=weaponhand::native_module||!reporter)return;
    __try {reporter(revision,kind,frame);}
    __except(EXCEPTION_EXECUTE_HANDLER){}
}
}
