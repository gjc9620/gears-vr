#include "weapon_boltok_grasp_pose.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace api=gears_hand_authored_grasp;namespace pose=gears_boltok_grasp_pose;
unsigned checks{};
#define C(x) do{++checks;if(!(x)){std::printf("FAIL line%u %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    // Independent original68-byte reference records, not a copy of this table.
    FILE* input{};C(fopen_s(&input,"analysis/solo-m1-20261028/observed-backend-fixture/reference.bin","rb")==0&&input);
    std::array<unsigned char,115*68> bytes{};C(std::fread(bytes.data(),1,bytes.size(),input)==bytes.size());C(std::fgetc(input)==EOF);std::fclose(input);
    api::Binding binding{};binding.geometry_signature=gears_hand_authored_mount::marcus115_signature;
    for(unsigned j=0;j<15;++j){float q[4]{};std::memcpy(q,bytes.data()+(36+j)*68+12,sizeof q);binding.reference[j]={q[0],q[1],q[2],q[3]};}
    gears_weapon_hand::Frame source{};api::Request request{};C(pose::main_request(source,request));C(request.mode==api::Mode::right_main_fingers&&request.side==api::Side::right&&api::identity(request.grip));
    C(request.profile_id==pose::main_profile&&api::reference_matches(request,binding));
    api::Joints rest{},trigger{};C(api::rotations(request,binding,0,rest));C(api::rotations(request,binding,1,trigger));
    double index_change{};
    for(unsigned j=0;j<15;++j){
        C(api::unit(rest[j])&&api::unit(trigger[j]));
        const double dot=std::abs(rest[j].x*trigger[j].x+rest[j].y*trigger[j].y+rest[j].z*trigger[j].z+rest[j].w*trigger[j].w);
        if(j<3)index_change+=1-dot;else C(dot>1-1e-6);
    }
    C(index_change>.01);auto wrong=binding;wrong.geometry_signature[31]^=1;C(!api::reference_matches(request,wrong));
    wrong=binding;wrong.reference[14]=gears_vr::normalized({.001,0,0,1});C(!api::reference_matches(request,wrong));
    C(std::memcmp(&source,&request.source,sizeof source)==0);
    std::printf("Boltok main skin draft: %u independent original-rig/reference/trigger-data checks. No source admission, wrist fitting or pixel proof\n",checks);
}
