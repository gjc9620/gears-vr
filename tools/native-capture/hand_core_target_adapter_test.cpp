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
// Fixed authored held profile, with explicit HOST input only. No runtime query
// or equipment/permission inference is made by selecting MountKind here.
void held_mount_cases(){
    ++groups;auto source=host_core();const auto source_before=saved(source);
    auto stock=binding(source,a::Side::right);C(stock.mount_kind==a::MountKind::free_hand);
    auto held=stock;held.mount_kind=a::MountKind::held_lancer;gears_weapon_lancer_wrist_mount::Profile profile{};
    C(gears_weapon_lancer_wrist_mount::marcus115(held.geometry_signature,1,source.units_per_metre,profile));held.authored_mount=profile.mount;
    const auto binding_before=saved(held);a::Target target{};
    C(a::prepare_hand(source,held,a::Side::right,now,age,false,{true},target)==a::Status::okay);
    // RH=I means full wrist Q equals Grip Q, including nonzero roll. Expected
    // position uses the free profile's fixed wrist-local palm marker relation.
    const auto& free=stock.authored_mount.calibration;
    const auto fixed=v::transform(v::transpose(v::rotation_matrix(free.grip_to_wrist_rotation)),free.grip_to_wrist_metres);
    const auto offset=v::transform(v::rotation_matrix(source.world_grip[1].orientation),{fixed.x*source.units_per_metre,fixed.y*source.units_per_metre,fixed.z*source.units_per_metre});
    near_pose(target.wrist_world,{add(source.world_grip[1].position,offset),source.world_grip[1].orientation});
    C(a::same_source(target,source)&&unchanged(source,source_before)&&unchanged(held,binding_before));
    auto masquerade=stock;masquerade.mount_kind=a::MountKind::held_lancer;reject(source,masquerade,a::Side::right);
    masquerade=held;masquerade.mount_kind=a::MountKind::free_hand;reject(source,masquerade,a::Side::right);
    auto left=binding(source,a::Side::left);left.mount_kind=a::MountKind::held_lancer;reject(source,left,a::Side::left);
    left.authored_mount=held.authored_mount;reject(source,left,a::Side::left);
    auto unknown=held;unknown.mount_kind=static_cast<a::MountKind>(99);reject(source,unknown,a::Side::right);
    auto tampered=held;tampered.authored_mount.calibration.grip_to_wrist_metres.x+=.001;reject(source,tampered,a::Side::right);
    C(unchanged(source,source_before));
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
} // namespace
void current_metadata_cases(){
    namespace src=gears_body_tick_source;namespace tick=gears_body_tick_input;
    ++groups;auto original=host_core();original.scene_qpc=now-60000000;original.published_ns=now-55000000;
    for(unsigned side=0;side<2;++side){original.stereo.eyes[side].pose={{side?.032:-.032,0,0},{0,0,0,1}};
        original.stereo.eyes[side].fov={-.7,.7,.7,-.7};}
    original.stereo.received_ns=now-70000000;original.controllers.received_ns=now-71000000;original.controllers.sync_ns=now-72000000;
    src::Input input{};input.scene_cache_revision=2;input.metadata_begin_ns=now-10000000;input.metadata_end_ns=now-8000000;
    auto& t=input.observed;t.parent_scene=original;t.tick=1;t.thread=original.native.game_thread;
    t.begin_ns=now-7000000;t.end_ns=now-3000000;t.stereo=original.stereo;t.stereo.sequence+=5;t.stereo.received_ns=now-4000000;
    t.controllers=original.controllers;t.controllers.sequence=t.stereo.sequence;t.controllers.received_ns=now-5000000;t.controllers.sync_ns=now-6000000;
    t.continuity.input=t.controllers;t.continuity.stream_epoch=original.producer.stream;
    t.continuity.tracking_epoch[0]=original.producer.tracking[0];t.continuity.tracking_epoch[1]=original.producer.tracking[1];
    t.native_reference=original.native_reference;t.reference_revision=original.native_reference_revision;
    t.publisher_pid=original.controller_publisher_pid;t.publisher_start=original.controller_publisher_start;
    const auto before=saved(original);
    C(src::classify(input,original,now,false)==src::Verdict::okay);
    C(tick::classify(t,original,now,false)==tick::Verdict::source_age_only);
    C(unchanged(original,before));
    // Recorded failure: the historical Scene matched controllers but missed
    // its optional continuity read; current independently authenticated input
    // was fresh and complete. This metadata contract uses that current input
    // without modifying or admitting the old parent through legacy tracking.
    {auto parent=original;parent.flags&=~r::producer_continuity_available;parent.producer={};
        auto current=input;current.observed.parent_scene=parent;const auto parent_before=saved(parent);
        C(src::classify(current,parent,now,false)==src::Verdict::okay);
        C(tick::classify(current.observed,parent,now,false)==tick::Verdict::hard);
        a::SourceTarget target{};auto right=binding(parent,a::Side::right);
        C(a::prepare_hand(current,right,a::Side::right,parent.base_view,now,false,target)==a::Status::okay);
        C(unchanged(parent,parent_before));
        for(unsigned field=0;field<9;++field){auto bad=current;auto bad_parent=parent;
            switch(field){
            case 0:bad.observed.continuity.stream_epoch=0;break;
            case 1:bad.observed.continuity.tracking_epoch[1]=0;break;
            case 2:++bad.observed.publisher_start;break;
            case 3:++bad.observed.continuity.input.sequence;break;
            case 4:++bad.observed.controllers.input_epoch;bad.observed.continuity.input=bad.observed.controllers;break;
            case 5:++bad.observed.native_reference.epoch;break;
            case 6:bad.observed.controllers.sync_ns=now-40000001;bad.observed.continuity.input=bad.observed.controllers;break;
            case 7:bad_parent.producer.stream=1;break;
            case 8:bad_parent.flags&=~r::controllers_matched;break;
            }
            bad.observed.parent_scene=bad_parent;
            C(src::classify(bad,bad_parent,now,false)!=src::Verdict::okay);
        }
        auto previous=current.observed.continuity;auto next=previous;++next.input.sequence;
        C(gears_current_continuity::not_regressed(next,previous));
        for(unsigned field=0;field<4;++field){auto bad=next;
            switch(field){case 0:--bad.stream_epoch;break;case 1:--bad.tracking_epoch[0];break;
            case 2:--bad.tracking_epoch[1];break;case 3:bad.input.sequence=previous.input.sequence-1;break;}
            C(!gears_current_continuity::not_regressed(bad,previous));}
        auto same=previous;++same.stream_epoch;C(!gears_current_continuity::not_regressed(same,previous));
        auto new_input=next;++new_input.input.input_epoch;new_input.input.sequence=1;
        C(gears_current_continuity::not_regressed(new_input,previous));
        --new_input.stream_epoch;C(!gears_current_continuity::not_regressed(new_input,previous));
    }
    // Render metadata may legitimately be head-only. The current-source POD
    // can still describe complete tracking, but the live latch additionally
    // requires the previous successful independent input domain.
    {auto parent=original;parent.flags&=~(r::controllers_matched|r::producer_continuity_available);
        parent.controllers={};parent.producer={};parent.controller_publisher_pid=0;parent.controller_publisher_start=0;
        parent.world_pose_mask=r::head_pose;parent.world_grip[0]={};parent.world_grip[1]={};
        auto current=input;current.observed.parent_scene=parent;const auto parent_before=saved(parent);
        C(src::classify(current,parent,now,false)==src::Verdict::okay);
        C(tick::classify(current.observed,parent,now,false)==tick::Verdict::hard);
        C(src::independent_domain_matches(current,input));
        C(!src::independent_domain_matches(current,src::Input{}));
        C(unchanged(parent,parent_before));
        a::SourceTarget target{};auto right=binding(parent,a::Side::right);
        C(a::prepare_hand(current,right,a::Side::right,parent.base_view,now,false,target)==a::Status::okay);
        for(unsigned field=0;field<19;++field){auto changed=current;auto& v=changed.observed;auto& p=v.parent_scene;
            switch(field){case 0:++p.native.native_started_qpc;break;case 1:++p.native.game_start;break;
            case 2:++p.world_generation;break;case 3:++p.pawn.name_number;break;case 4:++p.mesh.name_number;break;
            case 5:++p.controller.name_number;break;case 6:++p.reference_generation;break;case 7:++v.reference_revision;break;
            case 8:++v.native_reference.epoch;break;case 9:++v.publisher_pid;break;case 10:++v.publisher_start;break;
            case 11:++v.controllers.input_epoch;break;case 12:++v.controllers.profile_generation;break;
            case 13:++v.continuity.stream_epoch;break;case 14:--v.continuity.tracking_epoch[1];break;
            case 15:++v.stereo.session;break;case 16:++v.stereo.space_epoch;break;case 17:++v.parent_cookie;break;
            case 18:++p.binding_generation;break;}
            C(!src::independent_domain_matches(changed,input));}
        for(unsigned field=0;field<8;++field){auto bad=current;auto& v=bad.observed;auto& p=v.parent_scene;
            switch(field){case 0:p.controllers.sequence=1;break;case 1:p.controller_publisher_pid=1;break;
            case 2:p.producer.stream=1;break;case 3:p.world_pose_mask|=r::right_grip_pose;break;
            case 4:v.publisher_pid=0;break;case 5:v.continuity.stream_epoch=0;break;
            case 6:v.controllers.sync_ns=now-40000001;v.continuity.input=v.controllers;break;
            case 7:v.stereo.received_ns=now-40000001;break;}
            C(src::classify(bad,p,now,false)!=src::Verdict::okay);}
    }
    // A tracked right hand remains usable when only the left disappears.
    // No old left target or malformed/future/stale frame becomes admissible.
    for(unsigned missing=0;missing<2;++missing){auto partial=input;
        auto& observed=partial.observed;
        observed.controllers.hands[missing].grip_active=0;
        observed.controllers.hands[missing].grip_flags=0;
        observed.continuity.input=observed.controllers;
        C(src::classify(partial,original,now,false)==src::Verdict::okay);
        C(tick::classify(observed,original,now,false)==tick::Verdict::hard);
        const auto lost=static_cast<a::Side>(missing),tracked=static_cast<a::Side>(1-missing);
        auto lost_binding=binding(original,lost),tracked_binding=binding(original,tracked);
        a::SourceTarget target{};const auto target_before=saved(target);
        C(a::prepare_hand(partial,lost_binding,lost,original.base_view,now,false,target)==a::Status::hand_untracked);
        C(unchanged(target,target_before));
        C(a::prepare_hand(partial,tracked_binding,tracked,original.base_view,now,false,target)==a::Status::okay);
        C(src::classify(partial,original,now+33000001,false)==src::Verdict::source_age_only);
        auto malformed=partial;malformed.observed.controllers.hands[1-missing].grip.orientation={0,0,0,0};
        malformed.observed.continuity.input=malformed.observed.controllers;
        C(src::classify(malformed,original,now,false)==src::Verdict::hard);
        auto changed=partial;++changed.observed.continuity.tracking_epoch[missing];
        C(src::classify(changed,original,now,false)==src::Verdict::okay);
        C(tick::classify(changed.observed,original,now,false)==tick::Verdict::hard);
        C(a::prepare_hand(changed,tracked_binding,tracked,original.base_view,now,false,target)==a::Status::okay);
        const auto current_target=saved(target);
        C(a::prepare_hand(changed,lost_binding,lost,original.base_view,now,false,target)==a::Status::hand_untracked);
        C(unchanged(target,current_target));
        // Recovery has a fresh current grip/epoch, never the parent hand pose.
        auto recovered=changed;recovered.observed.controllers=input.observed.controllers;
        recovered.observed.continuity.input=recovered.observed.controllers;
        C(src::classify(recovered,original,now,false)==src::Verdict::okay);
        C(a::prepare_hand(recovered,lost_binding,lost,original.base_view,now,false,target)==a::Status::okay);
        C(target.input.observed.continuity.tracking_epoch[missing]==changed.observed.continuity.tracking_epoch[missing]);
        C(unchanged(original,before));
        auto regressed=changed;regressed.observed.continuity.tracking_epoch[missing]=original.producer.tracking[missing]-1;
        C(src::classify(regressed,original,now,false)==src::Verdict::hard);
        auto other_stream=changed;++other_stream.observed.continuity.stream_epoch;
        C(src::classify(other_stream,original,now,false)==src::Verdict::hard);
        // Even with a fresh metadata association, the same cohort cannot
        // claim a changed tracking epoch. Keep all parent bytes immutable.
        auto same_cohort=changed;
        auto& same=same_cohort.observed;
        same.stereo=original.stereo;same.controllers=original.controllers;
        same.continuity.input=same.controllers;
        C(src::classify(same_cohort,original,now,false)==src::Verdict::hard);
        C(src::classify(changed,original,now+33000001,false)==src::Verdict::source_age_only);
    }
    for(unsigned field=0;field<5;++field){auto bad=input;
        switch(field){case 0:bad.metadata_begin_ns=0;break;case 1:bad.metadata_end_ns=bad.metadata_begin_ns-1;break;
        case 2:bad.metadata_end_ns=t.begin_ns+1;break;case 3:bad.metadata_begin_ns=bad.metadata_end_ns=now+1;break;
        case 4:++bad.observed.publisher_start;break;}
        C(src::classify(bad,original,now,false)==src::Verdict::hard);}
    {auto later=input;++later.metadata_begin_ns;C(src::classify(later,original,now,false)==src::Verdict::okay);}
    // A delayed render does not age a newly observed Tick. Never rewrite the
    // old Scene to achieve this, and still reject it on the legacy path.
    {auto delayed=original;delayed.scene_qpc-=200000000;delayed.published_ns-=200000000;
        delayed.stereo.received_ns-=200000000;delayed.controllers.received_ns-=200000000;delayed.controllers.sync_ns-=200000000;
        auto current=input;current.observed.parent_scene=delayed;const auto bytes=saved(delayed);
        C(src::classify(current,delayed,now,false)==src::Verdict::okay);
        C(tick::classify(current.observed,delayed,now,false)==tick::Verdict::source_age_only);C(unchanged(delayed,bytes));}
    for(auto metadata_age:{50000000LL,50000001LL}){auto old=input;old.metadata_begin_ns=now-metadata_age;
        C((src::classify(old,original,now,false)==src::Verdict::okay)==(metadata_age==50000000));}
    for(auto elapsed:{33000000LL,33000001LL})
        C((src::classify(input,original,now+elapsed,false)==src::Verdict::okay)==(elapsed==33000000));
    {auto old=input;old.observed.begin_ns=now-40000001;old.metadata_end_ns=old.observed.begin_ns;old.metadata_begin_ns=old.metadata_end_ns;
        C(src::classify(old,original,now,false)==src::Verdict::source_age_only);}
    // Compose may acquire later than outer Tick, but neither the metadata
    // observation nor the XR publication receives a fresh timestamp for free.
    {auto late=input;late.metadata_begin_ns=now-49000000;late.metadata_end_ns=now-48000000;
        late.observed.begin_ns=now-2000000;late.observed.end_ns=now-1000000;
        C(src::classify(late,original,now,false)==src::Verdict::okay);
        C(src::classify(late,original,now+1000001,false)==src::Verdict::source_age_only);
        for(auto raw_age:{40000000LL,40000001LL}){auto raw_old=late;
            raw_old.observed.stereo.received_ns=now-raw_age;
            C((src::classify(raw_old,original,now,false)==src::Verdict::okay)==(raw_age==40000000));}
        C(unchanged(original,before));}
}
int main(int argc,char** argv){
    current_metadata_cases();
    if(argc==2&&!std::strcmp(argv[1],"--current-metadata")){
        std::printf("current GT metadata: %u checks, %u failures; owned source data only\n",checks,failures);return failures?1:0;}
    if(argc!=2||!read_ro3(argv[1])){std::fprintf(stderr,"Expected pinned RO3 matrix fixture HCTRO3V1\n");return 2;}
    prepare_cases();held_mount_cases();age_tracking_cases();binding_cases();units_source_cases();limb_single_cases();single_tail_cases();wrapper_rejection_cases();
    std::printf("hand_core_target_adapter: %u groups, %u checks, %u failures; max_position=%.17g max_basis_rotation=%.17g; HOST Core/binding + observed RO3 matrices, CPU math only\n",
        groups,checks,failures,worst_position,worst_rotation);return failures?1:0;
}
