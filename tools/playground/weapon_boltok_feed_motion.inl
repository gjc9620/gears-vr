// Included after motion: independent ammo geometry history, no action owner.
namespace feed_motion {
namespace source=gears_hand_continuity;
using Builder=gears_boltok_feed_motion::Builder;
Builder builder;
motion::Context previous{}; // Domain witnesses only; never a current permit.
std::uint64_t item{};
namespace snap=gears_boltok_loading_snap;
// Geometry and original acquisition timestamp only. Never a retained input,
// permission or source clock. Display requires current source/lease/Native
// admission; insertion additionally requires the original RAW Builder path.
snap::Correction correction{};
std::int64_t acquired_sync{};
bool alignment_ready{};
void reset() noexcept {builder.reset();previous={};item=0;correction={};acquired_sync=0;alignment_ready=false;}
P compose(P const& a,P const& b) noexcept {
    return {gears_weapon::sum(a.position,gears_weapon::rotate(a.orientation,b.position)),
        gears_vr::normalized(gears_vr::multiply(a.orientation,b.orientation))};
}
P relative(P const& a,P const& b) noexcept {
    const auto q=gears_vr::inverse(gears_vr::normalized(a.orientation));
    return {gears_weapon::rotate(q,gears_weapon::difference(b.position,a.position)),
        gears_vr::normalized(gears_vr::multiply(q,b.orientation))};
}
void report(hand::Frame const& f,unsigned gate,unsigned failure,unsigned history,unsigned count) noexcept {
    static unsigned rows{},last_gate{~0u},last_failure{~0u};static std::uint64_t last_item{};
    if(!log_file||rows>=12||(gate==last_gate&&failure==last_failure&&last_item==round_item))return;
    ++rows;last_gate=gate;last_failure=failure;last_item=round_item;
    fwprintf(log_file,L"boltok_feed_history gate=%u builder_failure=%u context_gate=%u history=%u points=%u item=%llu frame=%llu sequence=%llu\n",
        gate,failure,motion::current.gate,history,count,round_item,f.key.frame,f.controllers.sequence);fflush(log_file);
}
bool display_grip(hand::Frame const& f,hand::Decision const& d,
    boltok_parts::Mechanical const& parts,P& out) noexcept {
    if(!round_item||round_item!=f.press_token||!granted(f,d,hand::LeaseKind::ammo)||
       !hand::same(held,d.lease)||!motion::current.valid||
       !hand::same(motion::current.frame.key,f.key)||
       !nativeequipment::current(nullptr,f))return false;

    gears_committed_interaction::Frame pair{};
    const bool allow_sim=
        f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated;
    if(!handrig::read_interaction(pair))return false;
    const auto now=weaponhand::clock_ns(nullptr);
    if(
       !gears_committed_interaction::matches(
           pair,f,vrequip::model.hold_generation(),now,allow_sim)||
       !gears_committed_interaction::same_left_control_cohort(pair,f)||
       pair.right.binding_generation!=motion::current.binding_generation||
       pair.right.native_reference.epoch!=motion::current.reference.epoch||
       pair.right.producer.stream!=motion::current.raw.stream_epoch||
       pair.right.producer.tracking[0]!=motion::current.raw.tracking_epoch[0]||
       pair.right.producer.tracking[1]!=motion::current.raw.tracking_epoch[1]||
       pair.right.units_per_metre!=parts.units||
       !nativeequipment::current(nullptr,f))return false;

    P candidate=pair.left_grip;
    if(!gears_vr::stereo_pose_valid(candidate))return false;
    out=candidate;

    const double distance=gears_weapon::length(gears_weapon::difference(
        pair.left_grip.position,actual_left_world.position))/parts.units;
    static unsigned rows{};static std::uint64_t last_token{};
    static bool known{},last_reach_limited{};
    const bool reach_limited=distance>.05;
    if(log_file&&rows<8&&(!known||last_token!=f.press_token||last_reach_limited!=reach_limited)){
        ++rows;fwprintf(log_file,L"boltok_loading_avatar accepted=1 frame=%llu sequence=%llu token=%llu endpoint_serial=%llu raw_to_avatar_metres=%.9g reach_limited=%u cosmetic_only=1\n",
            f.key.frame,f.controllers.sequence,f.press_token,pair.right.serial,distance,unsigned(reach_limited));fflush(log_file);
    }
    known=true;last_token=f.press_token;last_reach_limited=reach_limited;
    return true;
}

gears_weapon::FeedResult observe(hand::Frame const& f,hand::Decision const& d,
    boltok_parts::Mechanical const& parts,gears_weapon::BoltokReloadStamp const& stamp,
    gears_weapon::FeedSample const& sample) noexcept {
    bool visual_current=false;
    const auto reject=[&](unsigned gate,unsigned failure=0,unsigned history=0,
                          bool invalidate_visual=false) noexcept {
        report(f,gate,failure,history,0);reset();
        // A refused historical path grants no insertion. It does not revoke
        // this frame's independently admitted held display. Live source/lease
        // failure still withdraws that display immediately.
        if(invalidate_visual||!visual_current)(void)boltok_visual::hide_held();
        return insertion.observe_original_geometry_path(stamp,weaponhand::clock_ns(nullptr),round_item,nullptr,0,false);
    };
    auto const& c=motion::current;
    auto now=weaponhand::clock_ns(nullptr);
    if(!round_item||!granted(f,d,hand::LeaseKind::ammo)||!hand::same(held,d.lease)||
       !c.valid||!hand::same(c.frame.key,f.key)||!source::same_input(c.raw.input,f.controllers)||
       !parts.native_observed||parts.observed_ns<=0||now<parts.observed_ns||now-parts.observed_ns>100000000||
       !nativeequipment::current(nullptr,f))return reject(1);
    if(c.endpoint_serial){
        gears_committed_hand::Frame endpoint{};
        if(!handrig::read_endpoint(endpoint)||endpoint.serial!=c.endpoint_serial||
           !gears_committed_hand::current_result(endpoint,now,f.stereo.source==gears_vr::StereoSource::MetaOperatorSimulated))return reject(2);
    }
    source::TrackedInput raw{};source::Publisher publisher{};source::MotionHistory history{};
    gears_current_tracking::Failure failure{};
    if(!nativeequipment::read_input_continuity(f,raw,publisher,nullptr,&failure,&history)||
       publisher.pid!=c.publisher.pid||publisher.start!=c.publisher.start||!source::same_tracking(raw,c.raw))return reject(3,unsigned(failure));
    gears_weapon::FeedPortMount socket{};
    if(!boltok_visual::whole_socket(socket))return reject(4);
    const auto mount=socket.item_in_left_grip;
    const auto face_in_right=compose(c.canonical,relative(parts.root_metres,sample.weapon_pose));
    const auto actual=relative(sample.weapon_pose,compose(sample.left_grip,mount));
    unsigned selected{};
    if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::grip))selected=gears_vr::control_squeeze;
    else if(f.interaction_control==unsigned(gears_vr::LeftInteractionControl::pinch))selected=gears_vr::control_trigger;
    else return reject(5);
    const bool context_changed=builder.bound()&&
        (item!=round_item||!motion::same_context(previous,c));
    if(context_changed){
        // Clear interpolation and approach before using the new current
        // domain for cosmetic display. This grants no historical continuity.
        motion::report_context_difference(f,previous,c);reset();
        (void)insertion.observe_original_geometry_path(stamp,weaponhand::clock_ns(nullptr),round_item,nullptr,0,false);
    }
    const double scale=boltok_visual::geometry.physical_scale();
    const P raw_item=actual;
    const auto display_sync=raw.input.sync_ns;
    const bool leaving=acquired_sync&&!snap::keep(raw_item,scale);
    if(leaving){correction={};acquired_sync=0;alignment_ready=false;}
    if(!acquired_sync&&!leaving&&snap::settings.enabled&&snap::settings.insertion_assist){
        snap::Correction next{};
        if(snap::make(raw_item,mount,scale,next)){
            correction=next;acquired_sync=display_sync;alignment_ready=false;
            static unsigned acquire_rows{};
            if(log_file&&acquire_rows<12){++acquire_rows;fwprintf(log_file,L"boltok_loading_snap acquire=1 frame=%llu sequence=%llu item=%llu source_profile=%llu profile=%llu blend_seconds=%.3g lateral_m=%.9g,%.9g axial_raw_m=%.9g\n",
                f.key.frame,f.controllers.sequence,round_item,gears_boltok_owner_profiles::ammo.profile_id,
                gears_boltok_ammo_avatar::profile_id,snap::settings.seconds,
                correction.lateral_m.x,correction.lateral_m.z,raw_item.position.y);fflush(log_file);}
        }
    }
    constexpr std::int64_t blend_ns=120000000;
    P shown=raw_item;
    if(acquired_sync){
        if(display_sync<acquired_sync||
           !snap::constrain(raw_item,scale,true,
               (std::min)(1.,double(display_sync-acquired_sync)/blend_ns),shown))return reject(10);
    }
    const auto inverse_mount_rotation=gears_vr::inverse(mount.orientation);
    const P inverse_mount{gears_weapon::rotate(inverse_mount_rotation,gears_weapon::scaled(mount.position,-1)),inverse_mount_rotation};
    const P shown_grip=compose(sample.weapon_pose,compose(shown,inverse_mount));
    P world_grip=shown_grip;world_grip.position=gears_weapon::scaled(world_grip.position,parts.units);
    // The existing sole Native C0 owner publishes cosmetic hand placement.
    // The authenticated controller/Frame/world_grip source remains untouched.
    P avatar_grip{};
    // Keep the requested RAW+Snap hand target and physical insertion source.
    // The held mesh follows the independently current C0 palm after the
    // avatar's fixed-length arm solve, as the existing main gun already does.
    if(!part_skin::publish_loading_target(f,d,held,world_grip)||
       !display_grip(f,d,parts,avatar_grip)||
       !boltok_visual::held(f,d,avatar_grip))return reject(11);
    visual_current=true;
    if(context_changed)return reject(6);
    Builder::Path path{};now=weaponhand::clock_ns(nullptr);
    const bool okay=builder.bound()?
        builder.advance(raw,publisher,history,round_item,face_in_right,actual.position,actual.orientation,now,true,path):
        builder.bind(raw,publisher,selected,round_item,face_in_right,mount,actual.position,actual.orientation,now,true,path);
    // Recheck live guards after building geometry; history grants no action.
    now=weaponhand::clock_ns(nullptr);
    if(now<parts.observed_ns||now-parts.observed_ns>100000000||!nativeequipment::current(nullptr,f)||
       !granted(f,d,hand::LeaseKind::ammo)||!hand::same(held,d.lease))return reject(8,0,0,true);
    if(!okay)return reject(7,unsigned(builder.failure()),history.count);
    previous=c;item=round_item;
    report(f,0,0,history.count,path.count);
    if(!path.count)return reject(9);
    const auto& last=path.points[path.count-1];
    if(!acquired_sync||last.monotonic_ns-acquired_sync<blend_ns)
        return insertion.observe_original_geometry_path(stamp,weaponhand::clock_ns(nullptr),round_item,nullptr,0,false);
    if(!alignment_ready){
        // Explicit one-time discard at the blend boundary, including a frame
        // gap which skipped the intermediate cosmetic samples.
        (void)insertion.observe_original_geometry_path(stamp,now,round_item,nullptr,0,false);
        alignment_ready=true;
    }
    Builder::Path aligned{};
    for(unsigned i=0;i<path.count;++i){
        const auto& point=path.points[i];
        // No pre-acquisition or partially blended point can establish approach.
        // After the blend, real axial approach -> seat is still mandatory.
        if(point.monotonic_ns<acquired_sync||point.monotonic_ns-acquired_sync<blend_ns)continue;
        const P raw_pose{point.tip_in_face_metres,point.item_orientation_in_face};P pose{};
        if(!snap::constrain(raw_pose,scale,true,1,pose))return reject(12);
        auto corrected=point;corrected.tip_in_face_metres=pose.position;
        corrected.item_orientation_in_face=pose.orientation;
        aligned.points[aligned.count++]=corrected;
    }
    return insertion.observe_original_geometry_path(stamp,weaponhand::clock_ns(nullptr),round_item,aligned.points.data(),aligned.count,true);
}
}
