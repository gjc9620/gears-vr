#include "../vr-common/hand_authored_mount_profile.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <filesystem>
#ifdef NDEBUG
#error Profile checks require assertions enabled
#endif
namespace mount=gears_hand_authored_mount;
namespace wrist=gears_hand_wrist;
namespace vr=gears_vr;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %u: %s\n",__LINE__,#x);}}while(0)
bool near(double a,double b,double tolerance=2e-11){return std::abs(a-b)<=tolerance;}
bool near(vr::Vector a,vr::Vector b){return near(a.x,b.x)&&near(a.y,b.y)&&near(a.z,b.z);}
bool equivalent(vr::Quaternion a,vr::Quaternion b){
    a=vr::normalized(a);b=vr::normalized(b);return std::abs(a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w)>1-1e-12;
}
vr::Pose compose(vr::Pose const& a,vr::Pose const& b){
    const auto d=vr::transform(vr::rotation_matrix(a.orientation),b.position);
    return {{a.position.x+d.x,a.position.y+d.y,a.position.z+d.z},vr::multiply(a.orientation,b.orientation)};
}
double determinant(vr::Matrix const& m){return
    m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])-m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])+m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);}
bool read_exact(wchar_t const* path,void* destination,std::size_t size){
    FILE* file{};if(_wfopen_s(&file,path,L"rb")||!file)return false;
    const auto count=fread(destination,1,size,file);const auto trailing=fgetc(file);fclose(file);return count==size&&trailing==EOF;
}
int wmain(int argc,wchar_t** argv){
    C(argc==4);if(argc!=4)return 1;
    std::array<mount::ReferenceRecord68,115> reference{};double marker_data[6]{};
    C(read_exact(argv[1],reference.data(),sizeof reference));C(read_exact(argv[2],marker_data,sizeof marker_data));
    mount::CanonicalGeometry canonical{};C(mount::canonical_geometry(reference.data(),reference.size(),canonical));
    C(!std::filesystem::exists(argv[3]));FILE* file{};C(_wfopen_s(&file,argv[3],L"wb")==0&&file!=nullptr);
    if(file){C(fwrite(canonical.data(),1,canonical.size(),file)==canonical.size());C(fclose(file)==0);}
    for(auto side:{mount::Side::left,mount::Side::right}){
        const unsigned index=side==mount::Side::left?0u:1u;
        const vr::Vector marker{marker_data[index*3],marker_data[index*3+1],marker_data[index*3+2]};
        for(double units:{50.,100.,200.,500.})for(double scale:{.5,1.,2.}){
            mount::Mount profile{};C(mount::marcus115(mount::marcus115_signature,side,scale,units,profile));
            C(profile.model==mount::Model::marcus115&&profile.provenance==mount::Provenance::designer_authored&&profile.calibration.known);
            C(profile.input_space==mount::Space::ue_grip_forward_right_up&&profile.side==side&&profile.bone_count==115);
            C(profile.hand_bone==(index?35u:11u));
            const auto rotation=vr::rotation_matrix(profile.calibration.grip_to_wrist_rotation);
            C(near(determinant(rotation),1));
            C(near(vr::transform(rotation,{1,0,0}),{0,0,1}));
            C(near(vr::transform(rotation,{0,1,0}),{0,index?-1.:1.,0}));
            C(near(vr::transform(rotation,{0,0,1}),{index?1.:-1.,0,0}));
            const auto centre=vr::transform(rotation,marker);const auto& p=profile.calibration.grip_to_wrist_metres;
            C(near(vr::Vector{p.x*units,p.y*units,p.z*units},{-centre.x*scale,-centre.y*scale,-centre.z*scale}));
            for(unsigned trial=0;trial<9;++trial){
                const double a=double(trial)*.31;const auto world_q=vr::normalized(vr::Quaternion{std::sin(a),.2*std::cos(a),-.3,std::cos(a)});
                const vr::Pose wrist_world{{11+trial*7.,-33+trial*3.,55-trial*2.},world_q};
                const vr::Pose marker_local{{marker.x*scale,marker.y*scale,marker.z*scale},vr::inverse(profile.calibration.grip_to_wrist_rotation)};
                const auto grip_world=compose(wrist_world,marker_local);vr::Pose recovered{};
                C(wrist::native_wrist_target(grip_world,profile.calibration,units,recovered)==wrist::Status::okay);
                C(near(recovered.position,wrist_world.position)&&equivalent(recovered.orientation,wrist_world.orientation));
            }
        }
        mount::Mount a{},b{};C(mount::marcus115(mount::marcus115_signature,side,1,100,a));
        C(mount::marcus115(mount::marcus115_signature,side,1,200,b));vr::Pose wa{},wb{};
        const vr::Pose grip{{33,-14,62},vr::normalized(vr::Quaternion{.1,.2,.3,.9})};
        C(wrist::native_wrist_target(grip,a.calibration,100,wa)==wrist::Status::okay);
        C(wrist::native_wrist_target(grip,b.calibration,200,wb)==wrist::Status::okay);
        C(near(wa.position,wb.position)&&equivalent(wa.orientation,wb.orientation));
    }
    mount::Mount output{};std::memset(&output,0x5a,sizeof output);unsigned char saved[sizeof output];std::memcpy(saved,&output,sizeof output);
    auto signature=mount::marcus115_signature;signature[0]^=1;
    C(!mount::marcus115(signature,mount::Side::left,1,100,output));C(!memcmp(saved,&output,sizeof output));
    for(auto side:{mount::Side::unknown,static_cast<mount::Side>(7)}){
        C(!mount::marcus115(mount::marcus115_signature,side,1,100,output));C(!memcmp(saved,&output,sizeof output));}
    for(double scale:{0.,-1.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        C(!mount::marcus115(mount::marcus115_signature,mount::Side::left,scale,100,output));C(!memcmp(saved,&output,sizeof output));}
    for(double units:{0.,9.,501.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        C(!mount::marcus115(mount::marcus115_signature,mount::Side::left,1,units,output));C(!memcmp(saved,&output,sizeof output));}
    mount::CanonicalGeometry unchanged{};unchanged.fill(0x5a);const auto original=unchanged;
    C(!mount::canonical_geometry(nullptr,115,unchanged)&&unchanged==original);
    C(!mount::canonical_geometry(reference.data(),114,unchanged)&&unchanged==original);
    C(!mount::canonical_geometry(reference.data(),116,unchanged)&&unchanged==original);
    auto bad=reference;const std::int32_t self=12;memcpy(bad[12].bytes+56,&self,4);
    C(!mount::canonical_geometry(bad.data(),115,unchanged)&&unchanged==original);
    bad=reference;const float nan=std::numeric_limits<float>::quiet_NaN();memcpy(bad[12].bytes+28,&nan,4);
    C(!mount::canonical_geometry(bad.data(),115,unchanged)&&unchanged==original);
    bad=reference;std::memset(bad[12].bytes+12,0,16);
    C(!mount::canonical_geometry(bad.data(),115,unchanged)&&unchanged==original);
    auto names_changed=reference;for(auto& record:names_changed)std::memset(record.bytes,0xaf,8);
    mount::CanonicalGeometry independent_names{};C(mount::canonical_geometry(names_changed.data(),115,independent_names)&&independent_names==canonical);
    const auto alias_before=reference;
    C(!mount::canonical_geometry(reference.data(),115,*reinterpret_cast<mount::CanonicalGeometry*>(reference.data())));
    C(!memcmp(reference.data(),alias_before.data(),sizeof reference));
    printf("hand_authored_mount_profile: %u checks, %u failures; static/RO3 data, authored default, no physical measurement/Game/XR\n",checks,failures);
    return failures?1:0;
}
