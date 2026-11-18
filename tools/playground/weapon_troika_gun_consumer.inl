// Within mounted_aim_candidates, after current/typed actor readback helpers.
// Compares the next observed original gun with the previous accepted request.
// No engine setter, scheduling override, C0 grant or fire permission here.
bool same_configuration(parts::Observation const& a,parts::Observation const& b) noexcept {
    const auto name=[](auto const& x,auto const& y){return x.index==y.index&&x.number==y.number;};
    if(!a.controls_known||!b.controls_known||!a.control_enums_known||!b.control_enums_known||!a.control_binding_known||!b.control_binding_known||
       !hand::same(a.asset,b.asset)||!hand::same(a.component,b.component)||a.yaw_limit!=b.yaw_limit||a.pitch_min!=b.pitch_min||
       a.pitch_max!=b.pitch_max||a.turret_rotation!=b.turret_rotation||a.hand_handle[0]!=b.hand_handle[0]||a.hand_handle[1]!=b.hand_handle[1]||
       !name(a.handle_name[0],b.handle_name[0])||!name(a.handle_name[1],b.handle_name[1]))return false;
    auto const& x=a.control_enums;auto const& y=b.control_enums;
    if(x.look!=y.look||x.up!=y.up||x.rotation!=y.rotation||x.target!=y.target)return false;
    for(unsigned i=0;i<2;++i){auto const& p=a.original_controls.controls[i];auto const& q=b.original_controls.controls[i];
        if(!hand::same(p.object,q.object)||!hand::same(p.component,q.component)||!hand::same(p.object_reference,q.object_reference)||
           !hand::same(p.component_reference,q.component_reference)||!name(p.name,q.name)||!name(p.target_bone,q.target_bone)||
           !name(p.other_bone,q.other_bone)||p.flags!=q.flags||p.bool_masks!=q.bool_masks||p.max_angle!=q.max_angle||
           p.dead_zone!=q.dead_zone||p.strength!=q.strength||p.alpha!=q.alpha)return false;
        for(unsigned n=0;n<4;++n)if(!hand::same(p.bytes[n].enumeration,q.bytes[n].enumeration)||p.bytes[n].value!=q.bytes[n].value)return false;
        // TargetLocation/BaseLookDir/LimitLookDir/BaseBonePos are runtime
        // values, not configuration. Actual matrices below certify output.
    }
    auto const& p=a.original_binding;auto const& q=b.original_binding;
    const auto table=[](auto const& l,auto const& r){return l.data==r.data&&l.count==r.count&&l.capacity==r.capacity;};
    if(!hand::same(p.tree,q.tree)||!hand::same(p.tree_reference,q.tree_reference)||!hand::same(p.list_type,q.list_type)||
       !hand::same(p.head_reference,q.head_reference)||!hand::same(p.asset,q.asset)||!hand::same(p.asset_reference,q.asset_reference)||
       !hand::same(p.parent_anim_reference,q.parent_anim_reference)||!table(p.lists,q.lists)||!table(p.indices,q.indices)||!table(p.references,q.references)||
       p.stride!=q.stride||p.node_count!=q.node_count||p.list_index!=q.list_index||p.bone_index!=q.bone_index||
       p.index_values!=q.index_values||p.reference_bytes!=q.reference_bytes)return false;
    for(unsigned i=0;i<p.rows.size();++i)if(!name(p.rows[i].bone,q.rows[i].bone)||p.rows[i].head!=q.rows[i].head||p.rows[i].draw_y!=q.rows[i].draw_y)return false;
    for(unsigned i=0;i<p.nodes.size();++i)if(!hand::same(p.nodes[i].object,q.nodes[i].object)||!hand::same(p.nodes[i].component,q.nodes[i].component)||
       !hand::same(p.nodes[i].component_reference,q.nodes[i].component_reference)||!hand::same(p.nodes[i].next_reference,q.nodes[i].next_reference)||
       p.nodes[i].next!=q.nodes[i].next)return false;
    return true;
}
enum class GunStatus:unsigned {no_pending,source,configuration,actor,matrices,contact,socket,ready};
unsigned gun_log_rows{},last_gun_status=UINT_MAX;
void observe_gun(fire::Run& run,gears_weapon::TroikaMountedMapping const& mapped,parts::Observation const& measured) noexcept {
    gun_receipt.ready=false;GunStatus status=GunStatus::no_pending;gun::Match match{};
    try{
        const auto observe=[&](){
            if(!gun_pending.active)return false;
            status=GunStatus::source;
            if(!current(&run)||!gun::later(gun_pending.run,run,weaponhand::clock_ns(nullptr))||
               gun_pending.predicted.calibration!=anchors.calibration||mapped.hands.tracked_mask!=3)return false;
            status=GunStatus::configuration;if(!same_configuration(gun_pending.configuration,measured))return false;
            status=GunStatus::actor;std::array<std::int32_t,3> controller{},aim{};
            if(!gun_pending.actor.verified_actor()||!controller_direction(run,controller)||!aim_direction(run,aim)||
               !action::rotation_matches(controller,gun_pending.actor.requested)||!action::rotation_matches(aim,controller))return false;
            status=GunStatus::matrices;if(!gun::matches(gun_pending.predicted,measured.bones,match))return false;
            status=GunStatus::contact;
            for(unsigned side=0;side<2;++side){gears_weapon::troika_handle_geometry::Contact contact{};
                const auto raw=mapped.hands.world[side].position,target=world(measured.bones[5],anchors.local[side]);
                const double dx=(raw.x-target.x)/mapped.units_per_metre,dy=(raw.y-target.y)/mapped.units_per_metre,dz=(raw.z-target.z)/mapped.units_per_metre;
                if(!std::isfinite(dx)||!std::isfinite(dy)||!std::isfinite(dz)||dx*dx+dy*dy+dz*dz>.000625||
                   !gears_weapon::troika_handle_geometry::contact(measured.bones[5],raw,measured.hand_handle[side],mapped.units_per_metre,contact)||!contact.touched)return false;
            }
            status=GunStatus::socket;
            if(!measured.physical_start_known||!measured.socket_binding_known||
               (measured.selected_socket.bone!=7&&measured.selected_socket.bone!=8)||
               measured.selected_socket.socket_name.index!=measured.muzzle_socket.index||measured.selected_socket.socket_name.number!=measured.muzzle_socket.number)return false;
            auto const& bone=measured.bones[measured.selected_socket.bone==7?3:4];
            if(!gun::selected_socket(bone,measured.selected_socket.location,
                {measured.socket_position.x,measured.socket_position.y,measured.socket_position.z},
                {measured.physical_start.x,measured.physical_start.y,measured.physical_start.z},mapped.units_per_metre)||
               !gun::socket_rotation(bone,measured.selected_socket.rotation,measured.socket_rotation)||!current(&run))return false;
            gun_receipt={};gun_receipt.run=run;gun_receipt.asset=measured.asset;gun_receipt.calibration=anchors.calibration;
            gun_receipt.written_publication=gun_pending.run.source.frame.publication_sequence;gun_receipt.written_scene=gun_pending.run.source.frame.scene_invocation;
            gun_receipt.match=match;gun_receipt.socket_bone=measured.selected_socket.bone;gun_receipt.actor_direction=aim;gun_receipt.ready=true;
            status=GunStatus::ready;return true;
        };
        observe();
    }catch(...){gun_receipt.ready=false;}
    if(log_file&&gun_log_rows<12&&last_gun_status!=unsigned(status)){++gun_log_rows;last_gun_status=unsigned(status);
        fwprintf(log_file,L"troika_gun_observed status=%u publication=%llu scene=%llu written_publication=%llu written_scene=%llu position_error_m=%.9g rotation_error_rad=%.9g selected_bone=%u ready=%u previous_request_only=1 C0_qualified=0 fire_ready=0\n",
            unsigned(status),run.source.frame.publication_sequence,run.source.frame.scene_invocation,gun_pending.run.source.frame.publication_sequence,
            gun_pending.run.source.frame.scene_invocation,match.maximum_position_error_metres,match.maximum_rotation_error_radians,
            measured.selected_socket.bone,unsigned(gun_receipt.ready));fflush(log_file);}
}
