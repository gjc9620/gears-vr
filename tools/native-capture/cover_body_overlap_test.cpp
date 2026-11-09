#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "cover_body_overlap.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
namespace p=gears_cover_body_overlap;namespace g=p::geometry;namespace w=g::world;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool hash(void const* bytes,std::size_t size,g::body::rig::Signature& out){
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE h{};
    bool okay=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(okay)okay=BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0;
    if(okay)okay=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;
    if(okay)okay=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(alg)BCryptCloseAlgorithmProvider(alg,0);return okay;
}
bool read(std::string const& path,void* data,std::size_t size){std::ifstream file(path,std::ios::binary);
    return bool(file.read(static_cast<char*>(data),std::streamsize(size)))&&file.peek()==EOF;}
struct Fixture {
    g::body::Model model{};g::body::Frame frame{};g::names::Mapping mapping{};
    std::array<g::Inventory,8> inventory{};std::array<g::Shape,g::max_primitives> authored{};
    g::Workspace workspace{};g::Geometry geometry{};
    bool init(std::string const& path){
        if(!read(path+"/reference.bin",frame.reference.data(),sizeof frame.reference)||
           !read(path+"/C0-local.bin",frame.local.data(),sizeof frame.local)||
           !read(path+"/C0-component.bin",&frame.component_to_world,sizeof frame.component_to_world)||
           !g::body::prepare_model(frame.reference,hash,model))return false;
        const unsigned selected[]={55,11,1,10,8,91,92,93};std::array<g::names::Input,8> inputs{};
        for(unsigned i=0;i<8;++i)inputs[i]={i,g::names::detail::name(frame.reference[selected[i]])};
        if(g::names::bind(model,frame,inputs.data(),8,2,mapping)!=g::names::Status::okay)return false;
        unsigned at=0;
        for(unsigned b=0;b<8;++b){inventory[b].count={2u+(b<5),2,2,0};
            for(unsigned k=0;k<3;++k)for(unsigned n=0;n<inventory[b].count[k];++n){
                auto& row=authored[at++];auto& s=row.authored;row.array_index=n;
                s.body_index=b;s.bone=mapping.rows[b].name;s.kind=static_cast<g::metrics::Kind>(k);
                s.receiver_scale={1,1,1};s.raw_dimensions=k==0?std::array<float,3>{3,0,0}:
                    k==1?std::array<float,3>{3,4,5}:std::array<float,3>{3,8,0};
                s.tm.values[0]=s.tm.values[5]=s.tm.values[10]=s.tm.values[15]=1;
                s.tm.values[12]=float(n+1);s.tm.values[13]=float(b);s.tm.values[14]=3;
            }
        }
        return at==53&&g::convert(model,frame,mapping,inventory.data(),8,authored.data(),at,workspace,geometry)==w::Code::converted;
    }
    p::Code run(p::Query const& query,p::Result& output){return p::classify(mapping,inventory.data(),8,geometry,query,output);}
};
std::uint32_t seed=0x166611u;
int random_int(int lo,int count){seed=seed*1664525u+1013904223u;return lo+int(seed%unsigned(count));}
long long gap(int lo,int hi,int other_lo,int other_hi){return hi<other_lo?other_lo-hi:lo>other_hi?lo-other_hi:0;}
// Integer lattice fixtures are unrelated to the saved C0. They allow a true
// per-shape union oracle independent of the production distance/SAT functions.
void lattice(Fixture& f,std::array<bool,g::max_primitives>& inside,std::array<bool,g::max_primitives>& strict,
             p::Query const& query){
    f.geometry={};f.geometry.body_count=8;unsigned at=0;
    const int lo[]={int(query.minimum.x),int(query.minimum.y),int(query.minimum.z)};
    const int hi[]={int(query.maximum.x),int(query.maximum.y),int(query.maximum.z)};
    for(unsigned b=0;b<8;++b){f.inventory[b].count={2,2,2,0};
        for(unsigned k=0;k<3;++k)for(unsigned n=0;n<2;++n){
            auto& row=f.geometry.rows[at];auto& s=row.primitive;row.array_index=n;
            s.body_index=b;s.bone_index=f.mapping.rows[b].bone_index;s.bone=f.mapping.rows[b].name;
            s.shape_world.orientation={0,0,0,1};s.dimensions.kind=static_cast<g::metrics::Kind>(k);
            const int c[]={random_int(-25,51),random_int(-25,51),random_int(-25,51)};
            s.shape_world.position={double(c[0]),double(c[1]),double(c[2])};
            const int radius=random_int(1,5),half=random_int(0,7),axis=random_int(0,3);
            if(k==1){const int ext[]={random_int(1,6),random_int(1,6),random_int(1,6)};
                bool hit=true,deep=true;
                for(unsigned j=0;j<3;++j){s.dimensions.box_half_extents[j]=float(ext[j]);
                    hit&=c[j]+ext[j]>=lo[j]&&c[j]-ext[j]<=hi[j];
                    deep&=c[j]+ext[j]>lo[j]&&c[j]-ext[j]<hi[j];}
                inside[at]=hit;strict[at]=deep;
            }else{
                s.dimensions.radius=float(radius);long long squared=0;
                if(k==2){s.dimensions.half_segment=float(half);
                    if(axis==0)s.segment_axis_world.x=1;else if(axis==1)s.segment_axis_world.y=1;else s.segment_axis_world.z=1;
                    s.segment_start=s.segment_end=s.shape_world.position;
                    if(axis==0){s.segment_start.x-=half;s.segment_end.x+=half;}
                    else if(axis==1){s.segment_start.y-=half;s.segment_end.y+=half;}
                    else{s.segment_start.z-=half;s.segment_end.z+=half;}
                }
                for(unsigned j=0;j<3;++j){const int extent=k==2&&j==unsigned(axis)?half:0;
                    const auto d=gap(c[j]-extent,c[j]+extent,lo[j],hi[j]);squared+=d*d;}
                inside[at]=squared<=radius*radius;strict[at]=squared<radius*radius;
            }
            ++at;
        }
    }
    f.geometry.count=at;
}
int main(int argc,char** argv){if(argc!=2)return 2;
    auto f=std::make_unique<Fixture>();C(f->init(argv[1]));if(failures)return 1;
    const p::Query distant{{1e6,1e6,1e6},{1e6+1,1e6+1,1e6+1}},all{{-1e6,-1e6,-1e6},{1e6,1e6,1e6}};
    p::Result out{};C(f->run(distant,out)==p::Code::evaluated);
    C(out.relation==p::Relation::separated&&out.evaluated==53&&out.separated==53);
    C((out.per_kind==std::array<unsigned,3>{21,16,16}));
    C(f->run(all,out)==p::Code::evaluated);C(out.relation==p::Relation::overlapping&&out.evaluated==53&&out.overlapping==53);
    C(out.first_overlap==0&&out.first_indeterminate==p::none);
    // Query each converted shape's own center: a mixed complete inventory must
    // report intersection, without losing any other shape from the check.
    for(unsigned i=0;i<f->geometry.count;++i){const auto c=f->geometry.rows[i].primitive.shape_world.position;
        C(f->run({c,c},out)==p::Code::evaluated);C(out.relation==p::Relation::overlapping&&out.evaluated==53);}
    const auto actual_mapping=f->mapping;auto actual_geometry=std::make_unique<g::Geometry>(f->geometry);
    const auto actual_inventory=f->inventory;
    for(unsigned trial=0;trial<120;++trial){const int lo[]={random_int(-20,31),random_int(-20,31),random_int(-20,31)};
        const p::Query query{{double(lo[0]),double(lo[1]),double(lo[2])},
            {double(lo[0]+random_int(0,12)),double(lo[1]+random_int(0,12)),double(lo[2]+random_int(0,12))}};
        std::array<bool,g::max_primitives> inside{},strict{};lattice(*f,inside,strict,query);
        bool any=false,deep=false;unsigned outside=0;
        for(unsigned i=0;i<f->geometry.count;++i){any|=inside[i];deep|=strict[i];outside+=!inside[i];}
        const auto map_before=f->mapping;const auto counts_before=f->inventory;
        auto shape_before=std::make_unique<g::Geometry>(f->geometry);
        C(f->run(query,out)==p::Code::evaluated);C(out.evaluated==48&&out.separated==outside);
        C(out.separated+out.overlapping+out.indeterminate==48);C((out.per_kind==std::array<unsigned,3>{16,16,16}));
        if(deep)C(out.relation==p::Relation::overlapping);
        else if(any)C(out.relation!=p::Relation::separated);
        else C(out.relation==p::Relation::separated);
        C(out.first_overlap==p::none||inside[out.first_overlap]);
        C(out.first_indeterminate==p::none||(inside[out.first_indeterminate]&&!strict[out.first_indeterminate]));
        C(!std::memcmp(&map_before,&f->mapping,sizeof map_before)&&!std::memcmp(&counts_before,&f->inventory,sizeof counts_before)&&
            !std::memcmp(shape_before.get(),&f->geometry,sizeof f->geometry));
    }
    // Preserve public output even when a first shape overlaps but the last row
    // is corrupt. No successful prefix may stand in for the complete inventory.
    for(unsigned defect=0;defect<19;++defect){f->mapping=actual_mapping;f->geometry=*actual_geometry;f->inventory=actual_inventory;
        p::Code wanted=p::Code::inventory;auto& last=f->geometry.rows[f->geometry.count-1].primitive;
        switch(defect){
        case 0:f->inventory[0].count[0]--;break;
        case 1:f->inventory[0].count[0]++;break;
        case 2:f->geometry.count--;break;
        case 3:f->geometry.body_count=7;break;
        case 4:std::swap(f->geometry.rows[0],f->geometry.rows[1]);break;
        case 5:f->geometry.rows[52].array_index=99;break;
        case 6:f->inventory[7].count[3]=1;wanted=p::Code::unsupported;break;
        case 7:f->geometry.count=0;break;
        case 8:f->inventory[0].count[0]=UINT32_MAX;break;
        case 9:f->mapping.count=7;wanted=p::Code::mapping;break;
        case 10:last.bone.number++;wanted=p::Code::mapping;break;
        case 11:last.bone_index=115;wanted=p::Code::mapping;break;
        case 12:f->mapping.root_bone_index=114;wanted=p::Code::mapping;break;
        case 13:f->mapping.rows[7].body_index=6;wanted=p::Code::mapping;break;
        case 14:f->mapping.rows[7].bone_index=f->mapping.rows[6].bone_index;f->mapping.rows[7].name=f->mapping.rows[6].name;wanted=p::Code::mapping;break;
        case 15:last.dimensions.radius=std::numeric_limits<float>::quiet_NaN();wanted=p::Code::invalid;break;
        case 16:last.dimensions.kind=g::metrics::Kind::convex;break;
        case 17:last.shape_world.position={1e308,0,0};last.dimensions.half_segment=0;
            last.segment_start=last.segment_end=last.shape_world.position;wanted=p::Code::numeric;break;
        default:last.segment_end.x+=1;wanted=p::Code::invalid;break;
        }
        p::Result saved{};saved.evaluated=7;saved.relation=p::Relation::overlapping;saved.first_overlap=3;out=saved;
        C(f->run(all,out)==wanted);C(!std::memcmp(&out,&saved,sizeof out));
    }
    f->mapping=actual_mapping;f->geometry=*actual_geometry;f->inventory=actual_inventory;
    const auto alias_before=std::make_unique<g::Geometry>(f->geometry);
    C(p::classify(f->mapping,f->inventory.data(),8,f->geometry,all,*reinterpret_cast<p::Result*>(&f->geometry))==p::Code::alias);
    C(!std::memcmp(alias_before.get(),&f->geometry,sizeof f->geometry));
    const auto inventory_before=f->inventory;
    C(p::classify(f->mapping,f->inventory.data(),8,f->geometry,all,*reinterpret_cast<p::Result*>(f->inventory.data()))==p::Code::alias);
    C(!std::memcmp(&inventory_before,&f->inventory,sizeof inventory_before));
    C(p::classify(f->mapping,nullptr,8,f->geometry,all,out)==p::Code::request);
    C(p::classify(f->mapping,f->inventory.data(),65,f->geometry,all,out)==p::Code::request);
    // Capacity and shrink reuse: all declared records must be evaluated, and
    // result counts/first indices from a larger prior query must not survive.
    f->geometry={};f->geometry.body_count=8;f->geometry.count=g::max_primitives;f->inventory={};f->inventory[0].count[0]=g::max_primitives;
    for(unsigned i=0;i<g::max_primitives;++i){auto& row=f->geometry.rows[i];row.array_index=i;
        auto& s=row.primitive;s.bone=f->mapping.rows[0].name;s.bone_index=f->mapping.rows[0].bone_index;
        s.shape_world={{double(i),0,0},{0,0,0,1}};s.dimensions.kind=g::metrics::Kind::sphere;s.dimensions.radius=1;}
    C(f->run(all,out)==p::Code::evaluated&&out.evaluated==256&&out.overlapping==256);
    const p::Query unit{{-1,-1,-1},{1,1,1}};
    for(unsigned i=0;i<256;++i)f->geometry.rows[i].primitive.shape_world.position={double(1000+i),0,0};
    f->geometry.rows[0].primitive.shape_world.position={2,0,0};
    C(f->run(unit,out)==p::Code::evaluated&&out.relation==p::Relation::indeterminate);
    C(out.evaluated==256&&out.separated==255&&out.indeterminate==1&&out.first_indeterminate==0&&out.first_overlap==p::none);
    f->geometry.rows[255].primitive.shape_world.position={0,0,0};
    C(f->run(unit,out)==p::Code::evaluated&&out.relation==p::Relation::overlapping);
    C(out.evaluated==256&&out.overlapping==1&&out.indeterminate==1&&out.first_overlap==255);
    const auto before_late_failure=out;
    f->geometry.rows[255].primitive.dimensions.radius=std::numeric_limits<float>::quiet_NaN();
    C(f->run(unit,out)==p::Code::invalid);C(!std::memcmp(&out,&before_late_failure,sizeof out));
    f->geometry.count=1;f->inventory[0].count[0]=1;
    C(f->run(distant,out)==p::Code::evaluated&&out.evaluated==1&&out.separated==1);
    C(out.first_overlap==p::none&&out.first_indeterminate==p::none&&out.per_kind[1]==0&&out.per_kind[2]==0);
    std::printf("CoverBodyOverlapTest checks=%u failures=%u saved_C0_synthetic_inventory=true live_source_closed=false scene_occupancy_verified=false\n",checks,failures);
    return failures?1:0;
}
