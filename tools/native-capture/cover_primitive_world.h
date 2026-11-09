#pragma once
#include "cover_physics_bone_map.h"
#include "cover_primitive_metrics.h"

// Caller-owned geometry preparation, not an active-physics adapter. The caller
// must authenticate SAME-source body/setup/name, shape transform, dimensions,
// scalar and conventions before using these values. The optional Authored
// path below implements the retained original aggregate/primitive consumers;
// it still requires authenticated current field/code correspondence. No native
// layout or active physics pose is inferred. Unsupported convex must stop a
// complete-body consumer, never disappear from its collision volume.
namespace gears_cover_primitive_world {
namespace bones=gears_cover_physics_bones;
namespace body=bones::body;
namespace metrics=gears_cover_primitive_metrics;
using Vec=body::Vec;using Pose=body::Pose;using Quat=body::Quat;
struct Input {
    std::uint32_t body_index{};bones::Name bone{};
    // Already converted to a rigid bone-local pose in original game units.
    // Any original scalar effect on translation must be handled upstream.
    Pose shape_in_bone{};
    metrics::Input dimensions{};
    // Explicit authenticated unit axis IN SHAPE SPACE. No +X/+Z assumption.
    // Sphere and box require zero here, as they have no capsule segment.
    Vec sphyl_axis{};
};
struct Authored {
    std::uint32_t body_index{};bones::Name bone{};
    body::Matrix64 tm{};
    metrics::Kind kind{};std::array<float,3> raw_dimensions{};
    // The original aggregate receiver's incoming vector, NOT a scale guessed
    // from TM or the current animated bone. This path supports exact positive
    // uniform scale only; nonuniform/near-uniform cases remain unsupported.
    std::array<float,3> receiver_scale{};
};
struct Primitive {
    std::uint32_t body_index{},bone_index{};bones::Name bone{};
    Pose shape_world{};metrics::Metrics dimensions{};
    Vec segment_axis_world{},segment_start{},segment_end{};
};
enum class Code {converted,request,alias,mapping,model,scale,component,atom,shape,dimensions,unsupported,numeric};
// Large scratch stays on the caller's heap/TLS, never the live game stack.
struct Workspace {
    std::array<bones::Input,bones::max_bodies> names{};
    bones::Mapping checked{};
    std::array<Pose,body::bones> world{};
};
namespace detail {
inline bool same_mapping(bones::Mapping const& a,bones::Mapping const& b) noexcept {
    if(a.count!=b.count||a.root_body_index!=b.root_body_index||a.root_bone_index!=b.root_bone_index||a.geometry!=b.geometry)return false;
    for(unsigned i=0;i<body::bones;++i)if(!bones::same(a.reference_names[i],b.reference_names[i]))return false;
    for(unsigned i=0;i<a.count;++i){const auto& x=a.rows[i];const auto& y=b.rows[i];
        if(x.body_index!=y.body_index||x.bone_index!=y.bone_index||!bones::same(x.name,y.name)||
           x.skeletal_parent!=y.skeletal_parent||x.nearest_named_body_ancestor!=y.nearest_named_body_ancestor)return false;}
    return true;
}
inline bool rigid(body::ik::Matrix const& m) noexcept {
    if(!body::d::similarity(m))return false;
    for(unsigned i=0;i<3;++i)if(std::abs(body::d::length(body::d::row(m,i))-1)>2e-5)return false;
    return true;
}
inline bool compose(Pose const& parent,Pose const& local,Pose& output) noexcept {
    Quat q{};
    if(!body::d::normalized(body::d::product(parent.orientation,local.orientation),q))return false;
    const Vec p=body::d::add(parent.position,body::d::rotate(parent.orientation,local.position));
    if(!body::d::finite_vector(p))return false;
    output={p,q};return true;
}
}
// Original 0x140130e7c scales TM translation componentwise before TM*parent;
// dimensions use scale.x. 0x1400cf080 uses local +Z/-Z capsule endpoints.
// This decodes OWNED fields only. It does not authenticate their current native
// offsets, source bracket, original caller or active physics consumers.
inline Code prepare_authored(Authored const& input,Input& output) noexcept {
    const body::d::Range ranges[]={body::d::range(&input,sizeof input),body::d::range(&output,sizeof output)};
    if(!body::d::disjoint(ranges,2))return Code::alias;
    if(input.kind==metrics::Kind::convex||unsigned(input.kind)>unsigned(metrics::Kind::convex))return Code::unsupported;
    for(float s:input.receiver_scale)if(!std::isfinite(s)||s<=0)return Code::scale;
    if(input.receiver_scale[0]!=input.receiver_scale[1]||input.receiver_scale[0]!=input.receiver_scale[2])return Code::unsupported;
    body::ik::Matrix tm{};for(unsigned i=0;i<16;++i)tm.m[i]=input.tm.values[i];
    if(!detail::rigid(tm))return Code::shape;
    Input next{};next.body_index=input.body_index;next.bone=input.bone;
    next.dimensions={input.kind,input.receiver_scale[0],input.raw_dimensions};
    metrics::Metrics measured{};const auto result=metrics::compute(next.dimensions,measured);
    if(result==metrics::Code::numeric)return Code::numeric;
    if(result!=metrics::Code::computed)return Code::dimensions;
    // Preserve original vmulss float32 translation products before the later
    // rigid composition. Do not scale the rotation rows or apply scalar twice.
    const float x=input.tm.values[12]*input.receiver_scale[0];
    const float y=input.tm.values[13]*input.receiver_scale[1];
    const float z=input.tm.values[14]*input.receiver_scale[2];
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||
       (input.tm.values[12]!=0&&x==0)||(input.tm.values[13]!=0&&y==0)||(input.tm.values[14]!=0&&z==0))return Code::numeric;
    next.shape_in_bone={{x,y,z},body::d::rotation(tm)};
    if(input.kind==metrics::Kind::sphyl)next.sphyl_axis={0,0,1};
    output=next;return Code::converted;
}
inline Code convert(body::Model const& model,body::Frame const& frame,bones::Mapping const& mapping,
                    Input const& input,Workspace& work,Primitive& output) noexcept {
    const body::d::Range ranges[]={body::d::range(&model,sizeof model),body::d::range(&frame,sizeof frame),
        body::d::range(&mapping,sizeof mapping),body::d::range(&input,sizeof input),
        body::d::range(&work,sizeof work),body::d::range(&output,sizeof output)};
    if(!body::d::disjoint(ranges,6))return Code::alias;
    if(!mapping.count||mapping.count>bones::max_bodies||mapping.root_body_index>=mapping.count||input.body_index>=mapping.count)return Code::request;
    for(unsigned i=0;i<mapping.count;++i)work.names[i]={mapping.rows[i].body_index,mapping.rows[i].name};
    const auto bound=bones::bind(model,frame,work.names.data(),mapping.count,mapping.root_body_index,work.checked);
    if(bound==bones::Status::model)return Code::model;
    if(bound!=bones::Status::okay||!detail::same_mapping(mapping,work.checked))return Code::mapping;
    const auto& row=work.checked.rows[input.body_index];
    if(!bones::same(row.name,input.bone))return Code::mapping;
    if(frame.model_scale!=1)return Code::scale;
    body::ik::Matrix component{};for(unsigned j=0;j<16;++j)component.m[j]=frame.component_to_world.values[j];
    if(!detail::rigid(component))return Code::component;
    if(body::d::validate(frame.local.data(),model.parents.data(),body::bones,component)!=body::ik::Status::okay)return Code::atom;
    // This converter deliberately has unit-scale scope. It does not silently
    // strip animation/model scale or derive the original receiver scalar.
    for(auto const& atom:frame.local)if(atom.scale!=1)return Code::scale;
    Quat local_rotation{};
    if(!body::d::finite_vector(input.shape_in_bone.position)||
       !body::d::normalized(input.shape_in_bone.orientation,local_rotation))return Code::shape;
    const bool sphyl=input.dimensions.kind==metrics::Kind::sphyl;
    Vec axis=input.sphyl_axis;
    if(!body::d::finite_vector(axis))return Code::shape;
    const double axis_length=body::d::length(axis);
    if(sphyl){if(std::abs(axis_length-1)>1e-6)return Code::shape;axis=body::d::mul(axis,1/axis_length);}
    else if(axis_length!=0)return Code::shape;
    Primitive next{};const auto measured=metrics::compute(input.dimensions,next.dimensions);
    if(measured==metrics::Code::unsupported)return Code::unsupported;
    if(measured==metrics::Code::numeric)return Code::numeric;
    if(measured!=metrics::Code::computed)return Code::dimensions;
    const Pose component_pose{body::d::position(component),body::d::rotation(component)};
    // Normalized quaternion FK avoids numerical scale accumulating along the
    // chain. Source translation bytes are used unchanged; no length fitting.
    for(unsigned i=0;i<body::bones;++i){const auto& a=frame.local[i];Quat q{};
        if(!body::d::normalized({a.q[0],a.q[1],a.q[2],a.q[3]},q))return Code::atom;
        const Pose local{{a.translation[0],a.translation[1],a.translation[2]},q};
        const Pose& parent=i?work.world[unsigned(model.parents[i])]:component_pose;
        if(!detail::compose(parent,local,work.world[i]))return Code::numeric;
    }
    if(!detail::compose(work.world[row.bone_index],{input.shape_in_bone.position,local_rotation},next.shape_world))return Code::numeric;
    next.body_index=input.body_index;next.bone_index=row.bone_index;next.bone=row.name;
    if(sphyl){next.segment_axis_world=body::d::rotate(next.shape_world.orientation,axis);
        const Vec half=body::d::mul(next.segment_axis_world,next.dimensions.half_segment);
        next.segment_start=body::d::sub(next.shape_world.position,half);
        next.segment_end=body::d::add(next.shape_world.position,half);
        if(!body::d::finite_vector(next.segment_start)||!body::d::finite_vector(next.segment_end))return Code::numeric;
    }
    output=next;return Code::converted;
}
inline Code convert_authored(body::Model const& model,body::Frame const& frame,bones::Mapping const& mapping,
                             Authored const& input,Workspace& work,Primitive& output) noexcept {
    const body::d::Range ranges[]={body::d::range(&model,sizeof model),body::d::range(&frame,sizeof frame),
        body::d::range(&mapping,sizeof mapping),body::d::range(&input,sizeof input),
        body::d::range(&work,sizeof work),body::d::range(&output,sizeof output)};
    if(!body::d::disjoint(ranges,6))return Code::alias;
    Input decoded{};const auto result=prepare_authored(input,decoded);
    if(result!=Code::converted)return result;
    return convert(model,frame,mapping,decoded,work,output);
}
} // namespace gears_cover_primitive_world
