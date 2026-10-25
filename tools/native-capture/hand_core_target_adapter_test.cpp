#include "hand_core_target_adapter.h"
#include <cstdio>
#include <fstream>
#include <limits>

// Actual header integration with a HOST-SYNTHETIC Core/session/binding. Optional
// RO3 input contains observed v238 matrices only, not real grips or permission.
namespace {
namespace a=gears_hand_core_target;
namespace c=gears_native_rig_tracking;
namespace r=gears_rig_tracking;
namespace m=gears_hand_authored_mount;
namespace p=gears_cover_control_output_pose;
namespace v=gears_vr;
namespace h=gears_weapon_hand;
using Pose=v::Pose;using Vec=v::Vector;using Quat=v::Quaternion;
constexpr std::int64_t now=2000000000;
constexpr std::uint64_t age=50000000;
unsigned checks{},failures{},groups{};
double worst_position{},worst_rotation{};
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL line %u: %s\n",static_cast<unsigned>(__LINE__),#x);}}while(false)
template<class T>bool bytes(T const& x,T const& y){return std::memcmp(&x,&y,sizeof(T))==0;}
template<class T>auto saved(T const& x){std::array<unsigned char,sizeof(T)> result{};std::memcpy(result.data(),&x,sizeof(T));return result;}
template<class T>bool unchanged(T const& x,std::array<unsigned char,sizeof(T)> const& copy){return std::memcmp(&x,copy.data(),sizeof(T))==0;}
Vec add(Vec x,Vec y){return {x.x+y.x,x.y+y.y,x.z+y.z};}
Vec sub(Vec x,Vec y){return {x.x-y.x,x.y-y.y,x.z-y.z};}
double distance(Vec x,Vec y){const auto d=sub(x,y);return std::hypot(d.x,d.y,d.z);}
Quat angles(double x,double y,double z){
    return v::normalized(v::multiply(v::multiply(Quat{std::sin(x/2),0,0,std::cos(x/2)},
        Quat{0,std::sin(y/2),0,std::cos(y/2)}),Quat{0,0,std::sin(z/2),std::cos(z/2)}));
}
double rotation_error(Quat x,Quat y){
    const auto xm=v::rotation_matrix(x),ym=v::rotation_matrix(y);double error{};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)error=(std::max)(error,std::abs(xm[i][j]-ym[i][j]));return error;
}
void near_rotation(Quat actual,Quat expected,double tolerance){
    const double error=rotation_error(actual,expected);worst_rotation=(std::max)(worst_rotation,error);C(error<=tolerance);
}
void near_pose(Pose const& actual,Pose const& expected,double position_tolerance=1e-9,double rotation_tolerance=1e-9){
    const double dp=distance(actual.position,expected.position),dq=rotation_error(actual.orientation,expected.orientation);
    worst_position=(std::max)(worst_position,dp);worst_rotation=(std::max)(worst_rotation,dq);
    C(dp<=position_tolerance);C(dq<=rotation_tolerance);
}
h::ObjectIdentity object(std::uint64_t address){return {address,0x8000,0x9000,7,8,9,1};}
c::CoreSnapshot host_core(bool sim=false){
    c::CoreSnapshot s{};s.status=r::Status::ready;s.flags=r::controllers_matched|r::producer_continuity_available;
    s.native={100,101,102,103};s.publication_sequence=17;s.scene_invocation=18;s.world_generation=19;s.binding_generation=20;
    s.reference_generation=21;s.published_ns=now-500000;s.scene_qpc=now-2000000;s.qpc_frequency=1000000000;
    s.observed={2,3,4,{5,6}};s.producer={7,{8,9}};s.native_reference={10,r::reference_stable,0};
    s.controller_publisher_pid=200;s.controller_publisher_start=201;s.view_policy=r::first_person;
    s.world=object(0x1000);s.controller=object(0x2000);s.pawn=object(0x3000);s.mesh=object(0x4000);
    s.units_per_metre=100;s.source_view={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};s.base_view=s.source_view;
    s.reference={};s.stereo.source=sim?v::StereoSource::MetaOperatorSimulated:v::StereoSource::RuntimeTracked;
    s.stereo.session=22;s.stereo.sequence=23;s.stereo.received_ns=now-1000000;s.stereo.predicted_time=123456;
    s.stereo.head_flags=sim?3u:15u;s.stereo.view_flags=15;s.stereo.focused=1;s.stereo.should_render=1;s.stereo.head={};s.stereo.space_epoch=24;
    s.controllers.source=s.stereo.source;s.controllers.session=s.stereo.session;s.controllers.sequence=s.stereo.sequence;
    s.controllers.space_epoch=s.stereo.space_epoch;s.controllers.profile_generation=25;s.controllers.input_epoch=26;
    s.controllers.predicted_time=s.stereo.predicted_time;s.controllers.received_ns=now-1100000;s.controllers.sync_ns=now-1200000;s.controllers.focused=1;
    for(unsigned i=0;i<2;++i){s.controllers.hands[i].grip_active=1;s.controllers.hands[i].grip_flags=15;
        s.world_grip[i]={{30,double(i)*40-20,50},angles(.3,double(i)*.5,.8)};}
    s.world_pose_mask=r::head_pose|r::left_grip_pose|r::right_grip_pose;s.world_head={};s.source_ticket=27;s.native_reference_revision=28;
    return s;
}
a::ModelBinding binding(c::CoreSnapshot const& source,a::Side side,double scale=1){
    a::ModelBinding b{};b.native=source.native;b.world=source.world;b.pawn=source.pawn;b.mesh=source.mesh;b.asset=object(0x5000);
    b.world_generation=source.world_generation;b.binding_generation=source.binding_generation;b.model=m::Model::marcus115;
    b.geometry_signature=m::marcus115_signature;b.bone_count=115;b.model_to_world_scale=scale;
    C(m::marcus115(b.geometry_signature,side,scale,source.units_per_metre,b.authored_mount));return b;
}
a::Target prepared(c::CoreSnapshot const& source,a::Side side,bool sim=false,bool strict=true){
    const auto b=binding(source,side);a::Target target{};
    C(a::prepare_hand(source,b,side,now,age,sim,{strict},target)==a::Status::okay);return target;
}
void reject(c::CoreSnapshot const& source,a::ModelBinding const& b,a::Side side=a::Side::left,
    std::int64_t stamp=now,std::uint64_t max_age=age,bool sim=false,bool strict=true){
    a::Target target{};std::memset(&target,0xa5,sizeof(target));const auto before=saved(target);
    C(a::prepare_hand(source,b,side,stamp,max_age,sim,{strict},target)!=a::Status::okay);C(unchanged(target,before));
}
void prepare_cases(){
    ++groups;
    for(bool sim:{false,true})for(auto side:{a::Side::left,a::Side::right}){
        auto source=host_core(sim);const auto b=binding(source,side);const auto original=saved(source);const auto bound=saved(b);a::Target target{};
        C(c::core_snapshot_valid(source,now,age,sim));C(a::prepare_hand(source,b,side,now,age,sim,{true},target)==a::Status::okay);
        C(unchanged(source,original));C(unchanged(b,bound));C(a::same_source(target,source));C(bytes(target.binding,b));
        C(target.source.producer.stream==source.producer.stream);C(target.source.controllers.source==source.controllers.source);
        Pose expected{};C(gears_hand_wrist::native_wrist_target(source.world_grip[static_cast<unsigned>(side)],
            b.authored_mount.calibration,source.units_per_metre,expected)==gears_hand_wrist::Status::okay);near_pose(target.wrist_world,expected);
        // Full q, including roll, is Hamilton grip * mount. No head direction.
        C(rotation_error(target.wrist_world.orientation,v::multiply(source.world_grip[static_cast<unsigned>(side)].orientation,
            b.authored_mount.calibration.grip_to_wrist_rotation))<1e-12);
        auto changed=source;changed.stereo.head.orientation=angles(1.1,-.9,.4);changed.world_head.orientation=angles(-.8,.7,1.4);
        a::Target head_target{};C(a::prepare_hand(changed,b,side,now,age,sim,{true},head_target)==a::Status::okay);
        C(bytes(head_target.wrist_world,target.wrist_world));C(!a::same_source(target,changed));
    }
    auto source=host_core();const auto b=binding(source,a::Side::left);auto unknown=source;
    unknown.flags&=~r::producer_continuity_available;unknown.producer={};a::Target target{};
    C(a::prepare_hand(unknown,b,a::Side::left,now,age,false,{false},target)==a::Status::okay);
    C(r::producer_empty(target.source.producer));C(!(target.source.flags&r::producer_continuity_available));reject(unknown,b);
    auto alias=prepared(source,a::Side::left);auto before=saved(alias);
    C(a::prepare_hand(alias.source,b,a::Side::left,now,age,false,{true},alias)==a::Status::alias);C(unchanged(alias,before));
    C(a::prepare_hand(source,alias.binding,a::Side::left,now,age,false,{true},alias)==a::Status::alias);C(unchanged(alias,before));
}
void age_tracking_cases(){
    ++groups;const auto original=host_core();const auto b=binding(original,a::Side::left);
    reject(original,b,a::Side::left,now,0);reject(original,b,a::Side::left,now,r::maximum_age_ns+1);reject(original,b,a::Side::left,now,age,true);
    auto s=original;s.scene_qpc=now-std::int64_t(age);a::Target target{};
    C(a::prepare_hand(s,b,a::Side::left,now,age,false,{true},target)==a::Status::okay);--s.scene_qpc;reject(s,b);
    s=original;s.scene_qpc=now+1;reject(s,b);s=original;s.qpc_frequency=(std::numeric_limits<std::int64_t>::max)();s.scene_qpc=s.qpc_frequency-1;reject(s,b);
    s=original;s.controllers.received_ns=now-std::int64_t(age);s.controllers.sync_ns=s.controllers.received_ns;
    C(a::prepare_hand(s,b,a::Side::left,now,age,false,{true},target)==a::Status::okay);--s.controllers.received_ns;--s.controllers.sync_ns;reject(s,b);
    s=original;s.controllers.sync_ns=now-std::int64_t(age)-1;C(c::core_snapshot_valid(s,now,age));reject(s,b);
    s=original;s.stereo.received_ns=now-std::int64_t(age)-1;reject(s,b);
    for(unsigned hand_index=0;hand_index<2;++hand_index){
        s=original;s.controllers.hands[hand_index].grip_active=0;s.world_pose_mask&=~(r::left_grip_pose<<hand_index);
        C(c::core_snapshot_valid(s,now,age));const auto side=static_cast<a::Side>(hand_index);reject(s,binding(s,side),side);
        const auto other=static_cast<a::Side>(1-hand_index);const auto other_b=binding(s,other);
        C(a::prepare_hand(s,other_b,other,now,age,false,{true},target)==a::Status::okay);
        for(unsigned bit=0;bit<4;++bit){s=original;s.controllers.hands[hand_index].grip_flags&=~(std::uint64_t(1)<<bit);
            s.world_pose_mask&=~(r::left_grip_pose<<hand_index);reject(s,binding(s,side),side);}
    }
    s=original;s.controllers.sequence++;reject(s,b);s=original;s.controllers.focused=0;reject(s,b);
    s=original;s.native_reference.flags=r::reference_pending;reject(s,b);s=original;s.native_reference_revision|=1;reject(s,b);
    s=original;s.source_ticket=0;reject(s,b);s=original;s.world_pose_mask&=~r::left_grip_pose;reject(s,b);
    s=original;s.flags&=~r::controllers_matched;s.flags&=~r::producer_continuity_available;s.producer={};s.world_pose_mask=r::head_pose;
    s.controller_publisher_pid=0;s.controller_publisher_start=0;C(c::core_snapshot_valid(s,now,age));reject(s,b);
}
void binding_cases(){
    ++groups;const auto source=host_core();const auto original=binding(source,a::Side::left);
    for(unsigned field=0;field<21;++field){auto b=original;
        switch(field){case 0:++b.native.game_pid;break;case 1:++b.native.game_thread;break;case 2:++b.native.game_start;break;
        case 3:++b.native.native_started_qpc;break;case 4:++b.world.registry_index;break;case 5:++b.pawn.name_number;break;
        case 6:b.mesh.klass+=8;break;case 7:b.asset.known=0;break;case 8:b.asset.address+=1;break;case 9:++b.world_generation;break;
        case 10:++b.binding_generation;break;case 11:b.model=m::Model::unknown;break;case 12:b.geometry_signature[0]^=1;break;
        case 13:--b.bone_count;break;case 14:b.model_to_world_scale=0;break;case 15:b.authored_mount.calibration.known=false;break;
        case 16:b.authored_mount.calibration.grip_to_wrist_rotation={0,0,0,0};break;case 17:b.authored_mount.provenance=m::Provenance::unknown;break;
        case 18:b.authored_mount.input_space=m::Space::unknown;break;case 19:++b.authored_mount.chain[1];break;
        default:b.authored_mount.calibration.grip_to_wrist_metres.x=std::numeric_limits<double>::quiet_NaN();break;}
        reject(source,b);
    }
    reject(source,original,a::Side::right);reject(source,original,a::Side::unknown);
    auto b=original;b.model_to_world_scale=std::numeric_limits<double>::infinity();reject(source,b);
    auto s=source;s.units_per_metre=200;reject(s,original);
    s=source;s.world_grip[0].position.x=std::numeric_limits<double>::infinity();reject(s,original);
}
void units_source_cases(){
    ++groups;const auto original=host_core();const auto reference=prepared(original,a::Side::left);
    auto s=original;s.units_per_metre=200;const auto twice_units=prepared(s,a::Side::left);
    // Profile converts authored model units to metres; application multiplies
    // metres by u once. A fixed model/world scale has the same game-unit offset.
    near_pose(twice_units.wrist_world,reference.wrist_world);
    auto b=binding(original,a::Side::left,2);a::Target scaled{};
    C(a::prepare_hand(original,b,a::Side::left,now,age,false,{true},scaled)==a::Status::okay);
    const auto expected=add(original.world_grip[0].position,sub(reference.wrist_world.position,original.world_grip[0].position));
    const auto one_offset=sub(expected,original.world_grip[0].position);
    C(distance(scaled.wrist_world.position,add(original.world_grip[0].position,{one_offset.x*2,one_offset.y*2,one_offset.z*2}))<1e-9);
    for(unsigned field=0;field<22;++field){s=original;
        switch(field){case 0:++s.native.native_started_qpc;break;case 1:++s.publication_sequence;break;case 2:++s.scene_invocation;break;
        case 3:++s.scene_qpc;break;case 4:++s.published_ns;break;case 5:++s.source_ticket;break;case 6:s.native_reference_revision+=2;break;
        case 7:++s.native_reference.epoch;break;case 8:++s.observed.hand_tracking[0];break;case 9:++s.producer.tracking[1];break;
        case 10:++s.stereo.sequence;break;case 11:++s.stereo.received_ns;break;case 12:++s.stereo.predicted_time;break;
        case 13:++s.controllers.sequence;break;case 14:++s.controllers.received_ns;break;case 15:++s.controllers.sync_ns;break;
        case 16:++s.controllers.profile_generation;break;case 17:++s.controllers.input_epoch;break;case 18:++s.controllers.space_epoch;break;
        case 19:s.base_view[12]=1;break;case 20:s.world_grip[0].position.x+=1;break;default:++s.mesh.registry_index;break;}
        C(!a::same_source(reference,s));
    }
}

struct Observed {p::Matrix64 limb[3]{},component{},single{};};
Observed ro3[2]{};
bool read_ro3(char const* path){
    std::ifstream stream(path,std::ios::binary);char magic[8]{};std::uint32_t count{},stride{};
    stream.read(magic,8);stream.read(reinterpret_cast<char*>(&count),4);stream.read(reinterpret_cast<char*>(&stride),4);
    if(!stream||std::memcmp(magic,"HCTRO3V1",8)||count!=2||stride!=sizeof(Observed))return false;
    stream.read(reinterpret_cast<char*>(ro3),sizeof(ro3));if(!stream||stream.peek()!=std::char_traits<char>::eof())return false;
    return true;
}
Pose world(p::Matrix64 const& component,p::Matrix64 const& transform){
    const auto matrix=p::ik::detail::multiply(p::detail::unpack(component),p::detail::unpack(transform));Pose result{};
    C(p::ik::world_pose(matrix,result)==p::ik::Status::okay);return result;
}
Quat single_world_orientation(p::Matrix64 const& component,p::Matrix64 const& transform){
    C(p::detail::single_rigid(p::detail::unpack(component)));C(p::detail::rigid(p::detail::unpack(transform)));
    // Inspect only the validated rotational basis. Keep all observed output
    // bits, including its non-unit homogeneous tail; do not normalize storage.
    const auto matrix=p::ik::detail::multiply(p::detail::unpack(component),p::detail::unpack(transform));
    return p::ik::detail::rotation(matrix);
}
void same_seven_fields(p::Matrix64 const& actual,p::Matrix64 const& original){
    for(unsigned index:{3u,7u,11u,12u,13u,14u,15u})C(std::memcmp(actual.values+index,original.values+index,sizeof(float))==0);
}
// This inverse chooses a HOST grip for a desired wrist math target. It is not
// a calibration estimate: the fixed designer mount remains unchanged.
void host_grip_for_wrist(c::CoreSnapshot& source,a::Side side,Pose const& wrist){
    const auto b=binding(source,side);const auto& calibration=b.authored_mount.calibration;
    const auto q=v::normalized(v::multiply(wrist.orientation,v::inverse(calibration.grip_to_wrist_rotation)));
    const auto metres=calibration.grip_to_wrist_metres;
    const auto offset=v::transform(v::rotation_matrix(q),{metres.x*source.units_per_metre,metres.y*source.units_per_metre,metres.z*source.units_per_metre});
    source.world_grip[static_cast<unsigned>(side)]={sub(wrist.position,offset),q};
}
a::LimbData limb_data(a::Target const& target,Observed const& observed){
    a::LimbData data{};data.asset=target.binding.asset;data.bone_count=115;data.affected_count=3;data.affected=target.binding.authored_mount.chain;
    for(unsigned j=0;j<3;++j)data.component[j]=observed.limb[j];data.component_to_world=observed.component;
    data.authored={true,world(observed.limb[1],observed.component).position};data.previous={true,{0,0,1}};return data;
}
a::SingleData single_data(a::Target const& target,Observed const& observed){
    a::SingleData data{};data.asset=target.binding.asset;data.bone_count=115;data.affected_count=1;
    data.affected=target.binding.authored_mount.hand_bone;data.component=observed.single;data.component_to_world=observed.component;return data;
}
void limb_single_cases(){
    ++groups;
    for(unsigned index=0;index<2;++index){const auto side=static_cast<a::Side>(index);const auto& observed=ro3[index];
        auto source=host_core();const auto original=world(observed.limb[2],observed.component);
        Pose wanted{original.position,v::normalized(v::multiply(angles(.7,-.4,1.2),original.orientation))};
        host_grip_for_wrist(source,side,wanted);const auto target=prepared(source,side);near_pose(target.wrist_world,wanted);
        const auto target_before=saved(target);const auto data=limb_data(target,observed);const auto data_before=saved(data);
        a::Workspace work{};p::Matrix64 output[3]{};
        C(a::solve_limb(target,data,a::arm::LengthMode::preserve,work,output)==a::Status::okay);
        near_pose(world(output[2],data.component_to_world),wanted,8e-5,3e-6);
        C(bytes(work.limb.wrist_world,target.wrist_world));C(bytes(work.limb.authored,data.authored));C(bytes(work.limb.previous,data.previous));
        C(!work.control_work.solution.length_adjusted);C(std::memcmp(output[0].values+12,data.component[0].values+12,3*sizeof(float))==0);
        C(unchanged(target,target_before));C(unchanged(data,data_before));
        p::Workspace direct_work{};p::LimbInput direct{};for(unsigned j=0;j<3;++j)direct.component[j]=data.component[j];
        direct.component_to_world=data.component_to_world;direct.wrist_world=target.wrist_world;direct.mode=a::arm::LengthMode::preserve;
        direct.authored=data.authored;direct.previous=data.previous;p::Matrix64 direct_output[3]{};
        C(p::solve(direct,direct_work,direct_output)==p::Status::solved);C(bytes(output,direct_output));
        const auto single=single_data(target,observed);p::Matrix64 rotation{};
        // Actual unmodified RO3 Single outputs, including their original tail.
        C(single.component.values[15]!=1);
        C(a::replace_wrist_rotation(target,single,work,&rotation)==a::Status::okay);
        near_rotation(single_world_orientation(rotation,single.component_to_world),target.wrist_world.orientation,3e-6);
        same_seven_fields(rotation,single.component);
        C(bytes(work.single.world_orientation,target.wrist_world.orientation));C(unchanged(target,target_before));
        p::SingleInput direct_single{single.component,single.component_to_world,target.wrist_world.orientation};p::Matrix64 direct_rotation{};
        C(p::replace_wrist_rotation(direct_single,direct_work,&direct_rotation)==p::Status::solved);C(bytes(rotation,direct_rotation));
        // Same target object, while the output component wrist is now positioned
        // by Limb. Single only replaces rotation; it does not promise position.
        auto chained=single;chained.component=output[2];p::Matrix64 chained_output{};
        C(a::replace_wrist_rotation(target,chained,work,&chained_output)==a::Status::okay);
        near_pose(world(chained_output,chained.component_to_world),wanted,8e-5,3e-6);
        // A pure 4cm synthetic wrist translation, then return, uses fresh targets.
        for(Vec delta:{Vec{4,0,0},Vec{0,0,0}}){wanted.position=add(original.position,delta);host_grip_for_wrist(source,side,wanted);
            const auto moving=prepared(source,side);C(a::solve_limb(moving,data,a::arm::LengthMode::adaptive,work,output)==a::Status::okay);
            near_pose(world(output[2],data.component_to_world),wanted,8e-5,3e-6);}
        wanted.position=add(original.position,{200,60,-20});host_grip_for_wrist(source,side,wanted);const auto far=prepared(source,side);
        const auto previous=saved(output);C(a::solve_limb(far,data,a::arm::LengthMode::preserve,work,output)==a::Status::unreachable);C(unchanged(output,previous));
        C(a::solve_limb(far,data,a::arm::LengthMode::adaptive,work,output)==a::Status::okay);
        C(work.control_work.solution.length_adjusted);near_pose(world(output[2],data.component_to_world),wanted,8e-5,3e-6);
    }
}
void single_tail_cases(){
    ++groups;constexpr float epsilon=std::numeric_limits<float>::epsilon();
    for(unsigned side_index=0;side_index<2;++side_index){const auto side=static_cast<a::Side>(side_index);auto source=host_core();
        const auto original=world(ro3[side_index].limb[2],ro3[side_index].component);
        host_grip_for_wrist(source,side,{original.position,v::normalized(v::multiply(angles(.8,-.5,1.3),original.orientation))});
        const auto target=prepared(source,side);const auto original_single=single_data(target,ro3[side_index]);a::Workspace work{};
        for(float tail:{1-2*epsilon,1-epsilon,1.0f,1+epsilon,1+2*epsilon}){
            auto data=original_single;data.component.values[15]=tail;
            // Negative-zero projection entries are still exact zero, and the
            // original seven non-rotation fields must survive byte-for-byte.
            data.component.values[3]=-0.0f;data.component.values[7]=-0.0f;data.component.values[11]=-0.0f;
            const auto before=saved(data);p::Matrix64 output{};
            C(a::replace_wrist_rotation(target,data,work,&output)==a::Status::okay);C(unchanged(data,before));
            same_seven_fields(output,data.component);
            near_rotation(single_world_orientation(output,data.component_to_world),target.wrist_world.orientation,3e-6);
        }
        p::Matrix64 output{};std::memset(&output,0x5a,sizeof(output));const auto before=saved(output);
        for(unsigned change=0;change<13;++change){auto data=original_single;
            switch(change){case 0:data.component.values[15]=1+3*epsilon;break;case 1:data.component.values[15]=1-3*epsilon;break;
            case 2:data.component.values[15]=std::numeric_limits<float>::quiet_NaN();break;
            case 3:data.component.values[15]=std::numeric_limits<float>::infinity();break;
            case 4:data.component.values[3]=epsilon;break;case 5:data.component.values[7]=-epsilon;break;case 6:data.component.values[11]=epsilon;break;
            case 7:for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)data.component.values[i*4+j]*=1.01f;break;
            case 8:for(unsigned j=0;j<3;++j)data.component.values[j]+=data.component.values[4+j]*.01f;break;
            case 9:for(unsigned j=0;j<3;++j)data.component.values[j]*=-1;break;
            case 10:data.component.values[5]=std::numeric_limits<float>::quiet_NaN();break;
            case 11:data.component.values[12]=std::numeric_limits<float>::infinity();break;
            default:data.component_to_world.values[15]=1+epsilon;break;}
            C(a::replace_wrist_rotation(target,data,work,&output)==a::Status::invalid_matrix);C(unchanged(output,before));
        }
        // The Single tail allowance does not reach Limb or component_to_world.
        auto limb=limb_data(target,ro3[side_index]);p::Matrix64 limb_output[3]{};const auto limb_before=saved(limb_output);
        limb.component[2].values[15]=1-epsilon;
        C(a::solve_limb(target,limb,a::arm::LengthMode::adaptive,work,limb_output)==a::Status::invalid_matrix);C(unchanged(limb_output,limb_before));
        limb=limb_data(target,ro3[side_index]);limb.component_to_world.values[15]=1+epsilon;
        C(a::solve_limb(target,limb,a::arm::LengthMode::adaptive,work,limb_output)==a::Status::invalid_matrix);C(unchanged(limb_output,limb_before));
    }
}
void wrapper_rejection_cases(){
    ++groups;auto source=host_core();host_grip_for_wrist(source,a::Side::left,world(ro3[0].limb[2],ro3[0].component));
    auto target=prepared(source,a::Side::left);const auto data=limb_data(target,ro3[0]);const auto single=single_data(target,ro3[0]);a::Workspace work{};
    p::Matrix64 output[3]{};std::memset(output,0x5a,sizeof(output));const auto before=saved(output);
    for(unsigned change=0;change<8;++change){auto bad=data;
        switch(change){case 0:++bad.asset.registry_index;break;case 1:bad.affected={32,34,35};break;case 2:bad.affected_count=2;break;
        case 3:bad.bone_count=114;break;case 4:bad.component[1].values[0]=std::numeric_limits<float>::quiet_NaN();break;
        case 5:bad.component[0].values[0]*=2;break;case 6:bad.component_to_world.values[0]=0;bad.component_to_world.values[1]=0;bad.component_to_world.values[2]=0;break;
        default:bad.component_to_world.values[0]=std::numeric_limits<float>::infinity();break;}
        C(a::solve_limb(target,bad,a::arm::LengthMode::adaptive,work,output)!=a::Status::okay);C(unchanged(output,before));
    }
    C(a::solve_limb(target,data,static_cast<a::arm::LengthMode>(99),work,output)==a::Status::invalid_policy);C(unchanged(output,before));
    auto overlap=data;const auto overlap_before=saved(overlap);
    C(a::solve_limb(target,overlap,a::arm::LengthMode::adaptive,work,overlap.component)==a::Status::alias);C(unchanged(overlap,overlap_before));
    C(a::solve_limb(target,overlap,a::arm::LengthMode::adaptive,work,overlap.component+1)==a::Status::alias);C(unchanged(overlap,overlap_before));
    const auto work_before=saved(work);C(a::solve_limb(target,data,a::arm::LengthMode::adaptive,work,work.control_work.candidate)==a::Status::alias);C(unchanged(work,work_before));
    C(a::solve_limb(target,data,a::arm::LengthMode::adaptive,work,nullptr)==a::Status::alias);
    auto bad_target=target;bad_target.binding.authored_mount.calibration.known=false;
    C(a::solve_limb(bad_target,data,a::arm::LengthMode::adaptive,work,output)==a::Status::invalid_target);C(unchanged(output,before));
    for(unsigned change=0;change<6;++change){auto bad=single;
        switch(change){case 0:++bad.asset.name_number;break;case 1:bad.affected=35;break;case 2:bad.affected_count=3;break;
        case 3:bad.bone_count=0;break;case 4:bad.component.values[15]=0;break;default:bad.component_to_world.values[0]*=-1;break;}
        C(a::replace_wrist_rotation(target,bad,work,output)!=a::Status::okay);C(unchanged(output,before));
    }
    auto overlapping_single=single;const auto single_before=saved(overlapping_single);
    C(a::replace_wrist_rotation(target,overlapping_single,work,&overlapping_single.component)==a::Status::alias);C(unchanged(overlapping_single,single_before));
    C(a::replace_wrist_rotation(target,single,work,work.control_work.candidate)==a::Status::alias);
    C(a::replace_wrist_rotation(target,single,work,nullptr)==a::Status::alias);
    target.wrist_world.orientation={0,0,0,0};C(a::replace_wrist_rotation(target,single,work,output)==a::Status::invalid_target);C(unchanged(output,before));
}
void original_matrix_policy_cases(){
    ++groups;
    using Policy=p::OriginalMatrixPolicy;
    auto source=host_core();
    host_grip_for_wrist(source,a::Side::left,world(ro3[0].limb[2],ro3[0].component));
    const auto target=prepared(source,a::Side::left);auto data=limb_data(target,ro3[0]);a::Workspace work{};
    for(unsigned i=0;i<3;++i)for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)
        data.component[i].values[row*4+column]*=1.00002f;
    const auto original=saved(data);p::Matrix64 output[3]{};std::memset(output,0x5a,sizeof output);const auto untouched=saved(output);
    C(data.original_matrix_policy==Policy::strict_unit);
    work.limb.original_matrix_policy=Policy::bounded_near_rigid;
    C(a::solve_limb(target,data,a::arm::LengthMode::preserve,work,output)==a::Status::invalid_matrix);
    C(work.limb.original_matrix_policy==Policy::strict_unit);C(unchanged(output,untouched));C(unchanged(data,original));
    // This tiny genuine scale lies inside the explicit approximation budget.
    // Accepting it is intentional and is not proof of native LocalScale=1.
    auto approximate=data;approximate.original_matrix_policy=Policy::bounded_near_rigid;const auto approximate_before=saved(approximate);
    C(a::solve_limb(target,approximate,a::arm::LengthMode::preserve,work,output)==a::Status::okay);
    C(work.limb.original_matrix_policy==Policy::bounded_near_rigid);C(unchanged(approximate,approximate_before));
    for(auto const& matrix:output)C(p::detail::rigid(p::detail::unpack(matrix)));
    std::memset(output,0x5a,sizeof output);
    C(a::solve_limb(target,data,a::arm::LengthMode::preserve,work,output)==a::Status::invalid_matrix);
    C(work.limb.original_matrix_policy==Policy::strict_unit);C(unchanged(output,untouched));
    auto bad=approximate;bad.original_matrix_policy=static_cast<Policy>(99);
    C(a::solve_limb(target,bad,a::arm::LengthMode::preserve,work,output)==a::Status::invalid_policy);C(unchanged(output,untouched));
    bad=approximate;for(unsigned row=0;row<3;++row)for(unsigned column=0;column<3;++column)
        bad.component_to_world.values[row*4+column]*=1.00002f;
    C(a::solve_limb(target,bad,a::arm::LengthMode::preserve,work,output)==a::Status::invalid_matrix);C(unchanged(output,untouched));
}
} // namespace
int main(int argc,char** argv){
    if(argc!=2||!read_ro3(argv[1])){std::fprintf(stderr,"Expected pinned RO3 matrix fixture HCTRO3V1\n");return 2;}
    prepare_cases();age_tracking_cases();binding_cases();units_source_cases();limb_single_cases();single_tail_cases();wrapper_rejection_cases();original_matrix_policy_cases();
    std::printf("hand_core_target_adapter: %u groups, %u checks, %u failures; max_position=%.17g max_basis_rotation=%.17g; HOST Core/binding + observed RO3 matrices, CPU math only\n",
        groups,checks,failures,worst_position,worst_rotation);return failures?1:0;
}
