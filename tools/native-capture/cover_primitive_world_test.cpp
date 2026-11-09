#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include "cover_primitive_world.h"
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
namespace api=gears_cover_primitive_world;
namespace b=api::body;namespace names=api::bones;namespace m=api::metrics;
using Vec=api::Vec;using Quat=api::Quat;using Pose=api::Pose;
unsigned checks{},failures{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
bool hash(void const* bytes,std::size_t size,b::rig::Signature& out){
    BCRYPT_ALG_HANDLE alg{};BCRYPT_HASH_HANDLE h{};
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(ok)ok=BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<void*>(bytes)),ULONG(size),0)>=0;
    if(ok)ok=BCryptFinishHash(h,out.data(),ULONG(out.size()),0)>=0;
    if(h)BCryptDestroyHash(h);if(alg)BCryptCloseAlgorithmProvider(alg,0);return ok;
}
bool read(std::string const& path,void* bytes,std::size_t size){std::ifstream f(path,std::ios::binary);
    return bool(f.read(static_cast<char*>(bytes),std::streamsize(size)))&&f.peek()==EOF;}
// Independent quaternion/vector oracle: no converter FK or matrix operations.
Quat unit(Quat q){const double n=std::sqrt(q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w);return {q.x/n,q.y/n,q.z/n,q.w/n};}
Quat product(Quat p,Quat q){return unit({p.w*q.x+p.x*q.w+p.y*q.z-p.z*q.y,p.w*q.y-p.x*q.z+p.y*q.w+p.z*q.x,
    p.w*q.z+p.x*q.y-p.y*q.x+p.z*q.w,p.w*q.w-p.x*q.x-p.y*q.y-p.z*q.z});}
Vec rotate(Quat q,Vec v){q=unit(q);const Vec u{q.x,q.y,q.z};const double d=u.x*v.x+u.y*v.y+u.z*v.z;
    const double uu=u.x*u.x+u.y*u.y+u.z*u.z;
    return {2*d*u.x+(q.w*q.w-uu)*v.x+2*q.w*(u.y*v.z-u.z*v.y),
        2*d*u.y+(q.w*q.w-uu)*v.y+2*q.w*(u.z*v.x-u.x*v.z),
        2*d*u.z+(q.w*q.w-uu)*v.z+2*q.w*(u.x*v.y-u.y*v.x)};}
Vec add(Vec a,Vec c){return {a.x+c.x,a.y+c.y,a.z+c.z};}
Vec mul(Vec a,double k){return {a.x*k,a.y*k,a.z*k};}
bool close(Vec a,Vec c,double e=.002){return std::hypot(a.x-c.x,a.y-c.y,a.z-c.z)<e;}
bool same_q(Quat a,Quat c){a=unit(a);c=unit(c);return std::abs(a.x*c.x+a.y*c.y+a.z*c.z+a.w*c.w)>1-1e-9;}
Pose compose(Pose p,Pose l){return {add(p.position,rotate(p.orientation,l.position)),product(p.orientation,l.orientation)};}
struct Fixture {
    b::Model model{};b::Frame frame{};names::Mapping mapping{};api::Input input{};
    api::Workspace work{};api::Primitive out{};
    Pose component{{-3900,0,509.94854736328125},{0,0,-std::sqrt(.5),std::sqrt(.5)}};
    bool init(std::string const& path){
        if(!read(path+"/reference.bin",frame.reference.data(),sizeof frame.reference)||
           !read(path+"/C0-local.bin",frame.local.data(),sizeof frame.local)||
           !read(path+"/C0-component.bin",&frame.component_to_world,sizeof frame.component_to_world)||
           !b::prepare_model(frame.reference,hash,model))return false;
        const unsigned selected[]={55,11,1,10,8,91,92,93};std::array<names::Input,8> body_names{};
        for(unsigned i=0;i<8;++i)body_names[i]={i,names::detail::name(frame.reference[selected[i]])};
        if(names::bind(model,frame,body_names.data(),8,2,mapping)!=names::Status::okay)return false;
        input.body_index=1;input.bone=mapping.rows[1].name;
        input.shape_in_bone={{2,-3,4},unit({.2,-.1,.3,.9})};
        input.dimensions={m::Kind::sphere,1.5f,{3,0,0}};return true;
    }
    void set_component(Pose p){component=p;const Vec axes[]={rotate(p.orientation,{1,0,0}),rotate(p.orientation,{0,1,0}),rotate(p.orientation,{0,0,1})};
        frame.component_to_world={};for(unsigned i=0;i<3;++i){frame.component_to_world.values[4*i]=float(axes[i].x);
            frame.component_to_world.values[4*i+1]=float(axes[i].y);frame.component_to_world.values[4*i+2]=float(axes[i].z);}
        frame.component_to_world.values[12]=float(p.position.x);frame.component_to_world.values[13]=float(p.position.y);
        frame.component_to_world.values[14]=float(p.position.z);frame.component_to_world.values[15]=1;
    }
    Pose expected()const {std::array<Pose,b::bones> world{};
        for(unsigned i=0;i<b::bones;++i){const auto& a=frame.local[i];const auto p=i?world[unsigned(model.parents[i])]:component;
            world[i]=compose(p,{{a.translation[0],a.translation[1],a.translation[2]},{a.q[0],a.q[1],a.q[2],a.q[3]}});}
        return compose(world[mapping.rows[input.body_index].bone_index],input.shape_in_bone);
    }
    api::Code run(){return api::convert(model,frame,mapping,input,work,out);}
};
int main(int argc,char** argv){if(argc!=2)return 2;const std::string path=argv[1];
    for(unsigned variant=0;variant<3;++variant){auto x=std::make_unique<Fixture>();C(x->init(path));if(failures)return 1;
        if(variant){x->set_component({{120,-240,70},unit({.1,.3,-.2,.9})});
            const auto q=unit(variant==1?Quat{.3,.1,.2,.9}:Quat{-.2,.4,.1,.8});
            const double values[]={q.x,q.y,q.z,q.w};for(unsigned j=0;j<4;++j)x->frame.local[0].q[j]=float(values[j]);}
        for(unsigned n=0;n<8;++n)for(auto kind:{m::Kind::sphere,m::Kind::box,m::Kind::sphyl}){
            x->input.body_index=n;x->input.bone=x->mapping.rows[n].name;
            x->input.dimensions={kind,1.5f,kind==m::Kind::sphere?std::array<float,3>{3,0,0}:
                kind==m::Kind::box?std::array<float,3>{3,4,5}:std::array<float,3>{3,8,0}};
            x->input.sphyl_axis=kind==m::Kind::sphyl?mul({1,2,3},1/std::sqrt(14.)):Vec{};
            const auto old_frame=x->frame;const auto old_map=x->mapping;const auto old_input=x->input;
            C(x->run()==api::Code::converted);const auto target=x->expected();
            C(close(x->out.shape_world.position,target.position));C(same_q(x->out.shape_world.orientation,target.orientation));
            C(x->out.body_index==n&&x->out.bone_index==x->mapping.rows[n].bone_index&&names::same(x->out.bone,x->input.bone));
            C(!std::memcmp(&old_frame,&x->frame,sizeof old_frame)&&!std::memcmp(&old_map,&x->mapping,sizeof old_map)&&!std::memcmp(&old_input,&x->input,sizeof old_input));
            if(kind==m::Kind::box)C((x->out.dimensions.box_half_extents==std::array<float,3>{2.25f,3,3.75f}));
            else C(x->out.dimensions.radius==4.5f);
            if(kind==m::Kind::sphyl){const auto axis=rotate(target.orientation,x->input.sphyl_axis);
                C(close(x->out.segment_axis_world,axis,1e-6));C(x->out.dimensions.half_segment==6);
                C(close(x->out.segment_start,add(target.position,mul(axis,-6))));C(close(x->out.segment_end,add(target.position,mul(axis,6))));
                // Receiver scalar changes dimensions once; it cannot silently
                // scale the already-converted rigid local translation.
                x->input.dimensions.receiver_scalar=2;C(x->run()==api::Code::converted);
                C(close(x->out.shape_world.position,target.position));C(x->out.dimensions.radius==6&&x->out.dimensions.half_segment==8);
            }else C(close(x->out.segment_start,{},1e-12)&&close(x->out.segment_end,{},1e-12)&&close(x->out.segment_axis_world,{},1e-12));
        }
    }
    for(unsigned defect=0;defect<25;++defect){auto x=std::make_unique<Fixture>();C(x->init(path));C(x->run()==api::Code::converted);
        const auto before=x->out;api::Code expected=api::Code::mapping;
        switch(defect){
        case 0:x->mapping.rows[1].bone_index=10;break;
        case 1:x->input.bone.number++;break;
        case 2:x->mapping.reference_names[114].number++;break;
        case 3:x->mapping.rows[1].nearest_named_body_ancestor=-1;break;
        case 4:x->mapping.root_bone_index++;break;
        case 5:x->mapping.count=65;expected=api::Code::request;break;
        case 6:x->input.body_index=8;expected=api::Code::request;break;
        case 7:x->frame.count--;expected=api::Code::model;break;
        case 8:x->frame.reference[55].bytes[12]^=1;expected=api::Code::model;break;
        case 9:x->frame.model_scale=2;expected=api::Code::scale;break;
        case 10:x->frame.local[55].scale=1.f-0x1p-24f;expected=api::Code::scale;break;
        case 11:x->frame.local[55].q[0]=2;expected=api::Code::atom;break;
        case 12:x->frame.component_to_world.values[0]=.25f;expected=api::Code::component;break;
        case 13:x->frame.component_to_world.values[0]=2;expected=api::Code::component;break;
        case 14:x->frame.component_to_world.values[1]=1;expected=api::Code::component;break;
        case 15:x->frame.component_to_world.values[3]=.1f;expected=api::Code::component;break;
        case 16:x->input.shape_in_bone.orientation={0,0,0,0};expected=api::Code::shape;break;
        case 17:x->input.shape_in_bone.position.x=std::numeric_limits<double>::infinity();expected=api::Code::shape;break;
        case 18:x->input.dimensions.raw[0]=0;expected=api::Code::dimensions;break;
        case 19:x->input.dimensions.kind=m::Kind::convex;expected=api::Code::unsupported;break;
        case 20:x->input.dimensions={m::Kind::sphyl,1,{3,4,0}};expected=api::Code::shape;break;
        case 21:x->input.sphyl_axis={1,0,0};expected=api::Code::shape;break;
        case 22:x->mapping.geometry[0]^=1;break;
        case 23:x->frame.local[114].translation[0]=std::numeric_limits<float>::quiet_NaN();expected=api::Code::atom;break;
        default:x->input.dimensions.receiver_scalar=(std::numeric_limits<float>::max)();expected=api::Code::numeric;break;
        }
        C(x->run()==expected);C(!std::memcmp(&before,&x->out,sizeof before));
    }
    auto x=std::make_unique<Fixture>();C(x->init(path));C(x->run()==api::Code::converted);const auto before=x->out;
    C(api::convert(x->model,x->frame,x->mapping,*reinterpret_cast<api::Input const*>(&x->out),x->work,x->out)==api::Code::alias);
    C(!std::memcmp(&before,&x->out,sizeof before));
    // A zero-length capsule is a sphere at the exact same center, with an
    // explicit axis retained. FName number participates in current binding.
    x->input.dimensions={m::Kind::sphyl,1,{3,0,0}};x->input.sphyl_axis={1,0,0};
    C(x->run()==api::Code::converted);C(close(x->out.segment_start,x->out.segment_end,1e-12));
    const std::uint32_t number=7;std::memcpy(x->frame.reference[11].bytes+4,&number,4);
    std::array<names::Input,8> updated{};for(unsigned i=0;i<8;++i)updated[i]={i,x->mapping.rows[i].name};
    updated[1].bone.number=number;
    C(names::bind(x->model,x->frame,updated.data(),8,2,x->mapping)==names::Status::okay);
    x->input.bone.number=number;C(x->run()==api::Code::converted);C(x->out.bone.number==number);
    // Retained aggregate convention: scale local translation once, then apply
    // the body transform. Capsule axis is Z in the original consumer's space,
    // regardless of which world axis the local TM rotates it onto.
    for(unsigned turn=0;turn<3;++turn)for(float scale:{.25f,1.f,2.5f}){
        auto f=std::make_unique<Fixture>();C(f->init(path));
        f->set_component({{-70,80,90},unit({.3,-.1,.2,.9})});
        const Quat rotation=turn==0?Quat{0,0,0,1}:turn==1?unit(Quat{0,1,0,1}):unit(Quat{.2,.3,-.1,.8});
        api::Authored raw{};raw.body_index=f->input.body_index;raw.bone=f->input.bone;raw.receiver_scale={scale,scale,scale};
        const Vec axes[]={rotate(rotation,{1,0,0}),rotate(rotation,{0,1,0}),rotate(rotation,{0,0,1})};
        for(unsigned i=0;i<3;++i){raw.tm.values[4*i]=float(axes[i].x);raw.tm.values[4*i+1]=float(axes[i].y);raw.tm.values[4*i+2]=float(axes[i].z);}
        raw.tm.values[12]=2;raw.tm.values[13]=-3;raw.tm.values[14]=4;raw.tm.values[15]=1;
        for(auto kind:{m::Kind::sphere,m::Kind::box,m::Kind::sphyl}){
            raw.kind=kind;raw.raw_dimensions=kind==m::Kind::sphere?std::array<float,3>{3,0,0}:
                kind==m::Kind::box?std::array<float,3>{3,4,5}:std::array<float,3>{3,8,0};
            const auto old_raw=raw;api::Input decoded{};C(api::prepare_authored(raw,decoded)==api::Code::converted);
            C(close(decoded.shape_in_bone.position,{2*scale,-3*scale,4*scale},1e-12));C(same_q(decoded.shape_in_bone.orientation,rotation));
            C(close(decoded.sphyl_axis,kind==m::Kind::sphyl?Vec{0,0,1}:Vec{},1e-12));
            C(decoded.dimensions.receiver_scalar==scale&&decoded.dimensions.raw==raw.raw_dimensions);
            C(api::convert_authored(f->model,f->frame,f->mapping,raw,f->work,f->out)==api::Code::converted);
            f->input.shape_in_bone={{2*scale,-3*scale,4*scale},rotation};const auto target=f->expected();
            C(close(f->out.shape_world.position,target.position));C(same_q(f->out.shape_world.orientation,target.orientation));
            if(kind==m::Kind::sphyl){const auto axis=rotate(target.orientation,{0,0,1});
                C(close(f->out.segment_start,add(target.position,mul(axis,-4*scale))));
                C(close(f->out.segment_end,add(target.position,mul(axis,4*scale))));C(f->out.dimensions.radius==3*scale);
            }
            C(!std::memcmp(&old_raw,&raw,sizeof raw));
        }
    }
    api::Authored raw{};raw.body_index=x->input.body_index;raw.bone=x->input.bone;raw.kind=m::Kind::sphere;
    raw.raw_dimensions={1,0,0};raw.receiver_scale={3,3,3};raw.tm.values[0]=raw.tm.values[5]=raw.tm.values[10]=raw.tm.values[15]=1;
    raw.tm.values[12]=.1f;api::Input decoded{};
    C(api::prepare_authored(raw,decoded)==api::Code::converted);
    const unsigned golden=0x3e99999a;float rounded{};std::memcpy(&rounded,&golden,4);C(decoded.shape_in_bone.position.x==double(rounded));
    for(unsigned defect=0;defect<11;++defect){auto bad=raw;auto expected=api::Code::shape;
        switch(defect){
        case 0:bad.receiver_scale[1]=2;expected=api::Code::unsupported;break;
        case 1:bad.receiver_scale[0]=0;expected=api::Code::scale;break;
        case 2:bad.receiver_scale[2]=std::numeric_limits<float>::quiet_NaN();expected=api::Code::scale;break;
        case 3:bad.tm.values[0]=2;break;
        case 4:bad.tm.values[5]=-1;break;
        case 5:bad.tm.values[3]=1;break;
        case 6:bad.tm.values[12]=(std::numeric_limits<float>::max)();expected=api::Code::numeric;break;
        case 7:bad.tm.values[12]=std::numeric_limits<float>::denorm_min();bad.receiver_scale={.25f,.25f,.25f};expected=api::Code::numeric;break;
        case 8:bad.raw_dimensions[0]=0;expected=api::Code::dimensions;break;
        case 9:bad.kind=m::Kind::convex;expected=api::Code::unsupported;break;
        default:bad.receiver_scale={1,1,1};bad.receiver_scale[1]=1.f+0x1p-23f;expected=api::Code::unsupported;break;
        }
        const auto old_decoded=decoded;const auto old_world=x->out;
        C(api::prepare_authored(bad,decoded)==expected);C(!std::memcmp(&old_decoded,&decoded,sizeof decoded));
        C(api::convert_authored(x->model,x->frame,x->mapping,bad,x->work,x->out)==expected);C(!std::memcmp(&old_world,&x->out,sizeof old_world));
    }
    C(api::prepare_authored(*reinterpret_cast<api::Authored const*>(&decoded),decoded)==api::Code::alias);
    const auto old_world=x->out;
    C(api::convert_authored(x->model,x->frame,x->mapping,*reinterpret_cast<api::Authored const*>(&x->out),x->work,x->out)==api::Code::alias);
    C(!std::memcmp(&old_world,&x->out,sizeof old_world));
    std::printf("CoverPrimitiveWorldTest checks=%u failures=%u retained_C0=true synthetic_shapes=true active_physics_verified=false sweep_verified=false\n",checks,failures);
    return failures?1:0;
}
