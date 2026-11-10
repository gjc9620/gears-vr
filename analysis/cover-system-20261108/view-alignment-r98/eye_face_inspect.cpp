// Offline extension of the frozen eye reader. No process, view or pose writes.
#define NOMINMAX
#include <windows.h>
#ifdef small
#undef small
#endif
#define main OriginalFrozenEyeInspectorMain
#include "../view-alignment-r80/eye_trace_inspect.cpp"
#undef main
#include "body_head_authored_mount.h"
#include <bcrypt.h>
#include <memory>

namespace body=gears_full_body_c0;
namespace marker=gears_body_head_authored;
bool digest(void const* bytes,size_t count,body::rig::Signature& result){
    BCRYPT_ALG_HANDLE algorithm{};BCRYPT_HASH_HANDLE hash{};
    bool okay=count<=ULONG_MAX&&BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(count),0)>=0;
    if(okay)okay=BCryptFinishHash(hash,result.data(),ULONG(result.size()),0)>=0;
    if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);return okay;
}
gears_vr::Vector minus(gears_vr::Vector a,gears_vr::Vector b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
double norm(gears_vr::Vector a){return std::hypot(a.x,a.y,a.z);}
bool unit(gears_vr::Vector v,gears_vr::Vector& out){
    const auto n=norm(v);if(!std::isfinite(n)||n<1e-8)return false;out={v.x/n,v.y/n,v.z/n};return true;
}
double degrees(gears_vr::Vector a,gears_vr::Vector b){
    gears_vr::Vector x{},y{};if(!unit(a,x)||!unit(b,y))return NAN;
    return std::acos(std::clamp(x.x*y.x+x.y*y.y+x.z*y.z,-1.,1.))*57.2957795130823208768;
}
int main(int argc,char** argv){
    static_assert(sizeof(trace::Row)==16120);
    if(argc!=2)return 2;
    std::ifstream in(argv[1],std::ios::binary);trace::Header header{};std::vector<trace::Row> rows;
    if(!read_trace(in,header,rows))return 2;
    auto model=std::make_unique<body::Model>();auto frame=std::make_unique<body::Frame>();
    auto work=std::make_unique<marker::Workspace>();marker::Marker mount{};
    unsigned matched{},rejected{};std::cout<<std::setprecision(12);
    std::cout<<"{\"offline_only\":true,\"cpu_read_bracket_only\":true,\"final_hmd_view_proven\":false,\"gpu_draw_proven\":false,\"rows\":[";
    for(unsigned i=0;i<rows.size();++i){
        auto const& row=rows[i];auto const& g=row.geometry;
        if(i)std::cout<<',';
        std::cout<<"{\"index\":"<<i<<",\"scene\":"<<row.scene<<",\"pawn\":"<<row.pawn.address
                 <<",\"world_generation\":"<<g.world_generation<<",\"begin_qpc\":"<<row.begin_qpc<<",\"end_qpc\":"<<row.end_qpc;
        frame->count=body::bones;frame->units_per_metre=row.units_per_metre;frame->model_scale=1.;
        static_assert(sizeof(frame->reference)==sizeof(g.reference));
        std::memcpy(frame->reference.data(),g.reference.data(),sizeof(g.reference));
        bool okay=eligible(row)&&body::prepare_model(frame->reference,digest,*model)&&
            marker::prepare(*model,*frame,marker::Basis::marcus_mesh_face_forward_y,*work,mount)==marker::Status::okay;
        const auto local=mount.head_to_eye_local();
        const auto mount_error=std::hypot(local.position.x-g.mount[0],local.position.y-g.mount[1],local.position.z-g.mount[2]);
        okay=okay&&std::isfinite(mount_error)&&mount_error<1e-8;
        const auto root=point(point({0,0,0},g.space[marker::head_bone]),g.component);
        const auto face=body::d::rotate(local.orientation,{1,0,0});
        gears_vr::Vector world{};
        okay=okay&&unit(minus(point(point({face.x,face.y,face.z},g.space[marker::head_bone]),g.component),root),world);
        if(okay){
            ++matched;
            const gears_vr::Vector view{row.prepared_view[2],row.prepared_view[6],row.prepared_view[10]};
            const auto left=point(point({0,0,0},g.space[marker::left_eye_bone]),g.component);
            const auto right=point(point({0,0,0},g.space[marker::right_eye_bone]),g.component);
            const gears_vr::Vector middle{(left.x+right.x)*.5,(left.y+right.y)*.5,(left.z+right.z)*.5};
            std::cout<<",\"authored_basis_and_mount_matched\":true,\"authored_face_forward_world\":";vector(world);
            std::cout<<",\"prepared_base_forward\":";vector(view);
            std::cout<<",\"base_vs_authored_face_degrees\":";number(degrees(world,view));
            std::cout<<",\"horizontal_base_vs_authored_face_degrees\":";number(degrees({world.x,world.y,0},{view.x,view.y,0}));
            std::cout<<",\"current_eye_bones_midpoint_world\":";vector(middle);
            std::cout<<",\"prepared_eye_minus_actual_eye_midpoint\":";vector(minus(row.eye,middle));
            std::cout<<",\"mount_error\":";number(mount_error);
        }else{++rejected;std::cout<<",\"authored_basis_and_mount_matched\":false";}
        std::cout<<'}';
    }
    std::cout<<"],\"count\":"<<rows.size()<<",\"matched\":"<<matched<<",\"rejected\":"<<rejected<<"}\n";
    return matched?0:3;
}
