// Original mounted geometry/limits only. No gun/controller/bone writer or
// action eligibility is created by observing these matrices and raw limits.
namespace troika_mounted_parts {
namespace hand=gears_weapon_hand;namespace meta=weaponhand::meta;
namespace socket_contract {
namespace metadata=weaponhand::meta;using U32=std::uint32_t;
#include "weapon_muzzle_getter_call.inl"
}
#include "weapon_troika_control_axes.inl"
#include "weapon_troika_enum_getter.inl"
#include "weapon_troika_control_binding.inl"
#include "weapon_troika_socket_binding.inl"
#include "weapon_troika_original_limits.inl"
#include "weapon_troika_original_aim.inl"
#include "weapon_troika_physical_start.inl"
enum class Status:unsigned {source,asset,limits,names,bones,physical_start,closing,ready,exception};
struct Observation {
    hand::ObjectIdentity component{},asset{};
    // Original row-vector world matrices: root, latitude, longitude,
    // right muzzle, left muzzle, handleroot. Bone labels are checked first.
    std::array<std::array<float,16>,6> bones{};
    std::array<float,2> yaw_limit{};
    std::array<int,3> turret_rotation{};int pitch_min{},pitch_max{};
    Name muzzle_socket{};playground::Vector socket_position{},physical_start{};std::array<std::int32_t,3> socket_rotation{};
    std::uint64_t publication{};std::int64_t input_sync_ns{};
    bool physical_start_known{};
    unsigned hand_handle[2]{};Name handle_name[2]{};
    control_axes::Snapshot original_controls{};bool controls_known{};
    control_axes::Decoded control_enums{};bool control_enums_known{};
    control_binding::Snapshot original_binding{};bool control_binding_known{};
    socket_binding::Snapshot selected_socket{};bool socket_binding_known{};
};
gears_equipment_names::Lookup pivot_names({L"b_troika_root",L"b_troika_gun_latitude",L"b_troika_gun_longitude"});
gears_equipment_names::Lookup barrel_names({L"b_troika_muzzle_R",L"b_troika_muzzle_L",L"b_troika_handleroot"});
bool read(troika_original::EntryOwner const& owner,gears_troika_source::Frame const& source,
    gears_weapon::TroikaMountedMapping const& mapping,Observation& output,Status& status,bool read_controls) noexcept {
    output={};status=Status::source;try{
        troika_mounted_context::Context before{},after{};
        const auto current=[&](){
            return owner.outstanding&&owner.receipt.result==troika_original::EntryResult::mounted&&
                weaponhand::client.phase()==hand::ClientPhase::active&&weaponhand::client.cookie()==source.native.registration_cookie&&
                weaponhand::client.native_started_qpc()==source.native.native_started_qpc&&
                source.native.game_pid==owner.receipt.source_key.game_pid&&source.native.game_start==owner.receipt.source_key.game_start&&
                source.native.native_started_qpc==owner.receipt.source_key.native_started_qpc&&source.native.game_thread==weaponhand::current_gt(nullptr)&&
                source.native.registration_cookie==owner.receipt.source_key.registration_cookie&&
                gears_troika_source::frame_valid(source,weaponhand::clock_ns(nullptr),100000000,
                    owner.receipt.source==gears_vr::StereoSource::MetaOperatorSimulated)&&
                troika_mounted_context::observe(owner,after)&&troika_tracking::same(before,after)&&
                hand::same(source.world,after.world)&&hand::same(source.controller,after.controller)&&
                hand::same(source.driver,after.driver)&&hand::same(source.turret,after.turret)&&
                hand::same(source.turret_mesh,after.turret_mesh)&&hand::same(source.driver_mesh,after.driver_mesh)&&
                hand::same(source.weapon,after.weapon)&&troika_tracking::source_current(source);
        };
        if(mapping.publication_sequence!=source.publication_sequence||mapping.source_ticket!=source.source_ticket||
           mapping.hands.input_sync_ns!=source.controllers.sync_ns||!troika_mounted_context::observe(owner,before)||!current())return false;
        meta::Snapshot schema{};schema.module_base=base;meta::Reader reader{schema,meta::Stage::final_check};
        status=Status::asset;
        Observation result{};result.component=before.turret_mesh;
        troika_reflection::ScriptStruct yaw_structure{},rotation_structure{};
        weaponhand::BoolField field{};
        if(!meta::tables(base,schema.objects,schema.names)||!weaponhand::object(reader,result.component)||
           !troika_mounted_context::pointer(reader,result.component.address,L"SkeletalMesh",result.asset)||
           !weaponhand::derives(reader,result.asset.address,L"SkeletalMesh",L"Engine"))return false;
        unsigned asset_slot{},package_slot{};
        if(!reader.path(result.asset.address,asset_slot)||!reader.named(asset_slot,L"TroikaTurret")||
           !reader.path(reader.result.identities[asset_slot].outer,package_slot)||!reader.named(package_slot,L"Geist_TroikaCabal")||
           reader.result.identities[package_slot].outer||
           !weaponhand::derives(reader,reader.result.identities[package_slot].address,L"Package",L"Core"))return false;
        status=Status::limits;unsigned yaw_offset{},rotation_offset{},minimum_offset{},maximum_offset{},socket_offset{};
        if(!weaponhand::field(reader,before.turret.address,L"YawLimit",L"StructProperty",8,field)||
           !troika_reflection::numeric(reader,field.property,L"Vector2D",L"FloatProperty",std::array<wchar_t const*,2>{L"X",L"Y"},yaw_structure)||
           !reader.at(before.turret.address,field.property.offset,result.yaw_limit)||
           !std::isfinite(result.yaw_limit[0])||!std::isfinite(result.yaw_limit[1])||
           std::abs(result.yaw_limit[0])>65536||std::abs(result.yaw_limit[1])>65536||result.yaw_limit[0]>=result.yaw_limit[1])return false;
        yaw_offset=field.property.offset;
        if(!weaponhand::field(reader,before.turret.address,L"Rotation",L"StructProperty",12,field)||
           !troika_reflection::numeric(reader,field.property,L"Rotator",L"IntProperty",std::array<wchar_t const*,3>{L"Pitch",L"Yaw",L"Roll"},rotation_structure)||
           !reader.at(before.turret.address,field.property.offset,result.turret_rotation))return false;
        rotation_offset=field.property.offset;
        if(!weaponhand::field(reader,before.turret.address,L"ViewPitchMin",L"IntProperty",4,field)||
           !troika_reflection::retain(reader,field.property)||!reader.at(before.turret.address,field.property.offset,result.pitch_min))return false;
        minimum_offset=field.property.offset;
        if(!weaponhand::field(reader,before.turret.address,L"ViewPitchMax",L"IntProperty",4,field)||
           !troika_reflection::retain(reader,field.property)||!reader.at(before.turret.address,field.property.offset,result.pitch_max))return false;
        maximum_offset=field.property.offset;
        if(!weaponhand::field(reader,before.turret.address,L"MuzzleSocketName",L"NameProperty",8,field)||
           !troika_reflection::retain(reader,field.property)||!reader.at(before.turret.address,field.property.offset,result.muzzle_socket))return false;
        socket_offset=field.property.offset;
        std::array<unsigned,2> handle_offsets{};
        if(!weaponhand::field(reader,before.turret.address,L"LeftHandBoneHandleName",L"NameProperty",8,field)||
           !troika_reflection::retain(reader,field.property)||!reader.at(before.turret.address,field.property.offset,result.handle_name[0]))return false;
        handle_offsets[0]=field.property.offset;
        if(!weaponhand::field(reader,before.turret.address,L"RightHandBoneHandleName",L"NameProperty",8,field)||
           !troika_reflection::retain(reader,field.property)||!reader.at(before.turret.address,field.property.offset,result.handle_name[1]))return false;
        handle_offsets[1]=field.property.offset;
        for(unsigned side=0;side<2;++side){meta::NameText label{};
            if(result.handle_name[side].number||!meta::fresh_name(schema.names,result.handle_name[side].index,label))return false;
            if(meta::exact_text(label,L"b_troika_handle_L"))result.hand_handle[side]=0;
            else if(meta::exact_text(label,L"b_troika_handle_R"))result.hand_handle[side]=1;
            else return false;
            playground::Args match(result.component.address,L"MatchRefBone");
            if(!hammerburst_animation::component_getter(match,L"MatchRefBone")||
               !hammerburst_animation::input_field(match,match.prop(L"BoneName"),L"NameProperty",8)||
               !vrequip::result_property(match,L"IntProperty",4))return false;
            const auto input_offset=get<unsigned>(match.prop(L"BoneName")+0x94),output_offset=get<unsigned>(match.prop(L"ReturnValue")+0x94);
            if(!(input_offset+8<=output_offset||output_offset+4<=input_offset)||!current()||
               !match.set(L"BoneName",result.handle_name[side]).run()||match.output<int>()!=int(10+result.hand_handle[side]))return false;
        }
        if(result.hand_handle[0]==result.hand_handle[1])return false;
        status=Status::names;
        std::array<unsigned,6> indices{};const auto started=weaponhand::clock_ns(nullptr);
        const auto read_table=[](gears_equipment_names::Table& table){const auto values=get<Array>(base+0x1c34800);
                table={values.data,unsigned(values.count),unsigned(values.capacity)};return values.count>=0&&values.capacity>=0;};
        const auto read_name=[](gears_equipment_names::Table table,unsigned index,std::array<wchar_t,32>& text){
                const auto entry=get<uptr>(table.data+std::size_t(index)*8);unsigned observed_name_index{};
                return entry&&copy(&observed_name_index,entry,4)&&observed_name_index==index&&copy(text.data(),entry+20,sizeof(text));};
        const auto budget=[&](){const auto now=weaponhand::clock_ns(nullptr);return now>=started&&now-started<1000000;};
        std::array<unsigned,3> pivots{},barrels{};
        if(pivot_names.step(read_table,read_name,current,budget,pivots,2048)!=gears_equipment_names::State::ready||
           barrel_names.step(read_table,read_name,current,budget,barrels,2048)!=gears_equipment_names::State::ready)return false;
        for(unsigned i=0;i<3;++i){indices[i]=pivots[i];indices[i+3]=barrels[i];}
        constexpr const wchar_t* labels[]={L"b_troika_root",L"b_troika_gun_latitude",L"b_troika_gun_longitude",
            L"b_troika_muzzle_R",L"b_troika_muzzle_L",L"b_troika_handleroot"};
        constexpr int expected[]={0,1,2,7,8,9};
        status=Status::bones;
        for(unsigned i=0;i<indices.size();++i){
            meta::NameText text{};playground::Args match(result.component.address,L"MatchRefBone");
            if(!meta::fresh_name(schema.names,indices[i],text)||text.length!=std::wcslen(labels[i])||
               std::wmemcmp(text.units.data(),labels[i],text.length+1)||
               !hammerburst_animation::component_getter(match,L"MatchRefBone")||
               !hammerburst_animation::input_field(match,match.prop(L"BoneName"),L"NameProperty",8)||
               !vrequip::result_property(match,L"IntProperty",4))return false;
            const auto a=get<unsigned>(match.prop(L"BoneName")+0x94),b=get<unsigned>(match.prop(L"ReturnValue")+0x94);
            if(!(a+8<=b||b+4<=a)||!current()||!match.set(L"BoneName",Name{indices[i],0}).run()||match.output<int>()!=expected[i])return false;
            playground::Args matrix(result.component.address,L"GetBoneMatrix");
            if(!hammerburst_animation::component_getter(matrix,L"GetBoneMatrix")||
               !hammerburst_animation::input_field(matrix,matrix.prop(L"BoneIndex"),L"IntProperty",4)||
               !vrequip::result_property(matrix,L"StructProperty",64))return false;
            const auto returned=matrix.prop(L"ReturnValue"),type=get<uptr>(returned+0xd0);
            const auto input_offset=get<unsigned>(matrix.prop(L"BoneIndex")+0x94),output_offset=get<unsigned>(returned+0x94);
            if(!registered(type)||!playground::named(type,L"Matrix")||get<unsigned>(type+0x88)!=64||
               !(input_offset+4<=output_offset||output_offset+64<=input_offset)||!current()||
               !matrix.set(L"BoneIndex",expected[i]).run())return false;
            result.bones[i]=matrix.output<std::array<float,16>>();gears_vr::Vector local{};double scale{};
            if(!gears_weapon::troika_handle_geometry::local_point(result.bones[i],
                {result.bones[i][12],result.bones[i][13],result.bones[i][14]},local,scale))return false;
        }
        // The actual original selected muzzle socket is observed independently
        // of the two bone origins. No local alternating-barrel recipe is made.
        status=Status::physical_start;bool socket_known{};meta::NameText socket_label{};
        if(meta::fresh_name(schema.names,result.muzzle_socket.index,socket_label)&&socket_label.length&&!meta::exact_text(socket_label,L"None")){
            meta::Snapshot socket_schema{};socket_schema.module_base=base;meta::Reader socket_reader{socket_schema,meta::Stage::final_check};
            playground::Args socket(result.component.address,L"GetSocketWorldLocationAndRotation");
            socket_contract::SocketGetterLayout layout{},closing{};unsigned mask{},closing_mask{},returned{};
            if(meta::tables(base,socket_schema.objects,socket_schema.names)&&
               socket_contract::socket_getter_schema(socket,socket_reader,mask,&layout)&&
               layout.write(socket,socket_contract::SocketGetterLayout::input_name,result.muzzle_socket)&&current()&&socket.run()&&
               socket_contract::socket_getter_schema(socket,socket_reader,closing_mask,&closing)&&mask==closing_mask&&
               layout.function==closing.function&&layout.bytes==closing.bytes&&layout.offsets==closing.offsets&&
               layout.read(socket,socket_contract::SocketGetterLayout::result,returned)&&(returned&mask)&&
               layout.read(socket,socket_contract::SocketGetterLayout::output_location,result.socket_position)&&
               layout.read(socket,socket_contract::SocketGetterLayout::output_rotation,result.socket_rotation)&&current())socket_known=true;
        }
        std::array<float,3> physical{};
        if(socket_known&&physical_start::native(before.turret,current,physical)){
                result.physical_start={physical[0],physical[1],physical[2]};
                const double dx=double(result.physical_start.x)-result.socket_position.x,dy=double(result.physical_start.y)-result.socket_position.y,
                    dz=double(result.physical_start.z)-result.socket_position.z;
                result.physical_start_known=std::isfinite(dx)&&std::isfinite(dy)&&std::isfinite(dz)&&dx*dx+dy*dy+dz*dz<=.0001;
        }
        // Contact remains independent of aim calibration. Unknown ORIGINAL
        // controls never certify an axis, but do not hide an otherwise valid
        // measured handle from the existing input ownership transaction.
        result.controls_known=read_controls&&control_axes::read(before.turret,before.turret_mesh,current,result.original_controls);
        if(result.controls_known){
            control_axes::Decoded decoded{};control_axes::Snapshot closing_controls{};
            if(control_axes::decode(before.turret,result.original_controls,
                [](uptr receiver){return playground::Args(receiver,L"GetEnum");},current,decoded)){
                control_binding::Snapshot binding{};
                const bool bound=control_binding::read(before.turret,before.turret_mesh,result.original_controls,current,binding)&&
                    hand::same(binding.asset,result.asset);
                if(control_axes::read(before.turret,before.turret_mesh,current,closing_controls)&&
                   !std::memcmp(&closing_controls,&result.original_controls,sizeof closing_controls)){
                    result.control_enums=decoded;result.control_enums_known=true;
                    if(bound){result.original_binding=binding;result.control_binding_known=true;
                        meta::RawName socket_name{result.muzzle_socket.index,result.muzzle_socket.number};
                        result.socket_binding_known=socket_binding::read(result.asset,socket_name,binding,current,result.selected_socket);}
                }
            }
        }
        status=Status::closing;std::array<float,2> closing_yaw{};std::array<int,3> closing_rotation{};
        int closing_min{},closing_max{};Name closing_socket{};
        if(!current()||!reader.at(before.turret.address,yaw_offset,closing_yaw)||closing_yaw!=result.yaw_limit||
           !reader.at(before.turret.address,rotation_offset,closing_rotation)||closing_rotation!=result.turret_rotation||
           !reader.at(before.turret.address,minimum_offset,closing_min)||closing_min!=result.pitch_min||
           !reader.at(before.turret.address,maximum_offset,closing_max)||closing_max!=result.pitch_max||
           !reader.at(before.turret.address,socket_offset,closing_socket)||closing_socket.index!=result.muzzle_socket.index||
           closing_socket.number!=result.muzzle_socket.number||
           !troika_reflection::current(reader,yaw_structure,L"Vector2D",L"Object",L"Core")||
           !troika_reflection::current(reader,rotation_structure,L"Rotator",L"Object",L"Core")||
           !troika_original::link(result.component.address,L"SkeletalMesh",result.asset.address)||
           !unified_weapon_fire::retained_metadata(reader))return false;
        for(unsigned side=0;side<2;++side){Name closing{};
            if(!reader.at(before.turret.address,handle_offsets[side],closing)||closing.index!=result.handle_name[side].index||
               closing.number!=result.handle_name[side].number)return false;
        }
        result.publication=source.publication_sequence;result.input_sync_ns=source.controllers.sync_ns;output=result;status=Status::ready;return true;
    }catch(...){status=Status::exception;return false;}
}
unsigned log_rows{},last_status=UINT_MAX;bool last_controls_known{},last_enums_known{},last_binding_known{};
bool observe(troika_original::EntryOwner const& owner,gears_troika_source::Frame const& source,
    gears_weapon::TroikaMountedMapping const& mapping,Observation& output,bool read_controls=false) noexcept {
    Status status{};const bool ready=read(owner,source,mapping,output,status,read_controls);
    if(log_file&&log_rows<8&&(last_status!=unsigned(status)||(read_controls&&
       (last_controls_known!=output.controls_known||last_enums_known!=output.control_enums_known||last_binding_known!=output.control_binding_known)))){
        fwprintf(log_file,L"troika_mounted_parts stage=%u ready=%u entry=%llu scene=%llu input_sync=%lld physical_start_known=%u measurements_only=1\n",
            unsigned(status),unsigned(ready),owner.last_token,source.scene_invocation,source.controllers.sync_ns,unsigned(output.physical_start_known));
        if(ready)fwprintf(log_file,L"troika_original_geometry yaw_limit_raw=%.9g,%.9g pitch_raw=%d,%d muzzle_R=%.9g,%.9g,%.9g muzzle_L=%.9g,%.9g,%.9g selected_socket=%u,%u physical_start=%.9g,%.9g,%.9g\n",
            output.yaw_limit[0],output.yaw_limit[1],output.pitch_min,output.pitch_max,
            output.bones[3][12],output.bones[3][13],output.bones[3][14],output.bones[4][12],output.bones[4][13],output.bones[4][14],
            output.muzzle_socket.index,output.muzzle_socket.number,output.physical_start.x,output.physical_start.y,output.physical_start.z);
        if(ready&&output.controls_known)for(unsigned i=0;i<2;++i){auto const& c=output.original_controls.controls[i];
            fwprintf(log_file,L"troika_original_control index=%u object=%llx component=%llx raw_enum=%u,%u,%u,%u flags=%u max_angle=%.9g dead_zone=%.9g strength=%.9g alpha=%.9g axis_certified=0\n",
                i,c.object.address,c.component.address,unsigned(c.bytes[0].value),unsigned(c.bytes[1].value),
                unsigned(c.bytes[2].value),unsigned(c.bytes[3].value),c.flags,c.max_angle,c.dead_zone,c.strength,c.alpha);}
        if(ready&&output.control_enums_known)for(unsigned i=0;i<2;++i)
            fwprintf(log_file,L"troika_original_control_names index=%u look=%u up=%u rotation_space=%u target_space=%u decoded_by_original_GetEnum=1 mechanical_axis_certified=0\n",
                i,unsigned(output.control_enums.look[i]),unsigned(output.control_enums.up[i]),unsigned(output.control_enums.rotation[i]),unsigned(output.control_enums.target[i]));
        if(ready&&output.control_binding_known)fwprintf(log_file,L"troika_original_control_binding tree=%llx lists=%d nodes=%u latitude_bone=%u latitude_list=%u longitude_bone=%u longitude_list=%u original_parent_chain=0,0,1,2 mechanical_calibration=0\n",
            output.original_binding.tree.address,output.original_binding.lists.count,output.original_binding.node_count,
            output.original_binding.bone_index[0],output.original_binding.list_index[0],output.original_binding.bone_index[1],output.original_binding.list_index[1]);
        fflush(log_file);++log_rows;last_status=unsigned(status);
        if(read_controls){last_controls_known=output.controls_known;last_enums_known=output.control_enums_known;last_binding_known=output.control_binding_known;}
    }
    return ready;
}
} // namespace troika_mounted_parts
